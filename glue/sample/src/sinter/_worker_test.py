import multiprocessing
import tempfile

import stim

import sinter
from sinter._worker import WorkIn, WorkOut, worker_loop


def test_worker_loop_infers_dem():
    c = stim.Circuit("""
        M(0.2) 0 1
        DETECTOR rec[-1]
        OBSERVABLE_INCLUDE(0) rec[-1] rec[-2]
    """)
    with tempfile.TemporaryDirectory() as tmp_dir:
        inp = multiprocessing.Queue()
        out = multiprocessing.Queue()
        inp.put(WorkIn(
            work_key='test1',
            task=sinter.Task(
                circuit=c,
                detector_error_model=None,
                decoder='pymatching',
                json_metadata=5,
            ),
            num_shots=1000,
        ))
        inp.put(None)
        worker_loop(tmp_dir, inp, out, 0)
        result: WorkOut = out.get(timeout=1)
        assert out.empty()

        assert result.stats.shots == 1000
        assert result.stats.discards == 0
        assert 0 < result.stats.errors < 1000
        assert result.work_key == 'test1'
        assert result.msg_error is None
        assert result.filled_in_dem == c.detector_error_model()
        assert result.filled_in_strong_id is not None


def test_worker_loop_does_not_recompute_dem():
    c = stim.Circuit("""
        M(0.2) 0 1
        DETECTOR rec[-1]
        OBSERVABLE_INCLUDE(0) rec[-1] rec[-2]
    """)
    with tempfile.TemporaryDirectory() as tmp_dir:
        inp = multiprocessing.Queue()
        out = multiprocessing.Queue()
        inp.put(WorkIn(
            work_key='test1',
            task=sinter.Task(
                circuit=c,
                detector_error_model=stim.DetectorErrorModel("""
                    error(0.234567) D0 L0
                """),
                decoder='pymatching',
                json_metadata=5,
            ),
            num_shots=1000,
        ))
        inp.put(None)
        worker_loop(tmp_dir, inp, out, 0)
        result: WorkOut = out.get(timeout=1)
        assert out.empty()

        assert result.stats.shots == 1000
        assert result.stats.discards == 0
        assert 0 < result.stats.errors < 1000
        assert result.work_key == 'test1'
        assert result.msg_error is None
        assert result.filled_in_dem is None
        assert result.filled_in_strong_id is None
