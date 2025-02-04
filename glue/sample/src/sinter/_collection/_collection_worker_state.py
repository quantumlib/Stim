import queue
import time
from typing import Any
from typing import Optional
from typing import TYPE_CHECKING

import stim

from sinter._data import AnonTaskStats
from sinter._data import CollectionOptions
from sinter._data import Task
from sinter._decoding import CompiledSampler
from sinter._decoding import Sampler

if TYPE_CHECKING:
    import multiprocessing


def _fill_in_task(task: Task) -> Task:
    changed = False
    circuit = task.circuit
    if circuit is None:
        circuit = stim.Circuit.from_file(task.circuit_path)
        changed = True
    dem = task.detector_error_model
    if dem is None:
        try:
            dem = circuit.detector_error_model(decompose_errors=True, approximate_disjoint_errors=True)
        except ValueError:
            try:
                dem = circuit.detector_error_model(approximate_disjoint_errors=True)
            except ValueError:
                dem = circuit.detector_error_model(approximate_disjoint_errors=True, flatten_loops=True)
        changed = True
    if not changed:
        return task
    return Task(
        circuit=circuit,
        decoder=task.decoder,
        detector_error_model=dem,
        postselection_mask=task.postselection_mask,
        postselected_observables_mask=task.postselected_observables_mask,
        json_metadata=task.json_metadata,
        collection_options=task.collection_options,
    )


