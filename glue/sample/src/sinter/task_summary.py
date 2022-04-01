import dataclasses
from typing import Union, Dict, List


JSON_TYPE = Union[Dict[str, 'JSON_TYPE'], List['JSON_TYPE'], str, int, float]


@dataclasses.dataclass(frozen=True)
class TaskSummary:
    """A serializable representation of a decoding problem."""
    strong_id: str
    decoder: str
    json_metadata: JSON_TYPE

    def __hash__(self) -> int:
        return self.strong_id.__hash__()