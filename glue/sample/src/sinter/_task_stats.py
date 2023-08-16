import collections
import dataclasses
from typing import Counter, List, Any

from sinter._anon_task_stats import AnonTaskStats
from sinter._csv_out import csv_line


@dataclasses.dataclass(frozen=True)
class TaskStats:
    """Statistics sampled from a task.

    The rows in the CSV files produced by sinter correspond to instances of
    `sinter.TaskStats`. For example, a row can be produced by printing a
    `sinter.TaskStats`.

    Attributes:
        strong_id: The cryptographically unique identifier of the task, from
            `sinter.Task.strong_id()`.
        decoder: The name of the decoder that was used to decode the task.
            Errors are counted when this decoder made a wrong prediction.
        json_metadata: A JSON-encodable value (such as a dictionary from strings
            to integers) that were included with the task in order to describe
            what the task was. This value can be a huge variety of things, but
            typically it will be a dictionary with fields such as 'd' for the
            code distance.
        shots: Number of times the task was sampled.
        errors: Number of times a sample resulted in an error.
        discards: Number of times a sample resulted in a discard. Note that
            discarded a task is not an error.
        seconds: The amount of CPU core time spent sampling the tasks, in
            seconds.
        custom_counts: A counter mapping string keys to integer values. Used for
            tracking arbitrary values, such as per-observable error counts or
            the number of times detectors fired. The meaning of the information
            in the counts is not specified; the only requirement is that it
            should be correct to add each key's counts when merging statistics.

            Although this field is an editable object, it's invalid to edit the
            counter after the stats object is initialized.
    """

    # Information describing the problem that was sampled.
    strong_id: str
    decoder: str
    json_metadata: Any

    # Information describing the results of sampling.
    shots: int = 0
    errors: int = 0
    discards: int = 0
    seconds: float = 0
    custom_counts: Counter[str] = dataclasses.field(default_factory=collections.Counter)

    def __post_init__(self):
        assert isinstance(self.errors, int)
        assert isinstance(self.shots, int)
        assert isinstance(self.discards, int)
        assert isinstance(self.seconds, (int, float))
        assert isinstance(self.custom_counts, collections.Counter)
        assert isinstance(self.decoder, str)
        assert isinstance(self.strong_id, str)
        assert self.json_metadata is None or isinstance(self.json_metadata, (int, float, str, dict, list, tuple))
        assert self.errors >= 0
        assert self.discards >= 0
        assert self.seconds >= 0
        assert self.shots >= self.errors + self.discards
        assert all(isinstance(k, str) and isinstance(v, int) for k, v in self.custom_counts.items())

    def __add__(self, other: 'TaskStats') -> 'TaskStats':
        if self.strong_id != other.strong_id:
            raise ValueError(f'{self.strong_id=} != {other.strong_id=}')
        total = self.to_anon_stats() + other.to_anon_stats()

        return TaskStats(
            decoder=self.decoder,
            strong_id=self.strong_id,
            json_metadata=self.json_metadata,
            shots=total.shots,
            errors=total.errors,
            discards=total.discards,
            seconds=total.seconds,
            custom_counts=total.custom_counts,
        )

    def to_anon_stats(self) -> AnonTaskStats:
        """Returns a `sinter.AnonTaskStats` with the same statistics.

        Examples:
            >>> import sinter
            >>> stat = sinter.TaskStats(
            ...     strong_id='test',
            ...     json_metadata={'a': [1, 2, 3]},
            ...     decoder='pymatching',
            ...     shots=22,
            ...     errors=3,
            ...     discards=4,
            ...     seconds=5,
            ... )
            >>> stat.to_anon_stats()
            sinter.AnonTaskStats(shots=22, errors=3, discards=4, seconds=5)
        """
        return AnonTaskStats(
            shots=self.shots,
            errors=self.errors,
            discards=self.discards,
            seconds=self.seconds,
            custom_counts=self.custom_counts.copy(),
        )

    def to_csv_line(self) -> str:
        """Converts into a line that can be printed into a CSV file.

        Examples:
            >>> import sinter
            >>> stat = sinter.TaskStats(
            ...     strong_id='test',
            ...     json_metadata={'a': [1, 2, 3]},
            ...     decoder='pymatching',
            ...     shots=22,
            ...     errors=3,
            ...     seconds=5,
            ... )
            >>> print(sinter.CSV_HEADER)
                 shots,    errors,  discards, seconds,decoder,strong_id,json_metadata,custom_counts
            >>> print(stat.to_csv_line())
                    22,         3,         0,       5,pymatching,test,"{""a"":[1,2,3]}",
        """
        return csv_line(
            shots=self.shots,
            errors=self.errors,
            seconds=self.seconds,
            discards=self.discards,
            strong_id=self.strong_id,
            decoder=self.decoder,
            json_metadata=self.json_metadata,
            custom_counts=self.custom_counts,
        )

    def _split_custom_counts(self, custom_keys: List[str]) -> List['TaskStats']:
        result = []
        for k in custom_keys:
            m = self.json_metadata
            if isinstance(m, dict):
                m = dict(m)
                m.setdefault('custom_error_count_key', k)
                m.setdefault('original_error_count', self.errors)
            result.append(TaskStats(
                strong_id=f'{self.strong_id}:{k}',
                decoder=self.decoder,
                json_metadata=m,
                shots=self.shots,
                errors=self.custom_counts[k],
                discards=self.discards,
                seconds=self.seconds,
                custom_counts=self.custom_counts,
            ))
        return result

    def __str__(self) -> str:
        return self.to_csv_line()

    def __repr__(self) -> str:
        terms = []
        terms.append(f'strong_id={self.strong_id!r}')
        terms.append(f'decoder={self.decoder!r}')
        terms.append(f'json_metadata={self.json_metadata!r}')
        if self.shots:
            terms.append(f'shots={self.shots!r}')
        if self.errors:
            terms.append(f'errors={self.errors!r}')
        if self.discards:
            terms.append(f'discards={self.discards!r}')
        if self.seconds:
            terms.append(f'seconds={self.seconds!r}')
        if self.custom_counts:
            terms.append(f'custom_counts={self.custom_counts!r}')
        return f'sinter.TaskStats({", ".join(terms)})'
