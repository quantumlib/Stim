import numpy as np
import pytest
import stim # type: ignore[import-untyped]

from stimside.dem_generators.dem_generator_marginal import MarginalLeakageDemGenerator
from stimside.op_handlers.leakage_handlers import leakage_tag_parsing_flip
from stimside.op_handlers.leakage_handlers import leakage_tag_parsing_tableau as ltp
from stimside.op_handlers.leakage_handlers.leakage_parameters import (
    LeakageTransition1Params,
    LeakageTransition2Params,
    LeakageConditioningParams,
    LeakageMeasurementParams
)
from stimside.op_handlers.leakage_handlers.common_parsing import try_as_integer
from stimside.op_handlers.leakage_handlers.leakage_uint8_coset import LeakageUint8Coset
from stimside.op_handlers.leakage_handlers.leakage_uint8_flip import (
    LeakageUint8 as LeakageUint8Flip,
)
from stimside.op_handlers.leakage_handlers.leakage_uint8_tableau import LeakageUint8
from stimside.simulator_coset import CosetsideSimulator
from stimside.simulator_flip import FlipsideSimulator
from stimside.simulator_tableau import TablesideSimulator

# ##############################################################################
# Tests for successful parsing
# ##############################################################################


def test_parse_tags_successfully():
    """Tests that all valid leakage tags are parsed correctly."""
    circuit = stim.Circuit(
        """
    I_ERROR[LEAKAGE_TRANSITION_1: (0.1, 2-->3) (0.2, 4<->U)]  0 1 2 3
    II_ERROR[LEAKAGE_TRANSITION_2: (0.1, 2_2-->3_2) (0.2, 2_U<->U_3) (0.3, 4_U-->5_V)] 0 1 2 3
    M[LEAKAGE_PROJECTION_Z: (0.1, 2) (0.2, 3)] 0
    MPAD[LEAKAGE_MEASUREMENT: (0.1, 0) (0.9, 2) : 1 3 5 7]
    DEPOLARIZE1[CONDITIONED_ON_OTHER: U 2 3: 10 12 14 16](0.1) 9 11 13 15
    CZ[CONDITIONED_ON_PAIR: (2,3) (U,2) (U,3)] 20 21
    I[UNRELATED TAG]
    """
    )
    results = [
        LeakageTransition1Params(
            args=((0.1, 2, 3), (0.2, 4, "U"), (0.2, "U", 4)),
            from_tag="LEAKAGE_TRANSITION_1: (0.1, 2-->3) (0.2, 4<->U)",
        ),
        LeakageTransition2Params(
            args=(
                (0.1, (2, 2), (3, 2)),
                (0.2, (2, "U"), ("U", 3)),
                (0.2, ("U", 3), (2, "U")),
                (0.3, (4, "U"), (5, "V")),
            ),
            from_tag="LEAKAGE_TRANSITION_2: (0.1, 2_2-->3_2) (0.2, 2_U<->U_3) (0.3, 4_U-->5_V)",
        ),
        LeakageMeasurementParams(
            args=((0.1, 2), (0.2, 3)), 
            targets=None, 
            from_tag="LEAKAGE_PROJECTION_Z: (0.1, 2) (0.2, 3)"
        ),
        LeakageMeasurementParams(
            args=((0.1, 0), (0.9, 2)),
            targets=(1, 3, 5, 7),
            from_tag="LEAKAGE_MEASUREMENT: (0.1, 0) (0.9, 2) : 1 3 5 7",
        ),
        LeakageConditioningParams(
            args=(("U", 2, 3),),
            targets=(10, 12, 14, 16),
            from_tag="CONDITIONED_ON_OTHER: U 2 3: 10 12 14 16",
        ),
        LeakageConditioningParams(
            args=((2, "U", "U"), (3, 2, 3)),
            targets=None,
            from_tag="CONDITIONED_ON_PAIR: (2,3) (U,2) (U,3)",
        ),
        None,
    ]
    for op, r in zip(circuit, results):
        got = ltp.parse_leakage_tag(op)
        assert got == r, f"Failed on parsing {op.tag}, expected {r}, got {got}"


# ##############################################################################
# Tests for expected failures
# ##############################################################################


