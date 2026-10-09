import pickle
import time

import numpy as np
import pymatching  # type: ignore[import-untyped]
import pytest
import sinter  # type: ignore[import-untyped]
import stim  # type: ignore[import-untyped]

import stimside
from stimside.dem_generators.dem_generator_marginal import MarginalLeakageDemGenerator
from stimside.op_handlers.leakage_handlers.leakage_uint8_coset import LeakageUint8Coset
from stimside.op_handlers.leakage_handlers.leakage_uint8_flip import (
    LeakageUint8 as LeakageUint8Flip,
)
from stimside.op_handlers.leakage_handlers.leakage_uint8_tableau import LeakageUint8
from stimside.sampler_coset import CosetsideSampler
from stimside.sampler_flip import FlipsideSampler
from stimside.dem_generators.dem_decoding import decode_with_generated_dems
from stimside.dem_generators.leakage_decoder import MarginalDecoder
from stimside.sampler_tableau import TablesideSampler
from stimside.sampler_tableau_test import _CallableDecoder

# Data qubits 0 and 2 around ancilla 1. Records: MR 1 -> 0, MPAD flags for
# qubits 0 and 2 -> 1 and 2, M 0 2 -> 3 and 4.
REP_CIRCUIT = stim.Circuit(
    """
    R 0 1 2
    TICK
    I_ERROR[LEAKAGE_TRANSITION_1: (0.01, U-->2) (0.02, U-->0)] 0 2
    X_ERROR(0.001) 0 2
    CX 0 1
    CX 2 1
    MR 1
    DETECTOR rec[-1]
    MPAD[LEAKAGE_MEASUREMENT: (1.0, 2) (0.01, 0) (0.03, 1): 0 2] 0 0
    I_ERROR[LEAKAGE_TRANSITION_1: (1.0, 2-->U)] 0 2
    M 0 2
    DETECTOR rec[-1] rec[-2] rec[-5]
    OBSERVABLE_INCLUDE(0) rec[-2]
    """
)


def _dem(text: str) -> stim.DetectorErrorModel:
    return stim.Circuit(text).detector_error_model(
        decompose_errors=False, approximate_disjoint_errors=True
    )


def _dem_dict(dem: stim.DetectorErrorModel) -> dict:
    """Symptom -> probability, merging duplicate mechanisms independently."""
    out: dict = {}
    for inst in dem.flattened():
        if inst.type != "error":
            continue
        targets = inst.targets_copy()
        key = (
            frozenset(t.val for t in targets if t.is_relative_detector_id()),
            frozenset(t.val for t in targets if t.is_logical_observable_id()),
        )
        p, q = inst.args_copy()[0], out.get(key, 0.0)
        out[key] = p + q - 2 * p * q
    return out


def _assert_same_mechanisms(actual, expected) -> None:
    actual_dict = _dem_dict(actual)
    expected_dict = _dem_dict(expected)
    assert actual_dict.keys() == expected_dict.keys()
    for key, p in expected_dict.items():
        assert actual_dict[key] == pytest.approx(p), key


def _records(circuit: stim.Circuit, *raised: int, shots: int = 1) -> np.ndarray:
    records = np.zeros((shots, circuit.num_measurements), dtype=bool)
    records[:, list(raised)] = True
    return records


def test_no_flags_gives_leakage_free_baseline():
    dems = MarginalLeakageDemGenerator()(REP_CIRCUIT, _records(REP_CIRCUIT, shots=2))
    expected = _dem(
        """
        R 0 1 2
        TICK
        PAULI_CHANNEL_1(0.005, 0.005, 0.005) 0 2
        X_ERROR(0.001) 0 2
        CX 0 1
        CX 2 1
        MR 1
        DETECTOR rec[-1]
        MPAD(0.02) 0 0
        M 0 2
        DETECTOR rec[-1] rec[-2] rec[-5]
        OBSERVABLE_INCLUDE(0) rec[-2]
        """
    )
    assert len(dems) == 2
    assert dems[0] == expected
    assert dems[0] is dems[1]


@pytest.mark.parametrize("uc", [True, False])
def test_single_flag_adds_hand_built_envelope(uc):
    dem = MarginalLeakageDemGenerator(uc)(REP_CIRCUIT, _records(REP_CIRCUIT, 1)[0])
    baseline = MarginalLeakageDemGenerator(uc)(REP_CIRCUIT, _records(REP_CIRCUIT)[0])
    # With uc the skipped CX needs the unknown frozen state -> depolarize before it.
    skipped_cx = "DEPOLARIZE1(0.75) 0" if uc else ""
    envelope = _dem(
        f"""
        R 0 1 2
        TICK
        {skipped_cx}
        CX 0 1
        CX 2 1
        MR 1
        DETECTOR rec[-1]
        MPAD 0 0
        DEPOLARIZE1(0.75) 0
        M 0 2
        DETECTOR rec[-1] rec[-2] rec[-5]
        OBSERVABLE_INCLUDE(0) rec[-2]
        """
    )
    _assert_same_mechanisms(dem, baseline + envelope)


def test_candidates_are_weighted_and_events_combine_independently():
    circuit = stim.Circuit(
        """
        R 0 1 2
        I_ERROR[LEAKAGE_TRANSITION_1: (0.03, U-->2)] 0 2
        CX 0 1
        CX 2 1
        I_ERROR[LEAKAGE_TRANSITION_1: (0.01, U-->2)] 0 2
        MR 1
        DETECTOR rec[-1]
        MPAD[LEAKAGE_MEASUREMENT: (1.0, 2): 0 2] 0 0
        I_ERROR[LEAKAGE_TRANSITION_1: (1.0, 2-->U)] 0 2
        """
    )
    gen = MarginalLeakageDemGenerator()
    one, both = gen(circuit, np.array([[0, 1, 0], [0, 1, 1]], dtype=bool))
    # Early leaks (weight 0.03) flip D0 w.p. 0.5 through the skipped CX; late leaks
    # (weight 0.01) do nothing visible, so each flag's envelope has D0 at 0.375.
    p = 0.75 * 0.5
    assert _dem_dict(one) == {(frozenset({0}), frozenset()): pytest.approx(p)}
    assert _dem_dict(both) == {
        (frozenset({0}), frozenset()): pytest.approx(2 * p - 2 * p * p)
    }


def test_candidate_weights_include_surviving_partial_unleaking():
    circuit = stim.Circuit(
        """
        R 0 1
        I_ERROR[LEAKAGE_TRANSITION_1: (0.01, U-->2)] 0
        CX 0 1
        I_ERROR[LEAKAGE_TRANSITION_1: (0.75, 2-->U)] 0
        I_ERROR[LEAKAGE_TRANSITION_1: (0.01, U-->2)] 0
        MR 1
        DETECTOR rec[-1]
        MPAD[LEAKAGE_MEASUREMENT: (1.0, 2): 0] 0
        I_ERROR[LEAKAGE_TRANSITION_1: (1.0, 2-->U)] 0
        """
    )
    dem = MarginalLeakageDemGenerator()(circuit, _records(circuit, 1)[0])
    # Only the early leak flips D0 (w.p. 0.5), and it is still there at the flag
    # w.p. 0.25, so it explains the flag w.p. 0.0025 / (0.0025 + 0.01) = 0.2.
    assert _dem_dict(dem) == {(frozenset({0}), frozenset()): pytest.approx(0.1)}


