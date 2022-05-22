import json
from typing import Any, Dict, Union, List, TYPE_CHECKING

import pandas as pd

from sinter.task_stats import TaskStats
from sinter.executable_task import ExecutableTask
from sinter.task_summary import TaskSummary
from sinter.decoding import AnonTaskStats

if TYPE_CHECKING:
    import sinter


class ExistingData:
    def __init__(self):
        self.data: Dict[str, TaskStats] = {}

    def stats_for(self, case: Union[ExecutableTask, TaskSummary]) -> AnonTaskStats:
        if isinstance(case, ExecutableTask):
            key = case.strong_id()
        elif isinstance(case, TaskSummary):
            key = case.strong_id
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
