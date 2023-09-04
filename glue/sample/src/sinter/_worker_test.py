import multiprocessing
import pathlib
import tempfile

import stim

from sinter._worker import WorkIn, WorkOut, worker_loop
from sinter._worker import auto_dem


def test_worker_loop_infers_dem():
    with tempfile.TemporaryDirectory() as tmp_dir:
        tmp_dir = pathlib.Path(tmp_dir)
        circuit =  stim.Circuit("""
            M(0.2) 0 1
            DETECTOR rec[-1]
            OBSERVABLE_INCLUDE(0) rec[-1] rec[-2]
        """)
        circuit_path = str(tmp_dir / 'input_circuit.stim')
        dem_path = str(tmp_dir / 'input_dem.dem')
        circuit.to_file(circuit_path)
        inp = multiprocessing.Queue()
        out = multiprocessing.Queue()
        inp.put(WorkIn(
            work_key='test1',
            circuit_path=circuit_path,
            dem_path=dem_path,
            decoder='pymatching',
            json_metadata=5,
            strong_id=None,
            num_shots=-1,
            postselected_observables_mask=None,
            postselection_mask=None,
            count_detection_events=False,
            count_observable_error_combos=False,
        ))
        inp.put(None)
        worker_loop(tmp_dir, inp, out, None, 0)
        result: WorkOut = out.get(timeout=1)
        assert out.empty()

        assert result.stats is None
        assert result.work_key == 'test1'
        assert result.msg_error is None
        assert stim.DetectorErrorModel.from_file(dem_path) == circuit.detector_error_model()
        assert result.strong_id is not None


def test_worker_loop_does_not_recompute_dem():
    with tempfile.TemporaryDirectory() as tmp_dir:
        tmp_dir = pathlib.Path(tmp_dir)
        circuit_path = str(tmp_dir / 'input_circuit.stim')
        dem_path = str(tmp_dir / 'input_dem.dem')
        stim.Circuit("""
            M(0.2) 0 1
            DETECTOR rec[-1]
            OBSERVABLE_INCLUDE(0) rec[-1] rec[-2]
        """).to_file(circuit_path)
        stim.DetectorErrorModel("""
            error(0.234567) D0 L0
        """).to_file(dem_path)

        inp = multiprocessing.Queue()
        out = multiprocessing.Queue()
        inp.put(WorkIn(
            work_key='test1',
            circuit_path=circuit_path,
            dem_path=dem_path,
            decoder='pymatching',
            json_metadata=5,
            strong_id="fake",
            num_shots=1000,
            postselected_observables_mask=None,
            postselection_mask=None,
            count_detection_events=False,
            count_observable_error_combos=False,
        ))
        inp.put(None)
        worker_loop(tmp_dir, inp, out, None, 0)
        result: WorkOut = out.get(timeout=1)
        assert out.empty()

        assert result.stats.shots == 1000
        assert result.stats.discards == 0
        assert 0 < result.stats.errors < 1000
        assert result.work_key == 'test1'
        assert result.msg_error is None
        assert result.strong_id == 'fake'


def test_auto_dem():
    assert auto_dem(stim.Circuit("""
        REPEAT 100 {
            CORRELATED_ERROR(0.125) X0 X1
            CORRELATED_ERROR(0.125) X0 X1 X2 X3
            MR 0 1 2 3
            DETECTOR rec[-4]
            DETECTOR rec[-3]
            DETECTOR rec[-2]
            DETECTOR rec[-1]
        }
    """)) == stim.DetectorErrorModel("""
        REPEAT 99 {
            error(0.125) D0 D1
            error(0.125) D0 D1 ^ D2 D3
            shift_detectors 4
        }
        error(0.125) D0 D1
        error(0.125) D0 D1 ^ D2 D3
    """)

    assert auto_dem(stim.Circuit("""
        CORRELATED_ERROR(0.125) X0 X1
        CORRELATED_ERROR(0.125) X0 X1 X2 X3
        M 0 1 2 3
        DETECTOR rec[-4]
        DETECTOR rec[-3]
        DETECTOR rec[-2]
        DETECTOR rec[-1]
    """)) == stim.DetectorErrorModel("""
        error(0.125) D0 D1
        error(0.125) D0 D1 ^ D2 D3
    """)

    assert auto_dem(stim.Circuit("""
        CORRELATED_ERROR(0.125) X0 X1 X2 X3
        M 0 1 2 3
        DETECTOR rec[-4]
        DETECTOR rec[-3]
        DETECTOR rec[-2]
        DETECTOR rec[-1]
    """)) == stim.DetectorErrorModel("""
        error(0.125) D0 D1 D2 D3
    """)
