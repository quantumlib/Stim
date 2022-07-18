import dataclasses

from sinter._anon_task_stats import AnonTaskStats
from sinter._json_type import JSON_TYPE
from sinter._csv_out import csv_line


@dataclasses.dataclass(frozen=True)
class TaskStats:
    """Results of sampling from a decoding problem."""

    # Information describing the problem that was sampled.
    strong_id: str
    decoder: str
    json_metadata: JSON_TYPE

    # Information describing the results of sampling.
    shots: int
    errors: int
    discards: int
    seconds: float

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
        )

    def to_anon_stats(self) -> AnonTaskStats:
        return AnonTaskStats(
            shots=self.shots,
            errors=self.errors,
            discards=self.discards,
            seconds=self.seconds,
        )

    def to_csv_line(self) -> str:
        return csv_line(
            shots=self.shots,
            errors=self.errors,
            seconds=self.seconds,
            discards=self.discards,
            strong_id=self.strong_id,
            decoder=self.decoder,
            json_metadata=self.json_metadata)

    def __str__(self):
        return self.to_csv_line()
