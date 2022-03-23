import dataclasses

from simmer.case_executable import CaseExecutable
from simmer.case_stats import CaseStats


@dataclasses.dataclass(frozen=True)
class CaseGoal:
    """A decoding problem and a description of how many samples are wanted."""
    case: CaseExecutable
    max_shots: int
    max_errors: int
    previous_stats: CaseStats = CaseStats()
