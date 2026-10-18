import pickle
import time

import numpy as np
import pymatching  # type: ignore[import-untyped]
import pytest
import sinter  # type: ignore[import-untyped]
import stim  # type: ignore[import-untyped]

import stimside
from stimside.dem_generators import dem_generator_marginal
from stimside.dem_generators.bnb_decoder import _has_fork_matching
from stimside.dem_generators.dem_generator_marginal import (
    _KNOWN_CODE_TO_PAULI_ID,
    _PAULI_STRINGS,
    _PAULI_TO_ID,
    MarginalLeakageDemGenerator,
    _Builder,
)
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
from stimside.util.known_states import compute_known_states_uint8
from stimside.util.leakage_events import LeakageEvent

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


_LEAK0 = "I_ERROR[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 0"
_FLAG0 = "MPAD[LEAKAGE_MEASUREMENT: (1.0, 2): 0] 0"


def _skipped(gates: str, uc: bool) -> str:
    """Lines of 1Q gates on qubit 0 that a leaked qubit 0 skips (conditioned on U when not uc)."""
    return "\n".join(f"{g} 0" if uc else f"{g}[CONDITIONED_ON_SELF: U] 0" for g in gates.split())


@pytest.mark.parametrize("uc", [True, False])
@pytest.mark.parametrize(
    "prep, gates, measure, conservative",
    [
        # The skipped gates fix the leaked qubit's reference state: skipping them is applying them.
        ("R 0", "I", "M 0\nDETECTOR rec[-1]", False),
        ("R 0", "Z", "M 0\nDETECTOR rec[-1]", False),
        ("R 0", "S", "M 0\nDETECTOR rec[-1]", False),
        ("R 0", "S_DAG", "M 0\nDETECTOR rec[-1]", False),
        ("RX 0", "X", "MX 0\nDETECTOR rec[-1]", False),
        ("RX 0", "SQRT_X", "MX 0\nDETECTOR rec[-1]", False),
        ("RY 0", "Y", "MY 0\nDETECTOR rec[-1]", False),
        ("RY 0", "SQRT_Y", "MY 0\nDETECTOR rec[-1]", False),
        # Still conservative: a skipped flip, a skipped basis change, skips that cancel, and a
        # leaked qubit whose state is unknown (entangled).
        ("R 0", "X", "M 0\nDETECTOR rec[-1]", True),
        ("R 0", "H", "MX 0\nDETECTOR rec[-1]", True),
        ("R 0", "H H", "M 0\nDETECTOR rec[-1]", True),
        ("R 0 1\nH 0\nCX 0 1", "Z", "M 0 1\nDETECTOR rec[-1] rec[-2]", True),
    ],
)
def test_skipped_1q_gate_that_fixes_the_reference_state_adds_no_site(
    uc, prep, gates, measure, conservative
):
    circuit = stim.Circuit(f"{prep}\n{_LEAK0}\n{_skipped(gates, uc)}\n{measure}\n{_FLAG0}")
    records, dets = _simulate(circuit, uc)
    assert records[:, -1].all()
    if not conservative:
        assert not dets.any()
    expected = {(frozenset({0}), frozenset()): pytest.approx(0.5)} if conservative else {}
    for dem in MarginalLeakageDemGenerator(uc)(circuit, records):
        assert _dem_dict(dem) == expected


@pytest.mark.parametrize("uc", [True, False])
@pytest.mark.parametrize(
    "prep, noise, gate, measure",
    [
        ("R", "X_ERROR", "S", "M"),
        ("R", "Y_ERROR", "S_DAG", "M"),
        ("RX", "Z_ERROR", "SQRT_X", "MX"),
        ("RX", "Y_ERROR", "X", "MX"),
        ("RY", "X_ERROR", "SQRT_Y", "MY"),
    ],
)
def test_fixing_gate_skipped_after_pre_leak_pauli_noise_is_exact(uc, prep, noise, gate, measure):
    # The simulators apply the error, then freeze the leaked qubit, still an eigenstate of the
    # same Pauli, so the skipped gate acts trivially: the DEM is the base error(0.2) alone.
    circuit = stim.Circuit(
        f"{prep} 0\n{noise}(0.2) 0\n{_LEAK0}\n{_skipped(gate, uc)}\n{measure} 0\nDETECTOR rec[-1]\n"
        f"{_FLAG0}"
    )
    dem = MarginalLeakageDemGenerator(uc)(circuit, _records(circuit, 1)[0])
    assert _dem_dict(dem) == {(frozenset({0}), frozenset()): pytest.approx(0.2)}
    handler = LeakageUint8Coset(unconditional_condition_on_U=uc).compile_op_handler(
        circuit=circuit, batch_size=4096
    )
    sim = stimside.CosetsideSimulator(circuit, compiled_op_handler=handler, batch_size=4096, seed=3)
    sim.run()
    assert sim.get_detector_flips()[:, 0].mean() == pytest.approx(0.2, abs=0.03)


@pytest.mark.parametrize("uc", [True, False])
def test_skipped_cx_keeps_its_site_on_a_reference_known_control(uc):
    # The leaked control is |0> in the reference, but after a pre-leak X error it is frozen in
    # |1>, and the skipped CX then really misses an X on the target (M0 ^ M1 fires). Unlike a
    # 1Q gate, a CX acts trivially for one eigenstate only, so the site must stay.
    cx = "CX" if uc else "CX[CONDITIONED_ON_PAIR: (U, U)]"
    circuit = stim.Circuit(
        f"R 0 1\nX_ERROR(0.2) 0\n{_LEAK0}\n{cx} 0 1\nM 0 1\n"
        f"DETECTOR rec[-2]\nDETECTOR rec[-1]\nDETECTOR rec[-2] rec[-1]\n{_FLAG0}"
    )
    dem = MarginalLeakageDemGenerator(uc)(circuit, _records(circuit, 2)[0])
    parity = 0.0
    for (dets, _), p in _dem_dict(dem).items():
        if 2 in dets:
            parity = parity + p - 2 * parity * p
    assert parity == pytest.approx(0.5)
    handler = LeakageUint8Coset(unconditional_condition_on_U=uc).compile_op_handler(
        circuit=circuit, batch_size=1024
    )
    sim = stimside.CosetsideSimulator(circuit, compiled_op_handler=handler, batch_size=1024, seed=5)
    sim.run()
    assert sim.get_detector_flips()[:, 2].mean() > 0.1


