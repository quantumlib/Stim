"""Tests of the opt-in per-shot LeakageEvent recording and its helpers."""

from collections import Counter

import numpy as np
import pytest
import stim  # type: ignore[import-untyped]

from stimside.op_handlers.leakage_handlers.leakage_uint8_coset import LeakageUint8Coset
from stimside.op_handlers.leakage_handlers.leakage_uint8_tableau import LeakageUint8
from stimside.simulator_coset import CosetsideSimulator
from stimside.simulator_tableau import TablesideSimulator
from stimside.util.known_states import _unroll_circuit
from stimside.util.leakage_events import (
    LeakageEvent,
    is_leakage_event,
    sort_shot_events,
    unrolled_to_flattened_indices,
)


def _leaky_surface_code(d: int = 3, r: int = 2, pl: float = 0.02, hop: float = 0.2) -> stim.Circuit:
    """A surface code with leakage, hops, spreads, heralds and partial unleaks."""
    base = stim.Circuit.generated(
        "surface_code:rotated_memory_z",
        distance=d,
        rounds=r,
        after_clifford_depolarization=0.001,
        before_measure_flip_probability=0.001,
        after_reset_flip_probability=0.001,
    )
    out = stim.Circuit()
    old_to_new: list[int] = []
    n = 0
    for inst in base.flattened():
        if inst.name in ("M", "MR"):
            for _ in inst.targets_copy():
                old_to_new.append(n)
                n += 1
            out.append(inst)
            if inst.name == "MR":
                qs = [t.qubit_value for t in inst.targets_copy()]
                out.append(
                    "MPAD", [0] * len(qs), [],
                    tag=f"LEAKAGE_MEASUREMENT: (1.0, 2): {' '.join(map(str, qs))}",
                )
                n += len(qs)
                out.append("I_ERROR", qs, [], tag="LEAKAGE_TRANSITION_1: (0.95, 2-->U)")
        elif inst.name in ("DETECTOR", "OBSERVABLE_INCLUDE"):
            cur = len(old_to_new)
            targets = [
                stim.target_rec(old_to_new[cur + t.value] - n)
                if t.is_measurement_record_target
                else t
                for t in inst.targets_copy()
            ]
            out.append(inst.name, targets, inst.gate_args_copy())
        else:
            out.append(inst)
            if inst.name == "DEPOLARIZE1":
                out.append(
                    "I_ERROR", inst.targets_copy(), [],
                    tag=f"LEAKAGE_TRANSITION_1: ({pl}, U-->2) (0.3, 2-->U)",
                )
            elif inst.name == "DEPOLARIZE2":
                out.append(
                    "II_ERROR", inst.targets_copy(), [],
                    tag=(
                        f"LEAKAGE_TRANSITION_2: ({pl}, U_U-->2_D) ({pl / 2}, U_U-->2_2) "
                        f"({hop}, 2_U-->U_2) ({hop}, U_2-->2_U) (0.1, 2_U-->2_2) "
                        "(0.3, 2_U-->U_D) (0.3, U_2-->D_U)"
                    ),
                )
    return out


LEAKY = _leaky_surface_code()

# Deterministic: leak, hop, leaked->leaked changes inside a REPEAT, spread, unleak.
DETERMINISTIC = stim.Circuit(
    """
    R 0 1 2 3
    I_ERROR[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 0
    H 3
    II_ERROR[LEAKAGE_TRANSITION_2: (1.0, 2_U-->U_2)] 0 1
    REPEAT 2 {
        I_ERROR[LEAKAGE_TRANSITION_1: (1.0, 2-->3) (1.0, 3-->2)] 1
        SHIFT_COORDS(0, 1)
        TICK
    }
    II_ERROR[LEAKAGE_TRANSITION_2: (1.0, 2_U-->2_2)] 1 2
    CX 3 0
    M 0 1 2 3
    I_ERROR[LEAKAGE_TRANSITION_1: (1.0, 2-->U)] 1 2
    M 1 2
    """
)
DETERMINISTIC_EVENTS = [
    LeakageEvent(1, 0, 0, 2),
    LeakageEvent(3, 0, 2, 0),
    LeakageEvent(3, 1, 0, 2),
    LeakageEvent(4, 1, 2, 3),
    LeakageEvent(7, 1, 3, 2),
    LeakageEvent(10, 2, 0, 2),
    LeakageEvent(13, 1, 2, 0),
    LeakageEvent(13, 2, 2, 0),
]

