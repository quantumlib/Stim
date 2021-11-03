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

import stim
import pytest


def test_identity():
    p = stim.PauliString(3)
    assert len(p) == 3
    assert p[0] == p[1] == p[2] == 0
    assert p.sign == +1


def test_from_str():
    p = stim.PauliString("-_XYZ_ZYX")
    assert len(p) == 8
    assert p[0] == 0
    assert p[1] == 1
    assert p[2] == 2
    assert p[3] == 3
    assert p[4] == 0
    assert p[5] == 3
    assert p[6] == 2
    assert p[7] == 1
    assert p.sign == -1

    p = stim.PauliString("")
    assert len(p) == 0
    assert p.sign == +1

    p = stim.PauliString("X")
    assert len(p) == 1
    assert p[0] == 1
    assert p.sign == +1

    p = stim.PauliString("+X")
    assert len(p) == 1
    assert p[0] == 1
    assert p.sign == +1

    p = stim.PauliString("iX")
    assert len(p) == 1
    assert p[0] == 1
    assert p.sign == 1j

    p = stim.PauliString("+iX")
    assert len(p) == 1
    assert p[0] == 1
    assert p.sign == 1j

    p = stim.PauliString("-iX")
    assert len(p) == 1
    assert p[0] == 1
    assert p.sign == -1j


def test_equality():
    assert not (stim.PauliString(4) == None)
    assert not (stim.PauliString(4) == "other object")
    assert not (stim.PauliString(4) == object())
    assert stim.PauliString(4) != None
    assert stim.PauliString(4) != "other object"
    assert stim.PauliString(4) != object()

    assert stim.PauliString(4) == stim.PauliString(4)
    assert stim.PauliString(3) != stim.PauliString(4)
    assert not (stim.PauliString(4) != stim.PauliString(4))
    assert not (stim.PauliString(3) == stim.PauliString(4))

    assert stim.PauliString("+X") == stim.PauliString("+X")
    assert stim.PauliString("+X") != stim.PauliString("-X")
    assert stim.PauliString("+X") != stim.PauliString("+Y")
    assert stim.PauliString("+X") != stim.PauliString("-Y")
    assert stim.PauliString("+X") != stim.PauliString("+iX")
    assert stim.PauliString("+X") != stim.PauliString("-iX")

    assert stim.PauliString("__") != stim.PauliString("_X")
    assert stim.PauliString("__") != stim.PauliString("X_")
    assert stim.PauliString("__") != stim.PauliString("XX")
    assert stim.PauliString("__") == stim.PauliString("__")


def test_random():
    p1 = stim.PauliString.random(100)
    p2 = stim.PauliString.random(100)
    assert p1 != p2

    seen_signs = {stim.PauliString.random(1).sign for _ in range(200)}
    assert seen_signs == {1, -1}

    seen_signs = {stim.PauliString.random(1, allow_imaginary=True).sign for _ in range(200)}
    assert seen_signs == {1, -1, 1j, -1j}


def test_str():
    assert str(stim.PauliString(3)) == "+___"
    assert str(stim.PauliString("XYZ")) == "+XYZ"
    assert str(stim.PauliString("-XYZ")) == "-XYZ"
    assert str(stim.PauliString("iXYZ")) == "+iXYZ"
    assert str(stim.PauliString("-iXYZ")) == "-iXYZ"


def test_repr():
    assert repr(stim.PauliString(3)) == 'stim.PauliString("+___")'
    assert repr(stim.PauliString("-XYZ")) == 'stim.PauliString("-XYZ")'
    vs = [
        stim.PauliString(""),
        stim.PauliString("ZXYZZ"),
        stim.PauliString("-XYZ"),
        stim.PauliString("I"),
        stim.PauliString("iIXYZ"),
        stim.PauliString("-iIXYZ"),
    ]
    for v in vs:
        r = repr(v)
        assert eval(r, {'stim': stim}) == v