def test_hop_branch_starts_with_an_unknown_frozen_state():
    # The leak hops onto qubit 1 (|0> in the reference) w.p. 1/2. A hop target's frozen state is
    # not taken from the reference, so the S it skips still desyncs it (conservative: D0 = M1 never
    # fires in the simulators).
    circuit = stim.Circuit(
        f"""
        R 0 1
        {_LEAK0}
        II_ERROR[LEAKAGE_TRANSITION_2: (0.5, 2_U-->U_2)] 0 1
        S 1
        M 1
        DETECTOR rec[-1]
        MPAD[LEAKAGE_MEASUREMENT: (1.0, 2): 1] 0
        """
    )
    dem = MarginalLeakageDemGenerator()(circuit, _records(circuit, 1)[0])
    assert _dem_dict(dem) == {(frozenset({0}), frozenset()): pytest.approx(0.5)}


@pytest.mark.parametrize("prep", ["R", "R\nX", "RX", "RX\nZ", "RY", "RY\nX"])
def test_known_state_codes_map_to_the_stabilizing_pauli(prep):
    circuit = stim.Circuit("\n".join(f"{line} 0" for line in prep.split("\n")))
    code = int(compute_known_states_uint8(circuit)[-1, 0])
    sim = stim.TableauSimulator()
    sim.do(circuit)
    assert sim.peek_observable_expectation(_PAULI_STRINGS[_KNOWN_CODE_TO_PAULI_ID[code]]) == 1


def test_source_known_state_of_a_qubit_absent_from_the_reference_is_plus_z():
    circuit = stim.Circuit(
        """
        RX 0
        I_ERROR[LEAKAGE_TRANSITION_1: (0.5, U-->2)] 0 2
        MPAD[LEAKAGE_MEASUREMENT: (1.0, 2): 0 2] 0 0
        """
    )
    builder = _Builder(circuit, True)
    builder.build()
    sources = builder._find_sources()
    builder._set_source_known_states(sources)
    assert [(s.qubit, s.tracked) for s in sources] == [(0, _PAULI_TO_ID["+X"]), (2, _PAULI_TO_ID["+Z"])]


def test_failed_known_state_analysis_leaves_sources_unknown(monkeypatch):
    def fail(*args, **kwargs):
        raise ValueError("Backprop known states disagreed with forward prop known states.")

    monkeypatch.setattr(dem_generator_marginal, "compute_known_states_uint8", fail)
    circuit = stim.Circuit(f"R 0\n{_LEAK0}\nS 0\nM 0\nDETECTOR rec[-1]\n{_FLAG0}")
    dem = MarginalLeakageDemGenerator()(circuit, _records(circuit, 1)[0])
    assert _dem_dict(dem) == {(frozenset({0}), frozenset()): pytest.approx(0.5)}


@pytest.mark.parametrize("loss_oracle", [False, True])
@pytest.mark.parametrize(
    "x, y, z", [(0.3, 0.35, 0.03), (0.1, 0.35, 0.01)]
)
def test_saturated_source_partner_channel_is_accepted(loss_oracle, x, y, z):
    # Every outcome that leaks qubit 0 hits qubit 1, so the source's partner channel given the
    # leak sums to 1, which float rounding must not push above 1 (stim rejects that).
    circuit = stim.Circuit(
        f"""
        R 0 1
        II_ERROR[LEAKAGE_TRANSITION_2: ({x}, U_U-->2_X) ({y}, U_U-->2_Y) ({z}, U_U-->2_Z)] 0 1
        M 1
        DETECTOR rec[-1]
        MPAD[LEAKAGE_MEASUREMENT: (1.0, 2): 0] 0
        """
    )
    gen = MarginalLeakageDemGenerator(loss_oracle=loss_oracle)
    if loss_oracle:
        dem = gen(circuit, _records(circuit)[0], leakage_events=[LeakageEvent(1, 0, 0, 2)])
    else:
        dem = gen(circuit, _records(circuit, 1)[0])
    assert _dem_dict(dem) == {(frozenset({0}), frozenset()): pytest.approx((x + y) / (x + y + z))}


_MODES = [(False, False), (True, False), (False, True), (True, True)]  # decompose_errors, reweight_only


@pytest.mark.parametrize("loss_oracle", [False, True])
@pytest.mark.parametrize("decompose_errors, reweight_only", _MODES)
@pytest.mark.parametrize("out", ["3", "2", "U"])
def test_leaked_key_partner_channel_rounded_above_1_is_accepted(out, decompose_errors, reweight_only, loss_oracle):
    # Every (2, U) outcome hits qubit 1, and 0.34 + 0.56 + 0.1 rounds above 1 (the tag parser's
    # compensated sum gives 1): stim rejects the partner channel unless it is clamped.
    circuit = stim.Circuit(
        f"""
        R 0 1
        {_LEAK0}
        II_ERROR[LEAKAGE_TRANSITION_2: (0.34, 2_U-->{out}_X) (0.56, 2_U-->{out}_Y) (0.1, 2_U-->{out}_Z)] 0 1
        MPAD[LEAKAGE_MEASUREMENT: (1.0, 2) (1.0, 3): 0] 0
        M 1
        DETECTOR rec[-1]
        """
    )
    events = [LeakageEvent(1, 0, 0, 2)] + ([LeakageEvent(2, 0, 2, 3 if out == "3" else 0)] if out != "2" else [])
    gen = MarginalLeakageDemGenerator(
        loss_oracle=loss_oracle, decompose_errors=decompose_errors, reweight_only=reweight_only
    )
    dem = gen(circuit, _records(circuit, 0)[0], **({"leakage_events": events} if loss_oracle else {}))
    if out != "U" and not reweight_only:
        assert _dem_dict(dem) == {(frozenset({0}), frozenset()): pytest.approx(0.9)}


@pytest.mark.parametrize("loss_oracle", [False, True])
@pytest.mark.parametrize("decompose_errors, reweight_only", _MODES)
def test_unleaked_channel_with_a_symptom_rounded_above_1_is_accepted(decompose_errors, reweight_only, loss_oracle):
    # Every (U, U) outcome flips M 1, and 0.34 + 0.56 + 0.1 rounds above 1: stim rejects the
    # baseline's error(p) unless the channel is clamped.
    circuit = stim.Circuit(
        """
        R 0 1
        II_ERROR[LEAKAGE_TRANSITION_2: (0.34, U_U-->U_X) (0.56, U_U-->U_Y) (0.1, U_U-->X_X)] 0 1
        M 1
        DETECTOR rec[-1]
        """
    )
    gen = MarginalLeakageDemGenerator(
        loss_oracle=loss_oracle, decompose_errors=decompose_errors, reweight_only=reweight_only
    )
    dem = gen(circuit, _records(circuit)[0], **({"leakage_events": []} if loss_oracle else {}))
    if not reweight_only:
        assert _dem_dict(dem) == {(frozenset({0}), frozenset()): pytest.approx(1.0)}


