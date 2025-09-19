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
import itertools
import numpy as np
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

    assert stim.PauliString("X5*Y10") == stim.PauliString("_____X____Y")
    assert stim.PauliString("X5*Y5") == stim.PauliString("iZ5")


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

def test_to_tableau():
    p = stim.PauliString("XZ_Y")
    t = p.to_tableau()
    assert t.x_output(0) == stim.PauliString("+X___")
    assert t.x_output(1) == stim.PauliString("-_X__")
    assert t.x_output(2) == stim.PauliString("+__X_")
    assert t.x_output(3) == stim.PauliString("-___X")
    assert t.z_output(0) == stim.PauliString("-Z___")
    assert t.z_output(1) == stim.PauliString("+_Z__")
    assert t.z_output(2) == stim.PauliString("+__Z_")
    assert t.z_output(3) == stim.PauliString("-___Z")

    p_random = stim.PauliString.random(32)
    p_random.sign = 1
    p_random_roundtrip = p_random.to_tableau().to_pauli_string()
    assert p_random == p_random_roundtrip

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
    with pytest.raises(ValueError):
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


def test_to_numpy():
    p = stim.PauliString("_XYZ___XYXZYZ")

    xs, zs = p.to_numpy()
    assert xs.dtype == np.bool_
    assert zs.dtype == np.bool_
    np.testing.assert_array_equal(xs, [0, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0])
    np.testing.assert_array_equal(zs, [0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 1, 1, 1])

    xs, zs = p.to_numpy(bit_packed=True)
    assert xs.dtype == np.uint8
    assert zs.dtype == np.uint8
    np.testing.assert_array_equal(xs, [0x86, 0x0B])
    np.testing.assert_array_equal(zs, [0x0C, 0x1D])


def test_from_numpy():
    p = stim.PauliString.from_numpy(
        xs=np.array([0, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0], dtype=np.bool_),
        zs=np.array([0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 1, 1, 1], dtype=np.bool_))
    assert p == stim.PauliString("_XYZ___XYXZYZ")

    p = stim.PauliString.from_numpy(
        xs=np.array([0x86, 0x0B], dtype=np.uint8),
        zs=np.array([0x0C, 0x1D], dtype=np.uint8),
        num_qubits=13)

    assert p == stim.PauliString("_XYZ___XYXZYZ")
    p = stim.PauliString.from_numpy(
        xs=np.array([0x86, 0x0B], dtype=np.uint8),
        zs=np.array([0x0C, 0x1D], dtype=np.uint8),
        num_qubits=15,
        sign=1j)
    assert p == stim.PauliString("i_XYZ___XYXZYZ__")


def test_from_numpy_bad_bit_packed_len():
    xs = np.array([0x86, 0x0B], dtype=np.uint8)
    zs = np.array([0x0C, 0x1D], dtype=np.uint8)
    with pytest.raises(ValueError, match="specify expected number"):
        stim.PauliString.from_numpy(xs=xs, zs=zs)

    with pytest.raises(ValueError, match="between 9 and 16 bits"):
        stim.PauliString.from_numpy(xs=xs, zs=zs, num_qubits=100)

    with pytest.raises(ValueError, match="between 9 and 16 bits"):
        stim.PauliString.from_numpy(xs=xs, zs=zs, num_qubits=0)

    with pytest.raises(ValueError, match="between 9 and 16 bits"):
        stim.PauliString.from_numpy(xs=xs, zs=zs, num_qubits=8)

    with pytest.raises(ValueError, match="between 9 and 16 bits"):
        stim.PauliString.from_numpy(xs=xs, zs=zs, num_qubits=17)

    with pytest.raises(ValueError, match="between 0 and 0 bits"):
        stim.PauliString.from_numpy(xs=xs[:0], zs=zs, num_qubits=9)

    with pytest.raises(ValueError, match="between 1 and 8 bits"):
        stim.PauliString.from_numpy(xs=xs[:1], zs=zs, num_qubits=9)

    with pytest.raises(ValueError, match="between 1 and 8 bits"):
        stim.PauliString.from_numpy(xs=xs, zs=zs[:1], num_qubits=9)

    with pytest.raises(ValueError, match="1-dimensional"):
        stim.PauliString.from_numpy(xs=np.array([xs, xs]), zs=np.array([zs, zs]), num_qubits=9)

    with pytest.raises(ValueError, match="uint8"):
        stim.PauliString.from_numpy(xs=np.array(xs, dtype=np.uint64), zs=np.array(xs, dtype=np.uint64), num_qubits=9)


