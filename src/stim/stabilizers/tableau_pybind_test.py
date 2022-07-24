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

import numpy as np
import stim
import pytest


def test_init_equality():
    assert stim.Tableau(3) != None
    assert stim.Tableau(3) != object()
    assert stim.Tableau(3) != "another type"
    assert not (stim.Tableau(3) == None)
    assert not (stim.Tableau(3) == object())
    assert not (stim.Tableau(3) == "another type")

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

    assert stim.Tableau.from_named_gate("S") == stim.Tableau.from_conjugated_generators(
        xs=[
            stim.PauliString("Y"),
        ],
        zs=[
            stim.PauliString("Z"),
        ],
    )

    assert stim.Tableau.from_named_gate("S_DAG") == stim.Tableau.from_conjugated_generators(
        xs=[
            stim.PauliString("-Y"),
        ],
        zs=[
            stim.PauliString("Z"),
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

    with pytest.raises(ValueError, match="imag"):
        stim.Tableau.from_conjugated_generators(
            xs=[
                stim.PauliString("iX_"),
                stim.PauliString("_X"),
            ],
            zs=[
                stim.PauliString("Z_"),
                stim.PauliString("_Z"),
            ],
        )
    with pytest.raises(ValueError, match="imag"):
        stim.Tableau.from_conjugated_generators(
            xs=[
                stim.PauliString("X_"),
                stim.PauliString("_X"),
            ],
            zs=[
                stim.PauliString("Z_"),
                stim.PauliString("i_Z"),
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


def test_copy():
    t = stim.Tableau(3)
    t2 = t.copy()
    assert t == t2
    assert t is not t2


def test_hash():
    # stim.Tableau is mutable. It must not also be value-hashable.
    # Defining __hash__ requires defining a FrozenTableau variant instead.
    with pytest.raises(TypeError, match="unhashable"):
        _ = hash(stim.Tableau(1))


def test_add():
    h = stim.Tableau.from_named_gate("H")
    swap = stim.Tableau.from_named_gate("SWAP")
    cnot = stim.Tableau.from_named_gate("CNOT")
    combo = h + swap + cnot
    assert str(combo).strip() == """
+-xz-xz-xz-xz-xz-
| ++ ++ ++ ++ ++
| ZX __ __ __ __
| __ __ XZ __ __
| __ XZ __ __ __
| __ __ __ XZ _Z
| __ __ __ X_ XZ
    """.strip()

    alias = h
    h += swap
    h += cnot
    assert h == combo
    h += stim.Tableau(0)
    assert h == combo
    assert h is alias
    assert h is not combo
    assert swap == stim.Tableau.from_named_gate("SWAP")

    assert stim.Tableau(0) + stim.Tableau(0) == stim.Tableau(0)
    assert stim.Tableau(1) + stim.Tableau(2) == stim.Tableau(3)
    assert stim.Tableau(100) + stim.Tableau(500) == stim.Tableau(600)
    assert stim.Tableau(0) + cnot + stim.Tableau(0) == cnot

    x = stim.Tableau.from_named_gate("X")
    y = stim.Tableau.from_named_gate("Y")
    z = stim.Tableau.from_named_gate("Z")
    assert str(y + y).strip() == """
+-xz-xz-
| -- --
| XZ __
| __ XZ
    """.strip()
    assert str(x + x).strip() == """
+-xz-xz-
| +- +-
| XZ __
| __ XZ
    """.strip()
    assert str(z + z).strip() == """
+-xz-xz-
| -+ -+
| XZ __
| __ XZ
    """.strip()
    assert str(x + z).strip() == """
+-xz-xz-
| +- -+
| XZ __
| __ XZ
    """.strip()
    assert str(z + x).strip() == """
+-xz-xz-
| -+ +-
| XZ __
| __ XZ
    """.strip()


def test_xyz_output_pauli():
    t = stim.Tableau.from_conjugated_generators(
        xs=[
            stim.PauliString("-__Z"),
            stim.PauliString("+XZ_"),
            stim.PauliString("+_ZZ"),
        ],
        zs=[
            stim.PauliString("-YYY"),
            stim.PauliString("+Z_Z"),
            stim.PauliString("-ZYZ"),
        ],
    )

    assert t.x_output_pauli(0, 0) == 0
    assert t.x_output_pauli(0, 1) == 0
    assert t.x_output_pauli(0, 2) == 3
    assert t.x_output_pauli(1, 0) == 1
    assert t.x_output_pauli(1, 1) == 3
    assert t.x_output_pauli(1, 2) == 0
    assert t.x_output_pauli(2, 0) == 0
    assert t.x_output_pauli(2, 1) == 3
    assert t.x_output_pauli(2, 2) == 3

    assert t.y_output_pauli(0, 0) == 2
    assert t.y_output_pauli(0, 1) == 2
    assert t.y_output_pauli(0, 2) == 1
    assert t.y_output_pauli(1, 0) == 2
    assert t.y_output_pauli(1, 1) == 3
    assert t.y_output_pauli(1, 2) == 3
    assert t.y_output_pauli(2, 0) == 3
    assert t.y_output_pauli(2, 1) == 1
    assert t.y_output_pauli(2, 2) == 0

    assert t.z_output_pauli(0, 0) == 2
    assert t.z_output_pauli(0, 1) == 2
    assert t.z_output_pauli(0, 2) == 2
    assert t.z_output_pauli(1, 0) == 3
    assert t.z_output_pauli(1, 1) == 0
    assert t.z_output_pauli(1, 2) == 3
    assert t.z_output_pauli(2, 0) == 3
    assert t.z_output_pauli(2, 1) == 2
    assert t.z_output_pauli(2, 2) == 3

    with pytest.raises(TypeError):
        t.x_output_pauli(-1, 0)
    with pytest.raises(ValueError):
        t.x_output_pauli(3, 0)
    with pytest.raises(TypeError):
        t.x_output_pauli(0, -1)
    with pytest.raises(ValueError):
        t.x_output_pauli(0, 3)

    with pytest.raises(TypeError):
        t.y_output_pauli(-1, 0)
    with pytest.raises(ValueError):
        t.y_output_pauli(3, 0)
    with pytest.raises(TypeError):
        t.y_output_pauli(0, -1)
    with pytest.raises(ValueError):
        t.y_output_pauli(0, 3)

    with pytest.raises(TypeError):
        t.z_output_pauli(-1, 0)
    with pytest.raises(ValueError):
        t.z_output_pauli(3, 0)
    with pytest.raises(TypeError):
        t.z_output_pauli(0, -1)
    with pytest.raises(ValueError):
        t.z_output_pauli(0, 3)


def test_inverse_xyz_output_pauli():
    t = stim.Tableau.from_conjugated_generators(
        xs=[
            stim.PauliString("-__Z"),
            stim.PauliString("+XZ_"),
            stim.PauliString("+_ZZ"),
        ],
        zs=[
            stim.PauliString("-YYY"),
            stim.PauliString("+Z_Z"),
            stim.PauliString("-ZYZ"),
        ],
    ).inverse()

    assert t.inverse_x_output_pauli(0, 0) == 0
    assert t.inverse_x_output_pauli(0, 1) == 0
    assert t.inverse_x_output_pauli(0, 2) == 3
    assert t.inverse_x_output_pauli(1, 0) == 1
    assert t.inverse_x_output_pauli(1, 1) == 3
    assert t.inverse_x_output_pauli(1, 2) == 0
    assert t.inverse_x_output_pauli(2, 0) == 0
    assert t.inverse_x_output_pauli(2, 1) == 3
    assert t.inverse_x_output_pauli(2, 2) == 3

    assert t.inverse_y_output_pauli(0, 0) == 2
    assert t.inverse_y_output_pauli(0, 1) == 2
    assert t.inverse_y_output_pauli(0, 2) == 1
    assert t.inverse_y_output_pauli(1, 0) == 2
    assert t.inverse_y_output_pauli(1, 1) == 3
    assert t.inverse_y_output_pauli(1, 2) == 3
    assert t.inverse_y_output_pauli(2, 0) == 3
    assert t.inverse_y_output_pauli(2, 1) == 1
    assert t.inverse_y_output_pauli(2, 2) == 0

    assert t.inverse_z_output_pauli(0, 0) == 2
    assert t.inverse_z_output_pauli(0, 1) == 2
    assert t.inverse_z_output_pauli(0, 2) == 2
    assert t.inverse_z_output_pauli(1, 0) == 3
    assert t.inverse_z_output_pauli(1, 1) == 0
    assert t.inverse_z_output_pauli(1, 2) == 3
    assert t.inverse_z_output_pauli(2, 0) == 3
    assert t.inverse_z_output_pauli(2, 1) == 2
    assert t.inverse_z_output_pauli(2, 2) == 3

    with pytest.raises(TypeError):
        t.inverse_x_output_pauli(-1, 0)
    with pytest.raises(ValueError):
        t.inverse_x_output_pauli(3, 0)
    with pytest.raises(TypeError):
        t.inverse_x_output_pauli(0, -1)
    with pytest.raises(ValueError):
        t.inverse_x_output_pauli(0, 3)

    with pytest.raises(TypeError):
        t.inverse_y_output_pauli(-1, 0)
    with pytest.raises(ValueError):
        t.inverse_y_output_pauli(3, 0)
    with pytest.raises(TypeError):
        t.inverse_y_output_pauli(0, -1)
    with pytest.raises(ValueError):
        t.inverse_y_output_pauli(0, 3)

    with pytest.raises(TypeError):
        t.inverse_z_output_pauli(-1, 0)
    with pytest.raises(ValueError):
        t.inverse_z_output_pauli(3, 0)
    with pytest.raises(TypeError):
        t.inverse_z_output_pauli(0, -1)
    with pytest.raises(ValueError):
        t.inverse_z_output_pauli(0, 3)


def test_inverse_xyz_output():
    t = stim.Tableau.from_conjugated_generators(
        xs=[
            stim.PauliString("-__Z"),
            stim.PauliString("+XZ_"),
            stim.PauliString("+_ZZ"),
        ],
        zs=[
            stim.PauliString("-YYY"),
            stim.PauliString("+Z_Z"),
            stim.PauliString("-ZYZ"),
        ],
    )
    t_inv = t.inverse()

    for k in range(3):
        assert t_inv.inverse_x_output(k) == t.x_output(k)
        assert t_inv.inverse_y_output(k) == t.y_output(k)
        assert t_inv.inverse_z_output(k) == t.z_output(k)
        assert t_inv.inverse_x_output(k, unsigned=True) == t.x_output(k) / t.x_output(k).sign
        assert t_inv.inverse_y_output(k, unsigned=True) == t.y_output(k) / t.y_output(k).sign
        assert t_inv.inverse_z_output(k, unsigned=True) == t.z_output(k) / t.z_output(k).sign

    with pytest.raises(TypeError):
        t.inverse_x_output(-1)
    with pytest.raises(ValueError):
        t.inverse_x_output(3)

    with pytest.raises(TypeError):
        t.inverse_y_output(-1)
    with pytest.raises(ValueError):
        t.inverse_y_output(3)

    with pytest.raises(TypeError):
        t.inverse_z_output(-1)
    with pytest.raises(ValueError):
        t.inverse_z_output(3)


def test_inverse():
    t = stim.Tableau.from_conjugated_generators(
        xs=[
            stim.PauliString("+XXX"),
            stim.PauliString("-XZY"),
            stim.PauliString("+Z_Z"),
        ],
        zs=[
            stim.PauliString("-_XZ"),
            stim.PauliString("-_X_"),
            stim.PauliString("-X__"),
        ],
    )
    assert t.inverse() == t.inverse(unsigned=False) == stim.Tableau.from_conjugated_generators(
        xs=[
            stim.PauliString("-__Z"),
            stim.PauliString("-_Z_"),
            stim.PauliString("+XZZ"),
        ],
        zs=[
            stim.PauliString("+ZZX"),
            stim.PauliString("+YX_"),
            stim.PauliString("+ZZ_"),
        ],
    )

    assert t.inverse(unsigned=True) == stim.Tableau.from_conjugated_generators(
        xs=[
            stim.PauliString("+__Z"),
            stim.PauliString("+_Z_"),
            stim.PauliString("+XZZ"),
        ],
        zs=[
            stim.PauliString("+ZZX"),
            stim.PauliString("+YX_"),
            stim.PauliString("+ZZ_"),
        ],
    )


def test_pickle():
    import pickle
    t = stim.Tableau.random(4)
    a = pickle.dumps(t)
    assert pickle.loads(a) == t


def test_unitary():
    swap = stim.Tableau.from_named_gate("SWAP")
    np.testing.assert_array_equal(swap.to_unitary_matrix(endian='big'), [
        [1, 0, 0, 0],
        [0, 0, 1, 0],
        [0, 1, 0, 0],
        [0, 0, 0, 1],
    ])


def test_iter_1q():
    r = stim.Tableau.iter_all(1, unsigned=True)
    assert len(set(repr(e) for e in r)) == 6
    assert len(set(repr(e) for e in r)) == 6  # Can re-iterate.
    assert sum(1 for _ in stim.Tableau.iter_all(1)) == 24


def test_iter_2q():
    u2 = stim.Tableau.iter_all(2, unsigned=True)
    assert sum(1 for _ in u2) == 720
    assert sum(1 for _ in stim.Tableau.iter_all(2, unsigned=False)) == 11520
    assert len(set(repr(e) for e in u2)) == 720


def test_iter_3q():
    n = 0
    for _ in stim.Tableau.iter_all(3, unsigned=True):
        n += 1
    assert n == 1451520


def test_from_unitary_matrix():
    s = 0.5**0.5
    t = stim.Tableau.from_unitary_matrix([
        [s, s],
        [s, -s]
    ], endian='little')
    assert t == stim.Tableau.from_named_gate("H")

    with pytest.raises(ValueError, match="Clifford operation"):
        stim.Tableau.from_unitary_matrix([
            [1, 0],
            [0, 0],
        ], endian='little')


def test_to_circuit_vs_from_circuit():
    t = stim.Tableau.random(4)
    c = t.to_circuit(method="elimination")
    sim = stim.TableauSimulator()
    sim.do_circuit(c)
    assert sim.current_inverse_tableau().inverse() == t
    assert stim.Tableau.from_circuit(c) == t
