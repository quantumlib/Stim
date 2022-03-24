import dataclasses
import multiprocessing
import pathlib
import tempfile
from typing import Any
from typing import Optional

from simmer.sample_stats import SampleStats
from simmer.case_executable import CaseExecutable
from simmer.case_stats import CaseStats
from simmer.case_summary import CaseSummary


@dataclasses.dataclass
class WorkIn:
    key: Any
    case: CaseExecutable
    summary: CaseSummary
    num_shots: int


@dataclasses.dataclass
class WorkOut:
    key: Any
    sample: Optional[SampleStats]
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
                out.put(WorkOut(key=work.key, sample=None, error=ex))
                continue

            sample = SampleStats(
                decoder=work.case.decoder,
                json_metadata=work.summary.json_metadata,
                strong_id=work.summary.strong_id,
                shots=stats.shots,
                discards=stats.discards,
                errors=stats.errors,
                seconds=stats.seconds,
            )
            out.put(WorkOut(sample=sample, key=work.key, error=None))
