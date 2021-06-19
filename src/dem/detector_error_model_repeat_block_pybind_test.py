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


def test_init_vs_properties():
    v = stim.DemRepeatBlock(5, stim.DetectorErrorModel('error(0.125) D1 L2'))
    assert v.repeat_count == 5
    assert v.body_copy() == stim.DetectorErrorModel('error(0.125) D1 L2')
    assert v.body_copy() is not v.body_copy()


def test_equality():
    m0 = stim.DetectorErrorModel()
    m1 = stim.DetectorErrorModel('error(0.125) D1 L2')
    assert stim.DemRepeatBlock(5, m0) == stim.DemRepeatBlock(5, m0)
    assert not (stim.DemRepeatBlock(5, m0) != stim.DemRepeatBlock(5, m0))
    assert stim.DemRepeatBlock(5, m0) != stim.DemRepeatBlock(5, m1)
    assert not (stim.DemRepeatBlock(5, m0) == stim.DemRepeatBlock(5, m1))
    assert stim.DemRepeatBlock(5, m0) != stim.DemRepeatBlock(6, m0)


def test_repr():
    v = stim.DemRepeatBlock(5, stim.DetectorErrorModel('error(0.125) D1 L2'))
    assert eval(repr(v), {"stim": stim}) == v