TABLESIDE_PATHS = {
    "cpp": dict(batch_size=1),
    "python_v2": dict(batch_size=1, use_cpp_kernels=False),
    "handler": dict(batch_size=1, sync_tableside_rng=True),
    "cpp_batch4": dict(batch_size=4),
    "handler_batch4": dict(batch_size=4, sync_tableside_rng=True),
}


def _tableside(circuit, seed, record, **kw):
    handler = LeakageUint8().compile_op_handler(circuit=circuit, batch_size=kw["batch_size"])
    return TablesideSimulator(
        circuit=circuit,
        compiled_op_handler=handler,
        seed=seed,
        record_unleaked_to_leaked=record,
        record_leakage_events=record,
        **kw,
    )


def _cosetside(circuit, seed, record, batch_size=16):
    handler = LeakageUint8Coset().compile_op_handler(circuit=circuit, batch_size=batch_size)
    return CosetsideSimulator(
        circuit=circuit,
        compiled_op_handler=handler,
        batch_size=batch_size,
        seed=seed,
        record_unleaked_to_leaked=record,
        record_leakage_events=record,
    )


def _replay_leaked_states(events, num_qubits):
    """Final leaked states (0 = unleaked) implied by a shot's events; checks consistency."""
    state = np.zeros(num_qubits, dtype=np.int64)
    for ev in events:
        assert is_leakage_event(ev.old_state, ev.new_state), ev
        cur = state[ev.qubit]
        assert cur == (ev.old_state if ev.old_state >= 2 else 0), (ev, cur)
        state[ev.qubit] = ev.new_state if ev.new_state >= 2 else 0
    return state


def _check_shot(events, u2l_ops):
    assert events == sort_shot_events(events)
    leaks = [ev.op_index for ev in events if ev.old_state < 2 <= ev.new_state]
    assert Counter(leaks) == Counter(u2l_ops.tolist())
    return len(leaks)


@pytest.mark.parametrize("path", list(TABLESIDE_PATHS))
def test_tableside_events_match_u2l_records_and_final_state(path):
    kw = TABLESIDE_PATHS[path]
    n_leaks = 0
    for seed in range(6):
        off = _tableside(LEAKY, seed, False, **kw)
        off.run()
        on = _tableside(LEAKY, seed, True, **kw)
        on.run()
        # Recording draws no random numbers.
        np.testing.assert_array_equal(
            off.get_final_measurement_records(), on.get_final_measurement_records()
        )
        events = on.get_leakage_events()
        u2l = on.get_unleaked_to_leaked_records()
        assert len(events) == kw["batch_size"]
        for shot in range(kw["batch_size"]):
            n_leaks += _check_shot(events[shot], u2l[shot])
        # The handler state is per-batch only on the unsynced paths (shot 0 is broadcast)
        # and for batch_size 1; with sync_tableside_rng and batch > 1 it holds the last shot.
        last = kw["batch_size"] - 1 if kw.get("sync_tableside_rng") else 0
        final = np.asarray(on._compiled_op_handler.state, dtype=np.int64)
        np.testing.assert_array_equal(
            _replay_leaked_states(events[last], LEAKY.num_qubits),
            np.where(final >= 2, final, 0),
        )
    assert n_leaks > 0


