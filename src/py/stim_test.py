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

import numpy as np
import stim
import pytest


def test_circuit_init_num_measurements_num_qubits():
    c = stim.Circuit()
    assert c.num_qubits == c.num_measurements == 0
    assert str(c).strip() == """
# Circuit [num_qubits=0, num_measurements=0]
    """.strip()

    c.append_operation("X", [3])
    assert c.num_qubits == 4
    assert c.num_measurements == 0
    assert str(c).strip() == """
# Circuit [num_qubits=4, num_measurements=0]
X 3
        """.strip()

    c.append_operation("M", [0])
    assert c.num_qubits == 4
    assert c.num_measurements == 1
    assert str(c).strip() == """
# Circuit [num_qubits=4, num_measurements=1]
X 3
M 0
        """.strip()


def test_circuit_append_operation():
    c = stim.Circuit()

    with pytest.raises(IndexError, match="Gate not found"):
        c.append_operation("NOT_A_GATE", [0])
    with pytest.raises(IndexError, match="even number of targets"):
        c.append_operation("CNOT", [0])
    with pytest.raises(IndexError, match="doesn't take"):
        c.append_operation("X", [0], 0.5)
    with pytest.raises(IndexError, match="invalid flags"):
        c.append_operation("X", [stim.target_inv(0)])
    with pytest.raises(IndexError, match="invalid flags"):
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
# Circuit [num_qubits=4, num_measurements=2]
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
    c.append_operation("X", [1, 2])
    c2 = stim.Circuit()
    c2.append_operation("Y", [3])
    c2.append_operation("M", [4])
    c += c2
    assert str(c).strip() == """
# Circuit [num_qubits=5, num_measurements=1]
X 1 2
Y 3
M 4
        """.strip()

    c += c
    assert str(c).strip() == """
# Circuit [num_qubits=5, num_measurements=2]
X 1 2
Y 3
M 4
X 1 2
Y 3
M 4
    """.strip()


def test_circuit_add():
    c = stim.Circuit()
    c.append_operation("X", [1, 2])
    c2 = stim.Circuit()
    c2.append_operation("Y", [3])
    c2.append_operation("M", [4])
    assert str(c + c2).strip() == """
    # Circuit [num_qubits=5, num_measurements=1]
X 1 2
Y 3
M 4
            """.strip()

    assert str(c2 + c2).strip() == """
# Circuit [num_qubits=5, num_measurements=2]
Y 3
M 4
Y 3
M 4
        """.strip()


def test_circuit_mul():
    c = stim.Circuit()
    c.append_operation("Y", [3])
    c.append_operation("M", [4])
    expected = """
# Circuit [num_qubits=5, num_measurements=2]
Y 3
M 4
Y 3
M 4
        """.strip()
    assert str(c * 2) == str(2 * c) == expected
    expected = """
# Circuit [num_qubits=5, num_measurements=3]
Y 3
M 4
Y 3
M 4
Y 3
M 4
    """.strip()
    assert str(c * 3) == str(3 * c) == expected
    c *= 3
    assert str(c) == expected
    c *= 1
    assert str(c) == expected
    c *= 0
    assert str(c) == "# Circuit [num_qubits=0, num_measurements=0]"


def test_circuit_compile_sampler():
    c = stim.Circuit()
    s = c.compile_sampler()
    c.append_operation("M", [0])
    assert str(s) == """
# reference sample: 
# Circuit [num_qubits=0, num_measurements=0]
    """.strip()
    s = c.compile_sampler()
    assert str(s) == """
# reference sample: 0
# Circuit [num_qubits=1, num_measurements=1]
M 0
    """.strip()

    c.append_operation("H", [0, 1, 2, 3, 4])
    c.append_operation("M", [0, 1, 2, 3, 4])
    s = c.compile_sampler()
    assert str(s) == """
# reference sample: 000000
# Circuit [num_qubits=5, num_measurements=6]
M 0
H 0 1 2 3 4
M 0 1 2 3 4
    """.strip() == str(stim.CompiledMeasurementSampler(c))


