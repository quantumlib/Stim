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


def test_sampler_sample_and_dem_gen():
    import pytest

    op_handler = _TrivialOpHandler()
    circuit = stim.Circuit(
        """
        R 0 1
        X_ERROR(0.1) 0
        M 0 1
        DETECTOR rec[-2]
        OBSERVABLE_INCLUDE(0) rec[-1]
    """
    )

    called_with_records = []

    def custom_dem_gen(circ: stim.Circuit, records):
        called_with_records.append(records.shape)
        return circ.detector_error_model()

    sampler = TablesideSampler(
        op_handler=op_handler, batch_size=4, dem_gen=custom_dem_gen, decoder=None
    )
    task = sinter.Task(circuit=circuit, decoder="pymatching")
    compiled = sampler.compiled_sampler_for_task(task=task)
    stats = compiled.sample(suggested_shots=4)
    assert stats.shots == 4
    assert called_with_records == [(2,)]

    with pytest.raises(ValueError, match="TablesideSampler requires a decoder"):
        sampler.compiled_sampler_for_task(sinter.Task(circuit=circuit, decoder=None))