class CollectionWorkerState:
    def __init__(
            self,
            *,
            flush_period: float,
            worker_id: int,
            inp: 'multiprocessing.Queue',
            out: 'multiprocessing.Queue',
            sampler: Sampler,
            custom_error_count_key: Optional[str],
    ):
        assert isinstance(flush_period, (int, float))
        assert isinstance(sampler, Sampler)
        self.max_flush_period = flush_period
        self.cur_flush_period = 0.01
        self.inp = inp
        self.out = out
        self.sampler = sampler
        self.compiled_sampler: CompiledSampler | None = None
        self.worker_id = worker_id

        self.current_task: Task | None = None
        self.current_error_cutoff: int | None = None
        self.custom_error_count_key = custom_error_count_key
        self.current_task_shots_left: int = 0
        self.unflushed_results: AnonTaskStats = AnonTaskStats()
        self.last_flush_message_time = time.monotonic()
        self.soft_error_flush_threshold: int = 1

    def _send_message_to_manager(self, message: Any):
        self.out.put(message)

    def state_summary(self) -> str:
        lines = [
            f'Worker(id={self.worker_id}) [',
            f'    max_flush_period={self.max_flush_period}',
            f'    cur_flush_period={self.cur_flush_period}',
            f'    sampler={self.sampler}',
            f'    compiled_sampler={self.compiled_sampler}',
            f'    current_task={self.current_task}',
            f'    current_error_cutoff={self.current_error_cutoff}',
            f'    custom_error_count_key={self.custom_error_count_key}',
            f'    current_task_shots_left={self.current_task_shots_left}',
            f'    unflushed_results={self.unflushed_results}',
            f'    last_flush_message_time={self.last_flush_message_time}',
            f'    soft_error_flush_threshold={self.soft_error_flush_threshold}',
            f']',
        ]
        return '\n' + '\n'.join(lines) + '\n'

    def flush_results(self):
        if self.unflushed_results.shots > 0:
            self.last_flush_message_time = time.monotonic()
            self.cur_flush_period = min(self.cur_flush_period * 1.4, self.max_flush_period)
            self._send_message_to_manager((
                'flushed_results',
                self.worker_id,
                (self.current_task.strong_id(), self.unflushed_results),
            ))
            self.unflushed_results = AnonTaskStats()
            return True
        return False

    def accept_shots(self, *, shots_delta: int):
        assert shots_delta >= 0
        self.current_task_shots_left += shots_delta
        self._send_message_to_manager((
            'accepted_shots',
            self.worker_id,
            (self.current_task.strong_id(), shots_delta),
        ))

    def return_shots(self, *, requested_shots: int):
        assert requested_shots >= 0
        returned_shots = max(0, min(requested_shots, self.current_task_shots_left))
        self.current_task_shots_left -= returned_shots
        if self.current_task_shots_left <= 0:
            self.flush_results()
        self._send_message_to_manager((
            'returned_shots',
            self.worker_id,
            (self.current_task.strong_id(), returned_shots),
        ))

    def compute_strong_id(self, *, new_task: Task):
        strong_id = _fill_in_task(new_task).strong_id()
        self._send_message_to_manager((
            'computed_strong_id',
            self.worker_id,
            strong_id,
        ))

    def change_job(self, *, new_task: Task, new_collection_options: CollectionOptions):
        self.flush_results()

        self.current_task = _fill_in_task(new_task)
        self.current_error_cutoff = new_collection_options.max_errors
        self.compiled_sampler = self.sampler.compiled_sampler_for_task(self.current_task)
        assert self.current_task.strong_id() is not None
        self.current_task_shots_left = 0
        self.last_flush_message_time = time.monotonic()

        self._send_message_to_manager((
            'changed_job',
            self.worker_id,
            (self.current_task.strong_id(),),
        ))

    def process_messages(self) -> int:
        num_processed = 0
        while True:
            try:
                message = self.inp.get_nowait()
            except queue.Empty:
                return num_processed

            num_processed += 1
            message_type, message_body = message

            if message_type == 'stop':
                return -1

            elif message_type == 'flush_results':
                self.flush_results()

            elif message_type == 'compute_strong_id':
                assert isinstance(message_body, Task)
                self.compute_strong_id(new_task=message_body)

            elif message_type == 'change_job':
                new_task, new_collection_options, soft_error_flush_threshold = message_body
                self.cur_flush_period = 0.01
                self.soft_error_flush_threshold = soft_error_flush_threshold
                assert isinstance(new_task, Task)
                self.change_job(new_task=new_task, new_collection_options=new_collection_options)

            elif message_type == 'set_soft_error_flush_threshold':
                soft_error_flush_threshold = message_body
                self.soft_error_flush_threshold = soft_error_flush_threshold

            elif message_type == 'accept_shots':
                job_key, shots_delta = message_body
                assert isinstance(shots_delta, int)
                assert job_key == self.current_task.strong_id()
                self.accept_shots(shots_delta=shots_delta)

            elif message_type == 'return_shots':
                job_key, requested_shots = message_body
                assert isinstance(requested_shots, int)
                assert job_key == self.current_task.strong_id()
                self.return_shots(requested_shots=requested_shots)

            else:
                raise NotImplementedError(f'{message_type=}')

    def num_unflushed_errors(self) -> int:
        if self.custom_error_count_key is not None:
            return self.unflushed_results.custom_counts[self.custom_error_count_key]
        return self.unflushed_results.errors

    def do_some_work(self) -> bool:
        did_some_work = False

        # Sample some stats.
        if self.current_task_shots_left > 0:
            # Don't keep sampling if we've exceeded the number of errors needed.
            if self.current_error_cutoff is not None and self.current_error_cutoff <= 0:
                return self.flush_results()

            some_work_done = self.compiled_sampler.sample(self.current_task_shots_left)
            if some_work_done.shots < 1:
                raise ValueError(f"Sampler didn't do any work. It returned statistics with shots == 0: {some_work_done}.")
            assert isinstance(some_work_done, AnonTaskStats)
            self.current_task_shots_left -= some_work_done.shots
            if self.current_error_cutoff is not None:
                errors_done = some_work_done.custom_counts[self.custom_error_count_key] if self.custom_error_count_key is not None else some_work_done.errors
                self.current_error_cutoff -= errors_done
            self.unflushed_results += some_work_done
            did_some_work = True

        # Report them periodically.
        should_flush = False
        if self.num_unflushed_errors() >= self.soft_error_flush_threshold:
            should_flush = True
        if self.unflushed_results.shots > 0:
            if self.current_task_shots_left <= 0 or self.last_flush_message_time + self.cur_flush_period < time.monotonic():
                should_flush = True
        if should_flush:
            did_some_work |= self.flush_results()

        return did_some_work

    def run_message_loop(self):
        try:
            while True:
                num_messages_processed = self.process_messages()
                if num_messages_processed == -1:
                    break
                did_some_work = self.do_some_work()
                if not did_some_work and num_messages_processed == 0:
                    time.sleep(0.01)

        except KeyboardInterrupt:
            pass

        except BaseException as ex:
            import traceback
            self._send_message_to_manager((
                'stopped_due_to_exception',
                self.worker_id,
                (None if self.current_task is None else self.current_task.strong_id(), self.current_task_shots_left, self.unflushed_results, traceback.format_exc(), ex),
            ))
