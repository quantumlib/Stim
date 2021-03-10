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


def test_equality():
    assert stim.PauliString(4) == stim.PauliString(4)
    assert stim.PauliString(3) != stim.PauliString(4)
    assert not (stim.PauliString(4) != stim.PauliString(4))
    assert not (stim.PauliString(3) == stim.PauliString(4))

    assert stim.PauliString("+X") == stim.PauliString("+X")
    assert stim.PauliString("+X") != stim.PauliString("-X")
    assert stim.PauliString("+X") != stim.PauliString("+Y")
    assert stim.PauliString("+X") != stim.PauliString("-Y")

    assert stim.PauliString("__") != stim.PauliString("_X")
    assert stim.PauliString("__") != stim.PauliString("X_")
    assert stim.PauliString("__") != stim.PauliString("XX")
    assert stim.PauliString("__") == stim.PauliString("__")


def test_random():
    p1 = stim.PauliString.random(100)
    p2 = stim.PauliString.random(100)
    assert p1 != p2


def test_str():
    assert str(stim.PauliString(3)) == "+___"
    assert str(stim.PauliString("XYZ")) == "+XYZ"
    assert str(stim.PauliString("-XYZ")) == "-XYZ"


def test_repr():
    assert repr(stim.PauliString(3)) == 'stim.PauliString("+___")'
    v = stim.PauliString("-XYZ")
    r = repr(v)
    assert r == 'stim.PauliString("-XYZ")'
    assert eval(r, {'stim': stim}) == v


def test_product():
    assert stim.PauliString("") * stim.PauliString("") == stim.PauliString("")

    x = stim.PauliString("X")
    y = stim.PauliString("Y")
    z = stim.PauliString("Z")

    assert x == +1 * x == x * +1 == +x
    assert x * -1 == -x == -1 * x
    assert (-x)[0] == 1
    assert (-x).sign == -1
    assert -(-x) == x

    with pytest.raises(ValueError, match="!= len"):
        _ = stim.PauliString(10) * stim.PauliString(11)
    with pytest.raises(ValueError, match="!= len"):
        _ = stim.PauliString(10).extended_product(stim.PauliString(11))

    with pytest.raises(ValueError, match="non-commut"):
        _ = x * z
    assert x * x == stim.PauliString(1)
    assert x.extended_product(y) == (1j, z)
    assert y.extended_product(x) == (1j, -z)
    assert x.extended_product(x) == (1, stim.PauliString(1))

    xx = stim.PauliString("+XX")
    yy = stim.PauliString("+YY")
    zz = stim.PauliString("+ZZ")
    assert xx * zz == -yy
    assert xx.extended_product(zz) == (1, -yy)


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
