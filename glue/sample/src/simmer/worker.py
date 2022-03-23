import dataclasses
import multiprocessing
from typing import Optional, Any

from simmer.case import CaseStats, Case


@dataclasses.dataclass
class WorkIn:
    key: Any
    case: Case
    num_shots: int


@dataclasses.dataclass
class WorkOut:
    input: WorkIn
    stats: Optional[CaseStats]
    error: Optional[BaseException]


def worker_loop(inp: multiprocessing.Queue, out: multiprocessing.Queue) -> None:
    while True:
        work = inp.get()
        if work is None:
            return
        assert isinstance(work, WorkIn)
        try:
            stats = work.case.sample_stats(num_shots=work.num_shots)
        except Exception as ex:
            out.put(WorkOut(input=work, stats=None, error=ex))
            continue
        out.put(WorkOut(input=work, stats=stats, error=None))
