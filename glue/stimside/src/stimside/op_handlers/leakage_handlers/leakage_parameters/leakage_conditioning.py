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

    def __eq__(self, other):
        if not isinstance(other, LeakageConditioningParams):
            return False
        if len(self.args) != len(other.args):
            return False
        if self.from_tag != other.from_tag:
            return False
        if self.targets != other.targets:
            return False
        for a, b in zip(self.args, other.args):
            if set(a) != set(b):
                return False
        return True

    def _validate(self):
        if type(self.args) == tuple[tuple[int | str, ...]]:
            for arg in self.args[0]:
                if isinstance(arg, str):
                    if arg != "U":
                        raise ValueError(
                            f"{self.name} state must be 'U' or integers, got {arg}"
                        )
                elif isinstance(arg, int):
                    if arg < 0 or arg > 9:
                        raise ValueError(
                            f"{self.name} state integers must be between 0 and 9, got {arg}"
                        )
                else:
                    raise ValueError(
                        f"{self.name} args must be a tuple of integers or strings, got {arg}"
                    )
        elif type(self.args) == tuple[tuple[int | str, ...], tuple[int | str, ...]]:
            for arg in self.args[0]:
                if isinstance(arg, str):
                    if arg != "U":
                        raise ValueError(
                            f"{self.name} state must be 'U' or integers, got {arg}"
                        )
                elif isinstance(arg, int):
                    if arg < 0 or arg > 9:
                        raise ValueError(
                            f"{self.name} state integers must be between 0 and 9, got {arg}"
                        )
                else:
                    raise ValueError(
                        f"{self.name} args must be a tuple of integers or strings, got {arg}"
                    )
            for arg in self.args[1]:
                if isinstance(arg, str):
                    if arg != "U":
                        raise ValueError(
                            f"{self.name} state must be 'U' or integers, got {arg}"
                        )
                elif isinstance(arg, int):
                    if arg < 0 or arg > 9:
                        raise ValueError(
                            f"{self.name} state integers must be between 0 and 9, got {arg}"
                        )
                else:
                    raise ValueError(
                        f"{self.name} args must be a tuple of integers or strings, got {arg}"
                    )
        else:
            raise ValueError(
                f"{self.name} args must be a tuple of one or two tuples, got {self.args}"
            )
