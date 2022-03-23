import multiprocessing
import pathlib

from simmer.case import Case
from simmer.case_stats import CaseStats


def worker_loop(tmp_dir: pathlib.Path,
                inp: multiprocessing.Queue,
                out: multiprocessing.Queue) -> None:
    while True:
        command = inp.get()
        if command == "end":
            return
        assert isinstance(command, tuple)
        key, case = command
        assert isinstance(key, int)
        assert isinstance(case, Case)
        try:
            stats: CaseStats = case.run(tmp_dir=tmp_dir)
        except BaseException as ex:
            out.put(("error", ex))
            continue
        out.put(("result", key, case, stats))
