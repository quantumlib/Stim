import multiprocessing
import time
from typing import Any, List, Union

import sinter
import stim

from sinter._collection._collection_manager import CollectionManager


def _assert_drain_queue(q: multiprocessing.Queue, expected_contents: List[Any]):
    for v in expected_contents:
        assert q.get(timeout=0.1) == v
    if not q.empty():
        assert False, f'queue had another item: {q.get()=}'


def _put_wait_not_empty(q: Union[multiprocessing.Queue, multiprocessing.SimpleQueue], item: Any):
    q.put(item)
    while q.empty():
        time.sleep(0.0001)


def test_manager():
    log = []
    t0 = sinter.Task(
        circuit=stim.Circuit('H 0'),
        detector_error_model=stim.DetectorErrorModel(),
        decoder='fusion_blossom',
        collection_options=sinter.CollectionOptions(max_shots=100_000_000, max_errors=100),
        json_metadata={'a': 3},
    )
    t1 = sinter.Task(
        circuit=stim.Circuit('M 0'),
        detector_error_model=stim.DetectorErrorModel(),
        decoder='pymatching',
        collection_options=sinter.CollectionOptions(max_shots=10_000_000),
        json_metadata=None,
    )
    manager = CollectionManager(
        num_workers=3,
        worker_flush_period=30,
        tasks=[t0, t1],
        progress_callback=log.append,
        existing_data={},
        collection_options=sinter.CollectionOptions(),
        custom_decoders={},
        allowed_cpu_affinity_ids=None,
    )

    assert manager.state_summary() == """
worker 0: asked_to_drop_shots=0 assigned_shots=0 assigned_work_key=None
worker 1: asked_to_drop_shots=0 assigned_shots=0 assigned_work_key=None
worker 2: asked_to_drop_shots=0 assigned_shots=0 assigned_work_key=None
"""

    manager.start_workers(actually_start_worker_processes=False)
    manager.shared_worker_output_queue.put(('computed_strong_id', 2, 'c03f7852e4579e2a99cefac80eeb6b09556907540ab3d7787a3d07309c3333aa'))
    manager.shared_worker_output_queue.put(('computed_strong_id', 1, 'a9165b6e4ab1053c04c017d0739a7bfff0910d62091fc9ee81716833eda7f604'))
    manager.start_distributing_work()

    assert manager.state_summary() == """
worker 0: asked_to_drop_shots=0 assigned_shots=100000000 assigned_work_key=a9165b6e4ab1053c04c017d0739a7bfff0910d62091fc9ee81716833eda7f604
worker 1: asked_to_drop_shots=0 assigned_shots=5000000 assigned_work_key=c03f7852e4579e2a99cefac80eeb6b09556907540ab3d7787a3d07309c3333aa
worker 2: asked_to_drop_shots=0 assigned_shots=5000000 assigned_work_key=c03f7852e4579e2a99cefac80eeb6b09556907540ab3d7787a3d07309c3333aa
task task.strong_id='a9165b6e4ab1053c04c017d0739a7bfff0910d62091fc9ee81716833eda7f604':
    workers_assigned=[0]
    shot_return_requests=0
    shots_left=100000000
    errors_left=100
    shots_unassigned=0
task task.strong_id='c03f7852e4579e2a99cefac80eeb6b09556907540ab3d7787a3d07309c3333aa':
    workers_assigned=[1, 2]
    shot_return_requests=0
    shots_left=10000000
    errors_left=10000000
    shots_unassigned=0
"""

    _assert_drain_queue(manager.worker_states[0].input_queue, [
        (
            'change_job',
            (t0, sinter.CollectionOptions(max_errors=100), 100),
        ),
        (
            'accept_shots',
            (t0.strong_id(), 100_000_000),
        ),
    ])
    _assert_drain_queue(manager.worker_states[1].input_queue, [
        ('compute_strong_id', t0),
        (
            'change_job',
            (t1, sinter.CollectionOptions(max_errors=10000000), 10000000),
        ),
        (
            'accept_shots',
            (t1.strong_id(), 5_000_000),
        ),
    ])
    _assert_drain_queue(manager.worker_states[2].input_queue, [
        ('compute_strong_id', t1),
        (
            'change_job',
            (t1, sinter.CollectionOptions(max_errors=10000000), 10000000),
        ),
        (
            'accept_shots',
            (t1.strong_id(), 5_000_000),
        ),
    ])

    assert manager.shared_worker_output_queue.empty()
    assert log.pop() is None
    assert log.pop() is None
    assert not log
    _put_wait_not_empty(manager.shared_worker_output_queue, (
        'flushed_results',
        2,
        (t1.strong_id(), sinter.AnonTaskStats(
            shots=5_000_000,
            errors=123,
            discards=0,
            seconds=1,
        )),
    ))

    assert manager.process_message()
    assert manager.state_summary() == """
worker 0: asked_to_drop_shots=0 assigned_shots=100000000 assigned_work_key=a9165b6e4ab1053c04c017d0739a7bfff0910d62091fc9ee81716833eda7f604
worker 1: asked_to_drop_shots=2500000 assigned_shots=5000000 assigned_work_key=c03f7852e4579e2a99cefac80eeb6b09556907540ab3d7787a3d07309c3333aa
worker 2: asked_to_drop_shots=0 assigned_shots=0 assigned_work_key=c03f7852e4579e2a99cefac80eeb6b09556907540ab3d7787a3d07309c3333aa
task task.strong_id='a9165b6e4ab1053c04c017d0739a7bfff0910d62091fc9ee81716833eda7f604':
    workers_assigned=[0]
    shot_return_requests=0
    shots_left=100000000
    errors_left=100
    shots_unassigned=0
task task.strong_id='c03f7852e4579e2a99cefac80eeb6b09556907540ab3d7787a3d07309c3333aa':
    workers_assigned=[1, 2]
    shot_return_requests=1
    shots_left=5000000
    errors_left=9999877
    shots_unassigned=0
"""

    assert log.pop() == sinter.TaskStats(
        strong_id=t1.strong_id(),
        decoder=t1.decoder,
        json_metadata=t1.json_metadata,
        shots=5_000_000,
        errors=123,
        discards=0,
        seconds=1,
    )
    assert not log

    _assert_drain_queue(manager.worker_states[0].input_queue, [])
    _assert_drain_queue(manager.worker_states[1].input_queue, [
        (
            'return_shots',
            (t1.strong_id(), 2_500_000),
        ),
    ])
    _assert_drain_queue(manager.worker_states[2].input_queue, [])

    _put_wait_not_empty(manager.shared_worker_output_queue, (
        'returned_shots',
        1,
        (t1.strong_id(), 2_000_000),
    ))
    assert manager.process_message()
    assert manager.state_summary() == """
worker 0: asked_to_drop_shots=0 assigned_shots=100000000 assigned_work_key=a9165b6e4ab1053c04c017d0739a7bfff0910d62091fc9ee81716833eda7f604
worker 1: asked_to_drop_shots=0 assigned_shots=3000000 assigned_work_key=c03f7852e4579e2a99cefac80eeb6b09556907540ab3d7787a3d07309c3333aa
worker 2: asked_to_drop_shots=0 assigned_shots=2000000 assigned_work_key=c03f7852e4579e2a99cefac80eeb6b09556907540ab3d7787a3d07309c3333aa
task task.strong_id='a9165b6e4ab1053c04c017d0739a7bfff0910d62091fc9ee81716833eda7f604':
    workers_assigned=[0]
    shot_return_requests=0
    shots_left=100000000
    errors_left=100
    shots_unassigned=0
task task.strong_id='c03f7852e4579e2a99cefac80eeb6b09556907540ab3d7787a3d07309c3333aa':
    workers_assigned=[1, 2]
    shot_return_requests=0
    shots_left=5000000
    errors_left=9999877
    shots_unassigned=0
"""

    _assert_drain_queue(manager.worker_states[0].input_queue, [])
    _assert_drain_queue(manager.worker_states[1].input_queue, [])
    _assert_drain_queue(manager.worker_states[2].input_queue, [
        (
            'accept_shots',
            (t1.strong_id(), 2_000_000),
        ),
    ])

    _put_wait_not_empty(manager.shared_worker_output_queue, (
        'flushed_results',
        1,
        (t1.strong_id(), sinter.AnonTaskStats(
            shots=3_000_000,
            errors=444,
            discards=1,
            seconds=2,
        ))
    ))
    assert manager.process_message()
    assert manager.state_summary() == """
worker 0: asked_to_drop_shots=0 assigned_shots=100000000 assigned_work_key=a9165b6e4ab1053c04c017d0739a7bfff0910d62091fc9ee81716833eda7f604
worker 1: asked_to_drop_shots=0 assigned_shots=0 assigned_work_key=c03f7852e4579e2a99cefac80eeb6b09556907540ab3d7787a3d07309c3333aa
worker 2: asked_to_drop_shots=1000000 assigned_shots=2000000 assigned_work_key=c03f7852e4579e2a99cefac80eeb6b09556907540ab3d7787a3d07309c3333aa
task task.strong_id='a9165b6e4ab1053c04c017d0739a7bfff0910d62091fc9ee81716833eda7f604':
    workers_assigned=[0]
    shot_return_requests=0
    shots_left=100000000
    errors_left=100
    shots_unassigned=0
task task.strong_id='c03f7852e4579e2a99cefac80eeb6b09556907540ab3d7787a3d07309c3333aa':
    workers_assigned=[1, 2]
    shot_return_requests=1
    shots_left=2000000
    errors_left=9999433
    shots_unassigned=0
"""

    _put_wait_not_empty(manager.shared_worker_output_queue, (
        'flushed_results',
        2,
        (t1.strong_id(), sinter.AnonTaskStats(
            shots=2_000_000,
            errors=555,
            discards=2,
            seconds=2.5,
        ))
    ))
    assert manager.process_message()
    assert manager.state_summary() == """
worker 0: asked_to_drop_shots=0 assigned_shots=100000000 assigned_work_key=a9165b6e4ab1053c04c017d0739a7bfff0910d62091fc9ee81716833eda7f604
worker 1: asked_to_drop_shots=0 assigned_shots=0 assigned_work_key=c03f7852e4579e2a99cefac80eeb6b09556907540ab3d7787a3d07309c3333aa
worker 2: asked_to_drop_shots=1000000 assigned_shots=0 assigned_work_key=c03f7852e4579e2a99cefac80eeb6b09556907540ab3d7787a3d07309c3333aa
task task.strong_id='a9165b6e4ab1053c04c017d0739a7bfff0910d62091fc9ee81716833eda7f604':
    workers_assigned=[0]
    shot_return_requests=0
    shots_left=100000000
    errors_left=100
    shots_unassigned=0
task task.strong_id='c03f7852e4579e2a99cefac80eeb6b09556907540ab3d7787a3d07309c3333aa':
    workers_assigned=[1, 2]
    shot_return_requests=1
    shots_left=0
    errors_left=9998878
    shots_unassigned=0
"""

    assert manager.shared_worker_output_queue.empty()
    _put_wait_not_empty(manager.shared_worker_output_queue, (
        'returned_shots',
        2,
        (t1.strong_id(), 0)
    ))
    assert manager.process_message()
    assert manager.shared_worker_output_queue.empty()
    assert manager.state_summary() == """
worker 0: asked_to_drop_shots=66666666 assigned_shots=100000000 assigned_work_key=a9165b6e4ab1053c04c017d0739a7bfff0910d62091fc9ee81716833eda7f604
worker 1: asked_to_drop_shots=0 assigned_shots=0 assigned_work_key=a9165b6e4ab1053c04c017d0739a7bfff0910d62091fc9ee81716833eda7f604
worker 2: asked_to_drop_shots=0 assigned_shots=0 assigned_work_key=a9165b6e4ab1053c04c017d0739a7bfff0910d62091fc9ee81716833eda7f604
task task.strong_id='a9165b6e4ab1053c04c017d0739a7bfff0910d62091fc9ee81716833eda7f604':
    workers_assigned=[0, 1, 2]
    shot_return_requests=1
    shots_left=100000000
    errors_left=100
    shots_unassigned=0
"""

    _assert_drain_queue(manager.worker_states[0].input_queue, [
        ('return_shots', (t0.strong_id(), 66666666)),
    ])
    _assert_drain_queue(manager.worker_states[1].input_queue, [
        ('change_job', (t0, sinter.CollectionOptions(max_errors=100), 100)),
    ])
    _assert_drain_queue(manager.worker_states[2].input_queue, [
        ('return_shots', (t1.strong_id(), 1000000)),
        ('change_job', (t0, sinter.CollectionOptions(max_errors=100), 100)),
    ])