def test_unleaked_channel_rounded_above_1_that_stim_accepts_is_unchanged():
    # The channel also sums above 1, but only X and Y flip M 1, so stim accepts it as it is.
    circuit = stim.Circuit(
        """
        R 0 1
        II_ERROR[LEAKAGE_TRANSITION_2: (0.34, U_U-->U_X) (0.56, U_U-->U_Y) (0.1, U_U-->U_Z)] 0 1
        M 1
        DETECTOR rec[-1]
        """
    )
    dem = MarginalLeakageDemGenerator()(circuit, _records(circuit)[0])
    assert [inst.args_copy()[0] for inst in dem.flattened() if inst.type == "error"] == [0.34 + 0.56]


def _certain_symptom_args(out) -> list:
    if isinstance(out, np.ndarray):  # decompose_errors and reweight_only: edge reweights
        return out.tolist()
    return [inst.args_copy() for inst in out.flattened() if inst.type == "error"]


@pytest.mark.parametrize("decompose_errors, reweight_only", _MODES)
@pytest.mark.parametrize("a, b", [(0.02, 0.17), (0.03, 0.08)])
def test_shot_probability_mixed_from_certain_entries_is_clamped_to_1(a, b, decompose_errors, reweight_only):
    # The flag's two candidate flows (one per source) both flip D0 for sure (a certain X on qubit
    # 1), and their normalized weights sum a few ulps above 1: stim rejects that error(p).
    circuit = stim.Circuit(
        f"""
        R 0 1
        II_ERROR[LEAKAGE_TRANSITION_2: ({a}, U_U-->2_X)] 0 1
        II_ERROR[LEAKAGE_TRANSITION_2: ({b}, U_U-->2_X)] 0 1
        M 1
        DETECTOR rec[-1]
        MPAD[LEAKAGE_MEASUREMENT: (1.0, 2): 0] 0
        """
    )
    gen = MarginalLeakageDemGenerator(decompose_errors=decompose_errors, reweight_only=reweight_only)
    want = [[0.0, -1.0, 0.0]] if decompose_errors and reweight_only else [[1.0]]
    assert _certain_symptom_args(gen(circuit, _records(circuit, 1)[0])) == want


@pytest.mark.parametrize("decompose_errors, reweight_only", _MODES)
@pytest.mark.parametrize("a, b", [(0.06, 0.08), (0.18, 0.1)])
def test_oracle_shot_probability_mixed_from_certain_entries_is_clamped_to_1(a, b, decompose_errors, reweight_only):
    # The source's root flow and its two leaked-state branches all flip D0 for sure, and their
    # weights sum a few ulps above 1.
    circuit = stim.Circuit(
        f"""
        R 0 1
        II_ERROR[LEAKAGE_TRANSITION_2: (1.0, U_U-->2_X)] 0 1
        I_ERROR[LEAKAGE_TRANSITION_1: ({a}, 2-->3) ({b}, 2-->4)] 0
        M 1
        DETECTOR rec[-1]
        """
    )
    gen = MarginalLeakageDemGenerator(
        loss_oracle=True, decompose_errors=decompose_errors, reweight_only=reweight_only
    )
    out = gen(circuit, _records(circuit)[0], leakage_events=[LeakageEvent(1, 0, 0, 2)])
    want = [[0.0, -1.0, 0.0]] if decompose_errors and reweight_only else [[1.0]]
    assert _certain_symptom_args(out) == want


def _spread_circuit(legs: str, cx: str, flagged: str) -> stim.Circuit:
    """Qubit 0 leaks for sure; w.p. 1/2 it spreads onto qubit 1 (which has no other leak source).

    A leaked qubit skips its CX: D0 = M1 ^ M2 is random iff qubit 1 leaked, D1 = M0 ^ M3 always.
    """
    lt2 = "(0.5, 2_U-->2_2)] 0 1" if legs == "01" else "(0.5, U_2-->2_2)] 1 0"
    zeros = " ".join("0" for _ in flagged.split())
    return stim.Circuit(
        f"""
        RX 0 1
        R 2 3
        {_LEAK0}
        II_ERROR[LEAKAGE_TRANSITION_2: {lt2}
        {cx} 1 2 0 3
        M 1 2 0 3
        DETECTOR rec[-3] rec[-4]
        DETECTOR rec[-1] rec[-2]
        MPAD[LEAKAGE_MEASUREMENT: (1.0, 2): {flagged}] {zeros}
        """
    )


@pytest.mark.parametrize("uc", [True, False])
@pytest.mark.parametrize("legs", ["01", "10"])
@pytest.mark.parametrize("flagged", ["0 1", "0"])
def test_spread_partner_is_traced_as_a_new_leak(uc, legs, flagged):
    cx = "CX" if uc else "CX[CONDITIONED_ON_PAIR: (U, U)]"
    circuit = _spread_circuit(legs, cx, flagged)
    shots = 4096
    handler = LeakageUint8Coset(unconditional_condition_on_U=uc).compile_op_handler(
        circuit=circuit, batch_size=shots
    )
    sim = stimside.CosetsideSimulator(
        circuit, compiled_op_handler=handler, batch_size=shots, seed=11, record_leakage_events=True
    )
    sim.run()
    records = np.asarray(sim.get_final_measurement_records(), dtype=bool)
    dets = sim.get_detector_flips()
    events = sim.get_leakage_events()
    spread = np.array([any(ev.qubit == 1 and ev.new_state >= 2 for ev in shot) for shot in events])
    assert 0.4 < spread.mean() < 0.6
    weighted = MarginalLeakageDemGenerator(uc)(circuit, records)
    oracle = MarginalLeakageDemGenerator(uc, loss_oracle=True)(circuit, records, leakage_events=events)
    d0, d1 = (frozenset({0}), frozenset()), (frozenset({1}), frozenset())
    both = {d0: pytest.approx(0.5), d1: pytest.approx(0.5)}
    for is_spread, group in ((True, spread), (False, ~spread)):
        assert abs(dets[group, 0].mean() - (0.5 if is_spread else 0.0)) < 0.05
        assert abs(dets[group, 1].mean() - 0.5) < 0.05
        shot = int(np.flatnonzero(group)[0])
        # The loss oracle conditions on the spread event (a source of its own): exact per shot.
        assert _dem_dict(oracle[shot]) == (both if is_spread else {d1: pytest.approx(0.5)})
        # The weighted model sees the partner's leak iff a flag on the partner follows (like
        # any leak); here its only candidate is the spread's flow.
        sees = is_spread and flagged == "0 1"
        assert _dem_dict(weighted[shot]) == (both if sees else {d1: pytest.approx(0.5)})