def test_commutes():
    def c(a: str, b: str) -> bool:
        return stim.PauliString(a).commutes(stim.PauliString(b))

    assert c("", "")
    assert c("X", "_")
    assert c("X", "X")
    assert not c("X", "Y")
    assert not c("X", "Z")

    assert c("XXXX", "YYYY")
    assert c("XXXX", "YYYZ")
    assert not c("XXXX", "XXXZ")
    assert not c("XXXX", "___Z")
    assert not c("XXXX", "Z___")
    assert c("XXXX", "Z_Z_")


def test_product():
    assert stim.PauliString("") * stim.PauliString("") == stim.PauliString("")
    assert stim.PauliString("i") * stim.PauliString("i") == stim.PauliString("-")
    assert stim.PauliString("i") * stim.PauliString("-i") == stim.PauliString("+")
    assert stim.PauliString("-i") * stim.PauliString("-i") == stim.PauliString("-")
    assert stim.PauliString("i") * stim.PauliString("-") == stim.PauliString("-i")

    x = stim.PauliString("X")
    y = stim.PauliString("Y")
    z = stim.PauliString("Z")

    assert x == +1 * x == x * +1 == +x
    assert x * -1 == -x == -1 * x
    assert (-x)[0] == 1
    assert (-x).sign == -1
    assert -(-x) == x

    assert stim.PauliString(10) * stim.PauliString(11) == stim.PauliString(11)

    assert x * z == stim.PauliString("-iY")
    assert x * x == stim.PauliString(1)
    assert x * y == stim.PauliString("iZ")
    assert y * x == stim.PauliString("-iZ")
    assert x * y == 1j * z
    assert y * x == z * -1j
    assert x.extended_product(y) == (1, 1j * z)
    assert y.extended_product(x) == (1, -1j * z)
    assert x.extended_product(x) == (1, stim.PauliString(1))

    xx = stim.PauliString("+XX")
    yy = stim.PauliString("+YY")
    zz = stim.PauliString("+ZZ")
    assert xx * zz == -yy
    assert xx.extended_product(zz) == (1, -yy)


def test_inplace_product():
    p = stim.PauliString("X")
    alias = p

    p *= 1j
    assert alias == stim.PauliString("iX")
    assert alias is p
    p *= 1j
    assert alias == stim.PauliString("-X")
    p *= 1j
    assert alias == stim.PauliString("-iX")
    p *= 1j
    assert alias == stim.PauliString("+X")

    p *= stim.PauliString("Z")
    assert alias == stim.PauliString("-iY")

    p *= -1j
    assert alias == stim.PauliString("-Y")
    p *= -1j
    assert alias == stim.PauliString("iY")
    p *= -1j
    assert alias == stim.PauliString("+Y")
    p *= -1j
    assert alias == stim.PauliString("-iY")

    p *= stim.PauliString("i_")
    assert alias == stim.PauliString("+Y")
    p *= stim.PauliString("i_")
    assert alias == stim.PauliString("iY")
    p *= stim.PauliString("i_")
    assert alias == stim.PauliString("-Y")
    p *= stim.PauliString("i_")
    assert alias == stim.PauliString("-iY")

    p *= stim.PauliString("-i_")
    assert alias == stim.PauliString("-Y")
    p *= stim.PauliString("-i_")
    assert alias == stim.PauliString("iY")
    p *= stim.PauliString("-i_")
    assert alias == stim.PauliString("+Y")
    p *= stim.PauliString("-i_")
    assert alias == stim.PauliString("-iY")

    assert alias is p


def test_imaginary_phase():
    p = stim.PauliString("IXYZ")
    ip = stim.PauliString("iIXYZ")
    assert 1j * p == p * 1j == ip == -stim.PauliString("-iIXYZ")
    assert p.sign == 1
    assert (-p).sign == -1
    assert ip.sign == 1j
    assert (-ip).sign == -1j
    assert stim.PauliString("X") * stim.PauliString("Y") == 1j * stim.PauliString("Z")
    assert stim.PauliString("Y") * stim.PauliString("X") == -1j * stim.PauliString("Z")


