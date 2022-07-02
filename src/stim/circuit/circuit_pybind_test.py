# Copyright 2021 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
import pathlib
import tempfile
from typing import cast

import stim
import pytest
import numpy as np


def test_circuit_init_num_measurements_num_qubits():
    c = stim.Circuit()
    assert c.num_qubits == c.num_measurements == 0
    assert str(c).strip() == ""

    c.append_operation("X", [3])
    assert c.num_qubits == 4
    assert c.num_measurements == 0
    assert str(c).strip() == """
X 3
        """.strip()

    c.append_operation("M", [0])
    assert c.num_qubits == 4
    assert c.num_measurements == 1
    assert str(c).strip() == """
X 3
M 0
        """.strip()


def test_circuit_append_operation():
    c = stim.Circuit()

    with pytest.raises(IndexError, match="Gate not found"):
        c.append_operation("NOT_A_GATE", [0])
    with pytest.raises(ValueError, match="even number of targets"):
        c.append_operation("CNOT", [0])
    with pytest.raises(ValueError, match="takes 0"):
        c.append_operation("X", [0], 0.5)
    with pytest.raises(ValueError, match="invalid modifiers"):
        c.append_operation("X", [stim.target_inv(0)])
    with pytest.raises(ValueError, match="invalid modifiers"):
        c.append_operation("X", [stim.target_x(0)])
    with pytest.raises(IndexError, match="lookback"):
        stim.target_rec(0)
    with pytest.raises(IndexError, match="lookback"):
        stim.target_rec(1)
    with pytest.raises(IndexError, match="lookback"):
        stim.target_rec(-2**30)
    assert stim.target_rec(-1) is not None
    assert stim.target_rec(-15) is not None

    c.append_operation("X", [0])
    c.append_operation("X", [1, 2])
    c.append_operation("X", [3])
    c.append_operation("CNOT", [0, 1])
    c.append_operation("M", [0, stim.target_inv(1)])
    c.append_operation("X_ERROR", [0], 0.25)
    c.append_operation("CORRELATED_ERROR", [stim.target_x(0), stim.target_y(1)], 0.5)
    c.append_operation("DETECTOR", [stim.target_rec(-1)])
    c.append_operation("OBSERVABLE_INCLUDE", [stim.target_rec(-1), stim.target_rec(-2)], 5)
    assert str(c).strip() == """
X 0 1 2 3
CX 0 1
M 0 !1
X_ERROR(0.25) 0
E(0.5) X0 Y1
DETECTOR rec[-1]
OBSERVABLE_INCLUDE(5) rec[-1] rec[-2]
    """.strip()


def test_circuit_iadd():
    c = stim.Circuit()
    alias = c
    c.append_operation("X", [1, 2])
    c2 = stim.Circuit()
    c2.append_operation("Y", [3])
    c2.append_operation("M", [4])
    c += c2
    assert c is alias
    assert str(c).strip() == """
X 1 2
Y 3
M 4
        """.strip()

    c += c
    assert str(c).strip() == """
X 1 2
Y 3
M 4
X 1 2
Y 3
M 4
    """.strip()
    assert c is alias


def test_circuit_add():
    c = stim.Circuit()
    c.append_operation("X", [1, 2])
    c2 = stim.Circuit()
    c2.append_operation("Y", [3])
    c2.append_operation("M", [4])
    assert str(c + c2).strip() == """
X 1 2
Y 3
M 4
            """.strip()

    assert str(c2 + c2).strip() == """
Y 3
M 4
Y 3
M 4
        """.strip()


def test_circuit_mul():
    c = stim.Circuit()
    c.append_operation("Y", [3])
    c.append_operation("M", [4])
    assert str(c * 2) == str(2 * c) == """
REPEAT 2 {
    Y 3
    M 4
}
        """.strip()
    assert str((c * 2) * 3) == """
REPEAT 6 {
    Y 3
    M 4
}
        """.strip()
    expected = """
REPEAT 3 {
    Y 3
    M 4
}
    """.strip()
    assert str(c * 3) == str(3 * c) == expected
    alias = c
    c *= 3
    assert alias is c
    assert str(c) == expected
    c *= 1
    assert str(c) == expected
    assert alias is c
    c *= 0
    assert str(c) == ""
    assert alias is c