def test_bad_tags():
    """Tests that various malformed and unrecognized tags raise ValueErrors."""
    circuit = stim.Circuit(
        """
    I_ERROR[LEAKAGE MALFORMED TAG]
    """
    )
    with pytest.raises(ValueError, match="Malformed leakage tag structure"):
        ltp.parse_leakage_tag(circuit[0])

    circuit = stim.Circuit(
        """
    I_ERROR[LEAKAGE_UNRECOGNIZED_TAG: (arg)]
    """
    )
    with pytest.raises(ValueError, match="Unrecognised leakage tag name"):
        ltp.parse_leakage_tag(circuit[0])

    circuit = stim.Circuit(
        """
        II_ERROR[LEAKAGE_CONTROLLED_ERROR: (0.1, 2-->X) (0.2, 3-->Z)] 0 1 2 3
        """
    )
    with pytest.raises(ValueError, match="is not supported in the"):
        ltp.parse_leakage_tag(circuit[0])

    circuit = stim.Circuit(
        """
        I_ERROR[LEAKAGE_TRANSITION_1: (0.1, 1-->2, 3)] 0
        """
    )
    with pytest.raises(ValueError, match="Malformed argument"):
        ltp.parse_leakage_tag(circuit[0])

    circuit = stim.Circuit(
        """
        I_ERROR[LEAKAGE_TRANSITION_1: (0.1, 1->2)] 0
        """
    )
    with pytest.raises(ValueError, match="Malformed transition"):
        ltp.parse_leakage_tag(circuit[0])

    circuit = stim.Circuit(
        """
        II_ERROR[LEAKAGE_TRANSITION_2: (0.1, 1_1-->2_0, 3)] 0 1
        """
    )
    with pytest.raises(ValueError, match="Malformed argument"):
        ltp.parse_leakage_tag(circuit[0])

    circuit = stim.Circuit(
        """
        II_ERROR[LEAKAGE_TRANSITION_2: (0.1, 1_1->2_0)] 0 1
        """
    )
    with pytest.raises(ValueError, match="Malformed transition"):
        ltp.parse_leakage_tag(circuit[0])

    """Tests that leakage tags are attached to gates of the correct arity."""
    circuit = stim.Circuit(
        """
    II_ERROR[LEAKAGE_TRANSITION_1: (0.1,2-->3) (0.2,3<->4)]  0 1 2 3
    """
    )
    with pytest.raises(ValueError, match="not-1Q stim gate"):
        ltp.parse_leakage_tag(circuit[0])

    circuit = stim.Circuit(
        """
    I_ERROR[LEAKAGE_TRANSITION_2: (0.1,2_2-->3_2) (0.2,2_U<->U_3)] 0 1 2 3
    """
    )
    with pytest.raises(ValueError, match="not-2Q stim gate"):
        ltp.parse_leakage_tag(circuit[0])


# ##############################################################################
# Tests for circuit-level parsing
# ##############################################################################


def test_parse_in_circuit():
    """Tests parsing leakage tags in a simple circuit, including REPEAT blocks."""
    circuit = stim.Circuit(
        """
    I_ERROR[LEAKAGE_TRANSITION_1: (0.2, 2-->3)] 0
    REPEAT 2 {
        I_ERROR[LEAKAGE_TRANSITION_1: (0.1, 2-->3)] 0
    }
    M 0
    """
    )
    parsed = ltp.parse_leakage_in_circuit(circuit)
    assert len(parsed) == 2
    assert parsed[circuit[0]] == LeakageTransition1Params(
        args=((0.2, 2, 3),), from_tag="LEAKAGE_TRANSITION_1: (0.2, 2-->3)"
    )
    assert parsed[circuit[1].body_copy()[0]] == LeakageTransition1Params(
        args=((0.1, 2, 3),), from_tag="LEAKAGE_TRANSITION_1: (0.1, 2-->3)"
    )


def test_parse_in_circuit_nested_repeats():
    """Tests parsing leakage tags in a circuit with nested REPEAT blocks."""
    circuit = stim.Circuit(
        """
    REPEAT 2 {
        I_ERROR[LEAKAGE_TRANSITION_1: (0.1, 2-->3)] 0
        REPEAT 3 {
            I_ERROR[LEAKAGE_TRANSITION_1: (0.1, 2-->3)] 0
        }
    }
    """
    )
    parsed = ltp.parse_leakage_in_circuit(circuit)
    assert len(parsed) == 1
    assert parsed[circuit[0].body_copy()[0]] == LeakageTransition1Params(
        args=((0.1, 2, 3),), from_tag="LEAKAGE_TRANSITION_1: (0.1, 2-->3)"
    )
    assert parsed[circuit[0].body_copy()[1].body_copy()[0]] == LeakageTransition1Params(
        args=((0.1, 2, 3),), from_tag="LEAKAGE_TRANSITION_1: (0.1, 2-->3)"
    )


