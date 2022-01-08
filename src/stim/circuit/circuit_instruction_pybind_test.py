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


def test_init_and_equality():
    i = stim.CircuitInstruction("X_ERROR", [stim.GateTarget(5)], [0.5])
    assert i.name == "X_ERROR"
    assert i.targets_copy() == [stim.GateTarget(5)]
    assert i.gate_args_copy() == [0.5]
    i2 = stim.CircuitInstruction(name="X_ERROR", targets=[stim.GateTarget(5)], gate_args=[0.5])
    assert i == i2

    assert i == stim.CircuitInstruction("X_ERROR", [stim.GateTarget(5)], [0.5])
    assert not (i == stim.CircuitInstruction("Z_ERROR", [stim.GateTarget(5)], [0.5]))
    assert i != stim.CircuitInstruction("Z_ERROR", [stim.GateTarget(5)], [0.5])
    assert i != stim.CircuitInstruction("X_ERROR", [stim.GateTarget(5), stim.GateTarget(6)], [0.5])
    assert i != stim.CircuitInstruction("X_ERROR", [stim.GateTarget(5)], [0.25])


@pytest.mark.parametrize("value", [
    stim.CircuitInstruction("X_ERROR", [stim.GateTarget(5)], [0.5]),
    stim.CircuitInstruction("M", [stim.GateTarget(stim.target_inv(3))]),
])
def test_repr(value):
    assert eval(repr(value), {'stim': stim}) == value
    assert repr(eval(repr(value), {'stim': stim})) == repr(value)


def test_str():
    assert str(stim.CircuitInstruction("X_ERROR", [stim.GateTarget(5)], [0.5])) == "X_ERROR(0.5) 5"


def test_hashable():
    a = stim.CircuitInstruction("X_ERROR", [stim.GateTarget(5)], [0.5])
    b = stim.CircuitInstruction("DEPOLARIZE1", [stim.GateTarget(5)], [0.5])
    c = stim.CircuitInstruction("X_ERROR", [stim.GateTarget(5)], [0.5])
    assert hash(a) == hash(c)
    assert len({a, b, c}) == 2