@pytest.mark.parametrize("uc", [True, False])
@pytest.mark.parametrize("legs", ["01", "10"])
def test_spread_onto_a_partner_with_its_own_leak_source(uc, legs):
    # Qubit 1 leaks either directly, before CX 1 4, which it then skips (D2 = M1 ^ M4 random), or
    # by the spread, after it (D2 = 0). With both flags raised, the weighted DEM mixes qubit 1's
    # candidate flows (C gives the DEM it gives without spreads) and the loss oracle matches either
    # leak as qubit 1's source: both cover every simulated detector (conservatively for D2).
    cx = "CX" if uc else "CX[CONDITIONED_ON_PAIR: (U, U)]"
    lt2 = "(0.5, 2_U-->2_2)] 0 1" if legs == "01" else "(0.5, U_2-->2_2)] 1 0"
    circuit = stim.Circuit(
        f"""
        RX 0 1
        R 2 3 4
        I_ERROR[LEAKAGE_TRANSITION_1: (0.3, U-->2)] 0 1
        {cx} 1 4
        II_ERROR[LEAKAGE_TRANSITION_2: {lt2}
        {cx} 1 2 0 3
        M 1 2 0 3 4
        DETECTOR rec[-4] rec[-5]
        DETECTOR rec[-2] rec[-3]
        DETECTOR rec[-1] rec[-5]
        MPAD[LEAKAGE_MEASUREMENT: (1.0, 2): 0 1] 0 0
        """
    )
    shots = 4096
    handler = LeakageUint8Coset(unconditional_condition_on_U=uc).compile_op_handler(
        circuit=circuit, batch_size=shots
    )
    sim = stimside.CosetsideSimulator(
        circuit, compiled_op_handler=handler, batch_size=shots, seed=13, record_leakage_events=True
    )
    sim.run()
    records = np.asarray(sim.get_final_measurement_records(), dtype=bool)
    dets = sim.get_detector_flips()
    events = sim.get_leakage_events()
    both_flagged = records[:, 5] & records[:, 6]
    by_spread = np.array(
        [any(e.qubit == 1 and e.op_index == 4 and e.new_state >= 2 for e in s) for s in events]
    )
    weighted = MarginalLeakageDemGenerator(uc)(circuit, records)
    oracle = MarginalLeakageDemGenerator(uc, loss_oracle=True)(circuit, records, leakage_events=events)

    def marginals(dem) -> list:
        p = [0.0] * 3
        for (dets_, _), q in _dem_dict(dem).items():
            for d in dets_:
                p[d] = p[d] + q - 2 * p[d] * q
        return p

    for spread, d2 in ((True, 0.0), (False, 0.5)):
        group = both_flagged & (by_spread == spread)
        assert group.sum() > 200
        assert abs(dets[group, 0].mean() - 0.5) < 0.08
        assert abs(dets[group, 2].mean() - d2) < 0.08
        shot = int(np.flatnonzero(group)[0])
        assert marginals(weighted[shot]) == pytest.approx([0.5, 0.5, 0.5])
        assert marginals(oracle[shot]) == pytest.approx([0.5, 0.5, 0.5])


def test_spread_from_a_hop_branch_is_not_seeded():
    # Only a source's root flow seeds a spread partner's flow (one level, like hops). Here the leak
    # first hops onto qubit 1 (w.p. 1/2), so qubit 2's flag has no candidate flow and the weighted
    # DEM misses qubit 2's leak; the loss oracle matches the spread from the shot's events.
    circuit = stim.Circuit(
        f"""
        R 0 1 3
        RX 2
        {_LEAK0}
        II_ERROR[LEAKAGE_TRANSITION_2: (0.5, 2_U-->U_2)] 0 1
        II_ERROR[LEAKAGE_TRANSITION_2: (1.0, 2_U-->2_2)] 1 2
        CX 2 3
        M 2 3
        DETECTOR rec[-1] rec[-2]
        MPAD[LEAKAGE_MEASUREMENT: (1.0, 2): 2] 0
        """
    )
    flagged = _records(circuit, 2)[0]
    assert _dem_dict(MarginalLeakageDemGenerator()(circuit, flagged)) == {}
    events = [
        LeakageEvent(2, 0, 0, 2),
        LeakageEvent(3, 0, 2, 0),
        LeakageEvent(3, 1, 0, 2),
        LeakageEvent(4, 2, 0, 2),
    ]
    oracle = MarginalLeakageDemGenerator(loss_oracle=True)(circuit, flagged, leakage_events=events)
    assert _dem_dict(oracle) == {(frozenset({0}), frozenset()): pytest.approx(0.5)}


def test_spread_free_circuit_dems_are_byte_identical():
    # The exact DEM texts from before spreads were modelled, for a circuit with hops and
    # leaked-state branches but no spread outcome.
    circuit = stim.Circuit(
        """
        R 0 1 2
        I_ERROR[LEAKAGE_TRANSITION_1: (0.01, U-->2)] 0
        II_ERROR[LEAKAGE_TRANSITION_2: (0.25, 2_U-->U_2)] 0 1
        I_ERROR[LEAKAGE_TRANSITION_1: (0.5, 2-->U) (0.25, 2-->3)] 0
        II_ERROR[LEAKAGE_TRANSITION_2: (0.25, 2_U-->U_2) (0.25, 2_U-->U_3)] 0 2
        CX 0 1 2 1
        MPAD[LEAKAGE_MEASUREMENT: (1.0, 2) (1.0, 3): 0 1 2] 0 0 0
        M 0 1 2
        DETECTOR rec[-1]
        DETECTOR rec[-2]
        DETECTOR rec[-3]
        """
    )
    records = _records(circuit, 0, 1, 2)[0]
    head = "detector D0\ndetector D1\ndetector D2\n"
    assert str(MarginalLeakageDemGenerator()(circuit, records)) == head + (
        "error(0.5) D0\nerror(0.5) D0 D1\nerror(0.5) D1\nerror(0.5) D1 D2\nerror(0.5) D2"
    )
    oracle = MarginalLeakageDemGenerator(loss_oracle=True)(
        circuit, records, leakage_events=[LeakageEvent(1, 0, 0, 2)]
    )
    assert str(oracle) == head + (
        "error(0.046875) D0\nerror(0.046875) D0 D1\nerror(0.125) D1\nerror(0.5) D1 D2\n"
        "error(0.328125) D2"
    )


