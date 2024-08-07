import collections
import dataclasses
from typing import Counter, List, Any
from typing import Optional
from typing import Union
from typing import overload

from sinter._data._anon_task_stats import AnonTaskStats
from sinter._data._csv_out import csv_line


def _is_equal_json_values(json1: Any, json2: Any):
    if json1 == json2:
        return True

    if type(json1) == type(json2):
        if isinstance(json1, dict):
            return json1.keys() == json2.keys() and all(_is_equal_json_values(json1[k], json2[k]) for k in json1.keys())
        elif isinstance(json1, (list, tuple)):
            return len(json1) == len(json2) and all(_is_equal_json_values(a, b) for a, b in zip(json1, json2))
    elif isinstance(json1, (list, tuple)) and isinstance(json2, (list, tuple)):
        return _is_equal_json_values(tuple(json1), tuple(json2))

    return False


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

    def with_edits(
        self,
        *,
        strong_id: Optional[str] = None,
        decoder: Optional[str] = None,
        json_metadata: Optional[Any] = None,
        shots: Optional[int] = None,
        errors: Optional[int] = None,
        discards: Optional[int] = None,
        seconds: Optional[float] = None,
        custom_counts: Optional[Counter[str]] = None,
    ) -> 'TaskStats':
        return TaskStats(
            strong_id=self.strong_id if strong_id is None else strong_id,
            decoder=self.decoder if decoder is None else decoder,
            json_metadata=self.json_metadata if json_metadata is None else json_metadata,
            shots=self.shots if shots is None else shots,
            errors=self.errors if errors is None else errors,
            discards=self.discards if discards is None else discards,
            seconds=self.seconds if seconds is None else seconds,
            custom_counts=self.custom_counts if custom_counts is None else custom_counts,
        )

    @overload
    def __add__(self, other: AnonTaskStats) -> AnonTaskStats:
        pass
    @overload
    def __add__(self, other: 'TaskStats') -> 'TaskStats':
        pass
    def __add__(self, other: Union[AnonTaskStats, 'TaskStats']) -> Union[AnonTaskStats, 'TaskStats']:
        if isinstance(other, AnonTaskStats):
            return self.to_anon_stats() + other

        if isinstance(other, TaskStats):
            if self.strong_id != other.strong_id:
                raise ValueError(f'{self.strong_id=} != {other.strong_id=}')
            if not _is_equal_json_values(self.json_metadata, other.json_metadata) or self.decoder != other.decoder:
                raise ValueError(
                    "A stat had the same strong id as another, but their other identifying information (json_metadata, decoder) differed.\n"
                    "The strong id is supposed to be a cryptographic hash that uniquely identifies what was sampled, so this is an error.\n"
                    "\n"
                    "This failure can occur when post-processing data (e.g. combining X basis stats and Z basis stats into synthetic both-basis stats).\n"
                    "To fix it, ensure any post-processing sets the strong id of the synthetic data in some cryptographically secure way.\n"
                    "\n"
                    "In some cases this can be caused by attempting to add a value that has gone through JSON serialization+parsing to one\n"
                    "that hasn't, which causes things like tuples transforming into lists.\n"
                    "\n"
                    f"The two stats:\n1. {self!r}\n2. {other!r}")

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

        return NotImplemented
    __radd__ = __add__

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
