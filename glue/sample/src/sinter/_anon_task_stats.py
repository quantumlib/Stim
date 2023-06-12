import collections
import dataclasses
from typing import Optional


@dataclasses.dataclass(frozen=True)
class AnonTaskStats:
    """Statistics sampled from an unspecified task.

    Attributes:
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

    shots: int = 0
    errors: int = 0
    discards: int = 0
    seconds: float = 0
    classified_errors: Optional[collections.Counter] = None

    def __post_init__(self):
        assert isinstance(self.errors, int)
        assert isinstance(self.shots, int)
        assert isinstance(self.discards, int)
        assert isinstance(self.seconds, (int, float))
        assert self.errors >= 0
        assert self.discards >= 0
        assert self.seconds >= 0
        assert self.shots >= self.errors + self.discards
        if self.classified_errors is not None:
            assert sum(self.classified_errors.values()) == self.errors

    def __repr__(self) -> str:
        terms = []
        if self.shots != 0:
            terms.append(f'shots={self.shots!r}')
        if self.errors != 0:
            terms.append(f'errors={self.errors!r}')
        if self.discards != 0:
            terms.append(f'discards={self.discards!r}')
        if self.seconds != 0:
            terms.append(f'seconds={self.seconds!r}')
        if self.classified_errors is not None:
            terms.append(f'classified_errors={self.classified_errors!r}')
        return f'sinter.AnonTaskStats({", ".join(terms)})'

    def __add__(self, other: 'AnonTaskStats') -> 'AnonTaskStats':
        """Returns the sum of the statistics from both anonymous stats.

        Adds the shots, the errors, the discards, and the seconds.

        Examples:
            >>> import sinter
            >>> a = sinter.AnonTaskStats(
            ...    shots=100,
            ...    errors=20,
            ... )
            >>> b = sinter.AnonTaskStats(
            ...    shots=1000,
            ...    errors=200,
            ... )
            >>> a + b
            sinter.AnonTaskStats(shots=1100, errors=220)
        """
        if not isinstance(other, AnonTaskStats):
            return NotImplemented

        if self.classified_errors is None and other.classified_errors is None:
            combined_classified_errors = None
        else:
            combined_classified_errors = collections.Counter()
            if self.errors > 0:
                if self.classified_errors is not None:
                    combined_classified_errors += self.classified_errors
                else:
                    combined_classified_errors[""] += self.errors
            if other.errors > 0:
                if other.classified_errors is not None:
                    combined_classified_errors += other.classified_errors
                else:
                    combined_classified_errors[""] += other.errors

        return AnonTaskStats(
            shots=self.shots + other.shots,
            errors=self.errors + other.errors,
            discards=self.discards + other.discards,
            seconds=self.seconds + other.seconds,
            classified_errors=combined_classified_errors,
        )