_REUSE_TEMPLATE = """
    R 0 1 2
    X 1
    I_ERROR[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 1
    {swaps}
    M 0
    DETECTOR rec[-1]
    MPAD[LEAKAGE_MEASUREMENT: (1.0, 2): 1] 0
"""


def _fused_and_split(gate: str, pairs: str) -> tuple[stim.Circuit, stim.Circuit]:
    """The pairs as one instruction, and as one instruction per pair separated by TICK."""
    targets = pairs.split()
    split = "\nTICK\n".join(f"{gate} {a} {b}" for a, b in zip(targets[::2], targets[1::2]))
    return (
        stim.Circuit(_REUSE_TEMPLATE.format(swaps=f"{gate} {pairs}")),
        stim.Circuit(_REUSE_TEMPLATE.format(swaps=split)),
    )


@pytest.mark.parametrize("loss_oracle", [False, True])
@pytest.mark.parametrize(
    "gate, uc",
    [
        ("SWAP", True),
        ("ISWAP", True),
        ("SWAP[CONDITIONED_ON_PAIR: (U, U)]", True),
        ("SWAP[CONDITIONED_ON_PAIR: (U, U)]", False),
    ],
)
def test_skipped_pair_whose_partner_a_later_pair_reuses_raises(gate, uc, loss_oracle):
    # Leaked qubit 1 skips the pair (1, 2), and the later pair (2, 0) acts on qubit 2 as the skip
    # left it, so qubit 0 ends up in |0> (D0 always fires in the simulators). Placed after the
    # whole instruction, the skipped pair's depolarization would miss qubit 0 (D0 never fires).
    fused, split = _fused_and_split(gate, "1 2 2 0")
    gen = MarginalLeakageDemGenerator(uc, loss_oracle=loss_oracle)
    kwargs = {"leakage_events": [LeakageEvent(2, 1, 0, 2)]} if loss_oracle else {}
    with pytest.raises(NotImplementedError, match="Separate the pairs with TICK"):
        gen(fused, _records(fused, 1)[0], **kwargs)
    dem = gen(split, _records(split, 1)[0], **kwargs)
    assert _dem_dict(dem) == {(frozenset({0}), frozenset()): pytest.approx(0.5)}


@pytest.mark.parametrize(
    "gate, pairs, uc",
    [
        ("CX", "1 2 2 0", True),  # a controlled Pauli: the site is on the leaked qubit, before
        ("II", "1 2 2 0", True),  # the identity spreads nothing
        ("SWAP", "1 2 1 0", True),  # the later pair reuses the leaked qubit, and is skipped too
        ("SWAP", "2 0 1 2", True),  # the reused qubit's pair comes first
        ("SWAP", "1 2 2 0", False),  # untagged without uc: applied to the frozen state
    ],
)
def test_reused_qubit_that_does_not_change_the_dem_does_not_raise(gate, pairs, uc):
    fused, split = _fused_and_split(gate, pairs)
    gen = MarginalLeakageDemGenerator(uc)
    assert _dem_dict(gen(fused, _records(fused, 1)[0])) == _dem_dict(gen(split, _records(split, 1)[0]))


_CONTROLLED_REUSE_TEMPLATE = """
    RX 0
    RY 2 3
    I_ERROR[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 0
    TICK
    SQRT_Y 2
    TICK
    {pre}
    TICK
    {pairs}
    TICK
    MPAD[LEAKAGE_MEASUREMENT: (1.0, 2): 0] 0
    {tail}
"""


def _controlled_fused_and_split(gate, pairs, pre="", tail="MY 2\nDETECTOR rec[-1]"):
    targets = pairs.split()
    split = "\nTICK\n".join(f"{gate} {a} {b}" for a, b in zip(targets[::2], targets[1::2]))
    return tuple(
        stim.Circuit(_CONTROLLED_REUSE_TEMPLATE.format(pre=pre, pairs=p, tail=tail))
        for p in (f"{gate} {pairs}", split)
    )


@pytest.mark.parametrize("loss_oracle", [False, True])
@pytest.mark.parametrize("gate, uc", [("CX", True), ("CX[CONDITIONED_ON_PAIR: (U, U)]", False)])
def test_controlled_pauli_reusing_the_leaked_qubit_with_another_control_basis_raises(gate, uc, loss_oracle):
    # Leaked qubit 0 skips CX 2 0 (target), CX 0 3 (control) and CX 3 0 (target). Each change of
    # control basis depolarizes it again, but the fused instruction has one site, before it, so its
    # DEM correlates D0 and D1 (D2 never fires) while the simulators give D2 = 0.5.
    tail = "MY 2\nMX 3\nDETECTOR rec[-2]\nDETECTOR rec[-1]\nDETECTOR rec[-2] rec[-1]"
    fused, split = _controlled_fused_and_split(gate, "2 0 0 3 3 0", tail=tail)
    gen = MarginalLeakageDemGenerator(uc, loss_oracle=loss_oracle)
    kwargs = {"leakage_events": [LeakageEvent(2, 0, 0, 2)]} if loss_oracle else {}
    with pytest.raises(NotImplementedError, match="Separate the pairs with TICK"):
        gen(fused, _records(fused, 0)[0], **kwargs)
    assert _dem_dict(gen(split, _records(split, 0)[0], **kwargs)) == {
        (frozenset({0, 1}), frozenset()): pytest.approx(0.5),
        (frozenset({1, 2}), frozenset()): pytest.approx(0.5),
    }


def test_controlled_pauli_reuse_after_a_reset_of_the_leaked_qubit_raises():
    # After `R 0` the leaked qubit is tracked in |0>, so CX 0 2 is skipped harmlessly and only
    # CX 3 0 depolarizes it; the fused instruction's site is before CX 0 2, which would spread its
    # X error to qubit 2 (D0 = 0.5 instead of 0).
    fused, split = _controlled_fused_and_split("CX", "0 2 3 0", pre="H 3\nTICK\nR 0")
    gen = MarginalLeakageDemGenerator(True)
    with pytest.raises(NotImplementedError, match="Separate the pairs with TICK"):
        gen(fused, _records(fused, 0)[0])
    assert _dem_dict(gen(split, _records(split, 0)[0])) == {}


