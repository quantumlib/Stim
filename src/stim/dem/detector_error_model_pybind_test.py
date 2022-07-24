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
import pathlib
import tempfile

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


def test_count_errors():
    assert stim.DetectorErrorModel().num_errors == 0

    assert stim.DetectorErrorModel("""
        logical_observable L100
        detector D100
        shift_detectors(100, 100, 100) 100
        error(0.125) D100
    """).num_errors == 1

    assert stim.DetectorErrorModel("""
        error(0.125) D0
        REPEAT 100 {
            REPEAT 5 {
                error(0.25) D1
            }
        }
    """).num_errors == 501


def test_shortest_graphlike_error_trivial():
    with pytest.raises(ValueError, match="any graphlike logical errors"):
        _ = stim.DetectorErrorModel().shortest_graphlike_error()
    with pytest.raises(ValueError, match="any graphlike logical errors"):
        _ = stim.DetectorErrorModel("""
            error(0.1) D0
        """).shortest_graphlike_error()
    with pytest.raises(ValueError, match="any graphlike logical errors"):
        _ = stim.DetectorErrorModel("""
            error(0.1) D0 L0
        """).shortest_graphlike_error()
    assert stim.DetectorErrorModel("""
        error(0.1) L0
    """).shortest_graphlike_error() == stim.DetectorErrorModel("""
        error(1) L0
    """)
    assert stim.DetectorErrorModel("""
        error(0.1) D0 D1 L0
        error(0.1) D0 D1
    """).shortest_graphlike_error() == stim.DetectorErrorModel("""
        error(1) D0 D1
        error(1) D0 D1 L0
    """)


def test_shortest_graphlike_error_line():
    assert stim.DetectorErrorModel("""
        error(0.125) D0
        error(0.125) D0 D1
        error(0.125) D1 L55
        error(0.125) D1
    """).shortest_graphlike_error() == stim.DetectorErrorModel("""
        error(1) D1
        error(1) D1 L55
    """)

    assert len(stim.DetectorErrorModel("""
        error(0.1) D0 D1 L5
        REPEAT 1000 {
            error(0.1) D0 D2
            error(0.1) D1 D3
            shift_detectors 2
        }
        error(0.1) D0
        error(0.1) D1
    """).shortest_graphlike_error()) == 2003


def test_shortest_graphlike_error_ignore():
    assert stim.DetectorErrorModel("""
        error(0.125) D0 D1 D2
        error(0.125) L0
    """).shortest_graphlike_error(ignore_ungraphlike_errors=True) == stim.DetectorErrorModel("""
        error(1) L0
    """)


def test_shortest_graphlike_error_rep_code():
    circuit = stim.Circuit.generated("repetition_code:memory",
                                     rounds=10,
                                     distance=7,
                                     before_round_data_depolarization=0.01)
    model = circuit.detector_error_model(decompose_errors=True)
    assert len(model.shortest_graphlike_error()) == 7


def test_coords():
    circuit = stim.Circuit("""
        M 0
        DETECTOR(1, 2, 3) rec[-1]
        REPEAT 3 {
            DETECTOR(2) rec[-1]
            SHIFT_COORDS(5)
        }
    """)
    dem = circuit.detector_error_model()

    assert dem.get_detector_coordinates() == {
        0: [1, 2, 3],
        1: [2],
        2: [7],
        3: [12],
    }
    assert circuit.get_detector_coordinates() == {
        0: [1, 2, 3],
        1: [2],
        2: [7],
        3: [12],
    }

    assert dem.get_detector_coordinates([1]) == {
        1: [2],
    }
    assert circuit.get_detector_coordinates([1]) == {
        1: [2],
    }
    assert dem.get_detector_coordinates(1) == {
        1: [2],
    }
    assert circuit.get_detector_coordinates(1) == {
        1: [2],
    }
    assert dem.get_detector_coordinates({1}) == {
        1: [2],
    }
    assert circuit.get_detector_coordinates({1}) == {
        1: [2],
    }
    assert dem.get_detector_coordinates(stim.DemTarget.relative_detector_id(1)) == {
        1: [2],
    }
    assert circuit.get_detector_coordinates(stim.DemTarget.relative_detector_id(1)) == {
        1: [2],
    }
    assert dem.get_detector_coordinates((stim.DemTarget.relative_detector_id(1),)) == {
        1: [2],
    }
    assert circuit.get_detector_coordinates((stim.DemTarget.relative_detector_id(1),)) == {
        1: [2],
    }

    assert dem.get_detector_coordinates(only=[2, 3]) == {
        2: [7],
        3: [12],
    }
    assert circuit.get_detector_coordinates(only=[2, 3]) == {
        2: [7],
        3: [12],
    }

    with pytest.raises(ValueError, match="Expected a detector id"):
        dem.get_detector_coordinates([-1])
    with pytest.raises(ValueError, match="too big"):
        dem.get_detector_coordinates([500])
    with pytest.raises(ValueError, match="Expected a detector id"):
        circuit.get_detector_coordinates([-1])
    with pytest.raises(ValueError, match="too big"):
        circuit.get_detector_coordinates([500])


