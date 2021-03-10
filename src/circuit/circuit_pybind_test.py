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


def test_circuit_repr():
    v = stim.Circuit("""
        X 0
        M 0
    """)
    r = repr(v)
    assert r == '''stim.Circuit("""
# Circuit [num_qubits=1, num_measurements=1]
X 0
M 0
""")'''
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
