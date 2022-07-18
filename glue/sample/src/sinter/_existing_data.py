import json
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
        # Import is done locally to reduce cost of importing sinter.
        import pandas as pd
        frame = pd.read_csv(path_or_file, skipinitialspace=True)
        expected_columns = sorted(["shots",
                                   "discards",
                                   "errors",
                                   "seconds",
                                   "strong_id",
                                   "decoder",
                                   "json_metadata"])
        actual_columns = sorted(frame.columns)
        if actual_columns != expected_columns:
            raise ValueError(
                    f"Bad CSV Data. Expected columns {expected_columns!r} "
                    f"but got {actual_columns!r}.")
        id_to_stats = (frame.
                       groupby(['strong_id', 'decoder', 'json_metadata'], sort=False).
                       sum().
                       to_dict(orient='index'))
        result = ExistingData()
        for (strong_id, decoder, custom_json), row in id_to_stats.items():
            assert strong_id not in result.data
            result.data[strong_id] = TaskStats(
                strong_id=strong_id,
                decoder=decoder,
                json_metadata=json.loads(custom_json),
                shots=row['shots'],
                discards=row['discards'],
                errors=row['errors'],
                seconds=row['seconds'],
            )
        return result


def stats_from_csv_files(*paths_or_files: Any) -> List['sinter.TaskStats']:
    result = ExistingData()
    for p in paths_or_files:
        result += ExistingData.from_file(p)
    return list(result.data.values())