def test_circuit_repr():
    v = stim.Circuit("""
        X 0
        M 0
    """)
    r = repr(v)
    assert r == """stim.Circuit('''
    X 0
    M 0
''')"""
    assert eval(r, {'stim': stim}) == v


def test_circuit_eq():
    a = """
        X 0
        M 0
    """
    b = """
        Y 0
        M 0
    """
    assert stim.Circuit() == stim.Circuit()
    assert stim.Circuit() != stim.Circuit(a)
    assert not (stim.Circuit() != stim.Circuit())
    assert not (stim.Circuit() == stim.Circuit(a))
    assert stim.Circuit(a) == stim.Circuit(a)
    assert stim.Circuit(b) == stim.Circuit(b)
    assert stim.Circuit(a) != stim.Circuit(b)

    assert stim.Circuit() != None
    assert stim.Circuit != object()
    assert stim.Circuit != "another type"
    assert not (stim.Circuit == None)
    assert not (stim.Circuit == object())
    assert not (stim.Circuit == "another type")


def test_circuit_clear():
    c = stim.Circuit("""
        X 0
        M 0
    """)
    c.clear()
    assert c == stim.Circuit()


def test_circuit_compile_sampler():
    c = stim.Circuit()
    s = c.compile_sampler()
    c.append_operation("M", [0])
    assert repr(s) == "stim.CompiledMeasurementSampler(stim.Circuit())"
    s = c.compile_sampler()
    assert repr(s) == """
stim.CompiledMeasurementSampler(stim.Circuit('''
    M 0
'''))
    """.strip()

    c.append_operation("H", [0, 1, 2, 3, 4])
    c.append_operation("M", [0, 1, 2, 3, 4])
    s = c.compile_sampler()
    r = repr(s)
    assert r == """
stim.CompiledMeasurementSampler(stim.Circuit('''
    M 0
    H 0 1 2 3 4
    M 0 1 2 3 4
'''))
    """.strip() == str(stim.CompiledMeasurementSampler(c))

    # Check that expression can be evaluated.
    _ = eval(r, {"stim": stim})


def test_circuit_compile_detector_sampler():
    c = stim.Circuit()
    s = c.compile_detector_sampler()
    c.append_operation("M", [0])
    assert repr(s) == "stim.CompiledDetectorSampler(stim.Circuit())"
    c.append_operation("DETECTOR", [stim.target_rec(-1)])
    s = c.compile_detector_sampler()
    r = repr(s)
    assert r == """
stim.CompiledDetectorSampler(stim.Circuit('''
    M 0
    DETECTOR rec[-1]
'''))
    """.strip()

    # Check that expression can be evaluated.
    _ = eval(r, {"stim": stim})


def test_circuit_flattened_operations():
    assert stim.Circuit('''
        H 0
        REPEAT 3 {
            X_ERROR(0.125) 1
        }
        CORRELATED_ERROR(0.25) X3 Y4 Z5
        M 0 !1
        DETECTOR rec[-1]
    ''').flattened_operations() == [
        ("H", [0], 0),
        ("X_ERROR", [1], 0.125),
        ("X_ERROR", [1], 0.125),
        ("X_ERROR", [1], 0.125),
        ("E", [("X", 3), ("Y", 4), ("Z", 5)], 0.25),
        ("M", [0, ("inv", 1)], 0),
        ("DETECTOR", [("rec", -1)], 0),
    ]


def test_copy():
    c = stim.Circuit("H 0")
    c2 = c.copy()
    assert c == c2
    assert c is not c2


def test_hash():
    # stim.Circuit is mutable. It must not also be value-hashable.
    # Defining __hash__ requires defining a FrozenCircuit variant instead.
    with pytest.raises(TypeError, match="unhashable"):
        _ = hash(stim.Circuit())