def test_flag_explained_by_earlier_flag_is_ignored():
    circuit = stim.Circuit(
        """
        R 0 1
        I_ERROR[LEAKAGE_TRANSITION_1: (0.02, U-->2)] 0
        CX 0 1
        MPAD[LEAKAGE_MEASUREMENT: (1.0, 2): 0] 0
        I_ERROR[LEAKAGE_TRANSITION_1: (0.01, U-->2)] 0
        CX 0 1
        MPAD[LEAKAGE_MEASUREMENT: (1.0, 2): 0] 0
        I_ERROR[LEAKAGE_TRANSITION_1: (1.0, 2-->U)] 0
        MR 1
        DETECTOR rec[-1]
        M 0
        DETECTOR rec[-1]
        """
    )
    gen = MarginalLeakageDemGenerator()
    first_only, both, second_only = gen(
        circuit, np.array([[1, 0, 0, 0], [1, 1, 0, 0], [0, 1, 0, 0]], dtype=bool)
    )
    # The first leak's frame crosses both skipped CXs (cancels on D0); only its
    # final depolarization shows up. The second leak only crosses the second CX.
    assert _dem_dict(first_only) == {(frozenset({1}), frozenset()): pytest.approx(0.5)}
    assert both is first_only
    assert _dem_dict(second_only) == {
        (frozenset({0, 1}), frozenset()): pytest.approx(0.5),
        (frozenset({1}), frozenset()): pytest.approx(0.5),
    }


def test_hyperedges_are_not_decomposed():
    circuit = stim.Circuit(
        """
        R 0 1 2
        I_ERROR[LEAKAGE_TRANSITION_1: (0.01, U-->2)] 0
        CX 0 1 0 2
        MR 1 2
        DETECTOR rec[-2]
        DETECTOR rec[-1]
        MPAD[LEAKAGE_MEASUREMENT: (1.0, 2): 0] 0
        I_ERROR[LEAKAGE_TRANSITION_1: (1.0, 2-->U)] 0
        M 0
        DETECTOR rec[-1]
        """
    )
    dem = MarginalLeakageDemGenerator()(circuit, _records(circuit, 2)[0])
    errors = [inst for inst in dem.flattened() if inst.type == "error"]
    assert not any(t.is_separator() for inst in errors for t in inst.targets_copy())
    assert _dem_dict(dem)[(frozenset({0, 1, 2}), frozenset())] == pytest.approx(0.5)


@pytest.mark.parametrize("role_change", [True, False])
def test_frozen_qubit_is_redepolarized_only_on_role_change(role_change):
    # Bell pair 0-2 with a Z check (ancilla 1) and a second check (ancilla 3).
    # With H between the CZs the leaked qubit changes role and needs a new D.
    h = "H 0 2" if role_change else ""
    body = f"""
        CZ 0 1 2 1
        {h}
        {{second_d}}
        CZ 0 3 2 3
        {h}
        MX 1 3
        DETECTOR rec[-2]
        DETECTOR rec[-1]
    """
    prefix = "R 0 2\nH 0\nCX 0 2\nRX 1 3\n"
    circuit = stim.Circuit(
        prefix
        + "I_ERROR[LEAKAGE_TRANSITION_1: (0.01, U-->2)] 0\n"
        + body.format(second_d="")
        + "MPAD[LEAKAGE_MEASUREMENT: (1.0, 2): 0] 0\n"
        + "I_ERROR[LEAKAGE_TRANSITION_1: (1.0, 2-->U)] 0\n"
    )
    dem = MarginalLeakageDemGenerator()(circuit, _records(circuit, 2)[0])

    def envelope(second_d: str) -> stim.DetectorErrorModel:
        return _dem(
            prefix
            + "DEPOLARIZE1(0.75) 0\n"
            + body.format(second_d=second_d)
            + "MPAD 0\nDEPOLARIZE1(0.75) 0\n"
        )

    single_d, double_d = envelope(""), envelope("DEPOLARIZE1(0.75) 0")
    _assert_same_mechanisms(dem, double_d if role_change else single_d)
    if not role_change:
        # A spurious second D would be visible here.
        assert _dem_dict(single_d) != _dem_dict(double_d)


def _simulate(circuit: stim.Circuit, uc: bool) -> tuple[np.ndarray, np.ndarray]:
    """Measurement records and detection events of 64 shots of the leakage simulator."""
    sampler = TablesideSampler(
        op_handler=LeakageUint8(unconditional_condition_on_U=uc),
        batch_size=64,
        dem_decoder=MarginalDecoder(MarginalLeakageDemGenerator(uc)),
    )
    compiled = sampler.compiled_sampler_for_task(
        sinter.Task(circuit=circuit, decoder="pymatching")
    )
    compiled.tab_simulator.clear()
    compiled.tab_simulator.run()
    records = compiled.tab_simulator.get_final_measurement_records().astype(bool)
    dets = circuit.compile_m2d_converter().convert(
        measurements=records, append_observables=False
    )
    return records, dets


@pytest.mark.parametrize(
    "uc, body, randomized",
    [
        # Without uc, untagged ops act on the leaked qubit like on the reference,
        # unless a conditioned gate skipped it before (|+> stays |+>, not |0>).
        (False, "H[CONDITIONED_ON_SELF: U] 0\nM 0\nDETECTOR rec[-1]", True),
        (False, "H 0\nM 0\nDETECTOR rec[-1]", False),
        (False, "H[CONDITIONED_ON_SELF: U] 0\nCX 0 1\nM 1\nDETECTOR rec[-1]", True),
        (False, "H 0\nCX 0 1\nM 1\nDETECTOR rec[-1]", False),
        # A skipped CX leaves the leaked qubit unentangled; the D before the CX
        # flips both halves of the Bell pair, so only a D before M 0 shows it.
        (False, "CX[CONDITIONED_ON_PAIR: (U, U)] 0 1\nM 0 1\nDETECTOR rec[-1] rec[-2]", True),
        (True, "CX 0 1\nM 0 1\nDETECTOR rec[-1] rec[-2]", True),
        # Nothing skipped: measuring the frozen qubit matches the reference.
        (True, "MX 0\nDETECTOR rec[-1]", False),
        (False, "MX 0\nDETECTOR rec[-1]", False),
    ],
)
def test_bare_op_on_leaked_qubit_depolarizes_only_after_skipped_gates(uc, body, randomized):
    circuit = stim.Circuit(
        f"""
        RX 0
        R 1
        I_ERROR[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 0
        {body}
        MPAD[LEAKAGE_MEASUREMENT: (1.0, 2): 0] 0
        """
    )
    records, dets = _simulate(circuit, uc)
    assert records[:, -1].all()
    assert dets[:, 0].any() == randomized
    assert not dets[:, 0].all()
    expected = {(frozenset({0}), frozenset()): pytest.approx(0.5)} if randomized else {}
    for dem in MarginalLeakageDemGenerator(uc)(circuit, records):
        assert _dem_dict(dem) == expected


@pytest.mark.parametrize("uc", [True, False])
def test_depolarizing_after_partial_unleak_forgets_the_tracked_state(uc):
    cx = "CX" if uc else "CX[CONDITIONED_ON_PAIR: (U, U)]"
    body = """
        RX 0
        R 1 2
        {leak}
        {d}
        {cx} 0 1
        MPAD{flag} 0
        {unleak}
        {d}
        {d}
        {cx} 0 2
        {d}
        M 0 1 2
        DETECTOR rec[-1] rec[-3]
        DETECTOR rec[-2] rec[-3]
    """
    circuit = stim.Circuit(
        body.format(
            leak="I_ERROR[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 0",
            unleak="I_ERROR[LEAKAGE_TRANSITION_1: (0.5, 2-->U)] 0",
            flag="[LEAKAGE_MEASUREMENT: (1.0, 2): 0]",
            d="",
            cx=cx,
        )
    )
    # If q0 is still leaked at CX 0 2, the simulator skips it too, so M 0 is random
    # against both M 1 and M 2 (the tableside simulator samples one leakage
    # trajectory per batch, so this is not checked empirically here). The D after
    # the partial unleak makes h's state unknown, so CX 0 2 needs a new D, which
    # leaves h desynced, so M 0 needs one too.
    envelope = _dem(body.format(leak="", unleak="", flag="", d="DEPOLARIZE1(0.75) 0", cx="CX"))
    dem = MarginalLeakageDemGenerator(uc)(circuit, _records(circuit, 0)[0])
    _assert_same_mechanisms(dem, envelope)
    assert _dem_dict(dem)[(frozenset({0, 1}), frozenset())] == pytest.approx(0.5)


