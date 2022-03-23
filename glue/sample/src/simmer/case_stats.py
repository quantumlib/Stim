import dataclasses


@dataclasses.dataclass(frozen=True)
class CaseStats:
    """Results of sampling from a decoding problem."""

    shots: int = 0
    errors: int = 0
    discards: int = 0
    seconds: float = 0

    def __post_init__(self):
        assert isinstance(self.errors, int)
        assert isinstance(self.shots, int)
        assert isinstance(self.discards, int)
        assert isinstance(self.seconds, (int, float))
        assert self.errors >= 0
        assert self.discards >= 0
        assert self.seconds >= 0
        assert self.shots >= self.errors + self.discards

    def __add__(self, other: 'CaseStats') -> 'CaseStats':
        if not isinstance(other, CaseStats):
            return NotImplemented
        return CaseStats(
            shots=self.shots + other.shots,
            errors=self.errors + other.errors,
            discards=self.discards + other.discards,
            seconds=self.seconds + other.seconds,
        )