def test_from_numpy_bad_bool_len():
    xs = np.array([0, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0], dtype=np.bool_)
    zs = np.array([0, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0], dtype=np.bool_)
    with pytest.raises(ValueError, match="shape=13"):
        stim.PauliString.from_numpy(xs=xs, zs=zs, num_qubits=12)

    with pytest.raises(ValueError, match="shape=13"):
        stim.PauliString.from_numpy(xs=xs, zs=zs, num_qubits=14)

    with pytest.raises(ValueError, match="shape=12"):
        stim.PauliString.from_numpy(xs=xs[:-1], zs=zs, num_qubits=13)

    with pytest.raises(ValueError, match="shape=12"):
        stim.PauliString.from_numpy(xs=xs, zs=zs[:-1], num_qubits=13)

    with pytest.raises(ValueError, match="Inconsistent"):
        stim.PauliString.from_numpy(xs=xs, zs=zs[:-1])

    with pytest.raises(RuntimeError, match="Unable to cast"):
        stim.PauliString.from_numpy(xs=xs, zs=zs, num_qubits=-1)


@pytest.mark.parametrize("n", [0, 1, 41, 42, 1023, 1024, 1025])
def test_to_from_numpy_round_trip(n: int):
    p = stim.PauliString.random(n)
    xs, zs = p.to_numpy()
    p2 = stim.PauliString.from_numpy(xs=xs, zs=zs, sign=p.sign)
    assert p2 == p
    xs, zs = p.to_numpy(bit_packed=True)
    p2 = stim.PauliString.from_numpy(xs=xs, zs=zs, num_qubits=n, sign=p.sign)
    assert p2 == p


def test_to_unitary_matrix():
    np.testing.assert_array_equal(
        stim.PauliString("").to_unitary_matrix(endian="little"),
        [[1]],
    )
    np.testing.assert_array_equal(
        stim.PauliString("-").to_unitary_matrix(endian="big"),
        [[-1]],
    )
    np.testing.assert_array_equal(
        stim.PauliString("i").to_unitary_matrix(endian="big"),
        [[1j]],
    )
    np.testing.assert_array_equal(
        stim.PauliString("-i").to_unitary_matrix(endian="big"),
        [[-1j]],
    )

    np.testing.assert_array_equal(
        stim.PauliString("I").to_unitary_matrix(endian="little"),
        [[1, 0], [0, 1]],
    )
    np.testing.assert_array_equal(
        stim.PauliString("X").to_unitary_matrix(endian="little"),
        [[0, 1], [1, 0]],
    )
    np.testing.assert_array_equal(
        stim.PauliString("Y").to_unitary_matrix(endian="little"),
        [[0, -1j], [1j, 0]],
    )
    np.testing.assert_array_equal(
        stim.PauliString("iY").to_unitary_matrix(endian="little"),
        [[0, 1], [-1, 0]],
    )
    np.testing.assert_array_equal(
        stim.PauliString("Z").to_unitary_matrix(endian="little"),
        [[1, 0], [0, -1]],
    )
    np.testing.assert_array_equal(
        stim.PauliString("-Z").to_unitary_matrix(endian="little"),
        [[-1, 0], [0, 1]],
    )
    np.testing.assert_array_equal(
        stim.PauliString("YY").to_unitary_matrix(endian="little"),
        [[0, 0, 0, -1], [0, 0, 1, 0], [0, 1, 0, 0], [-1, 0, 0, 0]],
    )
    np.testing.assert_array_equal(
        stim.PauliString("-YZ").to_unitary_matrix(endian="little"),
        [[0, 1j, 0, 0], [-1j, 0, 0, 0], [0, 0, 0, -1j], [0, 0, 1j, 0]],
    )
    np.testing.assert_array_equal(
        stim.PauliString("XYZ").to_unitary_matrix(endian="little"), [
            [0, 0, 0, -1j, 0, 0, 0, 0],
            [0, 0, -1j, 0, 0, 0, 0, 0],
            [0, 1j, 0, 0, 0, 0, 0, 0],
            [1j, 0, 0, 0, 0, 0, 0, 0],
            [0, 0, 0, 0, 0, 0, 0, 1j],
            [0, 0, 0, 0, 0, 0, 1j, 0],
            [0, 0, 0, 0, 0, -1j, 0, 0],
            [0, 0, 0, 0, -1j, 0, 0, 0],
        ])
    np.testing.assert_array_equal(
        stim.PauliString("ZYX").to_unitary_matrix(endian="big"), [
            [0, 0, 0, -1j, 0, 0, 0, 0],
            [0, 0, -1j, 0, 0, 0, 0, 0],
            [0, 1j, 0, 0, 0, 0, 0, 0],
            [1j, 0, 0, 0, 0, 0, 0, 0],
            [0, 0, 0, 0, 0, 0, 0, 1j],
            [0, 0, 0, 0, 0, 0, 1j, 0],
            [0, 0, 0, 0, 0, -1j, 0, 0],
            [0, 0, 0, 0, -1j, 0, 0, 0],
        ])


