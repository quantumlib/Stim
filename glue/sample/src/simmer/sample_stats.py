import dataclasses

from simmer.case_stats import CaseStats
from simmer.case_summary import JSON_TYPE, CaseSummary
from simmer.csv_out import csv_line


@dataclasses.dataclass(frozen=True)
class SampleStats:
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

    def __add__(self, other: 'SampleStats') -> 'SampleStats':
        assert self.to_case_summary() == other.to_case_summary()
        return SampleStats(
            decoder=self.decoder,
            strong_id=self.strong_id,
            json_metadata=self.json_metadata,
            shots=self.shots + other.shots,
            errors=self.errors + other.errors,
            discards=self.discards + other.discards,
            seconds=self.seconds + other.seconds,
        )

    def to_case_summary(self) -> CaseSummary:
        return CaseSummary(
            strong_id=self.strong_id,
            decoder=self.decoder,
            json_metadata=self.json_metadata,
        )

    def to_case_stats(self) -> CaseStats:
        return CaseStats(
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
