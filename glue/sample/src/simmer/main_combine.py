import io
import json
from typing import Any, Union, List, Optional
from typing import Dict
from typing import Tuple

import pandas as pd

from simmer import CaseExecutable
from simmer import CaseSummary
from simmer.case_summary import JSON_TYPE
from simmer.decoding import CaseStats
import csv


def escape_csv(text: Any, width: Optional[int]) -> str:
    output = io.StringIO()
    csv.writer(output).writerow([text])
    text = output.getvalue().strip()
    if width is not None:
        text = text.rjust(width)
    return text


def csv_line(*,
             shots: Any,
             errors: Any,
             discards: Any,
             seconds: Any,
             decoder: Any,
             strong_id: Any,
             custom_json: JSON_TYPE,
             is_header: bool = False) -> str:
    if isinstance(seconds, float):
        if seconds < 1:
            seconds = f'{seconds:0.3f}'
        elif seconds < 10:
            seconds = f'{seconds:0.2f}'
        else:
            seconds = f'{seconds:0.1f}'
    if not is_header:
        custom_json = json.dumps(custom_json,
                                 separators=(',', ':'),
                                 sort_keys=True)

    shots = escape_csv(shots, 10)
    errors = escape_csv(errors, 10)
    discards = escape_csv(discards, 10)
    seconds = escape_csv(seconds, 8)
    decoder = escape_csv(decoder, None)
    strong_id = escape_csv(strong_id, None)
    custom_json = escape_csv(custom_json, None)
    return (f'{shots},'
            f'{errors},'
            f'{discards},'
            f'{seconds},'
            f'{decoder},'
            f'{strong_id},'
            f'{custom_json}')


def csv_line_ex(problem: CaseSummary, stats: CaseStats) -> str:
    return csv_line(
        shots=stats.shots,
        errors=stats.errors,
        seconds=stats.seconds,
        discards=stats.discards,
        strong_id=problem.strong_id,
        decoder=problem.decoder,
        custom_json=problem.custom)


CSV_HEADER = csv_line(shots='shots',
                      errors='errors',
                      discards='discards',
                      seconds='seconds',
                      strong_id='strong_id',
                      decoder='decoder',
                      custom_json='custom_json',
                      is_header=True)


class ExistingData:
    def __init__(self):
        self.data: Dict[str, Tuple[CaseSummary, CaseStats]] = {}

    def stats_for(self, case: Union[CaseExecutable, CaseSummary]) -> CaseStats:
        if isinstance(case, CaseExecutable):
            key = case.strong_id
        elif isinstance(case, CaseSummary):
            key = case.strong_id
        else:
            raise NotImplementedError(f'{type(case)}')
        if key not in self.data:
            return CaseStats()
        return self.data[key][1]

    def __iadd__(self, other: 'ExistingData') -> 'ExistingData':
        for k, v in other.data.items():
            if k not in self.data:
                self.data[k] = v
            else:
                v2 = self.data[k]
                assert v2[0] == v[0]
                self.data[k] = (v[0], v[1] + v2[1])
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
                                   "custom_json"])
        actual_columns = sorted(frame.columns)
        if actual_columns != expected_columns:
            raise ValueError(
                    f"Bad CSV Data. Expected columns {expected_columns!r} "
                    f"but got {actual_columns!r}.")
        id_to_stats = (frame.
                       groupby(['strong_id', 'decoder', 'custom_json'], sort=False).
                       sum().
                       to_dict(orient='index'))
        result = ExistingData()
        for (strong_id, decoder, custom_json), row in id_to_stats.items():
            summary = CaseSummary(
                strong_id=strong_id,
                decoder=decoder,
                custom=json.loads(custom_json),
            )
            stats = CaseStats(shots=row['shots'],
                              discards=row['discards'],
                              errors=row['errors'],
                              seconds=row['seconds'])
            assert strong_id not in result.data
            result.data[strong_id] = (summary, stats)
        return result


def main_combine(*, command_line_args: List[str]):
    total = ExistingData()
    for path in command_line_args:
        total += ExistingData.from_file(path)

    print(CSV_HEADER)
    for key in total.data.keys():
        summary, stats = total.data[key]
        print(csv_line_ex(problem=summary, stats=stats))
