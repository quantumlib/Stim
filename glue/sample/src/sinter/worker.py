import dataclasses
import multiprocessing
import pathlib
import tempfile
from typing import Any
from typing import Optional

from sinter.task_stats import TaskStats
from sinter.executable_task import ExecutableTask
from sinter.anon_task_stats import AnonTaskStats
from sinter.task_summary import TaskSummary


@dataclasses.dataclass
class WorkIn:
    key: Any
    task: ExecutableTask
    summary: TaskSummary
    num_shots: int


@dataclasses.dataclass
class WorkOut:
    key: Any
    sample: Optional[TaskStats]
    error: Optional[BaseException]


def worker_loop(tmp_dir: pathlib.Path,
                inp: multiprocessing.Queue,
                out: multiprocessing.Queue) -> None:
    with tempfile.TemporaryDirectory(dir=tmp_dir) as child_dir:
        while True:
            work: Optional[WorkIn] = inp.get()
            if work is None:
                return
            assert isinstance(work, WorkIn)
            try:
                stats: AnonTaskStats = work.task.sample_stats(
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
