import abc

from sinter._data import AnonTaskStats, Task


class CompiledSampler(metaclass=abc.ABCMeta):
    """A sampler that has been configured for efficiently sampling some task."""

    @abc.abstractmethod
    def sample(self, shots: int) -> AnonTaskStats:
        """Perform the given number of samples, and return statistics.

        This method is permitted to perform fewer shots than specified, but must
        indicate this in its returned statistics.
        """
        pass

    def handles_throttling(self) -> bool:
        """Return True to disable sinter wrapping samplers with throttling.

        By default, sinter will wrap samplers so that they initially only do
        a small number of shots then slowly ramp up. Sometimes this behavior
        is not desired (e.g. in unit tests). Override this method to return True
        to disable it.
        """
        return False


class Sampler(metaclass=abc.ABCMeta):
    """A strategy for producing stats from tasks.

    Call `sampler.compiled_sampler_for_task(task)` to get a compiled sampler for
    a task, then call `compiled_sampler.sample(shots)` to collect statistics.

    A sampler differs from a `sinter.Decoder` because the sampler is responsible
    for the full sampling process (e.g. simulating the circuit), whereas a
    decoder can do nothing except predict observable flips from detection event
    data. This prevents the decoders from cheating, but makes them less flexible
    overall. A sampler can do things like use simulators other than stim, or
    really anything at all as long as it ends with returning statistics about
    shot counts, error counts, and etc.
    """

    @abc.abstractmethod
    def compiled_sampler_for_task(self, task: Task) -> CompiledSampler:
        """Creates, configures, and returns an object for sampling the task."""
        pass