def test_circuit_generation():
    surface_code_circuit = stim.Circuit.generated(
            "surface_code:rotated_memory_z",
            distance=5,
            rounds=10)
    samples = surface_code_circuit.compile_detector_sampler().sample(5)
    assert samples.shape == (5, 24 * 10)
    assert np.count_nonzero(samples) == 0


def test_circuit_generation_errors():
    with pytest.raises(ValueError, match="Known repetition_code tasks"):
        stim.Circuit.generated(
            "repetition_code:UNKNOWN",
            distance=3,
            rounds=1000)
    with pytest.raises(ValueError, match="Expected type to start with."):
        stim.Circuit.generated(
            "UNKNOWN:memory",
            distance=0,
            rounds=1000)
    with pytest.raises(ValueError, match="distance >= 2"):
        stim.Circuit.generated(
            "repetition_code:memory",
            distance=1,
            rounds=1000)

    with pytest.raises(ValueError, match="0 <= after_clifford_depolarization <= 1"):
        stim.Circuit.generated(
            "repetition_code:memory",
            distance=3,
            rounds=1000,
            after_clifford_depolarization=-1)
    with pytest.raises(ValueError, match="0 <= before_round_data_depolarization <= 1"):
        stim.Circuit.generated(
            "repetition_code:memory",
            distance=3,
            rounds=1000,
            before_round_data_depolarization=-1)
    with pytest.raises(ValueError, match="0 <= after_reset_flip_probability <= 1"):
        stim.Circuit.generated(
            "repetition_code:memory",
            distance=3,
            rounds=1000,
            after_reset_flip_probability=-1)
    with pytest.raises(ValueError, match="0 <= before_measure_flip_probability <= 1"):
        stim.Circuit.generated(
            "repetition_code:memory",
            distance=3,
            rounds=1000,
            before_measure_flip_probability=-1)


def test_num_detectors():
    assert stim.Circuit().num_detectors == 0
    assert stim.Circuit("DETECTOR").num_detectors == 1
    assert stim.Circuit("""
        REPEAT 1000 {
            DETECTOR
        }
    """).num_detectors == 1000
    assert stim.Circuit("""
        DETECTOR
        REPEAT 1000000 {
            REPEAT 1000000 {
                M 0
                DETECTOR rec[-1]
            }
        }
    """).num_detectors == 1000000**2 + 1


def test_num_observables():
    assert stim.Circuit().num_observables == 0
    assert stim.Circuit("OBSERVABLE_INCLUDE(0)").num_observables == 1
    assert stim.Circuit("OBSERVABLE_INCLUDE(1)").num_observables == 2
    assert stim.Circuit("""
        M 0
        OBSERVABLE_INCLUDE(2)
        REPEAT 1000000 {
            REPEAT 1000000 {
                M 0
                OBSERVABLE_INCLUDE(3) rec[-1]
            }
            OBSERVABLE_INCLUDE(4)
        }
    """).num_observables == 5


def test_indexing_operations():
    c = stim.Circuit()
    assert len(c) == 0
    assert list(c) == []
    with pytest.raises(IndexError):
        _ = c[0]
    with pytest.raises(IndexError):
        _ = c[-1]

    c = stim.Circuit('X 0')
    assert len(c) == 1
    assert list(c) == [stim.CircuitInstruction('X', [stim.GateTarget(0)])]
    assert c[0] == c[-1] == stim.CircuitInstruction('X', [stim.GateTarget(0)])
    with pytest.raises(IndexError):
        _ = c[1]
    with pytest.raises(IndexError):
        _ = c[-2]

    c = stim.Circuit('''
        X 5 6
        REPEAT 1000 {
            H 5
        }
        M !0
    ''')
    assert len(c) == 3
    with pytest.raises(IndexError):
        _ = c[3]
    with pytest.raises(IndexError):
        _ = c[-4]
    assert list(c) == [
        stim.CircuitInstruction('X', [stim.GateTarget(5), stim.GateTarget(6)]),
        stim.CircuitRepeatBlock(1000, stim.Circuit('H 5')),
        stim.CircuitInstruction('M', [stim.GateTarget(stim.target_inv(0))]),
    ]