def test_try_as_integer():
    """Tests the try_as_integer helper function."""
    assert try_as_integer("2") == 2
    assert try_as_integer("U") == "U"


def test_leakage_conditioning_params_validation_and_pair_eq():
    p1 = LeakageConditioningParams(
        args=((2, 3), (3, 2)), targets=None, from_tag="CONDITIONED_ON_PAIR: (2,3) (3,2)"
    )
    p2 = LeakageConditioningParams(
        args=((3, 2), (2, 3)), targets=None, from_tag="CONDITIONED_ON_PAIR: (2,3) (3,2)"
    )
    p_diff = LeakageConditioningParams(
        args=((2, 3), (2, 3)), targets=None, from_tag="CONDITIONED_ON_PAIR: (2,3) (3,2)"
    )
    assert p1 == p2
    assert p1 != p_diff

    with pytest.raises(ValueError):
        LeakageConditioningParams(
            args=((15,),), targets=None, from_tag="CONDITIONED_ON_SELF: 15"
        )

    c_bad = stim.Circuit("X[CONDITIONED_ON_SELF: INVALID_TOKEN] 0")
    with pytest.raises(ValueError):
        ltp.parse_leakage_tag(c_bad[0])


# ##############################################################################
# Validation at parse / compile time, in every simulator and the DEM generator
# ##############################################################################

_COMPILERS = {
    "tableau": lambda c: LeakageUint8().compile_op_handler(circuit=c, batch_size=1),
    "coset": lambda c: LeakageUint8Coset().compile_op_handler(circuit=c, batch_size=1),
    "flip": lambda c: LeakageUint8Flip().compile_op_handler(circuit=c, batch_size=1),
    "demgen": lambda c: MarginalLeakageDemGenerator().base_dem(c),
}
_ALL = ("tableau", "coset", "flip", "demgen")
_TABLEAU_FAMILY = ("tableau", "coset", "demgen")  # where CONDITIONED_ON_* tags are supported

# (circuit with a state placeholder, the compilers supporting the tag, the allowed range)
_STATE_CASES = {
    "LT1 output": ("R 0\nI_ERROR[LEAKAGE_TRANSITION_1: (0.1, U-->{s})] 0", _ALL, "between 0 and 9"),
    "LT1 input": ("R 0\nI_ERROR[LEAKAGE_TRANSITION_1: (0.1, {s}-->U)] 0", _ALL, "between 0 and 9"),
    "LT2": ("R 0 1\nII_ERROR[LEAKAGE_TRANSITION_2: (0.1, U_U-->{s}_U)] 0 1", _ALL, "between 0 and 9"),
    "LTZ": ("R 0\nI_ERROR[LEAKAGE_TRANSITION_Z: (0.1, 0-->{s})] 0", ("flip",), "between 0 and 9"),
    "LCE": ("R 0 1\nII_ERROR[LEAKAGE_CONTROLLED_ERROR: (0.1, {s}-->X)] 0 1", ("flip",), "between 2 and 9"),
    "LPZ": ("R 0\nM[LEAKAGE_PROJECTION_Z: (0.5, {s})] 0", _ALL, "between 0 and 9"),
    "LM": ("R 0\nMPAD[LEAKAGE_MEASUREMENT: (0.5, {s}) : 0] 0", _ALL, "between 0 and 9"),
    "COS": ("R 0\nX_ERROR[CONDITIONED_ON_SELF: {s}](0.1) 0", _TABLEAU_FAMILY, "between 0 and 9"),
    "COO": ("R 0 1\nX_ERROR[CONDITIONED_ON_OTHER: {s} : 1](0.1) 0", _TABLEAU_FAMILY, "between 0 and 9"),
    "COP": ("R 0 1\nCZ[CONDITIONED_ON_PAIR: ({s}, U)] 0 1", _TABLEAU_FAMILY, "between 0 and 9"),
}