def test_circuit_compile_detector_sampler():
    c = stim.Circuit()
    s = c.compile_detector_sampler()
    c.append_operation("M", [0])
    assert str(s) == """
# num_detectors: 0
# num_observables: 0
# Circuit [num_qubits=0, num_measurements=0]
    """.strip()
    c.append_operation("DETECTOR", [stim.target_rec(-1)])
    s = c.compile_detector_sampler()
    assert str(s) == """
# num_detectors: 1
# num_observables: 0
# Circuit [num_qubits=1, num_measurements=1]
M 0
DETECTOR rec[-1]
    """.strip()


def test_compiled_measurement_sampler_sample():
    c = stim.Circuit()
    c.append_operation("X", [1])
    c.append_operation("M", [0, 1, 2, 3])
    np.testing.assert_array_equal(
        c.compile_sampler().sample(5),
        np.array([
            [0, 1, 0, 0],
            [0, 1, 0, 0],
            [0, 1, 0, 0],
            [0, 1, 0, 0],
            [0, 1, 0, 0],
        ], dtype=np.uint8))
    np.testing.assert_array_equal(
        c.compile_sampler().sample_bit_packed(5),
        np.array([
            [0b00010],
            [0b00010],
            [0b00010],
            [0b00010],
            [0b00010],
        ], dtype=np.uint8))


def test_tableau_simulator():
    s = stim.TableauSimulator()
    assert s.measure(0) is False
    assert s.measure(0) is False
    s.x(0)
    assert s.measure(0) is True
    assert s.measure(0) is True
    s.reset(0)
    assert s.measure(0) is False
    s.h(0)
    s.h(0)
    s.sqrt_x(1)
    s.sqrt_x(1)
    assert s.measure_many(0, 1) == [False, True]


def test_compiled_detector_sampler_sample():
    c = stim.Circuit()
    c.append_operation("X_ERROR", [0], 1)
    c.append_operation("M", [0, 1, 2])
    c.append_operation("DETECTOR", [stim.target_rec(-3), stim.target_rec(-2)])
    c.append_operation("DETECTOR", [stim.target_rec(-3), stim.target_rec(-1)])
    c.append_operation("DETECTOR", [stim.target_rec(-1), stim.target_rec(-2)])
    c.append_from_stim_program_text("""
        OBSERVABLE_INCLUDE(0) rec[-3]
        OBSERVABLE_INCLUDE(3) rec[-2] rec[-1]
    """)
    c.append_operation("OBSERVABLE_INCLUDE", [stim.target_rec(-2)], 0)
    np.testing.assert_array_equal(
        c.compile_detector_sampler().sample(5),
        np.array([
            [1, 1, 0],
            [1, 1, 0],
            [1, 1, 0],
            [1, 1, 0],
            [1, 1, 0],
        ], dtype=np.uint8))
    np.testing.assert_array_equal(
        c.compile_detector_sampler().sample(5, prepend_observables=True),
        np.array([
            [1, 0, 0, 0, 1, 1, 0],
            [1, 0, 0, 0, 1, 1, 0],
            [1, 0, 0, 0, 1, 1, 0],
            [1, 0, 0, 0, 1, 1, 0],
            [1, 0, 0, 0, 1, 1, 0],
        ], dtype=np.uint8))
    np.testing.assert_array_equal(
        c.compile_detector_sampler().sample(5, append_observables=True),
        np.array([
            [1, 1, 0, 1, 0, 0, 0],
            [1, 1, 0, 1, 0, 0, 0],
            [1, 1, 0, 1, 0, 0, 0],
            [1, 1, 0, 1, 0, 0, 0],
            [1, 1, 0, 1, 0, 0, 0],
        ], dtype=np.uint8))
    np.testing.assert_array_equal(
        c.compile_detector_sampler().sample(5, append_observables=True, prepend_observables=True),
        np.array([
            [1, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0],
            [1, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0],
            [1, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0],
            [1, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0],
            [1, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0],
        ], dtype=np.uint8))
    np.testing.assert_array_equal(
        c.compile_detector_sampler().sample_bit_packed(5),
        np.array([
            [0b011],
            [0b011],
            [0b011],
            [0b011],
            [0b011],
        ], dtype=np.uint8))
