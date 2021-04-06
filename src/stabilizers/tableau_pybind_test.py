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
import re

import stim
import pytest


def test_init_equality():
    assert stim.Tableau(3) == stim.Tableau(3)
    assert not (stim.Tableau(3) != stim.Tableau(3))
    assert stim.Tableau(3) != stim.Tableau(4)
    assert not (stim.Tableau(3) == stim.Tableau(4))

    assert stim.Tableau.from_named_gate("S") == stim.Tableau.from_named_gate("S")
    assert stim.Tableau.from_named_gate("S") != stim.Tableau.from_named_gate("S_DAG")
    assert stim.Tableau.from_named_gate("S") != stim.Tableau.from_named_gate("H")
    assert stim.Tableau.from_named_gate("S_DAG") == stim.Tableau.from_named_gate("S_DAG")


def test_from_named_gate():
    assert str(stim.Tableau.from_named_gate("H")).strip() == """
+-xz-
| ++
| ZX
""".strip()
    assert str(stim.Tableau.from_named_gate("I")).strip() == """
+-xz-
| ++
| XZ
""".strip()

    assert stim.Tableau.from_named_gate("H_XZ") == stim.Tableau.from_named_gate("h")

    with pytest.raises(IndexError, match="not found"):
        stim.Tableau.from_named_gate("not a gate")
    with pytest.raises(IndexError, match="not unitary"):
        stim.Tableau.from_named_gate("X_ERROR")


def test_identity():
    t = stim.Tableau(3)
    assert len(t) == 3
    assert t.x_output(0) == stim.PauliString("X__")
    assert t.x_output(1) == stim.PauliString("_X_")
    assert t.x_output(2) == stim.PauliString("__X")
    assert t.z_output(0) == stim.PauliString("Z__")
    assert t.z_output(1) == stim.PauliString("_Z_")
    assert t.z_output(2) == stim.PauliString("__Z")
    assert t.y_output(0) == stim.PauliString("Y__")
    assert t.y_output(1) == stim.PauliString("_Y_")
    assert t.y_output(2) == stim.PauliString("__Y")


def test_pauli_output():
    h = stim.Tableau.from_named_gate("H")
    assert h.x_output(0) == stim.PauliString("Z")
    assert h.y_output(0) == stim.PauliString("-Y")
    assert h.z_output(0) == stim.PauliString("X")

    s = stim.Tableau.from_named_gate("S")
    assert s.x_output(0) == stim.PauliString("Y")
    assert s.y_output(0) == stim.PauliString("-X")
    assert s.z_output(0) == stim.PauliString("Z")

    s_dag = stim.Tableau.from_named_gate("S_DAG")
    assert s_dag.x_output(0) == stim.PauliString("-Y")
    assert s_dag.y_output(0) == stim.PauliString("X")
    assert s_dag.z_output(0) == stim.PauliString("Z")

    cz = stim.Tableau.from_named_gate("CZ")
    assert cz.x_output(0) == stim.PauliString("XZ")
    assert cz.y_output(0) == stim.PauliString("YZ")
    assert cz.z_output(0) == stim.PauliString("Z_")
    assert cz.x_output(1) == stim.PauliString("ZX")
    assert cz.y_output(1) == stim.PauliString("ZY")
    assert cz.z_output(1) == stim.PauliString("_Z")


def test_random():
    t = stim.Tableau.random(10)
    assert len(t) == 10
    assert t != stim.Tableau.random(10)


def test_str():
    assert str(stim.Tableau.from_named_gate("cnot")).strip() == """
+-xz-xz-
| ++ ++
| XZ _Z
| X_ XZ
""".strip()


