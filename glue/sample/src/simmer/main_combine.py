import collections
from typing import Any, Tuple, DefaultDict, List, Optional

import pandas as pd

from simmer.decoding import CaseStats


def escape_csv(text: Any, width: Optional[int]) -> str:
    text = str(text)
    if ',' in text or '"' in text:
        text = '"' + text + text.replace('"', '\\"') + '"'
    if width is not None:
        text = text.rjust(width)
    return text


def csv_line(*, shots: Any, errors: Any, discards: Any, elapsed: Any, strong_id: Any, name: Any) -> str:
    if isinstance(elapsed, float):
        elapsed = f'{elapsed:0.3f}'
    shots = escape_csv(shots, 10)
    errors = escape_csv(errors, 10)
    discards = escape_csv(discards, 10)
    elapsed = escape_csv(elapsed, 16)
    strong_id = escape_csv(strong_id, 256 // 4 + 1)
    name = escape_csv(name, None)
    return f'{shots},{errors},{discards},{elapsed},{strong_id}, {name}'


CSV_HEADER = csv_line(shots='shots',
                      errors='errors',
                      discards='discards',
                      elapsed='seconds_elapsed',
                      strong_id='strong_id',
                      name='name')


class ExistingData:
    def __init__(self):
        self.data: DefaultDict[Tuple[str, str], CaseStats] = collections.defaultdict(CaseStats)

    def stats_for(self, *, circuit_name: str, circuit_sha256: str) -> CaseStats:
        return self.data[(circuit_name, circuit_sha256)]

    def __iadd__(self, other: 'ExistingData') -> 'ExistingData':
        for k, v in other.data.items():
            self.data[k] += v
        return self

    @staticmethod
    def from_file(path_or_file: Any) -> 'ExistingData':
        frame = pd.read_csv(path_or_file, skipinitialspace=True)
        expected_columns = sorted(["shots", "discards", "errors", "seconds_elapsed", "strong_id", "name"])
        actual_columns = sorted(frame.columns)
        if actual_columns != expected_columns:
            raise ValueError(f"Bad CSV Data. Expected columns {expected_columns!r} but got {actual_columns!r}.")
        grouped = frame.groupby(['name', 'strong_id']).sum()
        result = ExistingData()
        for (name, sha), row in grouped.to_dict(orient='index').items():
            assert isinstance(name, str)
            assert isinstance(sha, str)
            stats = CaseStats(num_shots=row['shots'],
                              num_discards=row['discards'],
                              num_errors=row['errors'],
                              seconds_elapsed=row['seconds_elapsed'])
            result.data[(name, sha)] = stats
        return result


def main_combine(*, command_line_args: List[str]):
    total = ExistingData()
    for path in command_line_args:
        total += ExistingData.from_file(path)

    print(CSV_HEADER)
    for name, strong_id in sorted(total.data.keys()):
        stats: CaseStats = total.data[(name, strong_id)]
        print(csv_line(
            shots=stats.num_shots,
            errors=stats.num_errors,
            discards=stats.num_discards,
            elapsed=stats.seconds_elapsed,
            strong_id=strong_id,
            name=name,
        ))
