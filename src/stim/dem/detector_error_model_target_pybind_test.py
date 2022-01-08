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


def test_equality():
    assert stim.target_relative_detector_id(5) == stim.target_relative_detector_id(5)
    assert not (stim.target_relative_detector_id(5) != stim.target_relative_detector_id(5))
    assert stim.target_relative_detector_id(4) != stim.target_relative_detector_id(5)
    assert not (stim.target_relative_detector_id(4) == stim.target_relative_detector_id(5))

    assert stim.target_relative_detector_id(5) != stim.target_logical_observable_id(5)
    assert stim.target_logical_observable_id(5) == stim.target_logical_observable_id(5)
    assert stim.target_relative_detector_id(5) != stim.target_separator()
    assert stim.target_separator() == stim.target_separator()


def test_str():
    assert str(stim.target_relative_detector_id(5)) == "D5"
    assert str(stim.target_logical_observable_id(6)) == "L6"
    assert str(stim.target_separator()) == "^"


def test_properties():
    assert stim.target_relative_detector_id(6).val == 6
    assert stim.target_relative_detector_id(5).val == 5
    assert stim.target_relative_detector_id(5).is_relative_detector_id()
    assert not stim.target_relative_detector_id(5).is_logical_observable_id()
    assert not stim.target_relative_detector_id(5).is_separator()

    assert stim.target_logical_observable_id(6).val == 6
    assert stim.target_logical_observable_id(5).val == 5
    assert not stim.target_logical_observable_id(5).is_relative_detector_id()
    assert stim.target_logical_observable_id(5).is_logical_observable_id()
    assert not stim.target_logical_observable_id(5).is_separator()

    assert not stim.target_separator().is_relative_detector_id()
    assert not stim.target_separator().is_logical_observable_id()
    assert stim.target_separator().is_separator()
    with pytest.raises(ValueError, match="Separator"):
        _ = stim.target_separator().val


def test_repr():
    v = stim.target_relative_detector_id(5)
    assert eval(repr(v), {"stim": stim}) == v
    v = stim.target_logical_observable_id(6)
    assert eval(repr(v), {"stim": stim}) == v
    v = stim.target_separator()
    assert eval(repr(v), {"stim": stim}) == v


def test_static_constructors():
    assert stim.DemTarget.relative_detector_id(5) == stim.target_relative_detector_id(5)
    assert stim.DemTarget.logical_observable_id(5) == stim.target_logical_observable_id(5)
    assert stim.DemTarget.separator() == stim.target_separator()


def test_hashable():
    a = stim.DemTarget.relative_detector_id(3)
    b = stim.DemTarget.logical_observable_id(5)
    c = stim.DemTarget.relative_detector_id(3)
    assert hash(a) == hash(c)
    assert len({a, b, c}) == 2