def test_append():
    t = stim.Tableau(2)
    with pytest.raises(ValueError, match=re.escape("len(targets) != len(gate)")):
        t.append(stim.Tableau.from_named_gate("CY"), [0])
    with pytest.raises(ValueError, match="collision"):
        t.append(stim.Tableau.from_named_gate("CY"), [0, 0])
    with pytest.raises(ValueError, match=re.escape("target >= len(tableau)")):
        t.append(stim.Tableau.from_named_gate("CY"), [1, 2])

    t.append(stim.Tableau.from_named_gate("SQRT_X_DAG"), [1])
    t.append(stim.Tableau.from_named_gate("CY"), [0, 1])
    t.append(stim.Tableau.from_named_gate("SQRT_X"), [1])
    assert t == stim.Tableau.from_named_gate("CZ")

    t = stim.Tableau(2)
    t.append(stim.Tableau.from_named_gate("SQRT_X"), [1])
    t.append(stim.Tableau.from_named_gate("CY"), [0, 1])
    t.append(stim.Tableau.from_named_gate("SQRT_X_DAG"), [1])
    assert t != stim.Tableau.from_named_gate("CZ")


def test_prepend():
    t = stim.Tableau(2)
    with pytest.raises(ValueError, match=re.escape("len(targets) != len(gate)")):
        t.prepend(stim.Tableau.from_named_gate("CY"), [0])
    with pytest.raises(ValueError, match="collision"):
        t.prepend(stim.Tableau.from_named_gate("CY"), [0, 0])
    with pytest.raises(ValueError, match=re.escape("target >= len(tableau)")):
        t.prepend(stim.Tableau.from_named_gate("CY"), [1, 2])

    t.prepend(stim.Tableau.from_named_gate("SQRT_X_DAG"), [1])
    t.prepend(stim.Tableau.from_named_gate("CY"), [0, 1])
    t.prepend(stim.Tableau.from_named_gate("SQRT_X"), [1])
    assert t != stim.Tableau.from_named_gate("CZ")

    t = stim.Tableau(2)
    t.prepend(stim.Tableau.from_named_gate("SQRT_X"), [1])
    t.prepend(stim.Tableau.from_named_gate("CY"), [0, 1])
    t.prepend(stim.Tableau.from_named_gate("SQRT_X_DAG"), [1])
    assert t == stim.Tableau.from_named_gate("CZ")


def test_from_conjugated_generators():
    assert stim.Tableau(3) == stim.Tableau.from_conjugated_generators(
        xs=[
            stim.PauliString("X__"),
            stim.PauliString("_X_"),
            stim.PauliString("__X"),
        ],
        zs=[
            stim.PauliString("Z__"),
            stim.PauliString("_Z_"),
            stim.PauliString("__Z"),
        ],
    )

    assert stim.Tableau(2) == stim.Tableau.from_conjugated_generators(
        xs=[
            stim.PauliString("X_"),
            stim.PauliString("_X"),
        ],
        zs=[
            stim.PauliString("Z_"),
            stim.PauliString("_Z"),
        ],
    )

    with pytest.raises(ValueError, match=re.escape("len(p) == len(zs)")):
        stim.Tableau.from_conjugated_generators(
            xs=[
                stim.PauliString("X_"),
                stim.PauliString("_X"),
            ],
            zs=[
                stim.PauliString("Z_"),
                stim.PauliString("_Z_"),
            ],
        )

    with pytest.raises(ValueError, match=re.escape("len(p) == len(xs)")):
        stim.Tableau.from_conjugated_generators(
            xs=[
                stim.PauliString("X_"),
                stim.PauliString("_X_"),
            ],
            zs=[
                stim.PauliString("Z_"),
                stim.PauliString("_Z"),
            ],
        )

    with pytest.raises(ValueError, match=re.escape("len(xs) != len(zs)")):
        stim.Tableau.from_conjugated_generators(
            xs=[
                stim.PauliString("X_"),
            ],
            zs=[
                stim.PauliString("Z_"),
                stim.PauliString("_Z"),
            ],
        )

    with pytest.raises(ValueError, match=re.escape("len(xs) != len(zs)")):
        stim.Tableau.from_conjugated_generators(
            xs=[
                stim.PauliString("X_"),
                stim.PauliString("_X"),
            ],
            zs=[
                stim.PauliString("Z_"),
            ],
        )

    with pytest.raises(ValueError, match="commutativity"):
        stim.Tableau.from_conjugated_generators(
            xs=[
                stim.PauliString("X_"),
                stim.PauliString("_Z"),
            ],
            zs=[
                stim.PauliString("Z_"),
                stim.PauliString("_Z"),
            ],
        )

    with pytest.raises(ValueError, match="commutativity"):
        stim.Tableau.from_conjugated_generators(
            xs=[
                stim.PauliString("X_"),
                stim.PauliString("Z_"),
            ],
            zs=[
                stim.PauliString("Z_"),
                stim.PauliString("X_"),
            ],
        )

    with pytest.raises(ValueError, match="commutativity"):
        stim.Tableau.from_conjugated_generators(
            xs=[
                stim.PauliString("X_"),
                stim.PauliString("X_"),
            ],
            zs=[
                stim.PauliString("Z_"),
                stim.PauliString("Z_"),
            ],
        )


