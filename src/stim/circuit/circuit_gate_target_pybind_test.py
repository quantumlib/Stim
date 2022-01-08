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
    assert stim.GateTarget(5) == stim.GateTarget(5)
    assert stim.GateTarget(5) == stim.GateTarget(value=5)
    assert not (stim.GateTarget(4) == stim.GateTarget(5))
    assert stim.GateTarget(4) != stim.GateTarget(5)
    assert not (stim.GateTarget(5) != stim.GateTarget(5))
    assert stim.GateTarget(stim.target_x(5)) != stim.GateTarget(5)
    assert stim.GateTarget(5) == stim.GateTarget(stim.GateTarget(5))


def test_properties():
    g = stim.GateTarget(5)
    assert g.value == 5
    assert not g.is_x_target
    assert not g.is_y_target
    assert not g.is_z_target
    assert not g.is_inverted_result_target
    assert not g.is_measurement_record_target
    assert not g.is_combiner
    assert g.is_qubit_target

    g = stim.GateTarget(stim.target_rec(-4))
    assert g.value == -4
    assert not g.is_x_target
    assert not g.is_y_target
    assert not g.is_z_target
    assert not g.is_inverted_result_target
    assert g.is_measurement_record_target
    assert not g.is_combiner
    assert not g.is_qubit_target

    g = stim.GateTarget(stim.target_x(3))
    assert g.value == 3
    assert g.is_x_target
    assert not g.is_y_target
    assert not g.is_z_target
    assert not g.is_inverted_result_target
    assert not g.is_measurement_record_target
    assert not g.is_combiner
    assert not g.is_qubit_target

    g = stim.GateTarget(stim.target_y(3))
    assert g.value == 3
    assert not g.is_x_target
    assert g.is_y_target
    assert not g.is_z_target
    assert not g.is_inverted_result_target
    assert not g.is_measurement_record_target
    assert not g.is_combiner
    assert not g.is_qubit_target

    g = stim.GateTarget(stim.target_z(3))
    assert g.value == 3
    assert not g.is_x_target
    assert not g.is_y_target
    assert g.is_z_target
    assert not g.is_inverted_result_target
    assert not g.is_measurement_record_target
    assert not g.is_combiner
    assert not g.is_qubit_target

    g = stim.GateTarget(stim.target_z(3, invert=True))
    assert g.value == 3
    assert not g.is_x_target
    assert not g.is_y_target
    assert g.is_z_target
    assert g.is_inverted_result_target
    assert not g.is_measurement_record_target
    assert not g.is_combiner
    assert not g.is_qubit_target

    g = stim.GateTarget(stim.target_inv(3))
    assert g.value == 3
    assert not g.is_x_target
    assert not g.is_y_target
    assert not g.is_z_target
    assert g.is_inverted_result_target
    assert not g.is_measurement_record_target
    assert not g.is_combiner
    assert g.is_qubit_target

    g = stim.target_combiner()
    assert not g.is_x_target
    assert not g.is_y_target
    assert not g.is_z_target
    assert not g.is_inverted_result_target
    assert not g.is_measurement_record_target
    assert not g.is_qubit_target
    assert g.is_combiner


@pytest.mark.parametrize("value", [
    stim.GateTarget(5),
    stim.GateTarget(stim.target_rec(-5)),
    stim.GateTarget(stim.target_x(5)),
    stim.GateTarget(stim.target_y(5)),
    stim.GateTarget(stim.target_z(5)),
    stim.GateTarget(stim.target_inv(5)),
])
def test_repr(value):
    assert eval(repr(value), {'stim': stim}) == value
    assert repr(eval(repr(value), {'stim': stim})) == repr(value)


def test_hashable():
    a = stim.GateTarget(5)
    b = stim.GateTarget(6)
    c = stim.GateTarget(5)
    assert hash(a) == hash(c)
    assert len({a, b, c}) == 2