def test_slicing():
    c = stim.Circuit("""
        H 0
        REPEAT 5 {
            X 1
        }
        Y 2
        Z 3
    """)
    assert c[:] is not c
    assert c[:] == c
    assert c[1:-1] == stim.Circuit("""
        REPEAT 5 {
            X 1
        }
        Y 2
    """)
    assert c[::2] == stim.Circuit("""
        H 0
        Y 2
    """)
    assert c[1::2] == stim.Circuit("""
        REPEAT 5 {
            X 1
        }
        Z 3
    """)


def test_reappend_gate_targets():
    expected = stim.Circuit("""
        MPP !X0 * X1
        CX rec[-1] 5
    """)
    c = stim.Circuit()
    c.append_operation("MPP", cast(stim.CircuitInstruction, expected[0]).targets_copy())
    c.append_operation("CX", cast(stim.CircuitInstruction, expected[1]).targets_copy())
    assert c == expected


def test_append_instructions_and_blocks():
    c = stim.Circuit()

    c.append_operation("TICK")
    assert c == stim.Circuit("TICK")

    with pytest.raises(ValueError, match="no targets"):
        c.append_operation("TICK", [1, 2, 3])

    c.append_operation(stim.Circuit("H 1")[0])
    assert c == stim.Circuit("TICK\nH 1")

    c.append_operation(stim.Circuit("CX 1 2 3 4")[0])
    assert c == stim.Circuit("""
        TICK
        H 1
        CX 1 2 3 4
    """)

    c.append_operation((stim.Circuit("X 5") * 100)[0])
    assert c == stim.Circuit("""
        TICK
        H 1
        CX 1 2 3 4
        REPEAT 100 {
            X 5
        }
    """)

    c.append_operation(stim.Circuit("PAULI_CHANNEL_1(0.125, 0.25, 0.325) 4 5 6")[0])
    assert c == stim.Circuit("""
        TICK
        H 1
        CX 1 2 3 4
        REPEAT 100 {
            X 5
        }
        PAULI_CHANNEL_1(0.125, 0.25, 0.325) 4 5 6
    """)

    with pytest.raises(ValueError, match="must be a"):
        c.append_operation(object())

    with pytest.raises(ValueError, match="targets"):
        c.append_operation(stim.Circuit("H 1")[0], [2])

    with pytest.raises(ValueError, match="arg"):
        c.append_operation(stim.Circuit("H 1")[0], [], 0.1)

    with pytest.raises(ValueError, match="targets"):
        c.append_operation((stim.Circuit("H 1") * 5)[0], [2])

    with pytest.raises(ValueError, match="arg"):
        c.append_operation((stim.Circuit("H 1") * 5)[0], [], 0.1)

    with pytest.raises(ValueError, match="repeat 0"):
        c.append_operation(stim.CircuitRepeatBlock(0, stim.Circuit("H 1")))


def test_circuit_measurement_sampling_seeded():
    c = stim.Circuit("""
        H 0
        M 0
    """)
    with pytest.raises(ValueError, match="seed"):
        c.compile_sampler(seed=-1)
    with pytest.raises(ValueError, match="seed"):
        c.compile_sampler(seed=object())

    s1 = c.compile_sampler().sample(256)
    s2 = c.compile_sampler().sample(256)
    assert not np.array_equal(s1, s2)

    s1 = c.compile_sampler(seed=None).sample(256)
    s2 = c.compile_sampler(seed=None).sample(256)
    assert not np.array_equal(s1, s2)

    s1 = c.compile_sampler(seed=5).sample(256)
    s2 = c.compile_sampler(seed=5).sample(256)
    s3 = c.compile_sampler(seed=6).sample(256)
    assert np.array_equal(s1, s2)
    assert not np.array_equal(s1, s3)


