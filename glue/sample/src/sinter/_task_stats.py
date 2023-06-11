import collections
import dataclasses
from typing import List
from typing import Optional

from sinter._anon_task_stats import AnonTaskStats
from sinter._json_type import JSON_TYPE
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
        classified_errors: Defaults to None. When data is collecting using
            `--split_errors`, this counter has keys corresponding to observed
            symptoms and values corresponding to how often those errors
            occurred. The total of all the values is equal to the total number
            of errors. For example, the key 'E_E' means observable 0 and
            observable 2 flipped.
    """

    # Information describing the problem that was sampled.
    strong_id: str
    decoder: str
    json_metadata: JSON_TYPE

    # Information describing the results of sampling.
    shots: int
    errors: int
    discards: int
    seconds: float
    classified_errors: Optional[collections.Counter] = None

    def __add__(self, other: 'TaskStats') -> 'TaskStats':
        assert self.strong_id == other.strong_id
        return TaskStats(
            decoder=self.decoder,
            strong_id=self.strong_id,
            json_metadata=self.json_metadata,
            shots=self.shots + other.shots,
            errors=self.errors + other.errors,
            discards=self.discards + other.discards,
            seconds=self.seconds + other.seconds,
            classified_errors=None if self.classified_errors is None or other.classified_errors is None else self.classified_errors + other.classified_errors,
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
            classified_errors=self.classified_errors,
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
            ...     discards=4,
            ...     seconds=5,
            ... )
            >>> print(sinter.CSV_HEADER)
                 shots,    errors,  discards, seconds,decoder,strong_id,json_metadata
            >>> print(stat.to_csv_line())
                    22,         3,         4,       5,pymatching,test,"{""a"":[1,2,3]}"
        """
        return csv_line(
            shots=self.shots,
            errors={str(k): v for k, v in self.classified_errors.items()} if self.classified_errors is not None else self.errors,
            seconds=self.seconds,
            discards=self.discards,
            strong_id=self.strong_id,
            decoder=self.decoder,
            json_metadata=self.json_metadata,
        )

    def _split_errors(self) -> List['TaskStats']:
        if self.classified_errors is None:
            return [self]
        result = []
        for k, v in self.classified_errors.items():
            result.append(TaskStats(
                strong_id=f'{self.strong_id}:{k}',
                decoder=self.decoder,
                json_metadata=self.json_metadata,
                shots=self.shots,
                errors=v,
                discards=self.discards,
                seconds=self.seconds,
                classified_errors=collections.Counter({k: v}),
            ))
        return result

    def __str__(self):
        return self.to_csv_line()

    def __repr__(self) -> str:
        terms = []
        terms.append(f'strong_id={self.strong_id!r}')
        terms.append(f'decoder={self.decoder!r}')
        terms.append(f'json_metadata={self.json_metadata!r}')
        terms.append(f'shots={self.shots!r}')
        terms.append(f'errors={self.errors!r}')
        terms.append(f'discards={self.discards!r}')
        terms.append(f'seconds={self.seconds!r}')
        return f'sinter.TaskStats({", ".join(terms)})'