def test_get_set_sign():
    p = stim.PauliString(2)
    assert p.sign == +1
    p.sign = -1
    assert str(p) == "-__"
    assert p.sign == -1
    p.sign = +1
    assert str(p) == "+__"
    assert p.sign == +1
    with pytest.raises(ValueError, match="new_sign"):
        p.sign = 5

    p.sign = 1j
    assert str(p) == "+i__"
    assert p.sign == 1j

    p.sign = -1j
    assert str(p) == "-i__"
    assert p.sign == -1j


def test_get_set_item():
    p = stim.PauliString(5)
    assert list(p) == [0, 0, 0, 0, 0]
    assert p[0] == 0
    p[0] = 1
    assert p[0] == 1
    p[0] = 'Y'
    assert p[0] == 2
    p[0] = 'Z'
    assert p[0] == 3

    with pytest.raises(IndexError, match="new_pauli"):
        p[0] = 't'
    with pytest.raises(IndexError, match="new_pauli"):
        p[0] = 10

    assert p[1] == 0
    p[1] = 2
    assert p[1] == 2


def test_get_slice():
    p = stim.PauliString("XXXX__YYYY__ZZZZX")
    assert p[:7] == stim.PauliString("XXXX__Y")
    assert p[:-3] == stim.PauliString("XXXX__YYYY__ZZ")
    assert p[::2] == stim.PauliString("XX_YY_ZZX")
    assert p[::-1] == stim.PauliString("XZZZZ__YYYY__XXXX")
    assert p[-3:3] == stim.PauliString("")
    assert p[-6:-1] == stim.PauliString("_ZZZZ")
    assert p[3:5:-1] == stim.PauliString("")
    assert p[5:3:-1] == stim.PauliString("__")
    assert p[4:2:-1] == stim.PauliString("_X")
    assert p[2:0:-1] == stim.PauliString("XX")


def test_copy():
    p = stim.PauliString(3)
    p2 = p.copy()
    assert p == p2
    assert p is not p2

    p = stim.PauliString("-i_XYZ")
    p2 = p.copy()
    assert p == p2
    assert p is not p2


def test_hash():
    # stim.PauliString is mutable. It must not also be value-hashable.
    # Defining __hash__ requires defining a FrozenPauliString variant instead.
    with pytest.raises(TypeError, match="unhashable"):
        _ = hash(stim.PauliString(1))


def test_add():
    ps = stim.PauliString
    assert ps(0) + ps(0) == ps(0)
    assert ps(3) + ps(1000) == ps(1003)
    assert ps(1000) + ps(3) == ps(1003)
    assert ps("_XYZ") + ps("_ZZZ_") == ps("_XYZ_ZZZ_")

    p = ps("_XYZ")
    p += p
    assert p == ps("_XYZ_XYZ")
    for k in range(1, 8):
        p += p
        assert p == ps("_XYZ_XYZ" * 2**k)

    p = ps("_XXX")
    p += ps("Y")
    assert p == ps("_XXXY")

    p = ps("")
    alias = p
    p += ps("X")
    assert alias is p
    assert alias == ps("X")
    p += p
    assert alias is p
    assert alias == ps("XX")


def test_mul_different_sizes():
    ps = stim.PauliString
    assert ps("") * ps("X" * 1000) == ps("X" * 1000)
    assert ps("X" * 1000) * ps("") == ps("X" * 1000)
    assert ps("Z" * 1000) * ps("") == ps("Z" * 1000)

    p = ps("Z")
    alias = p
    p *= ps("ZZZ")
    assert p == ps("_ZZ")
    p *= ps("Z")
    assert p == ps("ZZZ")
    assert alias is p