def test_cosetside_events_match_u2l_records_and_final_state():
    n_leaks = 0
    for seed in range(3):
        off = _cosetside(LEAKY, seed, False)
        off.run()
        on = _cosetside(LEAKY, seed, True)
        on.run()
        np.testing.assert_array_equal(
            off.get_final_measurement_records(), on.get_final_measurement_records()
        )
        events = on.get_leakage_events()
        u2l = on.get_unleaked_to_leaked_records()
        final = np.asarray(on._compiled_op_handler._state, dtype=np.int64)
        assert len(events) == 16
        for shot in range(16):
            n_leaks += _check_shot(events[shot], u2l[shot])
            np.testing.assert_array_equal(
                _replay_leaked_states(events[shot], LEAKY.num_qubits),
                np.where(final[:, shot] >= 2, final[:, shot], 0),
            )
    assert n_leaks > 0


@pytest.mark.parametrize("path", list(TABLESIDE_PATHS))
def test_deterministic_events_are_identical_on_all_tableside_paths(path):
    kw = TABLESIDE_PATHS[path]
    sim = _tableside(DETERMINISTIC, 1, True, **kw)
    sim.run()
    assert sim.get_leakage_events() == [DETERMINISTIC_EVENTS] * kw["batch_size"]


def test_deterministic_events_on_cosetside():
    sim = _cosetside(DETERMINISTIC, 1, True)
    sim.run()
    assert sim.get_leakage_events() == [DETERMINISTIC_EVENTS] * 16


def test_deterministic_event_op_indices_are_unrolled_indices():
    unrolled = _unroll_circuit(DETERMINISTIC)
    for ev in DETERMINISTIC_EVENTS:
        assert "LEAKAGE_TRANSITION" in unrolled[ev.op_index].tag


def test_events_disabled_raise_and_clear_resets():
    for sim in (_tableside(DETERMINISTIC, 1, False, batch_size=1), _cosetside(DETERMINISTIC, 1, False)):
        sim.run()
        with pytest.raises(ValueError, match="record_leakage_events=False"):
            sim.get_leakage_events()
    for sim in (_tableside(DETERMINISTIC, 1, True, batch_size=2), _cosetside(DETERMINISTIC, 1, True)):
        sim.run()
        assert all(sim.get_leakage_events())
        sim.clear()
        assert not any(sim.get_leakage_events())
        sim.run()
        assert sim.get_leakage_events()[0] == DETERMINISTIC_EVENTS


def test_is_leakage_event_and_sort():
    assert is_leakage_event(0, 2) and is_leakage_event(3, 1) and is_leakage_event(2, 3)
    assert not is_leakage_event(0, 1) and not is_leakage_event(2, 2)
    evs = [LeakageEvent(5, 1, 0, 2), LeakageEvent(2, 3, 0, 2), LeakageEvent(5, 1, 2, 3), LeakageEvent(5, 0, 0, 2)]
    assert sort_shot_events(evs) == [evs[1], evs[3], evs[0], evs[2]]


def test_unrolled_to_flattened_indices():
    circuit = stim.Circuit(
        """
        QUBIT_COORDS(0, 0) 0
        R 0
        REPEAT 2 {
            SHIFT_COORDS(0, 1)
            X_ERROR(0.1) 0
            REPEAT 2 {
                M 0
                DETECTOR(0, 0) rec[-1]
            }
        }
        """
    )
    got = unrolled_to_flattened_indices(circuit)
    assert got.tolist() == [0, 1, -1, 2, 3, 4, 5, 6, -1, 7, 8, 9, 10, 11]
    assert len(_unroll_circuit(circuit)) == len(got)
    flat = list(circuit.flattened())
    unrolled = _unroll_circuit(circuit)
    for u, f in enumerate(got):
        if f >= 0:
            assert unrolled[u].name == flat[f].name


def test_unrolled_to_flattened_indices_detects_fusion():
    # circuit.flattened() fuses the identical ops of consecutive iterations.
    circuit = stim.Circuit("R 0 1\nREPEAT 2 {\n    X_ERROR(0.1) 0\n}\nM 0 1")
    with pytest.raises(ValueError, match="flattened"):
        unrolled_to_flattened_indices(circuit)
