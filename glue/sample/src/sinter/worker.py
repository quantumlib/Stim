from typing import Any, Optional, TYPE_CHECKING

import tempfile

if TYPE_CHECKING:
    import multiprocessing
    import pathlib
    import sinter


class WorkIn:
    def __init__(self, *, key: Any, task: 'sinter.ExecutableTask', summary: 'sinter.TaskSummary', num_shots: int):
        self.key = key
        self.task = task
        self.summary = summary
        self.num_shots = num_shots


class WorkOut:
    def __init__(self, *, key: Any, sample: Optional['sinter.TaskStats'], error: Optional[BaseException]):
        self.key = key
        self.sample = sample
        self.error = error


def worker_loop(tmp_dir: 'pathlib.Path',
                inp: 'multiprocessing.Queue',
                out: 'multiprocessing.Queue') -> None:
    try:
        from sinter.task_stats import TaskStats

        with tempfile.TemporaryDirectory(dir=tmp_dir) as child_dir:
            while True:
                work: Optional[WorkIn] = inp.get()
                if work is None:
                    return
                assert isinstance(work, WorkIn)
                try:
                    stats: 'sinter.AnonTaskStats' = work.task.sample_stats(
                        num_shots=work.num_shots, tmp_dir=child_dir)
                except BaseException as ex:
                    out.put(WorkOut(key=work.key, sample=None, error=ex))
                    continue

                sample = TaskStats(
                    decoder=work.task.decoder,
                    json_metadata=work.summary.json_metadata,
                    strong_id=work.summary.strong_id,
                    shots=stats.shots,
                    discards=stats.discards,
                    errors=stats.errors,
                    seconds=stats.seconds,
                )
                out.put(WorkOut(sample=sample, key=work.key, error=None))
    except KeyboardInterrupt:
        pass
