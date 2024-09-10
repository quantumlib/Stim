import collections
import json
import pathlib
from typing import Any, Dict, List, TYPE_CHECKING

from sinter._data._task_stats import TaskStats
from sinter._data._task import Task
from sinter._data._anon_task_stats import AnonTaskStats

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
        current = self.data.get(k)
        if current is not None:
            self.data[k] = current + sample
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
        if not (expected_fields <= actual_fields):
            raise ValueError(
                f"Bad CSV data. "
                f"Got columns {sorted(actual_fields)!r} "
                f"but expected columns {sorted(expected_fields)!r}")
        has_custom_counts = 'custom_counts' in actual_fields
        result = ExistingData()
        for row in reader:
            if has_custom_counts:
                custom_counts = row['custom_counts']
                if custom_counts is None or custom_counts == '':
                    custom_counts = collections.Counter()
                else:
                    custom_counts = json.loads(custom_counts)
                    if not isinstance(custom_counts, dict) or not all(isinstance(k, str) or not isinstance(v, int) for k, v in custom_counts.items()):
                        raise ValueError(f"{row['custom_counts']=} isn't empty or a dictionary from string keys to integer values.")
                    custom_counts = collections.Counter(custom_counts)
            else:
                custom_counts = collections.Counter()
            result.add_sample(TaskStats(
                shots=int(row['shots']),
                discards=int(row['discards']),
                errors=int(row['errors']),
                custom_counts=custom_counts,
                seconds=float(row['seconds']),
                strong_id=row['strong_id'],
                decoder=row['decoder'],
                json_metadata=json.loads(row['json_metadata']),
            ))
        return result


def stats_from_csv_files(*paths_or_files: Any) -> List['sinter.TaskStats']:
    """Reads and aggregates shot statistics from CSV files.

    (An old alias of `read_stats_from_csv_files`, kept around for backwards
    compatibility.)

    Assumes the CSV file was written by printing `sinter.CSV_HEADER` and then
    a list of `sinter.TaskStats`. When statistics from the same task appear
    in multiple files (identified by the strong id being the same), the
    statistics for that task are folded together (so only the total shots,
    total errors, etc for each task are included in the results).

    Args:
        *paths_or_files: Each argument should be either a path (in the form of
            a string or a pathlib.Path) or a TextIO object (e.g. as returned by
            `open`). File data is read from each argument.

    Returns:
        A list of task stats, where each task appears only once in the list and
        the stats associated with it are the totals aggregated from all files.

    Examples:
        >>> import sinter
        >>> import io
        >>> in_memory_file = io.StringIO()
        >>> _ = in_memory_file.write('''
        ...     shots,errors,discards,seconds,decoder,strong_id,json_metadata
        ...     1000,42,0,0.125,pymatching,9c31908e2b,"{""d"":9}"
        ...     3000,24,0,0.125,pymatching,9c31908e2b,"{""d"":9}"
        ...     1000,250,0,0.125,pymatching,deadbeef08,"{""d"":7}"
        ... '''.strip())
        >>> _ = in_memory_file.seek(0)
        >>> stats = sinter.stats_from_csv_files(in_memory_file)
        >>> for stat in stats:
        ...     print(repr(stat))
        sinter.TaskStats(strong_id='9c31908e2b', decoder='pymatching', json_metadata={'d': 9}, shots=4000, errors=66, seconds=0.25)
        sinter.TaskStats(strong_id='deadbeef08', decoder='pymatching', json_metadata={'d': 7}, shots=1000, errors=250, seconds=0.125)
    """
    result = ExistingData()
    for p in paths_or_files:
        result += ExistingData.from_file(p)
    return list(result.data.values())


def read_stats_from_csv_files(*paths_or_files: Any) -> List['sinter.TaskStats']:
    """Reads and aggregates shot statistics from CSV files.

    Assumes the CSV file was written by printing `sinter.CSV_HEADER` and then
    a list of `sinter.TaskStats`. When statistics from the same task appear
    in multiple files (identified by the strong id being the same), the
    statistics for that task are folded together (so only the total shots,
    total errors, etc for each task are included in the results).

    Args:
        *paths_or_files: Each argument should be either a path (in the form of
            a string or a pathlib.Path) or a TextIO object (e.g. as returned by
            `open`). File data is read from each argument.

    Returns:
        A list of task stats, where each task appears only once in the list and
        the stats associated with it are the totals aggregated from all files.

    Examples:
        >>> import sinter
        >>> import io
        >>> in_memory_file = io.StringIO()
        >>> _ = in_memory_file.write('''
        ...     shots,errors,discards,seconds,decoder,strong_id,json_metadata
        ...     1000,42,0,0.125,pymatching,9c31908e2b,"{""d"":9}"
        ...     3000,24,0,0.125,pymatching,9c31908e2b,"{""d"":9}"
        ...     1000,250,0,0.125,pymatching,deadbeef08,"{""d"":7}"
        ... '''.strip())
        >>> _ = in_memory_file.seek(0)
        >>> stats = sinter.read_stats_from_csv_files(in_memory_file)
        >>> for stat in stats:
        ...     print(repr(stat))
        sinter.TaskStats(strong_id='9c31908e2b', decoder='pymatching', json_metadata={'d': 9}, shots=4000, errors=66, seconds=0.25)
        sinter.TaskStats(strong_id='deadbeef08', decoder='pymatching', json_metadata={'d': 7}, shots=1000, errors=250, seconds=0.125)
    """
    result = ExistingData()
    for p in paths_or_files:
        result += ExistingData.from_file(p)
    return list(result.data.values())
