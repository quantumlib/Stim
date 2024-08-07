import collections
import contextlib
import math
import multiprocessing
import os
import pathlib
import queue
import tempfile
import threading
from typing import Any, Optional, List, Dict, Iterable, Callable, Tuple
from typing import Union
from typing import cast

from sinter._collection._collection_worker_loop import collection_worker_loop
from sinter._collection._mux_sampler import MuxSampler
from sinter._collection._sampler_ramp_throttled import RampThrottledSampler
from sinter._data import CollectionOptions, Task, AnonTaskStats, TaskStats
from sinter._decoding import Sampler, Decoder


class _ManagedWorkerState:
    def __init__(self, worker_id: int, *, cpu_pin: Optional[int] = None):
        self.worker_id: int = worker_id
        self.process: Union[multiprocessing.Process, threading.Thread, None] = None
        self.input_queue: Optional[multiprocessing.Queue[Tuple[str, Any]]] = None
        self.assigned_work_key: Any = None
        self.asked_to_drop_shots: int = 0
        self.cpu_pin = cpu_pin

        # Shots transfer into this field when manager sends shot requests to workers.
        # Shots transfer out of this field when clients flush results or respond to work return requests.
        self.assigned_shots: int = 0

    def send_message(self, message: Any):
        self.input_queue.put(message)

    def ask_to_return_all_shots(self):
        if self.asked_to_drop_shots == 0 and self.assigned_shots > 0:
            self.send_message((
                'return_shots',
                (
                    self.assigned_work_key,
                    self.assigned_shots,
                ),
            ))
            self.asked_to_drop_shots = self.assigned_shots

    def has_returned_all_shots(self) -> bool:
        return self.assigned_shots == 0 and self.asked_to_drop_shots == 0

    def is_available_to_reassign(self) -> bool:
        return self.assigned_work_key is None


class _ManagedTaskState:
    def __init__(self, *, partial_task: Task, strong_id: str, shots_left: int, errors_left: int):
        self.partial_task = partial_task
        self.strong_id = strong_id
        self.shots_left = shots_left
        self.errors_left = errors_left
        self.shots_unassigned = shots_left
        self.shot_return_requests = 0
        self.assigned_soft_error_flush_threshold: int = errors_left
        self.workers_assigned: list[int] = []

    def is_completed(self) -> bool:
        return self.shots_left <= 0 or self.errors_left <= 0