def test_repr():
    v = stim.Tableau.from_named_gate("H")
    r = repr(v)
    assert r == """stim.Tableau.from_conjugated_generators(
    xs=[
        stim.PauliString("+Z"),
    ],
    zs=[
        stim.PauliString("+X"),
    ],
)"""
    assert eval(r, {"stim": stim}) == v


def test_call():
    t = stim.Tableau.from_named_gate("CNOT")
    assert t(stim.PauliString("__")) == stim.PauliString("__")
    assert t(stim.PauliString("-__")) == stim.PauliString("-__")
    assert t(stim.PauliString("i__")) == stim.PauliString("i__")
    assert t(stim.PauliString("-i__")) == stim.PauliString("-i__")
    assert t(stim.PauliString("X_")) == stim.PauliString("XX")
    assert t(stim.PauliString("Y_")) == stim.PauliString("YX")
    assert t(stim.PauliString("Z_")) == stim.PauliString("Z_")
    assert t(stim.PauliString("_X")) == stim.PauliString("_X")
    assert t(stim.PauliString("_Y")) == stim.PauliString("ZY")
    assert t(stim.PauliString("_Z")) == stim.PauliString("ZZ")
    assert t(stim.PauliString("YY")) == stim.PauliString("-XZ")
    assert t(stim.PauliString("-YY")) == stim.PauliString("XZ")


def test_pow():
    s = stim.Tableau.from_named_gate("S")
    s_dag = stim.Tableau.from_named_gate("S_DAG")
    z = stim.Tableau.from_named_gate("Z")
    assert stim.Tableau(1) == s**0 == s**4 == s**-4
    assert s == s**1 == s**5 == s**-3 == s**(40000 + 1) == s**(-40000 + 1)
    assert s_dag == s**-1 == s**3 == s**7 == s**(40000 + 3) == s**(-40000 + 3)
    assert z == s**2 == s**6 == s**-2 == s**(40000 + 2) == s**(-40000 + 2)


def test_aliasing():
    t = stim.Tableau.random(4)
    t2 = t**1
    t.append(t2, range(4))
    t2.append(t2, range(4))
    assert t == t2

    t = stim.Tableau.random(4)
    t2 = t**1
    t.prepend(t2, range(4))
    t2.prepend(t2, range(4))
    assert t == t2


def test_composition():
    assert stim.Tableau(0) * stim.Tableau(0) == stim.Tableau(0)
    assert stim.Tableau(0).then(stim.Tableau(0)) == stim.Tableau(0)
    assert stim.Tableau(1) * stim.Tableau(1) == stim.Tableau(1)
    assert stim.Tableau(1).then(stim.Tableau(1)) == stim.Tableau(1)

    t = stim.Tableau.random(4)
    t2 = stim.Tableau.random(4)
    t3 = t.then(t2)
    assert t3 == t2 * t
    p = stim.PauliString.random(4)
    assert t2(t(p)) == t3(p)

    with pytest.raises(ValueError, match="!= len"):
        _ = stim.Tableau(3) * stim.Tableau(4)
    with pytest.raises(ValueError, match="!= len"):
        _ = stim.Tableau(3).then(stim.Tableau(4))
