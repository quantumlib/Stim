from typing import Tuple
import dataclasses

from sinter.anon_task_stats import AnonTaskStats
from sinter.probability_util import binomial_relative_likelihood_range
from sinter.task_summary import JSON_TYPE, TaskSummary
from sinter.csv_out import csv_line


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
        assert self.to_task_summary() == other.to_task_summary()
        return TaskStats(
            decoder=self.decoder,
            strong_id=self.strong_id,
            json_metadata=self.json_metadata,
            shots=self.shots + other.shots,
            errors=self.errors + other.errors,
            discards=self.discards + other.discards,
            seconds=self.seconds + other.seconds,
        )

    def to_task_summary(self) -> TaskSummary:
        return TaskSummary(
            strong_id=self.strong_id,
            decoder=self.decoder,
            json_metadata=self.json_metadata,
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
