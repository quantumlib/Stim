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


def test_init_get():
    model = stim.DetectorErrorModel("""
        error(0.125) D0 L0
        ERROR(0.25) D0 ^ D1
        repeat 100 {
            shift_detectors 1
            error(0.125) D0 D1
        }
        shift_detectors(1, 1.5, 2, 2.5) 1
        shift_detectors 5
    """)
    assert len(model) == 5
    assert model[0] == stim.DemInstruction(
        "error",
        [0.125],
        [stim.target_relative_detector_id(0), stim.target_logical_observable_id(0)])
    assert model[1] == stim.DemInstruction(
        "error",
        [0.25],
        [stim.target_relative_detector_id(0), stim.target_separator(), stim.target_relative_detector_id(1)])
    assert model[2] == stim.DemRepeatBlock(
        100,
        stim.DetectorErrorModel("""
            shift_detectors 1
            error(0.125) D0 D1
        """))
    assert model[3] == stim.DemInstruction(
        "shift_detectors",
        [1, 1.5, 2, 2.5],
        [1])
    assert model[4] == stim.DemInstruction(
        "shift_detectors",
        [],
        [5])


def test_equality():
    assert stim.DetectorErrorModel() == stim.DetectorErrorModel()
    assert not (stim.DetectorErrorModel() != stim.DetectorErrorModel())
    assert not (stim.DetectorErrorModel() == stim.DetectorErrorModel("error(0.125) D0"))
    assert stim.DetectorErrorModel() != stim.DetectorErrorModel("error(0.125) D0")

    assert stim.DetectorErrorModel("error(0.125) D0") == stim.DetectorErrorModel("error(0.125) D0")
    assert stim.DetectorErrorModel("error(0.125) D0") != stim.DetectorErrorModel("error(0.126) D0")
    assert stim.DetectorErrorModel("error(0.125) D0") != stim.DetectorErrorModel("detector(0.125) D0")
    assert stim.DetectorErrorModel("error(0.125) D0") != stim.DetectorErrorModel("error(0.125) D1")
    assert stim.DetectorErrorModel("error(0.125) D0") != stim.DetectorErrorModel("error(0.125) L0")
    assert stim.DetectorErrorModel("error(0.125) D0") != stim.DetectorErrorModel("error(0.125) D0 D1")
    assert stim.DetectorErrorModel("""
        REPEAT 3 {
            shift_detectors 4
        }
    """) == stim.DetectorErrorModel("""
        REPEAT 3 {
            shift_detectors 4
        }
    """)
    assert stim.DetectorErrorModel("""
        REPEAT 3 {
            shift_detectors 4
        }
    """) != stim.DetectorErrorModel("""
        REPEAT 4 {
            shift_detectors 4
        }
    """)
    assert stim.DetectorErrorModel("""
        REPEAT 3 {
            shift_detectors 4
        }
    """) != stim.DetectorErrorModel("""
        REPEAT 3 {
            shift_detectors 5
        }
    """)


def test_repr():
    v = stim.DetectorErrorModel()
    assert eval(repr(v), {"stim": stim}) == v
    v = stim.DetectorErrorModel("error(0.125) D0 D1")
    assert eval(repr(v), {"stim": stim}) == v


def test_approx_equals():
    base = stim.DetectorErrorModel("error(0.099) D0")
    assert not base.approx_equals(stim.DetectorErrorModel("error(0.101) D0"), atol=0)
    assert not base.approx_equals(stim.DetectorErrorModel("error(0.101) D0"), atol=0.00001)
    assert base.approx_equals(stim.DetectorErrorModel("error(0.101) D0"), atol=0.01)
    assert base.approx_equals(stim.DetectorErrorModel("error(0.101) D0"), atol=999)
    assert not base.approx_equals(stim.DetectorErrorModel("error(0.101) D0 D1"), atol=999)

    assert not base.approx_equals(object(), atol=999)
    assert not base.approx_equals(stim.PauliString("XYZ"), atol=999)


def test_append():
    m = stim.DetectorErrorModel()
    m.append("error", 0.125, [
        stim.DemTarget.relative_detector_id(1),
    ])
    m.append("error", 0.25, [
        stim.DemTarget.relative_detector_id(1),
        stim.DemTarget.separator(),
        stim.DemTarget.relative_detector_id(2),
        stim.DemTarget.logical_observable_id(3),
    ])
    m.append("shift_detectors", (1, 2, 3), [5])
    m += m * 3
    m.append(m[0])
    m.append(m[-2])
    assert m == stim.DetectorErrorModel("""
        error(0.125) D1
        error(0.25) D1 ^ D2 L3
        shift_detectors(1, 2, 3) 5
        repeat 3 {
            error(0.125) D1
            error(0.25) D1 ^ D2 L3
            shift_detectors(1, 2, 3) 5
        }
        error(0.125) D1
        repeat 3 {
            error(0.125) D1
            error(0.25) D1 ^ D2 L3
            shift_detectors(1, 2, 3) 5
        }
    """)


def test_append_bad():
    m = stim.DetectorErrorModel()
    m.append("error", 0.125, [stim.target_relative_detector_id(0)])
    m.append("error", [0.125], [stim.target_relative_detector_id(0)])
    m.append("shift_detectors", [], [5])
    m += m * 3

    with pytest.raises(ValueError, match="Bad target 'D0' for instruction 'shift_detectors'"):
        m.append("shift_detectors", [0.125, 0.25], [stim.target_relative_detector_id(0)])
    with pytest.raises(ValueError, match="takes 1 argument"):
        m.append("error", [0.125, 0.25], [stim.target_relative_detector_id(0)])

    with pytest.raises(ValueError, match="Bad target '0' for instruction 'error'"):
        m.append("error", [0.125], [0])

    with pytest.raises(ValueError, match="First argument"):
        m.append(None)
    with pytest.raises(ValueError, match="First argument"):
        m.append(object())
    with pytest.raises(ValueError, match="Must specify.*instruction name"):
        m.append("error")
    with pytest.raises(ValueError, match="Can't specify.*instruction is a"):
        m.append(m[0], 0.125, [])
    with pytest.raises(ValueError, match="Can't specify.*instruction is a"):
        m.append(m[-1], 0.125, [])