def test_circuit_detector_sampling_seeded():
    c = stim.Circuit("""
        X_ERROR(0.5) 0
        M 0
        DETECTOR rec[-1]
    """)
    with pytest.raises(ValueError, match="seed"):
        c.compile_detector_sampler(seed=-1)
    with pytest.raises(ValueError, match="seed"):
        c.compile_detector_sampler(seed=object())

    s1 = c.compile_detector_sampler().sample(256)
    s2 = c.compile_detector_sampler().sample(256)
    assert not np.array_equal(s1, s2)

    s1 = c.compile_detector_sampler(seed=None).sample(256)
    s2 = c.compile_detector_sampler(seed=None).sample(256)
    assert not np.array_equal(s1, s2)

    s1 = c.compile_detector_sampler(seed=5).sample(256)
    s2 = c.compile_detector_sampler(seed=5).sample(256)
    s3 = c.compile_detector_sampler(seed=6).sample(256)
    assert np.array_equal(s1, s2)
    assert not np.array_equal(s1, s3)


def test_approx_equals():
    base = stim.Circuit("X_ERROR(0.099) 0")
    assert not base.approx_equals(stim.Circuit("X_ERROR(0.101) 0"), atol=0)
    assert not base.approx_equals(stim.Circuit("X_ERROR(0.101) 0"), atol=0.00001)
    assert base.approx_equals(stim.Circuit("X_ERROR(0.101) 0"), atol=0.01)
    assert base.approx_equals(stim.Circuit("X_ERROR(0.101) 0"), atol=999)
    assert not base.approx_equals(stim.Circuit("DEPOLARIZE1(0.101) 0"), atol=999)

    assert not base.approx_equals(object(), atol=999)
    assert not base.approx_equals(stim.PauliString("XYZ"), atol=999)


def test_append_extended_cases():
    c = stim.Circuit()
    c.append("H", 5)
    c.append("CNOT", [0, 1])
    c.append("H", c[0].targets_copy()[0])
    c.append("X", (e + 1 for e in range(5)))
    assert c == stim.Circuit("""
        H 5
        CNOT 0 1
        H 5
        X 1 2 3 4 5
    """)


def test_pickle():
    import pickle

    t = stim.Circuit("""
        H 0
        REPEAT 100 {
            M 0
            CNOT rec[-1] 2
        }
    """)
    a = pickle.dumps(t)
    assert pickle.loads(a) == t


def test_backwards_compatibility_vs_safety_append_vs_append_operation():
    c = stim.Circuit()
    with pytest.raises(ValueError, match="takes 1 parens argument"):
        c.append("X_ERROR", [5])
    with pytest.raises(ValueError, match="takes 1 parens argument"):
        c.append("OBSERVABLE_INCLUDE", [])
    assert c == stim.Circuit()
    c.append_operation("X_ERROR", [5])
    assert c == stim.Circuit("X_ERROR(0) 5")
    c.append_operation("Z_ERROR", [5], 0.25)
    assert c == stim.Circuit("X_ERROR(0) 5\nZ_ERROR(0.25) 5")


def test_anti_commuting_mpp_error_message():
    with pytest.raises(ValueError, match="while analyzing a Pauli product measurement"):
        stim.Circuit("""
            MPP X0 Z0
            DETECTOR rec[-1]
        """).detector_error_model()


def test_blocked_remnant_edge_error():
    circuit = stim.Circuit("""
        X_ERROR(0.125) 0
        CORRELATED_ERROR(0.25) X0 X1
        M 0 1
        DETECTOR rec[-1]
        DETECTOR rec[-1]
        DETECTOR rec[-2]
        DETECTOR rec[-2]
    """)

    assert circuit.detector_error_model(decompose_errors=True) == stim.DetectorErrorModel("""
        error(0.125) D2 D3
        error(0.25) D2 D3 ^ D0 D1
    """)

    with pytest.raises(ValueError, match="Failed to decompose"):
        circuit.detector_error_model(
            decompose_errors=True,
            block_decomposition_from_introducing_remnant_edges=True)

    assert circuit.detector_error_model(
        decompose_errors=True,
        block_decomposition_from_introducing_remnant_edges=True,
        ignore_decomposition_failures=True) == stim.DetectorErrorModel("""
            error(0.25) D0 D1 D2 D3
            error(0.125) D2 D3
        """)


