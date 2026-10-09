import sinter
import stim # type: ignore[import-untyped]

from stimside.dem_generators.dem_decoding import decode_with_generated_dems
from stimside.dem_generators.leakage_decoder import (
    CompiledLeakageDecoder,
    BaseDecoder,
    LeakageDecoder,
    MarginalDecoder,
)
from stimside.op_handlers.abstract_op_handler import _TrivialOpHandler
from stimside.sampler_tableau import TablesideSampler


class _CallableCompiled(CompiledLeakageDecoder):
    def __init__(self, dem_gen, circuit):
        self.dem_gen = dem_gen
        self.circuit = circuit

    def decode_shots_bit_packed(
        self, *, bit_packed_detection_event_data, records=None, leakage_events=None
    ):
        return decode_with_generated_dems(
            "pymatching",
            self.dem_gen(self.circuit, records),
            bit_packed_detection_event_data,
        )


class _CallableDecoder(LeakageDecoder):
    """A custom LeakageDecoder decoding with the DEM(s) of dem_gen(circuit, records)."""

    def __init__(self, dem_gen):
        self.dem_gen = dem_gen

    @property
    def name(self):
        return "callable"

    @property
    def needs_records(self):
        return True

    def compile_for_task(self, task):
        return _CallableCompiled(self.dem_gen, task.circuit)


def test_sampler():

    op_handler = _TrivialOpHandler()

    sampler = TablesideSampler(op_handler=op_handler, dem_decoder=BaseDecoder())

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
        op_handler=op_handler, batch_size=4, dem_decoder=_CallableDecoder(custom_dem_gen)
    )
    task = sinter.Task(circuit=circuit, decoder="pymatching")
    compiled = sampler.compiled_sampler_for_task(task=task)
    stats = compiled.sample(suggested_shots=4)
    assert stats.shots == 4
    assert called_with_records == [(4, 2)]

    with pytest.raises(ValueError, match="MarginalDecoder requires a decoder"):
        MarginalDecoder(decoder=None)