@pytest.mark.parametrize("case", list(_STATE_CASES))
def test_leakage_states_above_9_raise_at_compile_time(case):
    text, compilers, allowed = _STATE_CASES[case]
    for name in compilers:
        _COMPILERS[name](stim.Circuit(text.format(s=9)))  # the largest leaked state
        for s in (10, 255):
            with pytest.raises(ValueError, match=f"invalid .*state.*{s}.*{allowed}"):
                _COMPILERS[name](stim.Circuit(text.format(s=s)))


_MEASURING_OPS_1Q = [
    "M[{tag}] 0",
    "MX[{tag}] 0",
    "MY[{tag}] 0",
    "MR[{tag}] 0",
    "MRX[{tag}] 0",
    "MRY[{tag}] 0",
    "HERALDED_ERASE[{tag}](0.1) 0",
    "HERALDED_PAULI_CHANNEL_1[{tag}](0.1, 0, 0, 0) 0",
    "MPAD[{tag}] 0",
    "MPP[{tag}] Z0*Z1",
]
_MEASURING_OPS_2Q = ["MXX[{tag}] 0 1", "MYY[{tag}] 0 1", "MZZ[{tag}] 0 1", "MPAD[{tag}] 0 1"]
_CONDITIONED_MEASUREMENTS = [
    op.format(tag=tag)
    for tag in ("CONDITIONED_ON_SELF: U", "CONDITIONED_ON_SELF: 1", "CONDITIONED_ON_OTHER: U : 1")
    for op in _MEASURING_OPS_1Q
] + [op.format(tag="CONDITIONED_ON_PAIR: (U, U)") for op in _MEASURING_OPS_2Q]


@pytest.mark.parametrize("op_text", _CONDITIONED_MEASUREMENTS)
def test_conditioned_tags_on_measuring_gates_raise_at_compile_time(op_text):
    # A conditioned measurement would drop its records in the shots where the
    # condition fails, so every gate that produces measurement results rejects them.
    circuit = stim.Circuit("R 0 1\n" + op_text + "\nM 0 1")
    for name in _TABLEAU_FAMILY:
        with pytest.raises(ValueError, match="which produces measurement results"):
            _COMPILERS[name](circuit)
    with pytest.raises(ValueError, match="not supported in the flip simulator"):
        _COMPILERS["flip"](circuit)


_RESETS = ("R", "RZ", "RX", "RY")  # MR, MRX and MRY are measuring gates (above)
_CONDITIONED_RESETS = [
    f"{gate}[{tag}] 0"
    for tag in ("CONDITIONED_ON_SELF: U", "CONDITIONED_ON_SELF: 0", "CONDITIONED_ON_OTHER: U : 1")
    for gate in _RESETS
] + [f"{gate}[CONDITIONED_ON_PAIR: (U, U)] 0 1" for gate in _RESETS]


@pytest.mark.parametrize("op_text", _CONDITIONED_RESETS)
def test_conditioned_tags_on_resets_raise_at_compile_time(op_text):
    circuit = stim.Circuit("R 0 1\n" + op_text + "\nM 0 1")
    for name in _TABLEAU_FAMILY:
        with pytest.raises(ValueError, match="conditioned resets are not supported"):
            _COMPILERS[name](circuit)
    with pytest.raises(ValueError, match="not supported in the flip simulator"):
        _COMPILERS["flip"](circuit)


def test_conditioned_reset_whose_condition_fails_raises():
    # Qubit 0 is in |1>, so the condition fails: TablesideSimulator didn't reset it
    # (M = 1), but CosetsideSimulator did (M = 0).
    circuit = stim.Circuit("X 0\nR[CONDITIONED_ON_SELF: 0] 0\nM 0")
    for name in _TABLEAU_FAMILY:
        with pytest.raises(ValueError, match="conditioned resets are not supported"):
            _COMPILERS[name](circuit)


@pytest.mark.parametrize(
    "text, compilers",
    [
        ("X_ERROR[CONDITIONED_ON_OTHER: U : 1 2](1.0) 0 0\nM 0", _TABLEAU_FAMILY),
        ("X_ERROR[CONDITIONED_ON_OTHER: 2 : 5](1.0) 0\nM 0", _TABLEAU_FAMILY),
        ("X_ERROR[CONDITIONED_ON_OTHER: U : 5](1.0) 0\nM 0", _TABLEAU_FAMILY),
        ("X_ERROR[CONDITIONED_ON_OTHER: 0 : 5](1.0) 0\nM 0", _TABLEAU_FAMILY),
        ("X_ERROR[CONDITIONED_ON_OTHER: U : 100000](1.0) 0\nM 0", _TABLEAU_FAMILY),
        ("MPAD[LEAKAGE_MEASUREMENT: (1.0, 2) : 3] 0\nM 0", _ALL),
    ],
)
def test_tag_qubits_beyond_num_qubits_raise_at_compile_time(text, compilers):
    for name in compilers:
        with pytest.raises(ValueError, match="in its tag, but the circuit has num_qubits=1"):
            _COMPILERS[name](stim.Circuit(text))