def test_two_qubit_transition_partner_channel_and_move():
    circuit = stim.Circuit(
        """
        R 0 1 2
        II_ERROR[LEAKAGE_TRANSITION_2: (0.01, U_U-->2_X) (0.01, U_U-->2_U)] 0 1
        II_ERROR[LEAKAGE_TRANSITION_2: (1.0, 2_U-->U_2)] 0 2
        MPAD[LEAKAGE_MEASUREMENT: (1.0, 2): 2] 0
        I_ERROR[LEAKAGE_TRANSITION_1: (1.0, 2-->U)] 2
        M 0 1 2
        DETECTOR rec[-3]
        DETECTOR rec[-2]
        DETECTOR rec[-1]
        """
    )
    dem = MarginalLeakageDemGenerator()(circuit, _records(circuit, 0)[0])
    expected = _dem(
        """
        R 0 1 2
        PAULI_CHANNEL_1(0.5, 0, 0) 1
        DEPOLARIZE1(0.75) 0
        MPAD 0
        DEPOLARIZE1(0.75) 2
        M 0 1 2
        DETECTOR rec[-3]
        DETECTOR rec[-2]
        DETECTOR rec[-1]
        """
    )
    _assert_same_mechanisms(dem, expected)


def test_partial_hop_is_followed_on_its_own_branch():
    # The leak on qubit 0 hops to qubit 2 w.p. 1/4. Then qubit 2 skips CX 2 3
    # (D0 = M2 xor M3 is randomized) and qubit 0 comes back depolarized (D1).
    circuit = stim.Circuit(
        """
        R 0 3
        RX 2
        I_ERROR[LEAKAGE_TRANSITION_1: (0.01, U-->2)] 0
        MPAD[LEAKAGE_MEASUREMENT: (0.5, 2): 0] 0
        II_ERROR[LEAKAGE_TRANSITION_2: (0.25, 2_U-->U_2)] 0 2
        I_ERROR[LEAKAGE_TRANSITION_1: (0.00125, U-->2)] 2
        CX 2 3
        M 2 3
        DETECTOR rec[-1] rec[-2]
        MPAD[LEAKAGE_MEASUREMENT: (1.0, 2): 0 2] 0 0
        M 0
        DETECTOR rec[-1]
        """
    )
    early, late0, late2 = 0, 3, 4
    records = np.stack(
        [_records(circuit, *r)[0] for r in ([late2], [early], [early, late2], [late0])]
    )
    hop, early_only, both, stay = MarginalLeakageDemGenerator()(circuit, records)
    d0, d1 = (frozenset({0}), frozenset()), (frozenset({1}), frozenset())
    # The hop branch (weight 0.01 * 1/4 * 1/2) and the leak starting on qubit 2
    # (weight 0.00125) explain the late flag on qubit 2 equally often.
    assert _dem_dict(hop) == {d0: pytest.approx(0.5), d1: pytest.approx(0.25)}
    # The leak seen by the early flag hops afterwards w.p. 1/4.
    assert _dem_dict(early_only) == {d1: pytest.approx(0.5), d0: pytest.approx(0.125)}
    assert both is early_only
    assert _dem_dict(stay) == {d1: pytest.approx(0.5)}


@pytest.mark.parametrize(
    "circuit_text, flags, randomized",
    [
        pytest.param(
            # Qubit 1 holds the leak. In order, the pair (0, 1) leaves it alone (no
            # U_2 branch) and the later pair (1, 2) of the same op moves it to qubit 2.
            """
            R 0 1 2
            I_ERROR[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 1
            II_ERROR[LEAKAGE_TRANSITION_2: (1.0, 2_U-->U_2)] 0 1 1 2
            MPAD[LEAKAGE_MEASUREMENT: (1.0, 2): 1 2] 0 0
            I_ERROR[LEAKAGE_TRANSITION_1: (1.0, 2-->U)] 1 2
            M 0 1 2
            DETECTOR rec[-3]
            DETECTOR rec[-2]
            DETECTOR rec[-1]
            """,
            [False, True],
            {1, 2},
            id="later_pair",
        ),
        pytest.param(
            # The leak hops from qubit 0 to qubit 1 at the pair (0, 1), then on to
            # qubit 2 at the later pair (1, 2) of the same op.
            """
            R 0 1 2
            I_ERROR[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 0
            II_ERROR[LEAKAGE_TRANSITION_2: (1.0, 2_U-->U_2)] 0 1 1 2
            MPAD[LEAKAGE_MEASUREMENT: (1.0, 2): 1 2] 0 0
            I_ERROR[LEAKAGE_TRANSITION_1: (1.0, 2-->U)] 1 2
            M 0 1 2
            DETECTOR rec[-3]
            DETECTOR rec[-2]
            DETECTOR rec[-1]
            """,
            [False, True],
            {0, 1, 2},
            id="hop_then_later_pair",
        ),
        pytest.param(
            # Qubit 0 leaks at the pair (0, 1); the later pair (0, 2) of the same op
            # moves the leak to qubit 2.
            """
            R 0 1 2
            II_ERROR[LEAKAGE_TRANSITION_2: (1.0, U_U-->2_U) (1.0, 2_U-->U_2)] 0 1 0 2
            MPAD[LEAKAGE_MEASUREMENT: (1.0, 2): 0 2] 0 0
            I_ERROR[LEAKAGE_TRANSITION_1: (1.0, 2-->U)] 0 2
            M 0 1 2
            DETECTOR rec[-3]
            DETECTOR rec[-2]
            DETECTOR rec[-1]
            """,
            [False, True],
            {0, 2},
            id="source_pair_then_later_pair",
        ),
        pytest.param(
            # Qubit 0 leaks at the first target; the second target takes it to state 3.
            """
            R 0
            I_ERROR[LEAKAGE_TRANSITION_1: (1.0, U-->2) (1.0, 2-->3)] 0 0
            MPAD[LEAKAGE_MEASUREMENT: (1.0, 3): 0] 0
            I_ERROR[LEAKAGE_TRANSITION_1: (1.0, 3-->U)] 0
            M 0
            DETECTOR rec[-1]
            """,
            [True],
            {0},
            id="source_target_then_later_target",
        ),
    ],
)
def test_repeated_qubits_in_transitions_are_processed_in_order(circuit_text, flags, randomized):
    # All transitions are certain, so the simulator follows a single leakage
    # trajectory: it raises exactly `flags` and randomizes exactly the detectors in
    # `randomized` (qubits the leak left, or that unleaked, are depolarized). The
    # generator must follow the same trajectory.
    circuit = stim.Circuit(circuit_text)
    records, dets = _simulate(circuit, uc=True)
    assert (records[:, : len(flags)] == flags).all()
    assert set(np.flatnonzero(dets.any(axis=0)).tolist()) == randomized
    expected = {(frozenset({d}), frozenset()): pytest.approx(0.5) for d in randomized}
    for dem in MarginalLeakageDemGenerator()(circuit, records):
        assert _dem_dict(dem) == expected


def test_hop_branch_goes_on_with_later_pairs_of_the_same_op():
    # The leak on qubit 0 hops to qubit 1 (as state 3) at the pair (0, 1) w.p. 1/2.
    # The hop branch then goes on to qubit 2 at the later pair (1, 2) of the same op.
    circuit = stim.Circuit(
        """
        R 0 1 2
        I_ERROR[LEAKAGE_TRANSITION_1: (0.01, U-->2)] 0
        II_ERROR[LEAKAGE_TRANSITION_2: (0.5, 2_U-->U_3) (1.0, 3_U-->U_3)] 0 1 1 2
        MPAD[LEAKAGE_MEASUREMENT: (1.0, 2) (1.0, 3): 0 1 2] 0 0 0
        I_ERROR[LEAKAGE_TRANSITION_1: (1.0, 2-->U) (1.0, 3-->U)] 0 1 2
        M 0 1 2
        DETECTOR rec[-3]
        DETECTOR rec[-2]
        DETECTOR rec[-1]
        """
    )
    records = np.stack([_records(circuit, flag)[0] for flag in (0, 1, 2)])
    stay, on_1, on_2 = MarginalLeakageDemGenerator()(circuit, records)
    d0, d1, d2 = ((frozenset({d}), frozenset()) for d in range(3))
    assert _dem_dict(stay) == {d0: pytest.approx(0.5)}
    assert _dem_dict(on_1) == {}
    # Qubits 0 and 1 are depolarized when the leak leaves them, qubit 2 when it unleaks.
    assert _dem_dict(on_2) == {
        d0: pytest.approx(0.5),
        d1: pytest.approx(0.5),
        d2: pytest.approx(0.5),
    }


