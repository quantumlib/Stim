import abc
import dataclasses
from typing import Union, TypeVar, TYPE_CHECKING, Generic

import stim  # type: ignore[import-untyped]

if TYPE_CHECKING:
    from stimside.simulator_flip import FlipsideSimulator
    from stimside.simulator_tableau import TablesideSimulator

T = TypeVar("T")


@dataclasses.dataclass
class OpHandler(abc.ABC, Generic[T]):
    """An object defining an extra behaviours in a stim circuit.

    A OpHandler can be given to a sampler to describe how to handle tags.
    Typically, one OpHandlers will handle several related behaviours,
    but OpHandlers can also be combined and composed.

    A OpHandler does not actually implement the asked for behaviour itself. Instead, it
    constructs a CompiledOpHandler on demand. This mirrors Sinter's Sampler / CompiledSampler
    architecture, and allows a CompiledSampler to ensure it has a CompiledOpHandler that it
    owns and controls, which can hold state specific to that CompiledSampler instance.
    """

    def compile_op_handler(
        self, *, circuit: stim.Circuit, batch_size: int
    ) -> "CompiledOpHandler[T]":
        """Compile a noise implementation of noise for a specific circuit.

        The returned CompiledOpHandler should be prepared to handle the operations in the
        circuit as fast as possible, so should front load any slow tasks like parsing tags.
        """
        raise NotImplementedError


class CompiledOpHandler(abc.ABC, Generic[T]):
    """An object for applying extra behaviour to a running simulator.

    In general, the CompiledOpHandler has immense freedom.
    It can store its own private state or history (e.g. for implementing non-Markovian noise),
    and it has access to the entire simulator to produce whatever effects it likes.

    When the CompiledOpHandler is built, it has access to the actual circuit it will run on.
    This means you can and should pre-compile any behaviour you want to be faster,
    for example parsing tags, extracting parameters, etc.

    A simulator demands the following interfaces from a CompiledOpHandler:
        handle_op: to perform behaviour related to each circuit op
        clear: to prepare for a new simulation
    """

    @abc.abstractmethod
    def handle_op(self, op: stim.CircuitInstruction, sss: T) -> None:
        """Given an operation, manipulate the given simulator to perform that operation."""
        raise NotImplementedError

    @abc.abstractmethod
    def clear(self) -> None:
        """clear any stored state, ready for a new simulation."""
        raise NotImplementedError


class _TrivialOpHandler(OpHandler[Union["FlipsideSimulator", "TablesideSimulator"]]):
    def compile_op_handler(
        self, *, circuit: stim.Circuit, batch_size: int
    ) -> "CompiledOpHandler[Union['FlipsideSimulator', 'TablesideSimulator']]":
        return _TrivialCompiledOpHandler()


class _TrivialCompiledOpHandler(
    CompiledOpHandler[Union["FlipsideSimulator", "TablesideSimulator"]]
):

    def handle_op(
        self,
        op: stim.CircuitInstruction,
        sss: Union["FlipsideSimulator", "TablesideSimulator"],
    ) -> None:
        """Given an operation, manipulate the given simulator to perform that operation."""
        return

    def clear(self) -> None:
        """clear any stored state, ready for a new simulation."""
        return