@pytest.mark.parametrize(
    "tag, parse, match",
    [
        ("I_ERROR[LEAKAGE_TRANSITION_1: (0.7, U-->2) (0.7, U-->3)] 0", ltp.parse_leakage_tag, "total probability 1.4>1"),
        ("I_ERROR[LEAKAGE_TRANSITION_1: (0.5, 2-->2)] 0", ltp.parse_leakage_tag, "identity transition 2->2"),
        ("II_ERROR[LEAKAGE_TRANSITION_2: (0.7, U_U-->2_U) (0.7, U_U-->U_2)] 0 1", ltp.parse_leakage_tag, "total probability 1.4>1"),
        ("II_ERROR[LEAKAGE_TRANSITION_2: (0.5, 2_U-->2_U)] 0 1", ltp.parse_leakage_tag, "identity transition"),
        ("I_ERROR[LEAKAGE_TRANSITION_Z: (0.7, 0-->2) (0.7, 0-->3)] 0", leakage_tag_parsing_flip.parse_leakage_tag, "LEAKAGE_TRANSITION_Z total probability 1.4>1"),
        ("I_ERROR[LEAKAGE_TRANSITION_Z: (0.5, 0-->0)] 0", leakage_tag_parsing_flip.parse_leakage_tag, "LEAKAGE_TRANSITION_Z has identity transition 0->0"),
    ],
)
def test_invalid_transitions_get_validation_errors(tag, parse, match):
    # Validated before the alias tables are built, so the error says what is wrong.
    with pytest.raises(ValueError, match=match):
        parse(stim.Circuit(tag)[0])


def test_measurement_tag_errors_name_the_tag():
    with pytest.raises(ValueError, match="^LEAKAGE_PROJECTION_Z has probability argument 1.5"):
        ltp.parse_leakage_tag(stim.Circuit("M[LEAKAGE_PROJECTION_Z: (1.5, 2)] 0")[0])
    with pytest.raises(ValueError, match="^LEAKAGE_MEASUREMENT has probability argument 1.5"):
        ltp.parse_leakage_tag(stim.Circuit("MPAD[LEAKAGE_MEASUREMENT: (1.5, 2) : 0] 0")[0])


# Totals off 1 by rounding used to be rejected when the alias tables were built: since Python
# 3.12 sum() is compensated, so the first three cases' probabilities plus the remainder 1 - sum
# came to 1 - 1 ulp; the last case's come to 1 + 1 ulp (a negative remainder). Each case: the
# circuit, the compilers supporting it, and the probability of each measurement record (the
# second record implies the first).
_TOTAL_NEAR_1_CASES = {
    "LT2": (
        """R 0 1
        I_ERROR[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 0
        II_ERROR[LEAKAGE_TRANSITION_2: (0.1, 2_U-->3_X) (0.13, 2_U-->3_Y) (0.11, 2_U-->3_Z)] 0 1
        MPAD[LEAKAGE_MEASUREMENT: (1.0, 3): 0] 0
        M 1""",
        _ALL,
        [0.34, 0.23],  # qubit 0 moved to 3; qubit 1 got X or Y
    ),
    "LT1": (
        """R 0
        I_ERROR[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 0
        I_ERROR[LEAKAGE_TRANSITION_1: (0.1, 2-->3) (0.13, 2-->4) (0.11, 2-->5)] 0
        MPAD[LEAKAGE_MEASUREMENT: (1.0, 3) (1.0, 4) (1.0, 5): 0] 0
        MPAD[LEAKAGE_MEASUREMENT: (1.0, 4): 0] 0""",
        _ALL,
        [0.34, 0.13],
    ),
    "LTZ": (
        """R 0
        I_ERROR[LEAKAGE_TRANSITION_Z: (0.1, 0-->2) (0.13, 0-->3) (0.11, 0-->4)] 0
        MPAD[LEAKAGE_MEASUREMENT: (1.0, 2) (1.0, 3) (1.0, 4): 0] 0
        MPAD[LEAKAGE_MEASUREMENT: (1.0, 3): 0] 0""",
        ("flip",),
        [0.34, 0.13],
    ),
    "LT2 total 1 + 1 ulp": (
        """R 0 1
        I_ERROR[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 0
        II_ERROR[LEAKAGE_TRANSITION_2: (0.5, 2_U-->3_X) (0.5000000000000002, 2_U-->2_Z)] 0 1
        MPAD[LEAKAGE_MEASUREMENT: (1.0, 3): 0] 0
        M 1""",
        _ALL,
        [0.5, 0.5],  # qubit 0 moved to 3 and qubit 1 got X (else qubit 1 got Z)
    ),
}