class CollectionManager:
    def __init__(
            self,
            *,
            existing_data: Dict[Any, TaskStats],
            collection_options: CollectionOptions,
            custom_decoders: dict[str, Union[Decoder, Sampler]],
            num_workers: int,
            worker_flush_period: float,
            tasks: Iterable[Task],
            progress_callback: Callable[[Optional[TaskStats]], None],
            allowed_cpu_affinity_ids: Optional[Iterable[int]],
            count_observable_error_combos: bool = False,
            count_detection_events: bool = False,
            custom_error_count_key: Optional[str] = None,
            use_threads_for_debugging: bool = False,
    ):
        assert isinstance(custom_decoders, dict)
        self.existing_data = existing_data
        self.num_workers: int = num_workers
        self.custom_decoders = custom_decoders
        self.worker_flush_period: float = worker_flush_period
        self.progress_callback = progress_callback
        self.collection_options = collection_options
        self.partial_tasks: list[Task] = list(tasks)
        self.task_strong_ids: List[Optional[str]] = [None] * len(self.partial_tasks)
        self.allowed_cpu_affinity_ids = None if allowed_cpu_affinity_ids is None else sorted(set(allowed_cpu_affinity_ids))
        self.count_observable_error_combos = count_observable_error_combos
        self.count_detection_events = count_detection_events
        self.custom_error_count_key = custom_error_count_key
        self.use_threads_for_debugging = use_threads_for_debugging

        self.shared_worker_output_queue: Optional[multiprocessing.SimpleQueue[Tuple[str, int, Any]]] = None
        self.task_states: Dict[Any, _ManagedTaskState] = {}
        self.started: bool = False
        self.total_collected = {k: v.to_anon_stats() for k, v in existing_data.items()}

        if self.allowed_cpu_affinity_ids is None:
            cpus = range(os.cpu_count())
        else:
            num_cpus = os.cpu_count()
            cpus = [e for e in self.allowed_cpu_affinity_ids if e < num_cpus]
        self.worker_states: List[_ManagedWorkerState] = []
        for index in range(num_workers):
            cpu_pin = None if len(cpus) == 0 else cpus[index % len(cpus)]
            self.worker_states.append(_ManagedWorkerState(index, cpu_pin=cpu_pin))
        self.tmp_dir: Optional[pathlib.Path] = None

    def __enter__(self):
        self.exit_stack = contextlib.ExitStack().__enter__()
        self.tmp_dir = pathlib.Path(self.exit_stack.enter_context(tempfile.TemporaryDirectory()))
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.hard_stop()
        self.exit_stack.__exit__(exc_type, exc_val, exc_tb)
        self.exit_stack = None
        self.tmp_dir = None

    def start_workers(self, *, actually_start_worker_processes: bool = True):
        assert not self.started

        sampler = RampThrottledSampler(
            sub_sampler=MuxSampler(
                custom_decoders=self.custom_decoders,
                count_observable_error_combos=self.count_observable_error_combos,
                count_detection_events=self.count_detection_events,
                tmp_dir=self.tmp_dir,
            ),
            target_batch_seconds=1,
            max_batch_shots=1024,
        )

        self.started = True
        current_method = multiprocessing.get_start_method()
        try:
            # To ensure the child processes do not accidentally share ANY state
            # related to random number generation, we use 'spawn' instead of 'fork'.
            multiprocessing.set_start_method('spawn', force=True)
            # Create queues after setting start method to work around a deadlock
            # bug that occurs otherwise.
            self.shared_worker_output_queue = multiprocessing.SimpleQueue()

            for worker_id in range(self.num_workers):
                worker_state = self.worker_states[worker_id]
                worker_state.input_queue = multiprocessing.Queue()
                worker_state.input_queue.cancel_join_thread()
                worker_state.assigned_work_key = None
                args = (
                    self.worker_flush_period,
                    worker_id,
                    sampler,
                    worker_state.input_queue,
                    self.shared_worker_output_queue,
                    worker_state.cpu_pin,
                    self.custom_error_count_key,
                )
                if self.use_threads_for_debugging:
                    worker_state.process = threading.Thread(
                        target=collection_worker_loop,
                        args=args,
                    )
                else:
                    worker_state.process = multiprocessing.Process(
                        target=collection_worker_loop,
                        args=args,
                    )

                if actually_start_worker_processes:
                    worker_state.process.start()
        finally:
            multiprocessing.set_start_method(current_method, force=True)

    def start_distributing_work(self):
        self._compute_task_ids()
        self._distribute_work()

    def _compute_task_ids(self):
        idle_worker_ids = list(range(self.num_workers))
        unknown_task_ids = list(range(len(self.partial_tasks)))
        worker_to_task_map = {}
        while worker_to_task_map or unknown_task_ids:
            while idle_worker_ids and unknown_task_ids:
                worker_id = idle_worker_ids.pop()
                unknown_task_id = unknown_task_ids.pop()
                worker_to_task_map[worker_id] = unknown_task_id
                self.worker_states[worker_id].send_message(('compute_strong_id', self.partial_tasks[unknown_task_id]))

            try:
                message = self.shared_worker_output_queue.get()
                message_type, worker_id, message_body = message
                if message_type == 'computed_strong_id':
                    assert worker_id in worker_to_task_map
                    assert isinstance(message_body, str)
                    self.task_strong_ids[worker_to_task_map.pop(worker_id)] = message_body
                    idle_worker_ids.append(worker_id)
                elif message_type == 'stopped_due_to_exception':
                    cur_task, cur_shots_left, unflushed_work_done, traceback, ex = message_body
                    raise ValueError(f'Worker failed: traceback={traceback}') from ex
                else:
                    raise NotImplementedError(f'{message_type=}')
                self.progress_callback(None)
            except queue.Empty:
                pass

        assert len(idle_worker_ids) == self.num_workers
        seen = set()
        for k in range(len(self.partial_tasks)):
            options = self.partial_tasks[k].collection_options.combine(self.collection_options)
            key: str = self.task_strong_ids[k]
            if key in seen:
                raise ValueError(f'Same task given twice: {self.partial_tasks[k]!r}')
            seen.add(key)

            shots_left = options.max_shots
            errors_left = options.max_errors
            if errors_left is None:
                errors_left = shots_left
            errors_left = min(errors_left, shots_left)
            if key in self.existing_data:
                val = self.existing_data[key]
                shots_left -= val.shots
                if self.custom_error_count_key is None:
                    errors_left -= val.errors
                else:
                    errors_left -= val.custom_counts[self.custom_error_count_key]
            if shots_left <= 0:
                continue
            self.task_states[key] = _ManagedTaskState(
                partial_task=self.partial_tasks[k],
                strong_id=key,
                shots_left=shots_left,
                errors_left=errors_left,
            )
            if self.task_states[key].is_completed():
                del self.task_states[key]

    def hard_stop(self):
        if not self.started:
            return

        removed_workers = [state.process for state in self.worker_states]
        for state in self.worker_states:
            if isinstance(state.process, threading.Thread):
                state.send_message('stop')
            state.process = None
            state.assigned_work_key = None
            state.input_queue = None
        self.shared_worker_output_queue = None
        self.started = False
        self.task_states.clear()

        # SIGKILL everything.
        for w in removed_workers:
            if isinstance(w, multiprocessing.Process):
                w.kill()
        # Wait for them to be done.
        for w in removed_workers:
            w.join()

    def _handle_task_progress(self, task_id: Any):
        task_state = self.task_states[task_id]
        if task_state.is_completed():
            workers_ready = all(self.worker_states[worker_id].has_returned_all_shots() for worker_id in task_state.workers_assigned)
            if workers_ready:
                # Task is fully completed and can be forgotten entirely. Re-assign the workers.
                del self.task_states[task_id]
                for worker_id in task_state.workers_assigned:
                    w = self.worker_states[worker_id]
                    assert w.assigned_shots <= 0
                    assert w.asked_to_drop_shots == 0
                    w.assigned_work_key = None
                self._distribute_work()
            else:
                # Task is sufficiently sampled, but some workers are still running.
                for worker_id in task_state.workers_assigned:
                    self.worker_states[worker_id].ask_to_return_all_shots()
            self.progress_callback(None)
        else:
            self._distribute_unassigned_workers_to_jobs()
            self._distribute_work_within_a_job(task_state)

    def state_summary(self) -> str:
        lines = []
        for worker_id, worker in enumerate(self.worker_states):
            lines.append(f'worker {worker_id}:'
                         f' asked_to_drop_shots={worker.asked_to_drop_shots}'
                         f' assigned_shots={worker.assigned_shots}'
                         f' assigned_work_key={worker.assigned_work_key}')
        for task in self.task_states.values():
            lines.append(f'task {task.strong_id=}:\n'
                         f'    workers_assigned={task.workers_assigned}\n'
                         f'    shot_return_requests={task.shot_return_requests}\n'
                         f'    shots_left={task.shots_left}\n'
                         f'    errors_left={task.errors_left}\n'
                         f'    shots_unassigned={task.shots_unassigned}')
        return '\n' + '\n'.join(lines) + '\n'

    def process_message(self) -> bool:
        try:
            message = self.shared_worker_output_queue.get()
        except queue.Empty:
            return False

        message_type, worker_id, message_body = message
        worker_state = self.worker_states[worker_id]

        if message_type == 'flushed_results':
            task_strong_id, anon_stat = message_body
            assert isinstance(anon_stat, AnonTaskStats)
            assert worker_state.assigned_work_key == task_strong_id
            task_state = self.task_states[task_strong_id]

            worker_state.assigned_shots -= anon_stat.shots
            task_state.shots_left -= anon_stat.shots
            if worker_state.assigned_shots < 0:
                # Worker over-achieved. Correct the imbalance by giving them the shots.
                extra_shots = abs(worker_state.assigned_shots)
                worker_state.assigned_shots += extra_shots
                task_state.shots_unassigned -= extra_shots
                worker_state.send_message((
                    'accept_shots',
                    (task_state.strong_id, extra_shots),
                ))

            if self.custom_error_count_key is None:
                task_state.errors_left -= anon_stat.errors
            else:
                task_state.errors_left -= anon_stat.custom_counts[self.custom_error_count_key]

            stat = TaskStats(
                strong_id=task_state.strong_id,
                decoder=task_state.partial_task.decoder,
                json_metadata=task_state.partial_task.json_metadata,
                shots=anon_stat.shots,
                discards=anon_stat.discards,
                seconds=anon_stat.seconds,
                errors=anon_stat.errors,
                custom_counts=anon_stat.custom_counts,
            )

            self._handle_task_progress(task_strong_id)

            if stat.strong_id not in self.total_collected:
                self.total_collected[stat.strong_id] = AnonTaskStats()
            self.total_collected[stat.strong_id] += stat.to_anon_stats()
            self.progress_callback(stat)

        elif message_type == 'changed_job':
            pass

        elif message_type == 'accepted_shots':
            pass

        elif message_type == 'returned_shots':
            task_key, shots_returned = message_body
            assert isinstance(shots_returned, int)
            assert shots_returned >= 0
            assert worker_state.assigned_work_key == task_key
            assert worker_state.asked_to_drop_shots or worker_state.asked_to_drop_errors
            task_state = self.task_states[task_key]
            task_state.shot_return_requests -= 1
            worker_state.asked_to_drop_shots = 0
            worker_state.asked_to_drop_errors = 0
            task_state.shots_unassigned += shots_returned
            worker_state.assigned_shots -= shots_returned
            assert worker_state.assigned_shots >= 0
            self._handle_task_progress(task_key)

        elif message_type == 'stopped_due_to_exception':
            cur_task, cur_shots_left, unflushed_work_done, traceback, ex = message_body
            raise RuntimeError(f'Worker failed: traceback={traceback}') from ex

        else:
            raise NotImplementedError(f'{message_type=}')

        return True

    def run_until_done(self) -> bool:
        try:
            while self.task_states:
                self.process_message()
            return True

        except KeyboardInterrupt:
            return False

        finally:
            self.hard_stop()

    def _distribute_unassigned_workers_to_jobs(self):
        idle_workers = [
            w
            for w in range(self.num_workers)[::-1]
            if self.worker_states[w].is_available_to_reassign()
        ]
        if not idle_workers or not self.started:
            return

        groups = collections.defaultdict(list)
        for work_state in self.task_states.values():
            if not work_state.is_completed():
                groups[len(work_state.workers_assigned)].append(work_state)
        for k in groups.keys():
            groups[k] = groups[k][::-1]
        if not groups:
            return
        min_assigned = min(groups.keys(), default=0)

        # Distribute workers to unfinished jobs with the fewest workers.
        while idle_workers:
            task_state: _ManagedTaskState = groups[min_assigned].pop()
            groups[min_assigned + 1].append(task_state)
            if not groups[min_assigned]:
                min_assigned += 1

            worker_id = idle_workers.pop()
            task_state.workers_assigned.append(worker_id)
            worker_state = self.worker_states[worker_id]
            worker_state.assigned_work_key = task_state.strong_id
            worker_state.send_message((
                'change_job',
                (task_state.partial_task, CollectionOptions(max_errors=task_state.errors_left), task_state.assigned_soft_error_flush_threshold),
            ))

    def _distribute_unassigned_work_to_workers_within_a_job(self, task_state: _ManagedTaskState):
        if not self.started or not task_state.workers_assigned or task_state.shots_left <= 0:
            return

        num_task_workers = len(task_state.workers_assigned)
        expected_shots_per_worker = (task_state.shots_left + num_task_workers - 1) // num_task_workers

        # Give unassigned shots to idle workers.
        for worker_id in sorted(task_state.workers_assigned, key=lambda wid: self.worker_states[wid].assigned_shots):
            worker_state = self.worker_states[worker_id]
            if worker_state.assigned_shots < expected_shots_per_worker:
                shots_to_assign = min(expected_shots_per_worker - worker_state.assigned_shots,
                                      task_state.shots_unassigned)
                if shots_to_assign > 0:
                    task_state.shots_unassigned -= shots_to_assign
                    worker_state.assigned_shots += shots_to_assign
                    worker_state.send_message((
                        'accept_shots',
                        (task_state.strong_id, shots_to_assign),
                    ))

    def status_message(self) -> str:
        num_known_tasks_ids = sum(e is not None for e in self.task_strong_ids)
        if num_known_tasks_ids < len(self.task_strong_ids):
            return f"Analyzed {num_known_tasks_ids}/{len(self.task_strong_ids)} tasks..."
        max_errors = self.collection_options.max_errors
        max_shots = self.collection_options.max_shots

        tasks_left = 0
        lines = []
        skipped_lines = []
        for k, strong_id in enumerate(self.task_strong_ids):
            if strong_id not in self.task_states:
                continue
            c = self.total_collected.get(strong_id, AnonTaskStats())
            tasks_left += 1
            w = len(self.task_states[strong_id].workers_assigned)
            dt = None
            if max_shots is not None and c.shots:
                dt = (max_shots - c.shots) * c.seconds / c.shots
            c_errors = c.custom_counts[self.custom_error_count_key] if self.custom_error_count_key is not None else c.errors
            if max_errors is not None and c_errors and c.seconds:
                dt2 = (max_errors - c_errors) * c.seconds / c_errors
                if dt is None:
                    dt = dt2
                else:
                    dt = min(dt, dt2)
            if dt is not None:
                dt /= 60
            if dt is not None and w > 0:
                dt /= w
            line = [
                f'{w}',
                self.partial_tasks[k].decoder,
                ("?" if dt is None or dt == 0 else "[draining]" if dt <= 0 else "<1m" if dt < 1 else str(round(dt)) + 'm') + ('·∞' if w == 0 else ''),
                f'{max_shots - c.shots}' if max_shots is not None else f'{c.shots}',
                f'{max_errors - c_errors}' if max_errors is not None else f'{c_errors}',
                ",".join(
                    [f"{k}={v}" for k, v in self.partial_tasks[k].json_metadata.items()]
                    if isinstance(self.partial_tasks[k].json_metadata, dict)
                    else str(self.partial_tasks[k].json_metadata)
                )
            ]
            if w == 0:
                skipped_lines.append(line)
            else:
                lines.append(line)
        if len(lines) < 50 and skipped_lines:
            missing_lines = 50 - len(lines)
            lines.extend(skipped_lines[:missing_lines])
            skipped_lines = skipped_lines[missing_lines:]

        if lines:
            lines.insert(0, [
                'workers',
                'decoder',
                'eta',
                'shots_left' if max_shots is not None else 'shots_taken',
                'errors_left' if max_errors is not None else 'errors_seen',
                'json_metadata'])
            justs = cast(list[Callable[[str, int], str]], [str.rjust, str.rjust, str.rjust, str.rjust, str.rjust, str.ljust])
            cols = len(lines[0])
            lengths = [
                max(len(lines[row][col]) for row in range(len(lines)))
                for col in range(cols)
            ]
            lines = [
                "  " + " ".join(justs[col](row[col], lengths[col]) for col in range(cols))
                for row in lines
            ]
        if skipped_lines:
            lines.append('        ... (' + str(len(skipped_lines)) + ' more tasks) ...')
        return f'{tasks_left} tasks left:\n' + '\n'.join(lines)

    def _update_soft_error_threshold_for_a_job(self, task_state: _ManagedTaskState):
        if task_state.errors_left <= len(task_state.workers_assigned):
            desired_threshold = 1
        elif task_state.errors_left <= task_state.assigned_soft_error_flush_threshold * self.num_workers:
            desired_threshold = max(1, math.ceil(task_state.errors_left * 0.5 / self.num_workers))
        else:
            return

        if task_state.assigned_soft_error_flush_threshold != desired_threshold:
            task_state.assigned_soft_error_flush_threshold = desired_threshold
            for wid in task_state.workers_assigned:
                self.worker_states[wid].send_message(('set_soft_error_flush_threshold', desired_threshold))

    def _take_work_if_unsatisfied_workers_within_a_job(self, task_state: _ManagedTaskState):
        if not self.started or not task_state.workers_assigned or task_state.shots_left <= 0:
            return

        if all(self.worker_states[w].assigned_shots > 0 for w in task_state.workers_assigned):
            return

        w = len(task_state.workers_assigned)
        expected_shots_per_worker = (task_state.shots_left + w - 1) // w

        # There are idle workers that couldn't be given any shots. Take shots from other workers.
        for worker_id in sorted(task_state.workers_assigned, key=lambda w: self.worker_states[w].assigned_shots, reverse=True):
            worker_state = self.worker_states[worker_id]
            if worker_state.asked_to_drop_shots or worker_state.assigned_shots <= expected_shots_per_worker:
                continue
            shots_to_take = worker_state.assigned_shots - expected_shots_per_worker
            assert shots_to_take > 0
            worker_state.asked_to_drop_shots = shots_to_take
            task_state.shot_return_requests += 1
            worker_state.send_message((
                'return_shots',
                (
                    task_state.strong_id,
                    shots_to_take,
                ),
            ))

    def _distribute_work_within_a_job(self, t: _ManagedTaskState):
        self._distribute_unassigned_work_to_workers_within_a_job(t)
        self._take_work_if_unsatisfied_workers_within_a_job(t)

    def _distribute_work(self):
        self._distribute_unassigned_workers_to_jobs()
        for w in self.task_states.values():
            if not w.is_completed():
                self._distribute_work_within_a_job(w)