def test_conditioned_pair_gate_is_skipped_like_unconditional_mode():
    template = """
        R 0 1 2
        I_ERROR[LEAKAGE_TRANSITION_1: (0.01, U-->2)] 0
        {cx} 0 1
        MR 1
        DETECTOR rec[-1]
        MPAD[LEAKAGE_MEASUREMENT: (1.0, 2): 0] 0
        I_ERROR[LEAKAGE_TRANSITION_1: (1.0, 2-->U)] 0
    """
    conditioned = stim.Circuit(template.format(cx="CX[CONDITIONED_ON_PAIR: (U, U)]"))
    plain = stim.Circuit(template.format(cx="CX"))
    dem_conditioned = MarginalLeakageDemGenerator(False)(
        conditioned, _records(conditioned, 1)[0]
    )
    dem_plain = MarginalLeakageDemGenerator(True)(plain, _records(plain, 1)[0])
    assert _dem_dict(dem_conditioned) == _dem_dict(dem_plain)
    assert _dem_dict(dem_plain) == {(frozenset({0}), frozenset()): pytest.approx(0.5)}


def test_inverted_mpad_target_raises_flag_on_zero():
    circuit = stim.Circuit(
        """
        R 0 1
        I_ERROR[LEAKAGE_TRANSITION_1: (0.01, U-->2)] 0
        CX 0 1
        MR 1
        DETECTOR rec[-1]
        MPAD[LEAKAGE_MEASUREMENT: (1.0, 2): 0] 1
        """
    )
    gen = MarginalLeakageDemGenerator()
    unflagged, flagged = gen(circuit, np.array([[0, 1], [0, 0]], dtype=bool))
    assert _dem_dict(unflagged) == {}
    assert _dem_dict(flagged) == {(frozenset({0}), frozenset()): pytest.approx(0.5)}


def test_identical_shots_share_dems_and_1d_records_return_one_dem():
    gen = MarginalLeakageDemGenerator()
    records = _records(REP_CIRCUIT, shots=4)
    records[1:3, 1] = True
    dems = gen(REP_CIRCUIT, records)
    assert dems[0] is dems[3]
    assert dems[1] is dems[2]
    assert dems[0] is not dems[1]
    single = gen(REP_CIRCUIT, records[1])
    assert isinstance(single, stim.DetectorErrorModel)
    assert single == dems[1]


def test_records_with_wrong_shape_are_rejected():
    gen = MarginalLeakageDemGenerator()
    with pytest.raises(ValueError, match="Expected records"):
        gen(REP_CIRCUIT, np.zeros((2, REP_CIRCUIT.num_measurements + 1), dtype=bool))
    with pytest.raises(ValueError, match="Expected records"):
        gen(REP_CIRCUIT, np.zeros((1, 1, REP_CIRCUIT.num_measurements), dtype=bool))


@pytest.mark.parametrize(
    "line",
    [
        "I_ERROR[LEAKAGE_TRANSITION_1: (0.1, 1-->2)] 0",
        "II_ERROR[LEAKAGE_TRANSITION_2: (0.1, 0_U-->2_U)] 0 1",
        "X_ERROR[CONDITIONED_ON_SELF: 0 1](0.1) 0",
        "X_ERROR[CONDITIONED_ON_SELF: 1](0.1) 0",
    ],
)
def test_computational_state_dependent_tags_are_rejected(line):
    circuit = stim.Circuit(f"R 0 1\n{line}\nM 0\nDETECTOR rec[-1]")
    with pytest.raises(NotImplementedError, match="use 'U'"):
        MarginalLeakageDemGenerator()(circuit, np.zeros(1, dtype=bool))


@pytest.mark.parametrize("gate", ["SPP", "SPP_DAG"])
def test_spp_is_rejected_with_unconditional_condition_on_u(gate):
    # TablesideSimulator skips an SPP term that touches a leaked qubit (and FlipsideSimulator applies it to the
    # scrambled qubit); the generator models neither, so it refuses SPP rather than build a DEM missing them.
    circuit = stim.Circuit(
        f"R 0 1\nI_ERROR[LEAKAGE_TRANSITION_1: (0.1, U-->2)] 0\n{gate} Z0*X1 Z0*X1\nM 1\nDETECTOR rec[-1]"
    )
    with pytest.raises(NotImplementedError, match=f"does not support {gate} with unconditional_condition_on_U=True"):
        MarginalLeakageDemGenerator(unconditional_condition_on_U=True)(circuit, np.zeros(1, dtype=bool))
    MarginalLeakageDemGenerator(unconditional_condition_on_U=False)(circuit, np.zeros(1, dtype=bool))


def test_generator_pickles_after_use_and_recomputes_for_new_circuit():
    gen = MarginalLeakageDemGenerator()
    expected = gen(REP_CIRCUIT, _records(REP_CIRCUIT, 1)[0])
    clone = pickle.loads(pickle.dumps(gen))
    assert clone(REP_CIRCUIT, _records(REP_CIRCUIT, 1)[0]) == expected
    other = stim.Circuit("R 0\nX_ERROR(0.1) 0\nM 0\nDETECTOR rec[-1]")
    assert clone(other, np.zeros(1, dtype=bool)) == other.detector_error_model()


def test_simulated_flags_line_up_with_records():
    circuit = stim.Circuit(
        """
        R 0 1
        I_ERROR[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 0
        CX 0 1
        MR 1
        DETECTOR rec[-1]
        MPAD[LEAKAGE_MEASUREMENT: (1.0, 2): 0] 0
        """
    )
    sampler = TablesideSampler(
        op_handler=LeakageUint8(),
        batch_size=3,
        dem_decoder=MarginalDecoder(MarginalLeakageDemGenerator()),
    )
    compiled = sampler.compiled_sampler_for_task(
        sinter.Task(circuit=circuit, decoder="pymatching")
    )
    compiled.tab_simulator.clear()
    compiled.tab_simulator.run()
    records = compiled.tab_simulator.get_final_measurement_records()
    assert records[:, 1].all()
    dems = MarginalLeakageDemGenerator()(circuit, records)
    assert all(
        _dem_dict(dem) == {(frozenset({0}), frozenset()): pytest.approx(0.5)}
        for dem in dems
    )


def test_decode_with_generated_dems_uses_each_shots_dem():
    decoder = sinter.BUILT_IN_DECODERS["pymatching"]
    flips = stim.DetectorErrorModel("error(0.1) D0 L0")
    keeps = stim.DetectorErrorModel("error(0.1) D0\nlogical_observable L0")
    dets = np.packbits(np.ones((3, 1), dtype=bool), axis=1, bitorder="little")
    predictions = decode_with_generated_dems(decoder, [flips, keeps, flips], dets)
    assert predictions.tolist() == [[1], [0], [1]]
    assert decode_with_generated_dems(decoder, flips, dets).tolist() == [[1]] * 3
    with pytest.raises(ValueError, match="2 DEMs for a batch of 3"):
        decode_with_generated_dems(decoder, [flips, keeps], dets)


