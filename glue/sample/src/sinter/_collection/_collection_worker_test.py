import collections
import multiprocessing
import time
from typing import Any, List

import sinter
import stim

from sinter._collection._collection_worker_state import CollectionWorkerState


class MockWorkHandler(sinter.Sampler, sinter.CompiledSampler):
    def __init__(self):
        self.expected_task = None
        self.expected = collections.deque()

    def compiled_sampler_for_task(self, task: sinter.Task) -> sinter.CompiledSampler:
        assert task == self.expected_task
        return self

    def handles_throttling(self) -> bool:
        return True

    def sample(self, shots: int) -> sinter.AnonTaskStats:
        assert self.expected
        expected_shots, response = self.expected.popleft()
        assert shots == expected_shots
        return response


def _assert_drain_queue(q: multiprocessing.Queue, expected_contents: List[Any]):
    for v in expected_contents:
        assert q.get(timeout=0.1) == v
    assert q.empty()


def _put_wait_not_empty(q: multiprocessing.Queue, item: Any):
    q.put(item)
    while q.empty():
        time.sleep(0.0001)


def test_worker_stop():
    handler = MockWorkHandler()

    inp = multiprocessing.Queue()
    out = multiprocessing.Queue()
    inp.cancel_join_thread()
    out.cancel_join_thread()

    worker = CollectionWorkerState(
        flush_period=-1,
        worker_id=5,
        sampler=handler,
        inp=inp,
        out=out,
        custom_error_count_key=None,
    )

    assert worker.process_messages() == 0
    _assert_drain_queue(out, [])

    t0 = sinter.Task(
        circuit=stim.Circuit('H 0'),
        detector_error_model=stim.DetectorErrorModel(),
        decoder='mock',
        collection_options=sinter.CollectionOptions(max_shots=100_000_000),
        json_metadata={'a': 3},
    )
    handler.expected_task = t0

    _put_wait_not_empty(inp, ('change_job', (t0, sinter.CollectionOptions(max_errors=100_000_000), 100_000_000)))
    assert worker.process_messages() == 1
    _assert_drain_queue(out, [('changed_job', 5, (t0.strong_id(),))])

    _put_wait_not_empty(inp, ('stop', None))
    assert worker.process_messages() == -1


def test_worker_skip_work():
    handler = MockWorkHandler()

    inp = multiprocessing.Queue()
    out = multiprocessing.Queue()
    inp.cancel_join_thread()
    out.cancel_join_thread()

    worker = CollectionWorkerState(
        flush_period=-1,
        worker_id=5,
        sampler=handler,
        inp=inp,
        out=out,
        custom_error_count_key=None,
    )

    assert worker.process_messages() == 0
    _assert_drain_queue(out, [])

    t0 = sinter.Task(
        circuit=stim.Circuit('H 0'),
        detector_error_model=stim.DetectorErrorModel(),
        decoder='mock',
        collection_options=sinter.CollectionOptions(max_shots=100_000_000),
        json_metadata={'a': 3},
    )
    handler.expected_task = t0
    _put_wait_not_empty(inp, ('change_job', (t0, sinter.CollectionOptions(max_errors=100_000_000), 100_000_000)))
    assert worker.process_messages() == 1
    _assert_drain_queue(out, [('changed_job', 5, (t0.strong_id(),))])

    _put_wait_not_empty(inp, ('accept_shots', (t0.strong_id(), 10000)))
    assert worker.process_messages() == 1
    _assert_drain_queue(out, [('accepted_shots', 5, (t0.strong_id(), 10000))])

    assert worker.current_task == t0
    assert worker.current_task_shots_left == 10000
    assert worker.process_messages() == 0
    _assert_drain_queue(out, [])

    _put_wait_not_empty(inp, ('return_shots', (t0.strong_id(), 2000)))
    assert worker.process_messages() == 1
    _assert_drain_queue(out, [
        ('returned_shots', 5, (t0.strong_id(), 2000)),
    ])

    _put_wait_not_empty(inp, ('return_shots', (t0.strong_id(), 20000000)))
    assert worker.process_messages() == 1
    _assert_drain_queue(out, [
        ('returned_shots', 5, (t0.strong_id(), 8000)),
    ])

    assert not worker.do_some_work()


def test_worker_finish_work():
    handler = MockWorkHandler()

    inp = multiprocessing.Queue()
    out = multiprocessing.Queue()
    inp.cancel_join_thread()
    out.cancel_join_thread()

    worker = CollectionWorkerState(
        flush_period=-1,
        worker_id=5,
        sampler=handler,
        inp=inp,
        out=out,
        custom_error_count_key=None,
    )

    assert worker.process_messages() == 0
    _assert_drain_queue(out, [])

    ta = sinter.Task(
        circuit=stim.Circuit('H 0'),
        detector_error_model=stim.DetectorErrorModel(),
        decoder='mock',
        collection_options=sinter.CollectionOptions(max_shots=100_000_000),
        json_metadata={'a': 3},
    )
    handler.expected_task = ta
    _put_wait_not_empty(inp, ('change_job', (ta, sinter.CollectionOptions(max_errors=100_000_000), 100_000_000)))
    _put_wait_not_empty(inp, ('accept_shots', (ta.strong_id(), 10000)))
    assert worker.process_messages() == 2
    _assert_drain_queue(out, [
        ('changed_job', 5, (ta.strong_id(),)),
        ('accepted_shots', 5, (ta.strong_id(), 10000)),
    ])

    assert worker.current_task == ta
    assert worker.current_task_shots_left == 10000
    assert worker.process_messages() == 0
    _assert_drain_queue(out, [])

    handler.expected.append((
        10000,
        sinter.AnonTaskStats(
            shots=1000,
            errors=23,
            discards=0,
            seconds=1,
        ),
    ))

    assert worker.do_some_work()
    worker.flush_results()
    _assert_drain_queue(out, [
        ('flushed_results', 5, (ta.strong_id(), sinter.AnonTaskStats(shots=1000, errors=23, discards=0, seconds=1)))])

    handler.expected.append((
        9000,
        sinter.AnonTaskStats(
            shots=9000,
            errors=13,
            discards=0,
            seconds=1,
        ),
    ))

    assert worker.do_some_work()
    worker.flush_results()
    _assert_drain_queue(out, [
        ('flushed_results', 5, (ta.strong_id(), sinter.AnonTaskStats(
            shots=9000,
            errors=13,
            discards=0,
            seconds=1,
        ))),
    ])
    assert not worker.do_some_work()
    worker.flush_results()
    _assert_drain_queue(out, [])