# (simulator, mode, shots)
_SAMPLING_PATHS = [
    ("tableau", "cpp", 400),
    ("tableau", "py", 200),
    ("tableau", "sync", 200),
    ("coset", "default", 2048),
    ("coset", "sync", 400),
    ("flip", "default", 4096),
]


def _sample_records(sim, mode, circuit, shots, seed=11):
    if sim == "flip":
        coh = LeakageUint8Flip().compile_op_handler(circuit=circuit, batch_size=shots)
        s = FlipsideSimulator(circuit, compiled_op_handler=coh, batch_size=shots, seed=seed)
    elif sim == "coset":
        coh = LeakageUint8Coset().compile_op_handler(circuit=circuit, batch_size=shots)
        s = CosetsideSimulator(
            circuit=circuit,
            compiled_op_handler=coh,
            batch_size=shots,
            seed=seed,
            sync_tableside_rng=mode == "sync",
        )
    else:
        coh = LeakageUint8().compile_op_handler(circuit=circuit, batch_size=1)
        s = TablesideSimulator(
            circuit,
            compiled_op_handler=coh,
            batch_size=shots if mode == "sync" else 1,
            seed=seed,
            use_cpp_kernels=mode == "cpp",
            sync_tableside_rng=mode == "sync",
        )
        if mode == "cpp":
            assert s._cpp_runner is not None, "the C++ kernels did not load"
        if mode != "sync":
            # Without sync_tableside_rng the shots of a batch share one leakage trajectory.
            rows = []
            for _ in range(shots):
                s.run()
                rows.append(np.asarray(s.get_final_measurement_records(), dtype=bool)[0])
                s.clear()
            return np.array(rows)
    s.run()
    return np.asarray(s.get_final_measurement_records(), dtype=bool)


@pytest.mark.parametrize("case", list(_TOTAL_NEAR_1_CASES))
def test_transition_totals_near_1_parse_and_sample_in_every_simulator(case):
    text, compilers, probs = _TOTAL_NEAR_1_CASES[case]
    circuit = stim.Circuit(text)
    for name in compilers:
        _COMPILERS[name](circuit)
    for sim, mode, shots in _SAMPLING_PATHS:
        if sim not in compilers:
            continue
        records = _sample_records(sim, mode, circuit, shots)
        assert records.shape == (shots, len(probs))
        assert not np.any(records[:, 1] & ~records[:, 0]), (sim, mode)
        for j, p in enumerate(probs):
            tol = 5 * np.sqrt(p * (1 - p) / shots)
            assert abs(records[:, j].mean() - p) <= tol, (sim, mode, j, records[:, j].mean(), p)


@pytest.mark.parametrize(
    "text",
    [
        "R 0\nI_ERROR[LEAKAGE_TRANSITION_1: (0.5, U-->2) (0.500005, U-->3)] 0",
        "R 0 1\nI_ERROR[LEAKAGE_TRANSITION_1: (1.0, U-->2)] 0\n"
        "II_ERROR[LEAKAGE_TRANSITION_2: (0.5, 2_U-->3_X) (0.500005, 2_U-->3_Y)] 0 1",
    ],
)
def test_transition_totals_above_1_by_more_than_rounding_still_raise(text):
    # 1.000005 is np.isclose to 1 but not rounding: the params' validation rejects it, with its
    # own error rather than the alias table's (the DEM generator would hand
    # PAULI_CHANNEL_1(0.5, 0.500005, 0) to stim, which rejects it).
    for name in _ALL:
        with pytest.raises(ValueError, match="total probability 1.000005>1"):
            _COMPILERS[name](stim.Circuit(text))