def test_div():
    assert stim.PauliString("+XYZ") / +1 == stim.PauliString("+XYZ")
    assert stim.PauliString("+XYZ") / -1 == stim.PauliString("-XYZ")
    assert stim.PauliString("+XYZ") / 1j == stim.PauliString("-iXYZ")
    assert stim.PauliString("+XYZ") / -1j == stim.PauliString("iXYZ")
    assert stim.PauliString("iXYZ") / 1j == stim.PauliString("XYZ")
    p = stim.PauliString("__")
    alias = p
    assert p / -1 == stim.PauliString("-__")
    assert alias == stim.PauliString("__")
    p /= -1
    assert alias == stim.PauliString("-__")
    p /= 1j
    assert alias == stim.PauliString("i__")
    p /= 1j
    assert alias == stim.PauliString("__")
    p /= -1j
    assert alias == stim.PauliString("i__")
    p /= 1
    assert alias == stim.PauliString("i__")


def test_mul_repeat():
    ps = stim.PauliString
    assert ps("") * 100 == ps("")
    assert ps("X") * 100 == ps("X" * 100)
    assert ps("XYZ_") * 1000 == ps("XYZ_" * 1000)
    assert ps("XYZ_") * 1 == ps("XYZ_")
    assert ps("XYZ_") * 0 == ps("")

    assert 100 * ps("") == ps("")
    assert 100 * ps("X") == ps("X" * 100)
    assert 1000 * ps("XYZ_") == ps("XYZ_" * 1000)
    assert 1 * ps("XYZ_") == ps("XYZ_")
    assert 0 * ps("XYZ_") == ps("")

    assert ps("i") * 0 == ps("+")
    assert ps("i") * 1 == ps("i")
    assert ps("i") * 2 == ps("-")
    assert ps("i") * 3 == ps("-i")
    assert ps("i") * 4 == ps("+")
    assert ps("i") * 5 == ps("i")

    assert ps("-i") * 0 == ps("+")
    assert ps("-i") * 1 == ps("-i")
    assert ps("-i") * 2 == ps("-")
    assert ps("-i") * 3 == ps("i")
    assert ps("-i") * 4 == ps("+")
    assert ps("-i") * 5 == ps("-i")

    assert ps("-") * 0 == ps("+")
    assert ps("-") * 1 == ps("-")
    assert ps("-") * 2 == ps("+")
    assert ps("-") * 3 == ps("-")
    assert ps("-") * 4 == ps("+")
    assert ps("-") * 5 == ps("-")

    p = ps("XYZ")
    alias = p
    p *= 1000
    assert p == ps("XYZ" * 1000)
    assert alias is p


def test_init_list():
    assert stim.PauliString([]) == stim.PauliString(0)
    assert stim.PauliString([0, 1, 2, 3]) == stim.PauliString("_XYZ")

    with pytest.raises(ValueError, match="pauli"):
        _ = stim.PauliString([-1])
    with pytest.raises(ValueError, match="pauli"):
        _ = stim.PauliString([4])
    with pytest.raises(TypeError):
        _ = stim.PauliString([2**500])


def test_init_copy():
    p = stim.PauliString("_XYZ")
    p2 = stim.PauliString(p)
    assert p is not p2
    assert p == p2

    p = stim.PauliString("-i_XYZ")
    p2 = stim.PauliString(p)
    assert p is not p2
    assert p == p2


def test_commutes_different_lengths():
    x1000 = stim.PauliString("X" * 1000)
    z1000 = stim.PauliString("Z" * 1000)
    x1 = stim.PauliString("X")
    z1 = stim.PauliString("Z")
    assert x1.commutes(x1000)
    assert x1000.commutes(x1)
    assert z1.commutes(z1000)
    assert z1000.commutes(z1)
    assert not z1.commutes(x1000)
    assert not x1000.commutes(z1)
    assert not x1.commutes(z1000)
    assert not z1000.commutes(x1)


def test_pickle():
    import pickle

    t = stim.PauliString.random(4)
    a = pickle.dumps(t)
    assert pickle.loads(a) == t

    t = stim.PauliString("i_XYZ")
    a = pickle.dumps(t)
    assert pickle.loads(a) == t