@pytest.mark.parametrize(
    "make_sampler",
    [
        lambda gen: TablesideSampler(op_handler=LeakageUint8(), batch_size=4, dem_decoder=_CallableDecoder(gen)),
        lambda gen: CosetsideSampler(
            op_handler=LeakageUint8Coset(), batch_size=16, dem_decoder=_CallableDecoder(gen)
        ),
        lambda gen: FlipsideSampler(
            op_handler=LeakageUint8Flip(), batch_size=64, dem_decoder=_CallableDecoder(gen)
        ),
    ],
    ids=["tableside", "cosetside", "flipside"],
)
def test_samplers_decode_with_per_shot_dems(make_sampler):
    circuit = REP_CIRCUIT.copy()
    calls = []
    gen = MarginalLeakageDemGenerator()

    def recording_gen(circ, records):
        dems = gen(circ, records)
        calls.append((records.shape, len(dems)))
        return dems

    sampler = make_sampler(recording_gen)
    compiled = sampler.compiled_sampler_for_task(
        sinter.Task(circuit=circuit, decoder="pymatching")
    )
    stats = compiled.sample(suggested_shots=32)
    assert stats.shots >= 32
    assert calls and all(shape[0] == n for shape, n in calls)


@pytest.mark.parametrize(
    "make_sampler",
    [
        lambda gen: TablesideSampler(op_handler=LeakageUint8(), batch_size=64, dem_decoder=_CallableDecoder(gen)),
        lambda gen: CosetsideSampler(
            op_handler=LeakageUint8Coset(), batch_size=64, dem_decoder=_CallableDecoder(gen)
        ),
        lambda gen: FlipsideSampler(
            op_handler=LeakageUint8Flip(), batch_size=64, dem_decoder=_CallableDecoder(gen)
        ),
    ],
    ids=["tableside", "cosetside", "flipside"],
)
def test_samplers_give_dem_gen_the_records_of_the_decoded_shots(make_sampler):
    # D0 and L0 both equal the random M 0. Choosing each shot's DEM from its own
    # M 0 record decodes every shot correctly, but not if the records belong to
    # other shots.
    circuit = stim.Circuit(
        "R 0\nX_ERROR(0.5) 0\nM 0\nDETECTOR rec[-1]\nOBSERVABLE_INCLUDE(0) rec[-1]"
    )
    flips = stim.DetectorErrorModel("error(0.1) D0 L0")
    keeps = stim.DetectorErrorModel("error(0.1) D0\nlogical_observable L0")

    def gen(circ, records):
        return [flips if row[0] else keeps for row in records]

    compiled = make_sampler(gen).compiled_sampler_for_task(
        sinter.Task(circuit=circuit, decoder="pymatching")
    )
    stats = compiled.sample(suggested_shots=256)
    assert stats.shots >= 256
    assert stats.errors == 0


def test_top_level_export_marginal_leakage_dem_generator():
    assert stimside.MarginalLeakageDemGenerator is MarginalLeakageDemGenerator
    assert "MarginalLeakageDemGenerator" in stimside.__all__


def test_pymatching_warns_on_undecomposed_hyperedges():
    circuit = stim.Circuit(
        """
        R 0 1 2
        I_ERROR[LEAKAGE_TRANSITION_1: (0.01, U-->2)] 0
        CX 0 1 0 2
        MR 1 2
        DETECTOR rec[-2]
        DETECTOR rec[-1]
        MPAD[LEAKAGE_MEASUREMENT: (1.0, 2): 0] 0
        I_ERROR[LEAKAGE_TRANSITION_1: (1.0, 2-->U)] 0
        M 0
        DETECTOR rec[-1]
        OBSERVABLE_INCLUDE(0) rec[-1]
        """
    )
    dem_undecomposed = MarginalLeakageDemGenerator()(circuit, _records(circuit, 2))
    decoder = sinter.BUILT_IN_DECODERS["pymatching"]
    dets = np.packbits(np.zeros((1, 3), dtype=bool), axis=1, bitorder="little")

    with pytest.warns(UserWarning, match="hyperedge"):
        try:
            decode_with_generated_dems(decoder, dem_undecomposed, dets)
        except Exception:
            pass


def test_decompose_errors_produces_matchable_dem():
    circuit = stim.Circuit(
        """
        R 0 1 2
        I_ERROR[LEAKAGE_TRANSITION_1: (0.01, U-->2)] 0
        CX 0 1 0 2
        MR 1 2
        DETECTOR rec[-2]
        DETECTOR rec[-1]
        MPAD[LEAKAGE_MEASUREMENT: (1.0, 2): 0] 0
        I_ERROR[LEAKAGE_TRANSITION_1: (1.0, 2-->U)] 0
        M 0
        DETECTOR rec[-1]
        OBSERVABLE_INCLUDE(0) rec[-1]
        """
    )
    gen = MarginalLeakageDemGenerator(decompose_errors=True)
    dem = gen(circuit, _records(circuit, 2)[0])
    assert isinstance(dem, stim.DetectorErrorModel)
    # Every component in the decomposed DEM has <= 2 detectors and is matchable by PyMatching
    m = pymatching.Matching.from_detector_error_model(dem)
    assert m.num_detectors == 3

    # Also verify passing decompose_errors=True to decode_with_generated_dems
    undecomposed = MarginalLeakageDemGenerator()(circuit, _records(circuit, 2))
    dets = np.packbits(np.array([[1, 1, 1]], dtype=bool), axis=1, bitorder="little")
    preds = decode_with_generated_dems(
        sinter.BUILT_IN_DECODERS["pymatching"],
        undecomposed,
        dets,
        decompose_errors=True,
    )
    assert preds.shape == (1, 1)


def test_reweight_only_produces_dem_update_and_decodes():
    gen = MarginalLeakageDemGenerator()
    records = _records(REP_CIRCUIT, shots=2)
    records[1, 1] = True  # shot 0 unflagged, shot 1 flag on qubit 0

    updates = gen(REP_CIRCUIT, records, reweight_only=True)
    assert len(updates) == 2
    assert isinstance(updates[0], stim.DetectorErrorModel)
    assert isinstance(updates[1], stim.DetectorErrorModel)
    # Shot 0 has no raised flags -> empty update DEM (does not copy baseline)
    assert len(updates[0]) == 0
    # Shot 1 has only the updated error mechanisms for qubit 0's envelope
    assert _dem_dict(updates[1]) == {
        (frozenset({0}), frozenset({0})): pytest.approx(0.5),
        (frozenset({1}), frozenset({0})): pytest.approx(0.5),
    }

    dets_bool = np.array([[1, 0], [0, 1]], dtype=bool)
    dets_packed = np.packbits(dets_bool, axis=1, bitorder="little")
    decoder = sinter.BUILT_IN_DECODERS["pymatching"]
    full_dems = gen(REP_CIRCUIT, records)
    expected_preds = decode_with_generated_dems(decoder, full_dems, dets_packed)
    actual_preds = decode_with_generated_dems(
        decoder, updates, dets_packed, reweight_only=True
    )
    assert np.array_equal(actual_preds, expected_preds)


