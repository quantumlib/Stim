import dataclasses
from typing import ClassVar


@dataclasses.dataclass(frozen=True)
class LeakageConditioningParams:

    name: ClassVar[str] = "LEAKAGE_CONDITIONING"
    args: (
        tuple[tuple[int | str, ...]]
        | tuple[tuple[int | str, ...], tuple[int | str, ...]]
    )
    targets: tuple[int, ...] | None
    from_tag: str

    def __post_init__(self):
        self._validate()

    def __eq__(self, other):
        if not isinstance(other, LeakageConditioningParams):
            return False
        if len(self.args) != len(other.args):
            return False
        if self.from_tag != other.from_tag:
            return False
        if self.targets != other.targets:
            return False
        if len(self.args) == 2:
            return set(zip(self.args[0], self.args[1])) == set(
                zip(other.args[0], other.args[1])
            )
        for a, b in zip(self.args, other.args):
            if set(a) != set(b):
                return False
        return True

    def _validate(self):
        if not isinstance(self.args, tuple) or not (
            (len(self.args) == 1 and isinstance(self.args[0], tuple))
            or (
                len(self.args) == 2
                and isinstance(self.args[0], tuple)
                and isinstance(self.args[1], tuple)
                and len(self.args[0]) == len(self.args[1])
            )
        ):
            raise ValueError(
                f"{self.name} args must be a tuple of one or two tuples, got {self.args}"
            )
        for group in self.args:
            for arg in group:
                if isinstance(arg, str):
                    if arg != "U":
                        raise ValueError(
                            f"{self.name} state must be 'U' or integers, got {arg}"
                        )
                elif isinstance(arg, int) and not isinstance(arg, bool):
                    if arg < 0 or arg > 9:
                        raise ValueError(
                            f"{self.name} state integers must be between 0 and 9, got {arg}"
                        )
                else:
                    raise ValueError(
                        f"{self.name} args must be a tuple of integers or strings, got {arg}"
                    )
