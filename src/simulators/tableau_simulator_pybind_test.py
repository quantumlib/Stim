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
import pytest
import stim


def test_basic():
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


def test_access_tableau():
    s = stim.TableauSimulator()
    assert s.current_inverse_tableau() == stim.Tableau(0)

    s.h(0)
    assert s.current_inverse_tableau() == stim.Tableau.from_named_gate("H")

    s.h(0)
    assert s.current_inverse_tableau() == stim.Tableau(1)

    s.h(1)
    s.h(1)
    assert s.current_inverse_tableau() == stim.Tableau(2)

    s.h(2)
    assert s.current_inverse_tableau() == stim.Tableau.from_conjugated_generators(
        xs=[
            stim.PauliString("X__"),
            stim.PauliString("_X_"),
            stim.PauliString("__Z"),
        ],
        zs=[
            stim.PauliString("Z__"),
            stim.PauliString("_Z_"),
            stim.PauliString("__X"),
        ],
    )


@pytest.mark.parametrize("name", [
    "x",
    "y",
    "z",
    "h",
    "h_xy",
    "h_yz",
    "sqrt_x",
    "sqrt_x_dag",
    "sqrt_y",
    "sqrt_y_dag",
    "s",
    "s_dag",
    "swap",
    "iswap",
    "iswap_dag",
    "xcx",
    "xcy",
    "xcz",
    "ycx",
    "ycy",
    "ycz",
    "cnot",
    "cy",
    "cz",
])
def test_gates_present(name: str):
    t = stim.Tableau.from_named_gate(name)
    n = len(t)
    s1 = stim.TableauSimulator()
    s2 = stim.TableauSimulator()
    for k in range(n):
        s1.h(k)
        s2.h(k)
        s1.cnot(k, k + n)
        s2.cnot(k, k + n)
    getattr(s1, name)(*range(n))
    s2.do(stim.Circuit(f"{name} " + " ".join(str(e) for e in range(n))))
    assert s1.current_inverse_tableau() == s2.current_inverse_tableau()


def test_do():
    s = stim.TableauSimulator()
    s.do(stim.Circuit("""
        S 0
    """))
    assert s.current_inverse_tableau() == stim.Tableau.from_named_gate("S_DAG")


def test_peek_bloch():
    s = stim.TableauSimulator()
    assert s.peek_bloch(0) == stim.PauliString("+Z")
    s.x(0)
    assert s.peek_bloch(0) == stim.PauliString("-Z")
    s.h(0)
    assert s.peek_bloch(0) == stim.PauliString("-X")
    s.sqrt_x(1)
    assert s.peek_bloch(1) == stim.PauliString("-Y")
    s.cz(0, 1)
    assert s.peek_bloch(0) == stim.PauliString("+I")
    assert s.peek_bloch(1) == stim.PauliString("+I")


def test_copy():
    s = stim.TableauSimulator()
    s.h(0)
    s2 = s.copy()
    assert s.current_inverse_tableau() == s2.current_inverse_tableau()
    assert s is not s2


def test_paulis():
    s = stim.TableauSimulator()
    s.h(*range(0, 22, 2))
    s.cnot(*range(22))

    s.do(stim.PauliString("ZZZ_YYY_XXX"))
    s.z(0, 1, 2)
    s.y(4, 5, 6)
    s.x(8, 9, 10)

    s.cnot(*range(22))
    s.h(*range(0, 22, 2))
    assert s.measure_many(*range(22)) == [False] * 22

    s = stim.TableauSimulator()
    s.do(stim.PauliString("Z" * 500))
    assert s.measure_many(*range(500)) == [False] * 500
    s.do(stim.PauliString("X" * 500))
    assert s.measure_many(*range(500)) == [True] * 500


def test_measure_kickback():
    s = stim.TableauSimulator()
    assert s.measure_kickback(0) == (False, None)
    assert s.measure_kickback(0) == (False, None)
    assert s.current_measurement_record() == [False, False]

    s.h(0)
    v = s.measure_kickback(0)
    assert isinstance(v[0], bool)
    assert v[1] == stim.PauliString("X")
    assert s.measure_kickback(0) == (v[0], None)
    assert s.current_measurement_record() == [False, False, v[0], v[0]]

    s = stim.TableauSimulator()
    s.h(0)
    s.cnot(0, 1)
    v = s.measure_kickback(0)
    assert isinstance(v[0], bool)
    assert v[1] == stim.PauliString("XX")
    assert s.measure_kickback(0) == (v[0], None)

    s = stim.TableauSimulator()
    s.h(0)
    s.cnot(0, 1)
    v = s.measure_kickback(1)
    assert isinstance(v[0], bool)
    assert v[1] == stim.PauliString("XX")
    assert s.measure_kickback(0) == (v[0], None)


def test_post_select_using_measure_kickback():
    s = stim.TableauSimulator()

    def pseudo_post_select(qubit, desired_result):
        m, kick = s.measure_kickback(qubit)
        if m != desired_result:
            if kick is None:
                raise ValueError("Deterministic measurement differed from desired result.")
            s.do(kick)

    s.h(0)
    s.cnot(0, 1)
    s.cnot(0, 2)
    pseudo_post_select(qubit=2, desired_result=True)
    assert s.measure_many(0, 1, 2) == [True, True, True]


def test_measure_kickback_random_branches():
    s = stim.TableauSimulator()
    s.set_inverse_tableau(stim.Tableau.random(8))

    r = s.peek_bloch(4)
    if r[0] == 3:  # +-Z?
        assert s.measure_kickback(4) == (r.sign == -1, None)
        return

    post_false = None
    post_true = None
    for _ in range(100):
        if post_false is not None and post_true is not None:
            break
        copy = s.copy()
        if copy.measure(4):
            post_true = copy
        else:
            post_false = copy
    assert post_false is not None and post_true is not None

    result, kick = s.measure_kickback(4)
    assert isinstance(kick, stim.PauliString) and len(kick) == 8
    if result:
        s.do(kick)
    assert s.canonical_stabilizers() == post_false.canonical_stabilizers()
    s.do(kick)
    assert s.canonical_stabilizers() == post_true.canonical_stabilizers()


def test_set_num_qubits():
    s = stim.TableauSimulator()
    s.h(0)
    s.cnot(0, 1)
    s.cnot(0, 2)
    s.cnot(0, 3)
    t = s.current_inverse_tableau()
    s.set_num_qubits(8)
    s.set_num_qubits(4)
    assert s.current_inverse_tableau() == t
    assert s.peek_bloch(0) == stim.PauliString("_")
    s.set_num_qubits(8)
    s.set_num_qubits(4)
    s.cnot(0, 4)
    s.set_num_qubits(4)
    assert s.peek_bloch(0) in [stim.PauliString("+Z"), stim.PauliString("-Z")]


def test_canonical_stabilizers():
    s = stim.TableauSimulator()
    s.h(0)
    s.h(1)
    s.h(2)
    s.cz(0, 1)
    s.cz(1, 2)
    assert s.canonical_stabilizers() == [
        stim.PauliString("+X_X"),
        stim.PauliString("+ZXZ"),
        stim.PauliString("+_ZX"),
    ]
    s.s(1)
    assert s.canonical_stabilizers() == [
        stim.PauliString("+X_X"),
        stim.PauliString("-ZXY"),
        stim.PauliString("+_ZX"),
    ]
