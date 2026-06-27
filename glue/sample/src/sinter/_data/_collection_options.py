import dataclasses
from typing import Optional, TYPE_CHECKING

if TYPE_CHECKING:
    import sinter


@dataclasses.dataclass(frozen=True)
class CollectionOptions:
    """Describes options for how data is collected for a decoding problem.

    Attributes:
        max_shots: Defaults to None (unused). Stops the sampling process
            after this many samples have been taken from the circuit.
        max_errors: Defaults to None (unused). Stops the sampling process
            after this many errors have been seen in samples taken from the
            circuit. The actual number sampled errors may be larger due to
            batching.
        start_batch_size: Defaults to None (collector's choice). The very
            first shots taken from the circuit will use a batch of this
            size, and no other batches will be taken in parallel. Once this
            initial fact finding batch is done, batches can be taken in
            parallel and the normal batch size limiting processes take over.
        max_batch_size: Defaults to None (unused). Limits batches from
            taking more than this many shots at once. For example, this can
            be used to ensure memory usage stays below some limit.
        max_batch_seconds: Defaults to None (unused). When set, the recorded
            data from previous shots is used to estimate how much time is
            taken per shot. This information is then used to predict the
            biggest batch size that can finish in under the given number of
            seconds. Limits each batch to be no larger than that.
    """

    max_shots: Optional[int] = None
    max_errors: Optional[int] = None
    start_batch_size: Optional[int] = None
    max_batch_size: Optional[int] = None
    max_batch_seconds: Optional[float] = None

    def __post_init__(self):
        if self.max_shots is not None and self.max_shots < 0:
            raise ValueError(f'max_shots is not None and max_shots={self.max_shots} < 0')
        if self.max_errors is not None and self.max_errors < 0:
            raise ValueError(f'max_errors is not None and max_errors={self.max_errors} < 0')
        if self.start_batch_size is not None and self.start_batch_size <= 0:
            raise ValueError(f'start_batch_size is not None and start_batch_size={self.start_batch_size} <= 0')
        if self.max_batch_size is not None and self.max_batch_size <= 0:
            raise ValueError(
                f'max_batch_size={self.max_batch_size} is not None and max_batch_size <= 0')
        if self.max_batch_seconds is not None and self.max_batch_seconds <= 0:
            raise ValueError(
                f'max_batch_seconds={self.max_batch_seconds} is not None and max_batch_seconds <= 0')

    def __repr__(self) -> str:
        terms = []
        if self.max_shots is not None:
            terms.append(f'max_shots={self.max_shots!r}')
        if self.max_errors is not None:
            terms.append(f'max_errors={self.max_errors!r}')
        if self.start_batch_size is not None:
            terms.append(f'start_batch_size={self.start_batch_size!r}')
        if self.max_batch_size is not None:
            terms.append(f'max_batch_size={self.max_batch_size!r}')
        if self.max_batch_seconds is not None:
            terms.append(f'max_batch_seconds={self.max_batch_seconds!r}')
        return f'sinter.CollectionOptions({", ".join(terms)})'

    def combine(self, other: 'sinter.CollectionOptions') -> 'sinter.CollectionOptions':
        """Returns a combination of multiple collection options.

        All fields are combined by taking the minimum from both collection
        options objects, with None treated as being infinitely large.

        Args:
            other: The collections options to combine with.

        Returns:
            The combined collection options.

        Examples:
            >>> import sinter
            >>> a = sinter.CollectionOptions(
            ...    max_shots=1_000_000,
            ...    start_batch_size=100,
            ... )
            >>> b = sinter.CollectionOptions(
            ...    max_shots=100_000,
            ...    max_errors=100,
            ... )
            >>> a.combine(b)
            sinter.CollectionOptions(max_shots=100000, max_errors=100, start_batch_size=100)
        """
        return CollectionOptions(
            max_shots=nullable_min(self.max_shots, other.max_shots),
            max_errors=nullable_min(self.max_errors, other.max_errors),
            start_batch_size=nullable_min(self.start_batch_size, other.start_batch_size),
            max_batch_size=nullable_min(self.max_batch_size, other.max_batch_size),
            max_batch_seconds=nullable_min(self.max_batch_seconds, other.max_batch_seconds))


def nullable_min(a: Optional[int], b: Optional[int]) -> Optional[int]:
    if a is None:
        return b
    if b is None:
        return a
    return min(a, b)