def test_from_unitary_matrix():
    assert stim.PauliString.from_unitary_matrix(
        [[1]]
    ) == stim.PauliString("")
    assert stim.PauliString.from_unitary_matrix(
        [[-1]]
    ) == stim.PauliString("-")
    assert stim.PauliString.from_unitary_matrix(
        [[1j]]
    ) == stim.PauliString("i")
    assert stim.PauliString.from_unitary_matrix(
        [[-1j]]
    ) == stim.PauliString("-i")

    assert stim.PauliString.from_unitary_matrix(
        [[1, 0], [0, 1]]
    ) == stim.PauliString("I")
    assert stim.PauliString.from_unitary_matrix(
        [[0, 1], [1, 0]]
    ) == stim.PauliString("X")
    assert stim.PauliString.from_unitary_matrix(
        [[0, -1j], [1j, 0]]
    ) == stim.PauliString("Y")
    assert stim.PauliString.from_unitary_matrix(
        [[1, 0], [0, -1]]
    ) == stim.PauliString("Z")

    assert stim.PauliString.from_unitary_matrix(
        [[0, 1], [-1, 0]]
    ) == stim.PauliString("iY")
    assert stim.PauliString.from_unitary_matrix(
        [[0, 1j], [-1j, 0]]
    ) == stim.PauliString("-Y")
    assert stim.PauliString.from_unitary_matrix(
        [[1j, 0], [0, -1j]]
    ) == stim.PauliString("iZ")
    assert stim.PauliString.from_unitary_matrix(
        [[-1, 0], [0, 1]]
    ) == stim.PauliString("-Z")

    assert stim.PauliString.from_unitary_matrix(
        [[1]], unsigned=True
    ) == stim.PauliString("")
    assert stim.PauliString.from_unitary_matrix(
        [[-1]], unsigned=True
    ) == stim.PauliString("")
    assert stim.PauliString.from_unitary_matrix(
        [[0, 1], [-1, 0]], unsigned=True
    ) == stim.PauliString("Y")
    assert stim.PauliString.from_unitary_matrix(
        [[0, +1 * 1j**0.1], [-1 * 1j**0.1, 0]], unsigned=True
    ) == stim.PauliString("Y")

    assert stim.PauliString.from_unitary_matrix([
        [0, 0, 0, -1j, 0, 0, 0, 0],
        [0, 0, -1j, 0, 0, 0, 0, 0],
        [0, 1j, 0, 0, 0, 0, 0, 0],
        [1j, 0, 0, 0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0, 0, 0, 1j],
        [0, 0, 0, 0, 0, 0, 1j, 0],
        [0, 0, 0, 0, 0, -1j, 0, 0],
        [0, 0, 0, 0, -1j, 0, 0, 0],
    ], endian="little") == stim.PauliString("XYZ")
    assert stim.PauliString.from_unitary_matrix([
        [0, 0, 0, -1j, 0, 0, 0, 0],
        [0, 0, -1j, 0, 0, 0, 0, 0],
        [0, 1j, 0, 0, 0, 0, 0, 0],
        [1j, 0, 0, 0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0, 0, 0, 1j],
        [0, 0, 0, 0, 0, 0, 1j, 0],
        [0, 0, 0, 0, 0, -1j, 0, 0],
        [0, 0, 0, 0, -1j, 0, 0, 0],
    ], endian="big") == stim.PauliString("ZYX")
    assert stim.PauliString.from_unitary_matrix(np.array([
        [0, 0, 0, -1j, 0, 0, 0, 0],
        [0, 0, -1j, 0, 0, 0, 0, 0],
        [0, 1j, 0, 0, 0, 0, 0, 0],
        [1j, 0, 0, 0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0, 0, 0, 1j],
        [0, 0, 0, 0, 0, 0, 1j, 0],
        [0, 0, 0, 0, 0, -1j, 0, 0],
        [0, 0, 0, 0, -1j, 0, 0, 0],
    ]) * 1j**0.1, endian="big", unsigned=True) == stim.PauliString("ZYX")
    assert stim.PauliString.from_unitary_matrix(np.array([
        [0, 0, 0, -1j, 0, 0, 0, 0],
        [0, 0, -1j, 0, 0, 0, 0, 0],
        [0, 1j, 0, 0, 0, 0, 0, 0],
        [1j, 0, 0, 0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0, 0, 0, 1j],
        [0, 0, 0, 0, 0, 0, 1j, 0],
        [0, 0, 0, 0, 0, -1j, 0, 0],
        [0, 0, 0, 0, -1j, 0, 0, 0],
    ]) * -1, endian="big", unsigned=True) == stim.PauliString("ZYX")


