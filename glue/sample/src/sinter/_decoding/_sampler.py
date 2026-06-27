import abc
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    import sinter


class CompiledSampler(metaclass=abc.ABCMeta):
    """A sampler that has been configured for efficiently sampling some task."""

    @abc.abstractmethod
    def sample(self, suggested_shots: int) -> 'sinter.AnonTaskStats':
        """Samples shots and returns statistics.

        Args:
            suggested_shots: The number of shots being requested. The sampler
                may perform more shots or fewer shots than this, so technically
                this argument can just be ignored. If a sampler is optimized for
                a specific batch size, it can simply return one batch per call
                regardless of this parameter.

                However, this parameter is a useful hint about the amount of
                work being done. The sampler can use this to optimize its
                behavior. For example, it could adjust its batch size downward
                if the suggested shots is very small. Whereas if the suggested
                shots is very high, the sampler should focus entirely on
                achieving the best possible throughput.

                Note that, in typical workloads, the sampler will be called
                repeatedly with the same value of suggested_shots. Therefore it
                is reasonable to allocate buffers sized to accomodate the
                current suggested_shots, expecting them to be useful again for
                the next shot.

        Returns:
            A sinter.AnonTaskStats saying how many shots were actually taken,
            how many errors were seen, etc.

            The returned stats must have at least one shot.
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
    def compiled_sampler_for_task(self, task: 'sinter.Task') -> 'sinter.CompiledSampler':
        """Creates, configures, and returns an object for sampling the task."""
        pass
