import dataclasses


@dataclasses.dataclass(frozen=True)
class CaseStats:
    num_shots: int = 0
    num_errors: int = 0
    num_discards: int = 0
    seconds_elapsed: float = 0

    def __post_init__(self):
        assert isinstance(self.num_errors, int)
        assert isinstance(self.num_shots, int)
        assert isinstance(self.num_discards, int)
        assert isinstance(self.seconds_elapsed, (int, float))
        assert self.num_errors >= 0
        assert self.num_discards >= 0
        assert self.seconds_elapsed >= 0
        assert self.num_shots >= self.num_errors + self.num_discards

    def __add__(self, other: 'CaseStats') -> 'CaseStats':
        if not isinstance(other, CaseStats):
            return NotImplemented
        return CaseStats(
            num_shots=self.num_shots + other.num_shots,
            num_errors=self.num_errors + other.num_errors,
            num_discards=self.num_discards + other.num_discards,
            seconds_elapsed=self.seconds_elapsed + other.seconds_elapsed,
        )