def test_controlled_pauli_reuse_that_does_not_change_the_dem_does_not_raise():
    # The leaked qubit's first pair always depolarizes it (its tracked state comes from the
    # reference), so the site before the instruction is where the separated pairs put the first
    # depolarization, and the second one (before CX 0 3) adds no symptom.
    fused, split = _controlled_fused_and_split("CX", "2 0 0 3", pre="H 3")
    gen = MarginalLeakageDemGenerator(True)
    dem = _dem_dict(gen(fused, _records(fused, 0)[0]))
    assert dem == _dem_dict(gen(split, _records(split, 0)[0]))
    assert dem == {(frozenset({0}), frozenset()): pytest.approx(0.5)}


@pytest.mark.parametrize(
    "gate, pairs, pre, tail, separated",
    [
        # (2, U) makes SWAP 0 3 act on the leaked qubit, whose frozen state differs from the
        # reference's because it skipped SWAP 2 0; depolarized after the whole instruction, the fused
        # DEM misses D0 = MY 3 (random in the simulators).
        ("SWAP", "2 0 0 3", "", "MY 3\nDETECTOR rec[-1]", {(frozenset({0}), frozenset()): 0.5}),
        # The same condition on CX 2 0 0 3 gives the same DEM fused or separated: no error.
        ("CX", "2 0 0 3", "H 3", "MY 2\nDETECTOR rec[-1]", None),
    ],
)
def test_reuse_under_a_condition_that_fires_on_a_leaked_qubit_raises_iff_the_dem_changes(
    gate, pairs, pre, tail, separated
):
    gate = f"{gate}[CONDITIONED_ON_PAIR: (U, U) (2, U)]"
    fused, split = _controlled_fused_and_split(gate, pairs, pre=pre, tail=tail)
    gen = MarginalLeakageDemGenerator(True)
    split_dem = _dem_dict(gen(split, _records(split, 0)[0]))
    if separated is None:
        assert _dem_dict(gen(fused, _records(fused, 0)[0])) == split_dem
        return
    assert split_dem == {key: pytest.approx(p) for key, p in separated.items()}
    with pytest.raises(NotImplementedError, match="Separate the pairs with TICK"):
        gen(fused, _records(fused, 0)[0])


@pytest.mark.parametrize("loss_oracle", [False, True])
def test_repeated_pair_of_an_entangling_gate_raises(loss_oracle):
    # Leaked qubit 0 skips both ISWAP 0 1. Separated, the reference applies the second ISWAP after
    # the first skip's depolarization, which gives the mechanism D0 D1 (MX 0 and M 1 together);
    # the fused instruction puts both depolarizations after it and lacks that mechanism (the
    # syndrome distribution is the same, the DEM is not).
    def circuit(gates: str) -> stim.Circuit:
        return stim.Circuit(
            f"""
            RX 0
            R 1
            {_LEAK0}
            {gates}
            {_FLAG0}
            MX 0
            M 1
            DETECTOR rec[-2]
            DETECTOR rec[-1]
            DETECTOR rec[-2] rec[-1]
            """
        )

    fused, split = circuit("ISWAP 0 1 0 1"), circuit("ISWAP 0 1\nTICK\nISWAP 0 1")
    gen = MarginalLeakageDemGenerator(loss_oracle=loss_oracle)
    kwargs = {"leakage_events": [LeakageEvent(2, 0, 0, 2)]} if loss_oracle else {}
    with pytest.raises(NotImplementedError, match="Separate the pairs with TICK"):
        gen(fused, _records(fused, 0)[0], **kwargs)
    assert _dem_dict(gen(split, _records(split, 0)[0], **kwargs)) == {
        (frozenset({0, 1}), frozenset()): pytest.approx(0.5),
        (frozenset({0, 2}), frozenset()): pytest.approx(0.5),
        (frozenset({1, 2}), frozenset()): pytest.approx(0.5),
    }


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


def _random_state_circuit(
    transition: str,
    uc: bool,
    flags: str = "MPAD[LEAKAGE_MEASUREMENT: (1.0, 2) (1.0, 3): 0] 0",
    unleak: int = 3,
    flags_first: bool = False,
) -> stim.Circuit:
    """Qubit 0 leaks to state 2 for sure, then makes `transition`.

    State `unleak` then unleaks (depolarized) before the CX, which makes D0 = M0 ^ M1
    deterministic again; a qubit still leaked at the CX skips it, and D0 is random.
    """
    cx = "CX" if uc else "CX[CONDITIONED_ON_PAIR: (U, U)]"
    return stim.Circuit(
        f"""
        RX 0
        R 1 2
        I_ERROR[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 0
        {flags if flags_first else ""}
        {transition}
        {"" if flags_first else flags}
        I_ERROR[LEAKAGE_TRANSITION_1: (1.0, {unleak}-->U)] 0
        {cx} 0 1
        M 0 1
        DETECTOR rec[-1] rec[-2]
        """
    )


@pytest.mark.parametrize("uc", [True, False])
@pytest.mark.parametrize(
    "transition, in_2",
    [
        # The flag heralds both states, so given it the leak is still in 2 w.p. in_2.
        ("I_ERROR[LEAKAGE_TRANSITION_1: (0.25, 2-->3)] 0", 0.75),
        ("I_ERROR[LEAKAGE_TRANSITION_1: (0.25, 2<->3)] 0", 0.75),
        ("I_ERROR[LEAKAGE_TRANSITION_1: (0.25, 2-->3) (0.5, 2-->U)] 0", 0.5),
        # Certain once the qubit stays leaked, like (1.0, 2-->3).
        ("I_ERROR[LEAKAGE_TRANSITION_1: (0.5, 2-->3) (0.5, 2-->U)] 0", 0.0),
        ("I_ERROR[LEAKAGE_TRANSITION_1: (1.0, 2-->3)] 0", 0.0),
        # A LEAKAGE_TRANSITION_2 that changes the leaked leg's state, also in a spread
        # (qubit 2's new leak seeds a flow of its own, but no flag on qubit 2 explains it).
        ("II_ERROR[LEAKAGE_TRANSITION_2: (0.25, 2_U-->3_U)] 0 2", 0.75),
        ("II_ERROR[LEAKAGE_TRANSITION_2: (0.25, U_2-->U_3)] 2 0", 0.75),
        ("II_ERROR[LEAKAGE_TRANSITION_2: (0.25, 2_U<->3_U)] 0 2", 0.75),
        ("II_ERROR[LEAKAGE_TRANSITION_2: (0.25, 2_U-->3_2)] 0 2", 0.75),
    ],
)
def test_random_next_leaked_state_is_followed_on_a_branch(uc, transition, in_2):
    circuit = _random_state_circuit(transition, uc)
    dem = MarginalLeakageDemGenerator(uc)(circuit, _records(circuit, 0)[0])
    expected = {(frozenset({0}), frozenset()): pytest.approx(0.5 * in_2)} if in_2 else {}
    assert _dem_dict(dem) == expected