def test_dem_from_file():
    with tempfile.TemporaryDirectory() as tmpdir:
        path = tmpdir + '/tmp.stim'
        with open(path, 'w') as f:
            print('error(0.125) D0 L5', file=f)
        assert stim.DetectorErrorModel.from_file(path) == stim.DetectorErrorModel('error(0.125) D0 L5')

    with tempfile.TemporaryDirectory() as tmpdir:
        path = pathlib.Path(tmpdir) / 'tmp.stim'
        with open(path, 'w') as f:
            print('error(0.125) D0 L5', file=f)
        assert stim.DetectorErrorModel.from_file(path) == stim.DetectorErrorModel('error(0.125) D0 L5')

    with tempfile.TemporaryDirectory() as tmpdir:
        path = tmpdir + '/tmp.stim'
        with open(path, 'w') as f:
            print('error(0.125) D0 L5', file=f)
        with open(path) as f:
            assert stim.DetectorErrorModel.from_file(f) == stim.DetectorErrorModel('error(0.125) D0 L5')

    with pytest.raises(ValueError, match="how to read"):
        stim.DetectorErrorModel.from_file(object())
    with pytest.raises(ValueError, match="how to read"):
        stim.DetectorErrorModel.from_file(123)


def test_dem_to_file():
    c = stim.DetectorErrorModel('error(0.125) D0 L5\n')
    with tempfile.TemporaryDirectory() as tmpdir:
        path = tmpdir + '/tmp.stim'
        c.to_file(path)
        with open(path) as f:
            assert f.read() == 'error(0.125) D0 L5\n'

    with tempfile.TemporaryDirectory() as tmpdir:
        path = pathlib.Path(tmpdir) / 'tmp.stim'
        c.to_file(path)
        with open(path) as f:
            assert f.read() == 'error(0.125) D0 L5\n'

    with tempfile.TemporaryDirectory() as tmpdir:
        path = tmpdir + '/tmp.stim'
        with open(path, 'w') as f:
            c.to_file(f)
        with open(path) as f:
            assert f.read() == 'error(0.125) D0 L5\n'

    with pytest.raises(ValueError, match="how to write"):
        c.to_file(object())
    with pytest.raises(ValueError, match="how to write"):
        c.to_file(123)


def test_flattened():
    dem = stim.DetectorErrorModel("""
        shift_detectors 5
        repeat 2 {
            error(0.125) D0 D1
        }
    """)
    assert dem.flattened() == stim.DetectorErrorModel("""
        error(0.125) D5 D6
        error(0.125) D5 D6
    """)


def test_rounded():
    dem = stim.DetectorErrorModel("""
        error(0.1248) D0 D1
    """)
    assert dem.rounded(1) == stim.DetectorErrorModel("""
        error(0.1) D0 D1
    """)
    assert dem.rounded(2) == stim.DetectorErrorModel("""
        error(0.12) D0 D1
    """)
    assert dem.rounded(3) == stim.DetectorErrorModel("""
        error(0.125) D0 D1
    """)
    assert dem.rounded(4) == stim.DetectorErrorModel("""
        error(0.1248) D0 D1
    """)
    assert dem.rounded(5) == stim.DetectorErrorModel("""
        error(0.1248) D0 D1
    """)

    dem = stim.DetectorErrorModel("""
        error(0.01248) D0 D1
    """)
    assert dem.rounded(1) == stim.DetectorErrorModel("""
        error(0) D0 D1
    """)
    assert dem.rounded(2) == stim.DetectorErrorModel("""
        error(0.01) D0 D1
    """)
    assert dem.rounded(3) == stim.DetectorErrorModel("""
        error(0.012) D0 D1
    """)
    assert dem.rounded(4) == stim.DetectorErrorModel("""
        error(0.0125) D0 D1
    """)