def test_decompose_and_reweight_produces_pymatching_edge_reweights():
    gen = MarginalLeakageDemGenerator(decompose_errors=True, reweight_only=True)
    records = _records(REP_CIRCUIT, shots=3)
    records[1, 1] = True  # shot 1 flag on qubit 0
    records[2, 1] = True  # shot 2 identical to shot 1

    rw_batch = gen(REP_CIRCUIT, records)
    assert isinstance(rw_batch, list)
    assert len(rw_batch) == 3
    # Unflagged shot has empty (0, 3) float64 array
    assert isinstance(rw_batch[0], np.ndarray)
    assert rw_batch[0].shape == (0, 3)
    assert rw_batch[0].dtype == np.float64
    # Flagged shots share the same (num_reweights, 3) float64 array
    assert rw_batch[1] is rw_batch[2]
    assert rw_batch[1].ndim == 2 and rw_batch[1].shape[1] == 3
    assert rw_batch[1].dtype == np.float64
    # Boundary edges use -1 as second node and weight = log((1 - 0.5) / 0.5) = 0.0
    assert np.allclose(rw_batch[1], np.array([[0.0, -1.0, 0.0], [1.0, -1.0, 0.0]]))

    # 1D records return a single 2D array
    rw_single = gen(REP_CIRCUIT, records[1])
    assert isinstance(rw_single, np.ndarray)
    assert np.allclose(rw_single, rw_batch[1])

    # Direct compatibility with pymatching.Matching.decode and decode_batch edge_reweights
    base_dem = gen.base_dem(REP_CIRCUIT, decompose_errors=True)
    matcher = pymatching.Matching.from_detector_error_model(base_dem)
    syndrome_single = np.array([0, 1], dtype=np.uint8)
    corr_single = matcher.decode(syndrome_single, edge_reweights=rw_single)
    assert corr_single.tolist() == [1]

    syndromes = np.array([[0, 0], [0, 1], [1, 1]], dtype=np.uint8)
    corr_batch = matcher.decode_batch(syndromes, edge_reweights=rw_batch)
    assert corr_batch.shape == (3, 1)

    # Compatibility with decode_with_generated_dems
    dets_packed = np.packbits(syndromes, axis=1, bitorder="little")
    decoder = sinter.BUILT_IN_DECODERS["pymatching"]
    preds_from_rw = decode_with_generated_dems(decoder, rw_batch, dets_packed)
    full_dems = MarginalLeakageDemGenerator(decompose_errors=True)(REP_CIRCUIT, records)
    preds_from_full = decode_with_generated_dems(decoder, full_dems, dets_packed)
    assert np.array_equal(preds_from_rw, preds_from_full)

    # Also test passing undecomposed dems to decode_with_generated_dems with both flags True
    raw_dems = MarginalLeakageDemGenerator()(REP_CIRCUIT, records)
    preds_from_flags = decode_with_generated_dems(
        decoder, raw_dems, dets_packed, decompose_errors=True, reweight_only=True
    )
    assert np.array_equal(preds_from_flags, preds_from_full)


def _make_surface_code_leakage_circuit(distance: int, rounds: int) -> stim.Circuit:
    base_sc = stim.Circuit.generated(
        "surface_code:rotated_memory_z",
        distance=distance,
        rounds=rounds,
        after_clifford_depolarization=0.001,
        before_measure_flip_probability=0.001,
        after_reset_flip_probability=0.001,
    )
    orig_to_new_meas: list[int] = []
    sc = stim.Circuit()
    new_meas_count = 0
    for inst in base_sc.flattened():
        if inst.name in ("M", "MR", "MX", "MY", "MRX", "MRY"):
            n = len(inst.targets_copy())
            for _ in range(n):
                orig_to_new_meas.append(new_meas_count)
                new_meas_count += 1
            sc.append(inst)
            if inst.name == "MR":
                qs = [t.qubit_value for t in inst.targets_copy()]
                q_str = " ".join(str(q) for q in qs)
                sc.append(
                    "MPAD",
                    [0] * len(qs),
                    [],
                    tag=f"LEAKAGE_MEASUREMENT: (1.0, 2): {q_str}",
                )
                new_meas_count += len(qs)
                sc.append("I_ERROR", qs, [], tag="LEAKAGE_TRANSITION_1: (0.95, 2-->U)")
        elif inst.name in ("DETECTOR", "OBSERVABLE_INCLUDE"):
            cur_orig = len(orig_to_new_meas)
            new_targets = []
            for t in inst.targets_copy():
                if t.is_measurement_record_target:
                    orig_idx = cur_orig + t.value
                    new_idx = orig_to_new_meas[orig_idx]
                    new_targets.append(stim.target_rec(new_idx - new_meas_count))
                else:
                    new_targets.append(t)
            sc.append(inst.name, new_targets, inst.gate_args_copy())
        else:
            sc.append(inst)
            if inst.name == "DEPOLARIZE1":
                sc.append(
                    "I_ERROR",
                    inst.targets_copy(),
                    [],
                    tag="LEAKAGE_TRANSITION_1: (0.001, U-->2) (0.001, 2-->U)",
                )
            elif inst.name == "DEPOLARIZE2":
                sc.append(
                    "II_ERROR",
                    inst.targets_copy(),
                    [],
                    tag="LEAKAGE_TRANSITION_2: (0.001, U_U-->2_D) (0.0005, 2_U-->U_2) (0.0005, U_2-->2_U) (0.95, 2_U-->U_D) (0.95, U_2-->D_U)",
                )
    return sc


def test_surface_code_marginal_dem_cpp_speed_and_edge_reweights_equivalence():
    sc = _make_surface_code_leakage_circuit(distance=5, rounds=5)
    sampler = CosetsideSampler(
        op_handler=LeakageUint8Coset(),
        batch_size=64,
        dem_decoder=MarginalDecoder(
            MarginalLeakageDemGenerator(decompose_errors=True, reweight_only=True)
        ),
        seed=123,
    )
    compiled = sampler.compiled_sampler_for_task(
        sinter.Task(circuit=sc, decoder="pymatching")
    )
    compiled.simulator.clear()
    compiled.simulator.run()
    records = compiled.simulator.get_final_measurement_records()
    det_and_obs = compiled.simulator.get_detector_flips(append_observables=True)
    det_events = det_and_obs[:, : compiled.simulator.num_detectors]
    dets_packed = np.packbits(det_events, axis=1, bitorder="little")

    gen = MarginalLeakageDemGenerator()
    t0 = time.perf_counter()
    rw_list = gen(sc, records, decompose_errors=True, reweight_only=True)
    elapsed = time.perf_counter() - t0
    assert elapsed < 2.0  # C++ kernel + deduplicated propagation builds in ~0.25s

    full_dems = gen(sc, records, decompose_errors=True, reweight_only=False)
    decoder = sinter.BUILT_IN_DECODERS["pymatching"]
    preds_full = decode_with_generated_dems(decoder, full_dems, dets_packed)
    preds_rw = decode_with_generated_dems(decoder, rw_list, dets_packed)
    assert np.array_equal(preds_rw, preds_full)

    # Also verify direct pymatching.Matching.decode_batch with edge_reweights
    matcher = pymatching.Matching.from_detector_error_model(
        gen.base_dem(sc, decompose_errors=True)
    )
    preds_direct = matcher.decode_batch(
        dets_packed,
        bit_packed_shots=True,
        bit_packed_predictions=True,
        edge_reweights=rw_list,
    )
    assert np.array_equal(preds_direct, preds_full)


def _reference_update_dem(analysis, raised_row) -> stim.DetectorErrorModel:
    """Pure-Python oracle for _Analysis.generate_batch(reweight_only=True) on one shot."""
    from stimside.dem_generators.dem_generator_marginal import _sym_to_targets, _xor

    flags = np.flatnonzero(raised_row).tolist()
    raised = set(flags)
    combined: dict = {}
    for f in flags:
        preds = analysis.pred_flags[analysis.pred_offsets[f] : analysis.pred_offsets[f + 1]]
        if raised.intersection(preds.tolist()):
            continue
        for idx in range(analysis.flag_env_offsets[f], analysis.flag_env_offsets[f + 1]):
            sym = analysis.symptoms[int(analysis.flag_env_sym_ids[idx])]
            combined[sym] = _xor(combined.get(sym, 0.0), float(analysis.flag_env_probs[idx]))
    sym_index = {sym: idx for idx, sym in enumerate(analysis.symptoms)}
    update = stim.DetectorErrorModel()
    for sym, p in combined.items():
        if p > 0:
            p_tot = _xor(float(analysis.base_sym_probs[sym_index[sym]]), p)
            if p_tot > 0:
                update.append("error", p_tot, _sym_to_targets(sym))
    return update