@pytest.mark.parametrize("uc", [True, False])
def test_flags_heralding_one_leaked_state_are_explained_by_its_branch(uc):
    # State 2 unleaks before the CX (D0 = 0); state 3 is still leaked there (D0 random).
    transition = "I_ERROR[LEAKAGE_TRANSITION_1: (0.25, 2-->3)] 0"
    flags = "MPAD[LEAKAGE_MEASUREMENT: (1.0, 2): 0] 0\nMPAD[LEAKAGE_MEASUREMENT: (1.0, 3): 0] 0"
    circuit = _random_state_circuit(transition, uc, flags=flags, unleak=2)
    in_2, in_3 = MarginalLeakageDemGenerator(uc)(
        circuit, np.stack([_records(circuit, 0)[0], _records(circuit, 1)[0]])
    )
    d0 = (frozenset({0}), frozenset())
    assert _dem_dict(in_2) == {}
    assert _dem_dict(in_3) == {d0: pytest.approx(0.5)}
    # A flag raised before the transition: the leak goes on in state 3 w.p. 1/4.
    flag_2 = "MPAD[LEAKAGE_MEASUREMENT: (1.0, 2): 0] 0"
    circuit = _random_state_circuit(transition, uc, flags=flag_2, unleak=2, flags_first=True)
    dem = MarginalLeakageDemGenerator(uc)(circuit, _records(circuit, 0)[0])
    assert _dem_dict(dem) == {d0: pytest.approx(0.125)}


def test_leaked_state_branch_gets_the_partner_channel_of_its_own_outcomes():
    # Qubit 1 (the CX target) gets an X exactly when qubit 0 goes to state 3.
    transition = "II_ERROR[LEAKAGE_TRANSITION_2: (0.25, 2_U-->3_X)] 0 1"
    flags = "MPAD[LEAKAGE_MEASUREMENT: (1.0, 2): 0] 0\nMPAD[LEAKAGE_MEASUREMENT: (1.0, 3): 0] 0"
    circuit = _random_state_circuit(transition, True, flags=flags)
    in_2, in_3 = MarginalLeakageDemGenerator()(
        circuit, np.stack([_records(circuit, 0)[0], _records(circuit, 1)[0]])
    )
    d0 = (frozenset({0}), frozenset())
    assert _dem_dict(in_2) == {d0: pytest.approx(0.5)}
    assert _dem_dict(in_3) == {d0: pytest.approx(1.0)}


@pytest.mark.parametrize("uc", [True, False])
@pytest.mark.parametrize(
    "transition, still_leaked, in_2, in_3",
    [
        # Qubit 1 (the CX target) gets an X when qubit 0 stays in state 2 by the second outcome.
        ("(0.25, 2_U-->3_U) (0.25, 2_U-->2_X)", 0.25, 1 / 3, 0.0),
        # The same with a hop at the op (the leak then skips the flags on qubit 0).
        ("(0.2, 2_U-->U_2) (0.2, 2_U-->3_U) (0.2, 2_U-->2_X)", 0.25, 1 / 3, 0.0),
        # Every outcome into state 3 (first) or that keeps state 2 (second) hits qubit 1: the
        # conditional channels have total probability 1, which float rounding must not exceed.
        ("(0.1, 2_U-->3_X) (0.08, 2_U-->3_Y) (0.02, 2_U-->3_Z)", 0.18, 0.0, 0.9),
        ("(0.03, 2_U-->2_X) (0.29, 2_U-->2_Y) (0.68, 2_U-->3_U)", 0.32, 1.0, 0.0),
    ],
)
def test_forking_flow_partner_channels_are_given_its_own_outcomes(
    uc, transition, still_leaked, in_2, in_3
):
    # D0 = M0 ^ M1 flips iff qubit 1 got an X or Y (qubit 0 unleaks from 2 or 3 before the CX).
    template = """
        RX 0
        R 1 2
        I_ERROR[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 0
        II_ERROR[LEAKAGE_TRANSITION_2: {transition}] 0 1
        {flags}
        I_ERROR[LEAKAGE_TRANSITION_1: (1.0, 2-->U) (1.0, 3-->U)] 0
        {cx} 0 1
        M 0 1
        DETECTOR rec[-1] rec[-2]
    """
    cx = "CX" if uc else "CX[CONDITIONED_ON_PAIR: (U, U)]"
    both_flag = "MPAD[LEAKAGE_MEASUREMENT: (1.0, 2) (1.0, 3): 0] 0"
    one_flag = "MPAD[LEAKAGE_MEASUREMENT: (1.0, 2): 0] 0\nMPAD[LEAKAGE_MEASUREMENT: (1.0, 3): 0] 0"
    d0 = (frozenset({0}), frozenset())

    def expected(p: float) -> dict:
        return {d0: pytest.approx(p)} if p else {}

    circuit = stim.Circuit(template.format(transition=transition, flags=both_flag, cx=cx))
    dem = MarginalLeakageDemGenerator(uc)(circuit, _records(circuit, 0)[0])
    assert _dem_dict(dem) == expected(still_leaked)
    circuit = stim.Circuit(template.format(transition=transition, flags=one_flag, cx=cx))
    dem_2, dem_3 = MarginalLeakageDemGenerator(uc)(
        circuit, np.stack([_records(circuit, 0)[0], _records(circuit, 1)[0]])
    )
    assert _dem_dict(dem_2) == expected(in_2)
    assert _dem_dict(dem_3) == expected(in_3)