def test_from_unitary_matrix_detect_bad_matrix():
    with pytest.raises(ValueError, match="power of 2"):
        stim.PauliString.from_unitary_matrix([])
    with pytest.raises(ValueError, match="row with no non-zero"):
        stim.PauliString.from_unitary_matrix([[]])
    with pytest.raises(ValueError, match="row with no non-zero"):
        stim.PauliString.from_unitary_matrix([[0]])
    with pytest.raises(ValueError, match="values besides 0, 1,"):
        stim.PauliString.from_unitary_matrix([[0.5]])
    with pytest.raises(ValueError, match="isn't square"):
        stim.PauliString.from_unitary_matrix([[1, 0]])
    with pytest.raises(ValueError, match="no non-zero entries"):
        stim.PauliString.from_unitary_matrix([[1], [0]])
    with pytest.raises(ValueError, match="different lengths"):
        stim.PauliString.from_unitary_matrix([[0, 1], [1]])
    with pytest.raises(ValueError, match="two non-zero entries"):
        stim.PauliString.from_unitary_matrix([[1, 1],
                                              [0, 1]])
    with pytest.raises(ValueError, match="which qubits are flipped"):
        stim.PauliString.from_unitary_matrix([[1, 0],
                                              [1, 0]])
    with pytest.raises(ValueError, match="isn't square"):
        stim.PauliString.from_unitary_matrix([[1, 0, 0],
                                              [0, 1, 0]])
    with pytest.raises(ValueError, match="consistent phase flips"):
        stim.PauliString.from_unitary_matrix([[1, 0],
                                              [0, 1j]])

    with pytest.raises(ValueError, match="power of 2"):
        stim.PauliString.from_unitary_matrix([[1, 0, 0],
                                              [0, 1, 0],
                                              [0, 0, 1]])
    with pytest.raises(ValueError, match="which qubits are flipped"):
        stim.PauliString.from_unitary_matrix([[1, 0, 0, 0],
                                              [0, 1, 0, 0],
                                              [0, 0, 0, 1],
                                              [0, 0, 1, 0]])
    with pytest.raises(ValueError, match="consistent phase flips"):
        stim.PauliString.from_unitary_matrix([[1, 0, 0, 0],
                                              [0, 1, 0, 0],
                                              [0, 0, 1, 0],
                                              [0, 0, 0, -1]])
    with pytest.raises(ValueError, match="consistent phase flips"):
        stim.PauliString.from_unitary_matrix([[1, 0, 0, 0],
                                              [0, 1, 0, 0],
                                              [0, 0, -1, 0],
                                              [0, 0, 0, 1]])


@pytest.mark.parametrize("n,endian", itertools.product(range(8), ['little', 'big']))
def test_fuzz_to_from_unitary_matrix(n: int, endian: str):
    p = stim.PauliString.random(n, allow_imaginary=True)
    u = p.to_unitary_matrix(endian=endian)
    r = stim.PauliString.from_unitary_matrix(u, endian=endian)
    assert p == r

    via_tableau = stim.Tableau.from_unitary_matrix(u, endian=endian).to_pauli_string()
    r.sign = +1
    assert via_tableau == r


def test_before_after():
    before = stim.PauliString("XXXYYYZZZ")
    after = stim.PauliString("XYXYZYXZZ")
    assert before.after(stim.Circuit("C_XYZ 1 4 6")) == after
    assert before.after(stim.Circuit("C_XYZ 1 4 6")[0]) == after
    assert before.after(stim.Tableau.from_named_gate("C_XYZ"), targets=[1, 4, 6]) == after
    assert after.before(stim.Circuit("C_XYZ 1 4 6")) == before
    assert after.before(stim.Circuit("C_XYZ 1 4 6")[0]) == before
    assert after.before(stim.Tableau.from_named_gate("C_XYZ"), targets=[1, 4, 6]) == before


