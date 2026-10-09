import sinter
import stim # type: ignore[import-untyped]

from stimside.op_handlers.abstract_op_handler import _TrivialOpHandler
from stimside.sampler_flip import FlipsideSampler


def test_sampler():

    op_handler = _TrivialOpHandler()

    sampler = FlipsideSampler(op_handler=op_handler)

    circuit = stim.Circuit(
        """
        R 0 1
        H 0 1
        M 0 1
    """
    )

    task = sinter.Task(circuit=circuit, decoder=sinter.BUILT_IN_DECODERS["pymatching"])

    compiled_sampler = sampler.compiled_sampler_for_task(task=task)

    sampler_with_seed = FlipsideSampler(op_handler=op_handler, decoder=None, seed=42)
    task_str_decoder = sinter.Task(
        circuit=stim.Circuit(
            """
            R 0
            X_ERROR(0.1) 0
            M 0
            DETECTOR rec[-1]
            OBSERVABLE_INCLUDE(0) rec[-1]
        """
        ),
        decoder="pymatching",
    )
    compiled_with_seed = sampler_with_seed.compiled_sampler_for_task(task=task_str_decoder)
    assert compiled_with_seed.simulator.seed == 42
    stats = compiled_with_seed.sample(suggested_shots=128)
    assert stats.shots >= 128
    assert isinstance(stats.errors, int)