def test_certain_hop_into_one_of_several_leaked_states_follows_each():
    circuit = stim.Circuit(
        """
        R 0 2
        RX 1
        I_ERROR[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 0
        II_ERROR[LEAKAGE_TRANSITION_2: (0.75, 2_U-->U_2) (0.25, 2_U-->U_3)] 0 1
        MPAD[LEAKAGE_MEASUREMENT: (1.0, 2): 1] 0
        MPAD[LEAKAGE_MEASUREMENT: (1.0, 3): 1] 0
        I_ERROR[LEAKAGE_TRANSITION_1: (1.0, 2-->U)] 1
        CX 1 2
        M 1 2
        DETECTOR rec[-1] rec[-2]
        M 0
        DETECTOR rec[-1]
        """
    )
    in_2, in_3 = MarginalLeakageDemGenerator()(
        circuit, np.stack([_records(circuit, 0)[0], _records(circuit, 1)[0]])
    )
    d0, d1 = (frozenset({0}), frozenset()), (frozenset({1}), frozenset())
    # Qubit 0 is depolarized when the leak leaves it; a leak still on qubit 1 at the CX
    # (state 3) randomizes D0.
    assert _dem_dict(in_2) == {d1: pytest.approx(0.5)}
    assert _dem_dict(in_3) == {d0: pytest.approx(0.5), d1: pytest.approx(0.5)}


@pytest.mark.parametrize("uc", [True, False])
def test_random_next_leaked_state_dems_match_simulation(uc):
    # Every still-leaked shot is flagged; D0 is random iff the leak is still in state 2.
    circuit = _random_state_circuit(
        "I_ERROR[LEAKAGE_TRANSITION_1: (0.3, 2-->3) (0.2, 2-->U)] 0", uc
    )
    shots = 8192
    handler = LeakageUint8Coset(unconditional_condition_on_U=uc).compile_op_handler(
        circuit=circuit, batch_size=shots
    )
    sim = stimside.CosetsideSimulator(
        circuit, compiled_op_handler=handler, batch_size=shots, seed=7
    )
    sim.run()
    records = np.asarray(sim.get_final_measurement_records(), dtype=bool)
    dets = circuit.compile_m2d_converter().convert(measurements=records, append_observables=False)
    dems = MarginalLeakageDemGenerator(uc)(circuit, records)
    flagged = records[:, 0]
    assert 0.7 < flagged.mean() < 0.9
    for group in (flagged, ~flagged):
        p = _dem_dict(dems[int(np.flatnonzero(group)[0])]).get((frozenset({0}), frozenset()), 0.0)
        rate = dets[group, 0].mean()
        assert abs(rate - p) <= 5 * np.sqrt(p * (1 - p) / group.sum())


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

    syndromes = np.array([[0, 0], [0, 1], [1, 1]], dtype=np.uint8)
    if _has_fork_matching():
        # Direct compatibility with pymatching.Matching.decode and decode_batch edge_reweights
        base_dem = gen.base_dem(REP_CIRCUIT, decompose_errors=True)
        matcher = pymatching.Matching.from_detector_error_model(base_dem)
        syndrome_single = np.array([0, 1], dtype=np.uint8)
        corr_single = matcher.decode(syndrome_single, edge_reweights=rw_single)
        assert corr_single.tolist() == [1]

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

    if _has_fork_matching():
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

    if _has_fork_matching():
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


# stim fuses identical consecutive instructions; TICK prevents it. The second
# source comes after the split copies, so its index in the builder's ops differs
# from its flattened index (which the loss oracle's event map must account for).
_FUSABLE_TAGS_CIRCUIT = """R 0 1
I_ERROR[LEAKAGE_TRANSITION_1: (0.3, U-->2)] 0
CX 0 1
M 1
DETECTOR rec[-1]
MPAD[LEAKAGE_MEASUREMENT: (1.0, 2): 0] 0
{SEP}MPAD[LEAKAGE_MEASUREMENT: (1.0, 2): 0] 0
X_ERROR[CONDITIONED_ON_OTHER: 2: 0](0.2) 1
{SEP}X_ERROR[CONDITIONED_ON_OTHER: 2: 0](0.2) 1
I_ERROR[LEAKAGE_TRANSITION_1: (0.3, U-->2)] 1
M 1
DETECTOR rec[-1]
"""


def test_stim_fused_mpad_and_conditioned_on_other_give_the_unfused_dems():
    # The fused MPAD (0 0) and X_ERROR (1 1) are split into copies, as in the simulators.
    fused = stim.Circuit(_FUSABLE_TAGS_CIRCUIT.replace("{SEP}", ""))
    unfused = stim.Circuit(_FUSABLE_TAGS_CIRCUIT.replace("{SEP}", "TICK\n"))
    assert [len(op.targets_copy()) for op in fused if op.name in ("MPAD", "X_ERROR")] == [2, 2]
    records = np.array([[0, a, b, 0] for a in (0, 1) for b in (0, 1)], dtype=bool)
    for dec in (False, True):
        gen = MarginalLeakageDemGenerator(decompose_errors=dec)
        assert gen.base_dem(fused) == gen.base_dem(unfused)
        assert [str(d) for d in gen(fused, records)] == [str(d) for d in gen(unfused, records)]


def test_reweight_decoding_without_fork_edge_reweights_matches_full_dems(monkeypatch):
    # Public PyMatching's decode_batch has no edge_reweights: the reweights are then
    # decoded by rebuilding each distinct reweighted DEM (forced here on any install).
    from stimside.dem_generators import dem_decoding

    monkeypatch.setattr(dem_decoding, "_matching_supports_edge_reweights", lambda: False)
    sc = _make_surface_code_leakage_circuit(distance=3, rounds=3)
    records = sc.compile_sampler(seed=5).sample(64)
    dets = np.packbits(
        sc.compile_m2d_converter().convert(measurements=records, append_observables=False),
        axis=1,
        bitorder="little",
    )
    gen = MarginalLeakageDemGenerator()
    flags = gen._get_analysis(sc, False).flag_records
    for shot in range(0, 64, 3):
        records[shot, flags[shot % len(flags)]] = True
    decoder = sinter.BUILT_IN_DECODERS["pymatching"]
    rw = gen(sc, records, decompose_errors=True, reweight_only=True)
    want = decode_with_generated_dems(
        decoder, gen(sc, records, decompose_errors=True, reweight_only=False), dets
    )
    np.testing.assert_array_equal(decode_with_generated_dems(decoder, rw, dets), want)
    np.testing.assert_array_equal(decode_with_generated_dems("pymatching", rw, dets), want)
    compiled = MarginalDecoder(
        gen, decompose_errors=True, reweight_only=True
    ).compile_for_task(sinter.Task(circuit=sc))
    np.testing.assert_array_equal(
        compiled.decode_shots_bit_packed(bit_packed_detection_event_data=dets, records=records),
        want,
    )
    assert decode_with_generated_dems(decoder, rw[0], dets[:0]).shape == (0, 1)
