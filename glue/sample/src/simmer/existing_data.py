import json
from typing import Any, Dict, Union

import pandas as pd

from simmer.sample_stats import SampleStats
from simmer.case_executable import CaseExecutable
from simmer.case_summary import CaseSummary
from simmer.decoding import CaseStats


class ExistingData:
    def __init__(self):
        self.data: Dict[str, SampleStats] = {}

    def stats_for(self, case: Union[CaseExecutable, CaseSummary]) -> CaseStats:
        if isinstance(case, CaseExecutable):
            key = case.to_strong_id()
        elif isinstance(case, CaseSummary):
            key = case.strong_id
        else:
            raise NotImplementedError(f'{type(case)}')
        if key not in self.data:
            return CaseStats()
        return self.data[key].to_case_stats()

    def add_sample(self, sample: SampleStats) -> None:
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
            result.data[strong_id] = SampleStats(
                strong_id=strong_id,
                decoder=decoder,
                json_metadata=json.loads(custom_json),
                shots=row['shots'],
                discards=row['discards'],
                errors=row['errors'],
                seconds=row['seconds'],
            )
        return result