def test_iter_small():
    assert list(stim.PauliString.iter_all(0)) == [stim.PauliString(0)]
    assert list(stim.PauliString.iter_all(1)) == [
        stim.PauliString("_"),
        stim.PauliString("X"),
        stim.PauliString("Y"),
        stim.PauliString("Z"),
    ]
    assert list(stim.PauliString.iter_all(1, max_weight=-1)) == [
    ]
    assert list(stim.PauliString.iter_all(1, max_weight=0)) == [
        stim.PauliString("_"),
    ]
    assert list(stim.PauliString.iter_all(1, max_weight=1)) == [
        stim.PauliString("_"),
        stim.PauliString("X"),
        stim.PauliString("Y"),
        stim.PauliString("Z"),
    ]
    assert list(stim.PauliString.iter_all(1, min_weight=1, max_weight=1)) == [
        stim.PauliString("X"),
        stim.PauliString("Y"),
        stim.PauliString("Z"),
    ]
    assert list(stim.PauliString.iter_all(2, min_weight=1, max_weight=1, allowed_paulis="XY")) == [
        stim.PauliString("X_"),
        stim.PauliString("Y_"),
        stim.PauliString("_X"),
        stim.PauliString("_Y"),
    ]

    with pytest.raises(ValueError, match="characters other than"):
        stim.PauliString.iter_all(2, allowed_paulis="A")


def test_iter_reusable():
    v = stim.PauliString.iter_all(2)
    vs1 = list(v)
    vs2 = list(v)
    assert vs1 == vs2
    assert len(vs1) == 4**2


def test_backwards_compatibility_init():
    assert stim.PauliString() == stim.PauliString("+")
    assert stim.PauliString(5) == stim.PauliString("+_____")
    assert stim.PauliString([1, 2, 3]) == stim.PauliString("+XYZ")
    assert stim.PauliString("XYZ") == stim.PauliString("+XYZ")
    assert stim.PauliString(stim.PauliString("XYZ")) == stim.PauliString("+XYZ")
    assert stim.PauliString("X" for _ in range(4)) == stim.PauliString("+XXXX")

    # These keywords have been removed from the documentation and the .pyi, but
    # their functionality needs to be maintained for backwards compatibility.
    # noinspection PyArgumentList
    assert stim.PauliString(num_qubits=5) == stim.PauliString("+_____")
    # noinspection PyArgumentList
    assert stim.PauliString(pauli_indices=[1, 2, 3]) == stim.PauliString("+XYZ")
    # noinspection PyArgumentList
    assert stim.PauliString(text="XYZ") == stim.PauliString("+XYZ")
    # noinspection PyArgumentList
    assert stim.PauliString(other=stim.PauliString("XYZ")) == stim.PauliString("+XYZ")


def test_pauli_indices():
    assert stim.PauliString().pauli_indices() == []
    assert stim.PauliString().pauli_indices("X") == []
    assert stim.PauliString().pauli_indices("I") == []
    assert stim.PauliString(5).pauli_indices() == []
    assert stim.PauliString(5).pauli_indices("X") == []
    assert stim.PauliString(5).pauli_indices("I") == [0, 1, 2, 3, 4]
    assert stim.PauliString("X1000").pauli_indices() == [1000]
    assert stim.PauliString("Y1000").pauli_indices() == [1000]
    assert stim.PauliString("Z1000").pauli_indices() == [1000]
    assert stim.PauliString("X1000").pauli_indices("YZ") == []
    assert stim.PauliString("Y1000").pauli_indices("XZ") == []
    assert stim.PauliString("Z1000").pauli_indices("XY") == []
    assert stim.PauliString("X1000").pauli_indices("X") == [1000]
    assert stim.PauliString("Y1000").pauli_indices("Y") == [1000]
    assert stim.PauliString("Z1000").pauli_indices("Z") == [1000]

    assert stim.PauliString("_XYZ").pauli_indices("x") == [1]
    assert stim.PauliString("_XYZ").pauli_indices("X") == [1]
    assert stim.PauliString("_XYZ").pauli_indices("y") == [2]
    assert stim.PauliString("_XYZ").pauli_indices("Y") == [2]
    assert stim.PauliString("_XYZ").pauli_indices("z") == [3]
    assert stim.PauliString("_XYZ").pauli_indices("Z") == [3]
    assert stim.PauliString("_XYZ").pauli_indices("I") == [0]
    assert stim.PauliString("_XYZ").pauli_indices("_") == [0]
    with pytest.raises(ValueError, match="Invalid character"):
        assert stim.PauliString("_XYZ").pauli_indices("k")


