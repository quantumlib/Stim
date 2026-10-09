import dataclasses
from typing import ClassVar


@dataclasses.dataclass(frozen=True)
class LeakageSwapParams:
    """SWAP[LEAKAGE_SWAP]: a SWAP that also swaps the leakage states of each target pair."""

    name: ClassVar[str] = "LEAKAGE_SWAP"
    from_tag: str