def test_shortest_graphlike_error():
    c = stim.Circuit("""
        TICK
        X_ERROR(0.125) 0
        Y_ERROR(0.125) 0
        M 0
        OBSERVABLE_INCLUDE(0) rec[-1]
    """)

    actual = c.shortest_graphlike_error()
    assert len(actual) == 1
    assert isinstance(actual[0], stim.ExplainedError)
    assert str(actual[0]) == """ExplainedError {
    dem_error_terms: L0
    CircuitErrorLocation {
        flipped_pauli_product: Y0
        Circuit location stack trace:
            (after 1 TICKs)
            at instruction #3 (Y_ERROR) in the circuit
            at target #1 of the instruction
            resolving to Y_ERROR(0.125) 0
    }
    CircuitErrorLocation {
        flipped_pauli_product: X0
        Circuit location stack trace:
            (after 1 TICKs)
            at instruction #2 (X_ERROR) in the circuit
            at target #1 of the instruction
            resolving to X_ERROR(0.125) 0
    }
}"""

    actual = c.shortest_graphlike_error(canonicalize_circuit_errors=True)
    assert len(actual) == 1
    assert isinstance(actual[0], stim.ExplainedError)
    assert str(actual[0]) == """ExplainedError {
    dem_error_terms: L0
    CircuitErrorLocation {
        flipped_pauli_product: X0
        Circuit location stack trace:
            (after 1 TICKs)
            at instruction #2 (X_ERROR) in the circuit
            at target #1 of the instruction
            resolving to X_ERROR(0.125) 0
    }
}"""


def test_shortest_graphlike_error_ignore():
    c = stim.Circuit("""
        TICK
        X_ERROR(0.125) 0
        M 0
        DETECTOR rec[-1]
        DETECTOR rec[-1]
        DETECTOR rec[-1]
    """)
    with pytest.raises(ValueError, match="Failed to decompose errors"):
        c.shortest_graphlike_error(ignore_ungraphlike_errors=False)
    with pytest.raises(ValueError, match="Failed to find any graphlike logical errors"):
        c.shortest_graphlike_error(ignore_ungraphlike_errors=True)


def test_coords():
    circuit = stim.Circuit("""
        QUBIT_COORDS(1, 2, 3) 0
        QUBIT_COORDS(2) 1
        SHIFT_COORDS(5)
        QUBIT_COORDS(3) 4
    """)
    assert circuit.get_final_qubit_coordinates() == {
        0: [1, 2, 3],
        1: [2],
        4: [8],
    }


def test_explain_errors():
    circuit = stim.Circuit("""
        H 0
        CNOT 0 1
        DEPOLARIZE1(0.01) 0
        CNOT 0 1
        H 0
        M 0 1
        DETECTOR rec[-1]
        DETECTOR rec[-2]
    """)
    r = circuit.explain_detector_error_model_errors()
    assert len(r) == 3
    assert str(r[0]) == """ExplainedError {
    dem_error_terms: D0
    CircuitErrorLocation {
        flipped_pauli_product: X0
        Circuit location stack trace:
            (after 0 TICKs)
            at instruction #3 (DEPOLARIZE1) in the circuit
            at target #1 of the instruction
            resolving to DEPOLARIZE1(0.01) 0
    }
}"""
    assert str(r[1]) == """ExplainedError {
    dem_error_terms: D0 D1
    CircuitErrorLocation {
        flipped_pauli_product: Y0
        Circuit location stack trace:
            (after 0 TICKs)
            at instruction #3 (DEPOLARIZE1) in the circuit
            at target #1 of the instruction
            resolving to DEPOLARIZE1(0.01) 0
    }
}"""
    assert str(r[2]) == """ExplainedError {
    dem_error_terms: D1
    CircuitErrorLocation {
        flipped_pauli_product: Z0
        Circuit location stack trace:
            (after 0 TICKs)
            at instruction #3 (DEPOLARIZE1) in the circuit
            at target #1 of the instruction
            resolving to DEPOLARIZE1(0.01) 0
    }
}"""

    r = circuit.explain_detector_error_model_errors(
        dem_filter=stim.DetectorErrorModel('error(1) D0 D1'),
        reduce_to_one_representative_error=True,
    )
    assert len(r) == 1
    assert str(r[0]) == """ExplainedError {
    dem_error_terms: D0 D1
    CircuitErrorLocation {
        flipped_pauli_product: Y0
        Circuit location stack trace:
            (after 0 TICKs)
            at instruction #3 (DEPOLARIZE1) in the circuit
            at target #1 of the instruction
            resolving to DEPOLARIZE1(0.01) 0
    }
}"""


