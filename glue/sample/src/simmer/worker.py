import dataclasses
import multiprocessing
import pathlib
import tempfile
from typing import Any
from typing import Optional

from simmer.case_executable import CaseExecutable
from simmer.case_stats import CaseStats


@dataclasses.dataclass
class WorkIn:
    key: Any
    case: CaseExecutable
    num_shots: int


@dataclasses.dataclass
class WorkOut:
    input: WorkIn
    stats: Optional[CaseStats]
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
                stats: CaseStats = work.case.sample_stats(
                    num_shots=work.num_shots, tmp_dir=child_dir)
            except BaseException as ex:
                out.put(WorkOut(input=work, stats=None, error=ex))
                continue
            out.put(WorkOut(input=work, stats=stats, error=None))