def test_before_reset():
    assert stim.PauliString("Z").before(stim.Circuit("R 0")) == stim.PauliString("_")
    assert stim.PauliString("Z").before(stim.Circuit("MR 0")) == stim.PauliString("_")
    assert stim.PauliString("Z").before(stim.Circuit("M 0")) == stim.PauliString("Z")

    assert stim.PauliString("X").before(stim.Circuit("RX 0")) == stim.PauliString("_")
    assert stim.PauliString("X").before(stim.Circuit("MRX 0")) == stim.PauliString("_")
    assert stim.PauliString("X").before(stim.Circuit("MX 0")) == stim.PauliString("X")

    assert stim.PauliString("Y").before(stim.Circuit("RY 0")) == stim.PauliString("_")
    assert stim.PauliString("Y").before(stim.Circuit("MRY 0")) == stim.PauliString("_")
    assert stim.PauliString("Y").before(stim.Circuit("MY 0")) == stim.PauliString("Y")

    with pytest.raises(ValueError):
        stim.PauliString("Z").before(stim.Circuit("RX 0"))
    with pytest.raises(ValueError):
        stim.PauliString("Z").before(stim.Circuit("RY 0"))
    with pytest.raises(ValueError):
        stim.PauliString("Z").before(stim.Circuit("MRX 0"))
    with pytest.raises(ValueError):
        stim.PauliString("Z").before(stim.Circuit("MRY 0"))
    with pytest.raises(ValueError):
        stim.PauliString("Z").before(stim.Circuit("MX 0"))
    with pytest.raises(ValueError):
        stim.PauliString("Z").before(stim.Circuit("MY 0"))

def test_constructor_from_dict():
    # Key is the qubit index:
    assert stim.PauliString({2: "X", 4: "Z"}) == stim.PauliString("__X_Z")
    assert stim.PauliString({0: 1, 1: 2}) == stim.PauliString("XY")
    assert stim.PauliString({1: 1, 3: 2, 5: "Z"}) == stim.PauliString("_X_Y_Z")
    assert stim.PauliString({1: 0, 3: "I", 4: "_"}) == stim.PauliString("_____")
    assert stim.PauliString({0: "X", 2: "x", 4: "y"}) == stim.PauliString("X_X_Y") # Case-insensitive
    assert stim.PauliString({}) == stim.PauliString("")

    # Key is the Pauli:
    assert stim.PauliString({"X": 0, "Z": 1}) == stim.PauliString("XZ")
    assert stim.PauliString({"X": 2, "Z": 4, "Y": 6, "I": 5}) == stim.PauliString("__X_Z_Y")
    assert stim.PauliString({"X": 0, "Z": [1,2]}) == stim.PauliString("XZZ")
    assert stim.PauliString({"x": [0,2], "Y": 4}) == stim.PauliString("X_X_Y") # Case-insensitive
    assert stim.PauliString({"I": [1,2]}) == stim.PauliString("___")

def test_constructor_from_dict_errors():
    with pytest.raises(ValueError):
        stim.PauliString({"A": 0})

    with pytest.raises(ValueError):
        stim.PauliString({0: "A"})

    with pytest.raises(ValueError):
        stim.PauliString({0: 4}) # Paulis correspond to 0-3

    with pytest.raises(ValueError):
        stim.PauliString({0: -1}) # Paulis correspond to 0-3

    with pytest.raises(ValueError):
        stim.PauliString({"ZX": 0}) # Paulis need to be single characters

    with pytest.raises(ValueError, match="Qubit index must be an int"):
        stim.PauliString({"X": "not an int"})

    with pytest.raises(ValueError, match="Qubit index must be an int"):
        stim.PauliString({"Y": [0, "not an int"]})

    with pytest.raises(ValueError, match="keys must all be ints or all strings"):
        stim.PauliString({"X": 0, 1: "Y"})

    # with pytest.raises(ValueError):
    #     stim.PauliString({"X": 0, "Y": 0})
    #
    # with pytest.raises(ValueError):
    #     stim.PauliString({"Z": 1, "Y": [4,1]})