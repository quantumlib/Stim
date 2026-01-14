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


def test_trivial():
    p = stim.CliffordString(3)
    assert repr(p) == 'stim.CliffordString("I,I,I")'
    assert len(p) == 3
    assert p[1:] == stim.CliffordString(2)
    assert p[0] == stim.gate_data('I')


def test_simple():
    p = stim.CliffordString("X,Y,Z,H,SQRT_X,C_XYZ,H_NXZ")
    assert repr(p) == 'stim.CliffordString("X,Y,Z,H,SQRT_X,C_XYZ,H_NXZ")'
    assert str(p) == 'X,Y,Z,H,SQRT_X,C_XYZ,H_NXZ'
    assert len(p) == 7
    assert p != stim.CliffordString("Y,Y,Z,H,SQRT_X,C_XYZ,H_NXZ")
    assert not (p != stim.CliffordString("X,Y,Z,H,SQRT_X,C_XYZ,H_NXZ"))
    assert not (p == stim.CliffordString("Y,Y,Z,H,SQRT_X,C_XYZ,H_NXZ"))
    assert p[1::2] == stim.CliffordString("Y,H,C_XYZ")

    assert stim.CliffordString(6) == stim.CliffordString("I,I,I,I,I,I")

    assert stim.CliffordString(stim.PauliString("XYZ_XYZ")) == stim.CliffordString("X,Y,Z,I,X,Y,Z")

    v = stim.CliffordString("X,Y,H")
    v2 = stim.CliffordString(v)
    assert v == v2
    assert v is not v2

    assert stim.CliffordString(['X', 'Y', 'Z', stim.gate_data('H'), 'S']) == stim.CliffordString('X,Y,Z,H,S')


def test_multiplication():
    a = stim.CliffordString("Z,H,S,C_XYZ")
    b = stim.CliffordString("S,Z,S,C_XYZ,I")
    assert a * b == stim.CliffordString("S_DAG,SQRT_Y,Z,C_ZYX,I")
    a *= b
    assert a == stim.CliffordString("S_DAG,SQRT_Y,Z,C_ZYX,I")

    assert stim.CliffordString("X") * stim.CliffordString("H") == stim.CliffordString("H") * stim.CliffordString("Z")
    assert stim.CliffordString("X") * stim.CliffordString("H") != stim.CliffordString("Z") * stim.CliffordString("H")
    assert stim.CliffordString("X") * stim.CliffordString("H") == stim.CliffordString("SQRT_Y")


def test_random():
    c1 = stim.CliffordString.random(128)
    c2 = stim.CliffordString.random(128)
    assert len(c1) == len(c2) == 128
    assert c1 != c2
