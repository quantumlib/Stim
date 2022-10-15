import json
import pathlib
from typing import Any, Dict, List, TYPE_CHECKING

from sinter._task_stats import TaskStats
from sinter._task import Task
from sinter._decoding import AnonTaskStats

if TYPE_CHECKING:
    import sinter


class ExistingData:
    def __init__(self):
        self.data: Dict[str, TaskStats] = {}

    def stats_for(self, case: Task) -> AnonTaskStats:
        if isinstance(case, Task):
            key = case.strong_id()
        else:
            raise NotImplementedError(f'{type(case)}')
        if key not in self.data:
            return AnonTaskStats()
        return self.data[key].to_anon_stats()

    def add_sample(self, sample: TaskStats) -> None:
        k = sample.strong_id
        if k in self.data:
            self.data[k] += sample
        else:
            self.data[k] = sample

    def __iadd__(self, other: 'ExistingData') -> 'ExistingData':
        for sample in other.data.values():
            self.add_sample(sample)
        return self

    @staticmethod
    def from_file(path_or_file: Any) -> 'ExistingData':
        expected_fields = {
            "shots",
            "discards",
            "errors",
            "seconds",
            "strong_id",
            "decoder",
            "json_metadata",
        }
        # Import is done locally to reduce cost of importing sinter.
        import csv
        if isinstance(path_or_file, (str, pathlib.Path)):
            with open(path_or_file) as csvfile:
                return ExistingData.from_file(csvfile)
        reader = csv.DictReader(path_or_file)
        reader.fieldnames = [e.strip() for e in reader.fieldnames]
        actual_fields = set(reader.fieldnames)
        if actual_fields != expected_fields:
            raise ValueError(
                f"Bad CSV data. "
                f"Got columns {sorted(actual_fields)!r} "
                f"but expected columns {sorted(expected_fields)!r}")
        result = ExistingData()
        for row in reader:
            result.add_sample(TaskStats(
                shots=int(row['shots']),
                discards=int(row['discards']),
                errors=int(row['errors']),
                seconds=float(row['seconds']),
                strong_id=row['strong_id'],
                decoder=row['decoder'],
                json_metadata=json.loads(row['json_metadata']),
            ))
        return result


def stats_from_csv_files(*paths_or_files: Any) -> List['sinter.TaskStats']:
    result = ExistingData()
    for p in paths_or_files:
        result += ExistingData.from_file(p)
    return list(result.data.values())