def test_decomposed_compound_symptoms_and_reweight_update_match_exact_weights():
    sc = _make_surface_code_leakage_circuit(distance=3, rounds=3)
    gen = MarginalLeakageDemGenerator()
    sampler = CosetsideSampler(
        op_handler=LeakageUint8Coset(),
        batch_size=16,
        dem_decoder=MarginalDecoder(MarginalLeakageDemGenerator(decompose_errors=True)),
        seed=7,
    )
    compiled = sampler.compiled_sampler_for_task(
        sinter.Task(circuit=sc, decoder="pymatching")
    )
    compiled.simulator.clear()
    compiled.simulator.run()
    records = compiled.simulator.get_final_measurement_records()

    analysis = gen._get_analysis(sc, True)
    records = records.copy()
    records[0, analysis.flag_records[0]] = True
    records[1, analysis.flag_records[len(analysis.flag_records) // 2]] = True
    records[2, analysis.flag_records[0]] = True
    records[2, analysis.flag_records[-1]] = True
    base_dem = analysis.matching_base_dem
    full_dems = gen(sc, records, decompose_errors=True, reweight_only=False)
    rw_list = gen(sc, records, decompose_errors=True, reweight_only=True)
    raised = np.asarray(records[:, analysis.flag_records], dtype=bool) ^ analysis.flag_invert
    upd_dems = [_reference_update_dem(analysis, row) for row in raised]
    upd_dems_cpp = analysis.generate_batch(
        raised, decompose_errors=False, reweight_only=True
    )

    from stimside.dem_generators.dem_decoding import _apply_reweight_update_dem

    def _edge_weights_map(dem: stim.DetectorErrorModel) -> dict[tuple[int, int], float]:
        m = pymatching.Matching.from_detector_error_model(dem)
        out: dict[tuple[int, int], float] = {}
        for u, v, data in m.to_networkx().edges(data=True):
            n0 = int(u) if u is not None and u < m.num_detectors else -1
            n1 = int(v) if v is not None and v < m.num_detectors else -1
            key = (min(n0, n1), max(n0, n1)) if n0 >= 0 and n1 >= 0 else (max(n0, n1), -1)
            out[key] = float(data["weight"])
        return out

    base_weights = _edge_weights_map(base_dem)
    flagged_checked = 0
    for shot_idx in range(records.shape[0]):
        if rw_list[shot_idx].shape[0] == 0:
            continue
        flagged_checked += 1
        full_w = _edge_weights_map(full_dems[shot_idx])
        merged_dem = _apply_reweight_update_dem(base_dem, upd_dems[shot_idx])
        merged_w = _edge_weights_map(merged_dem)
        merged_cpp_dem = _apply_reweight_update_dem(base_dem, upd_dems_cpp[shot_idx])
        merged_cpp_w = _edge_weights_map(merged_cpp_dem)
        rw_w = dict(base_weights)
        for n0, n1, w in rw_list[shot_idx]:
            u, v = int(n0), int(n1)
            key = (min(u, v), max(u, v)) if u >= 0 and v >= 0 else (max(u, v), -1)
            rw_w[key] = float(w)
        assert full_w.keys() == merged_w.keys() == merged_cpp_w.keys() == rw_w.keys()
        for k in full_w:
            assert merged_w[k] == pytest.approx(full_w[k], rel=1e-12, abs=1e-12)
            assert merged_cpp_w[k] == pytest.approx(full_w[k], rel=1e-12, abs=1e-12)
            assert rw_w[k] == pytest.approx(full_w[k], rel=1e-12, abs=1e-12)
    assert flagged_checked >= 3


def test_decode_with_generated_dems_plain_reweight_sequence_preserves_baseline():
    base_dem = stim.DetectorErrorModel(
        "error(0.1) D0 L0\nerror(0.1) D1\nerror(0.1) D0 D1\n"
    )
    # Reweight update only updates D1; un-updated D0 L0 must retain p=0.1 from base_dem
    update_dem = stim.DetectorErrorModel("error(0.2) D1\n")
    dets = np.packbits(np.array([[1, 0]], dtype=bool), axis=1, bitorder="little")
    decoder = sinter.BUILT_IN_DECODERS["pymatching"]

    preds_seq = decode_with_generated_dems(
        decoder,
        [update_dem],
        dets,
        decompose_errors=True,
        reweight_only=True,
        base_dem=base_dem,
    )
    preds_single = decode_with_generated_dems(
        decoder,
        update_dem,
        dets,
        decompose_errors=True,
        reweight_only=True,
        base_dem=base_dem,
    )
    assert preds_seq.tolist() == [[1]]
    assert preds_single.tolist() == [[1]]


def test_decompose_dem_graphlike_uses_known_graphlike_edges():
    from stimside.dem_generators.dem_decoding import _decompose_dem_graphlike

    dem = stim.DetectorErrorModel(
        "error(0.01) D0 D2 L0\n"
        "error(0.01) D1 D3\n"
        "error(0.2) D0 D1 D2 D3 L0\n"
    )
    decomposed = _decompose_dem_graphlike(dem)
    # Must decompose D0 D1 D2 D3 L0 into existing graphlike edges (D0 D2 L0 ^ D1 D3),
    # not introduce non-existent edges (D0 D1, D2 D3).
    m = pymatching.Matching.from_detector_error_model(decomposed)
    edges = {
        (min(int(u), int(v)), max(int(u), int(v)))
        for u, v in m.to_networkx().edges()
    }
    assert edges == {(0, 2), (1, 3)}

    dets = np.packbits(np.array([[1, 0, 1, 0]], dtype=bool), axis=1, bitorder="little")
    preds = decode_with_generated_dems(
        sinter.BUILT_IN_DECODERS["pymatching"],
        [dem],
        dets,
        decompose_errors=True,
    )
    assert preds.tolist() == [[1]]


def test_single_shot_1d_outputs_preserve_metadata_in_decode_with_generated_dems():
    records = _records(REP_CIRCUIT, shots=2)
    records[1, 1] = True  # flag on qubit 0
    rec_1d = records[1]

    dets_2d = np.packbits(np.array([[0, 1]], dtype=bool), axis=1, bitorder="little")
    dets_1d = dets_2d[0]
    decoder = sinter.BUILT_IN_DECODERS["pymatching"]

    for dec in (False, True):
        for rew in (False, True):
            gen = MarginalLeakageDemGenerator(decompose_errors=dec, reweight_only=rew)
            single_out = gen(REP_CIRCUIT, rec_1d)
            preds_2d = decode_with_generated_dems(decoder, single_out, dets_2d)
            preds_1d = decode_with_generated_dems(decoder, single_out, dets_1d)
            assert preds_2d.tolist() == [[1]]
            assert preds_1d.tolist() == [[1]]


def test_baseline_hyperedge_decomposed_using_envelope_symptoms_and_cached_matcher():
    circuit = stim.Circuit(
        """
        R 0 1 2
        I_ERROR[LEAKAGE_TRANSITION_1: (0.01, U-->2) (0.08, U-->0)] 0
        CX 0 1 0 2
        M 1 2
        DETECTOR rec[-2]
        DETECTOR rec[-1]
        MPAD[LEAKAGE_MEASUREMENT: (1.0, 2): 0] 0
        DEPOLARIZE1[CONDITIONED_ON_SELF: 2](0.75) 0
        M 0
        DETECTOR rec[-1]
        OBSERVABLE_INCLUDE(0) rec[-1]
        """
    )
    gen = MarginalLeakageDemGenerator()
    records = np.array([[False, False, False, False], [False, False, True, False]])
    full_dems = gen(circuit, records, decompose_errors=True, reweight_only=False)
    rw_list = gen(circuit, records, decompose_errors=True, reweight_only=True)

    dets = np.array([[0], [7]], dtype=np.uint8)
    decoder = sinter.BUILT_IN_DECODERS["pymatching"]
    preds_full = decode_with_generated_dems(decoder, full_dems, dets)
    preds_rw = decode_with_generated_dems(decoder, rw_list, dets)
    np.testing.assert_array_equal(preds_full, preds_rw)

    # Verify _cached_matcher is populated and reused across calls
    assert rw_list.analysis._cached_matcher is not None
    cached_matcher = rw_list.analysis._cached_matcher
    decode_with_generated_dems(decoder, rw_list, dets)
    assert rw_list.analysis._cached_matcher is cached_matcher


def test_sliced_indexed_and_plain_list_dems_preserve_metadata_and_transform_when_dets_none():
    sc = _make_surface_code_leakage_circuit(distance=3, rounds=3)
    gen = MarginalLeakageDemGenerator()
    records = np.zeros((2, sc.num_measurements), dtype=bool)
    records[1, gen._get_analysis(sc, False).flag_records[0]] = True
    raw_dems = gen(sc, records)

    # Indexed item with bit_packed_dets=None and decompose_errors=True, reweight_only=True
    out_indexed = decode_with_generated_dems(
        "pymatching", raw_dems[1], None, decompose_errors=True, reweight_only=True
    )
    assert isinstance(out_indexed, np.ndarray)
    assert out_indexed.ndim == 2 and out_indexed.shape[1] == 3

    # Sliced _GeneratedDemList with bit_packed_dets=None
    out_sliced = decode_with_generated_dems(
        "pymatching", raw_dems[:1], None, decompose_errors=True, reweight_only=True
    )
    assert isinstance(out_sliced[0], np.ndarray)
    assert out_sliced[0].shape == (0, 3)

    # Plain list(raw_dems) uses generator's circuit-level decomposition
    out_plain = decode_with_generated_dems(
        "pymatching", list(raw_dems), None, decompose_errors=True, reweight_only=False
    )
    direct_dec = gen(sc, records, decompose_errors=True, reweight_only=False)
    assert out_plain == direct_dec

    # Custom DEM (not from MarginalLeakageDemGenerator) with bit_packed_dets=None
    custom_dem = stim.DetectorErrorModel(
        "error(0.1) D0 D1 D2 L0\nerror(0.05) D0 D1\nerror(0.05) D2 L0\n"
    )
    out_custom = decode_with_generated_dems(
        "pymatching", custom_dem, None, decompose_errors=True
    )
    assert isinstance(out_custom, stim.DetectorErrorModel)
    m = pymatching.Matching.from_detector_error_model(out_custom)
    assert m.num_detectors == 3


def test_non_pymatching_decoder_and_all_none_reweights():
    from stimside.dem_generators.dem_generator_marginal import _decompose_hyperedge_component

    class RecordingDecoder(sinter.Decoder):
        def __init__(self) -> None:
            self.compiled_dems: list[stim.DetectorErrorModel] = []

        def compile_decoder_for_dem(self, *, dem: stim.DetectorErrorModel):
            self.compiled_dems.append(dem)

            class _Compiled:
                def decode_shots_bit_packed(self, *, bit_packed_detection_event_data):
                    return np.ones(
                        (bit_packed_detection_event_data.shape[0], 1), dtype=np.uint8
                    )

            return _Compiled()

    sc = _make_surface_code_leakage_circuit(distance=3, rounds=3)
    gen = MarginalLeakageDemGenerator(decompose_errors=True, reweight_only=True)
    records = np.zeros((2, sc.num_measurements), dtype=bool)
    records[1, gen._get_analysis(sc, True).flag_records[0]] = True
    rw_batch = gen(sc, records)
    dets = np.zeros((2, (sc.num_detectors + 7) // 8), dtype=np.uint8)

    dec1 = RecordingDecoder()
    res1 = decode_with_generated_dems(dec1, list(rw_batch), dets)
    assert len(dec1.compiled_dems) > 0
    assert np.all(res1 == 1)

    dec2 = RecordingDecoder()
    res2 = decode_with_generated_dems(dec2, rw_batch[1], dets[:1])
    assert len(dec2.compiled_dems) == 1
    assert np.all(res2 == 1)

    dec3 = RecordingDecoder()
    raw_arr = np.array([[0.0, 1.0, 2.5]], dtype=np.float64)
    res3 = decode_with_generated_dems(
        dec3, [raw_arr, None], dets, base_dem=gen.base_dem(sc, decompose_errors=True)
    )
    assert len(dec3.compiled_dems) == 2
    assert np.all(res3 == 1)

    # All-None reweights sequence with PyMatching
    res_none = decode_with_generated_dems(
        "pymatching",
        [None, None],
        dets,
        base_dem=gen.base_dem(sc, decompose_errors=True),
    )
    assert res_none.shape == (2, 1)

    # Maximum-cover backtracking on >=4-detector hyperedges
    known = {
        (0, 1): ((0, 1), ()),
        (0, 2): ((0, 2), ()),
        (1, 3): ((1, 3), ()),
    }
    res_4 = _decompose_hyperedge_component((0, 1, 2, 3), (0,), known)
    assert {d for d, _ in res_4} == {(0, 2), (1, 3)}
    res_6 = _decompose_hyperedge_component((0, 1, 2, 3, 4, 5), (), known)
    assert {d for d, _ in res_6} == {(0, 2), (1, 3), (4, 5)}


@pytest.mark.parametrize("decompose_errors", [False, True])
@pytest.mark.parametrize("which", ["rep", "surface"])
def test_kept_candidates_reproduce_flag_envelopes(which, decompose_errors):
    import dataclasses

    from stimside.dem_generators.dem_generator_marginal import _Builder

    circuit = (
        REP_CIRCUIT if which == "rep" else _make_surface_code_leakage_circuit(3, 2)
    )
    plain = _Builder(circuit, True).build(decompose_errors=decompose_errors)
    kept = _Builder(circuit, True).build(
        decompose_errors=decompose_errors, keep_candidates=True
    )
    cand_names = {
        "flag_cand_offsets", "flag_cand_flows", "flag_cand_weights", "flow_site_offsets",
        "flow_site_ids", "entry_site_ids", "entry_sym_ids", "entry_probs",
    }
    for field in dataclasses.fields(plain):
        if field.name in cand_names:
            assert getattr(plain, field.name) is None
            assert getattr(kept, field.name) is not None
        elif field.compare:
            a, b = getattr(plain, field.name), getattr(kept, field.name)
            if isinstance(a, np.ndarray):
                np.testing.assert_array_equal(a, b)
            else:
                assert a == b, field.name

    def xor(p, q):
        return p + q - 2 * p * q

    site_syms: dict = {}
    for site, sym, p in zip(kept.entry_site_ids, kept.entry_sym_ids, kept.entry_probs):
        d = site_syms.setdefault(int(site), {})
        d[int(sym)] = xor(d.get(int(sym), 0.0), float(p))
    num_flags = len(kept.flag_env_offsets) - 1
    num_multi = 0
    for f in range(num_flags):
        c0, c1 = kept.flag_cand_offsets[f], kept.flag_cand_offsets[f + 1]
        num_multi += c1 - c0 > 1
        total = float(kept.flag_cand_weights[c0:c1].sum())
        env: dict = {}
        for k, w in zip(kept.flag_cand_flows[c0:c1], kept.flag_cand_weights[c0:c1]):
            flow: dict = {}
            s0, s1 = kept.flow_site_offsets[k], kept.flow_site_offsets[k + 1]
            for site in kept.flow_site_ids[s0:s1]:
                for sym, p in site_syms.get(int(site), {}).items():
                    flow[sym] = xor(flow.get(sym, 0.0), p)
            for sym, p in flow.items():
                env[sym] = env.get(sym, 0.0) + float(w) / total * p
        e0, e1 = kept.flag_env_offsets[f], kept.flag_env_offsets[f + 1]
        want = dict(zip(kept.flag_env_sym_ids[e0:e1].tolist(), kept.flag_env_probs[e0:e1]))
        got = {sym: p for sym, p in env.items() if p > 0}
        assert got.keys() == want.keys()
        for sym, p in want.items():
            assert got[sym] == pytest.approx(p, rel=1e-12, abs=1e-15)
    assert num_multi > 0 or which == "rep"