def test_without_noise():
    assert stim.Circuit("""
        X_ERROR(0.25) 0
        CNOT 0 1
        M(0.125) 0
        REPEAT 50 {
            DEPOLARIZE1(0.25) 0 1 2
            X 0 1 2
        }
    """).without_noise() == stim.Circuit("""
        CNOT 0 1
        M 0
        REPEAT 50 {
            X 0 1 2
        }
    """)


def test_flattened():
    assert stim.Circuit("""
        SHIFT_COORDS(5, 0)
        QUBIT_COORDS(1, 2, 3) 0
        REPEAT 5 {
            MR 0 1
            DETECTOR(0, 0) rec[-2]
            DETECTOR(1, 0) rec[-1]
            SHIFT_COORDS(0, 1)
        }
    """).flattened() == stim.Circuit("""
        QUBIT_COORDS(6, 2, 3) 0
        MR 0 1
        DETECTOR(5, 0) rec[-2]
        DETECTOR(6, 0) rec[-1]
        MR 0 1
        DETECTOR(5, 1) rec[-2]
        DETECTOR(6, 1) rec[-1]
        MR 0 1
        DETECTOR(5, 2) rec[-2]
        DETECTOR(6, 2) rec[-1]
        MR 0 1
        DETECTOR(5, 3) rec[-2]
        DETECTOR(6, 3) rec[-1]
        MR 0 1
        DETECTOR(5, 4) rec[-2]
        DETECTOR(6, 4) rec[-1]
    """)


def test_complex_slice_does_not_seg_fault():
    with pytest.raises(TypeError):
        _ = stim.Circuit()[1j]


def test_circuit_from_file():
    with tempfile.TemporaryDirectory() as tmpdir:
        path = tmpdir + '/tmp.stim'
        with open(path, 'w') as f:
            print('H 5', file=f)
        assert stim.Circuit.from_file(path) == stim.Circuit('H 5')

    with tempfile.TemporaryDirectory() as tmpdir:
        path = pathlib.Path(tmpdir) / 'tmp.stim'
        with open(path, 'w') as f:
            print('H 5', file=f)
        assert stim.Circuit.from_file(path) == stim.Circuit('H 5')

    with tempfile.TemporaryDirectory() as tmpdir:
        path = tmpdir + '/tmp.stim'
        with open(path, 'w') as f:
            print('CNOT 4 5', file=f)
        with open(path) as f:
            assert stim.Circuit.from_file(f) == stim.Circuit('CX 4 5')

    with pytest.raises(ValueError, match="how to read"):
        stim.Circuit.from_file(object())
    with pytest.raises(ValueError, match="how to read"):
        stim.Circuit.from_file(123)


def test_circuit_to_file():
    c = stim.Circuit('H 5\ncnot 0 1')
    with tempfile.TemporaryDirectory() as tmpdir:
        path = tmpdir + '/tmp.stim'
        c.to_file(path)
        with open(path) as f:
            assert f.read() == 'H 5\nCX 0 1\n'

    with tempfile.TemporaryDirectory() as tmpdir:
        path = pathlib.Path(tmpdir) / 'tmp.stim'
        c.to_file(path)
        with open(path) as f:
            assert f.read() == 'H 5\nCX 0 1\n'

    with tempfile.TemporaryDirectory() as tmpdir:
        path = tmpdir + '/tmp.stim'
        with open(path, 'w') as f:
            c.to_file(f)
        with open(path) as f:
            assert f.read() == 'H 5\nCX 0 1\n'

    with pytest.raises(ValueError, match="how to write"):
        c.to_file(object())
    with pytest.raises(ValueError, match="how to write"):
        c.to_file(123)
