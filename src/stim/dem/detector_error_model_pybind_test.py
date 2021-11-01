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


def test_pickle():
    import pickle

    t = stim.DetectorErrorModel("""
        repeat 100 {
            error(0.25) D0 L1
            shift_detectors(1, 2) 3
        }
    """)
    a = pickle.dumps(t)
    assert pickle.loads(a) == t
