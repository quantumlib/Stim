import sinter
import stim # type: ignore[import-untyped]

from stimside.op_handlers.abstract_op_handler import _TrivialOpHandler
from stimside.sampler_tableau import TablesideSampler


def test_sampler():

    op_handler = _TrivialOpHandler()

    sampler = TablesideSampler(op_handler=op_handler)

    circuit = stim.Circuit(
        """
        R 0 1
        H 0 1
        M 0 1
    """
    )

    task = sinter.Task(circuit=circuit, decoder=sinter.BUILT_IN_DECODERS["pymatching"])

    compiled_sampler = sampler.compiled_sampler_for_task(task=task)
