import time
from typing import Callable

import numpy as np
import sinter  # type: ignore[import-untyped]
import stim  # type: ignore[import-untyped]

from numpy.typing import NDArray

from stimside.op_handlers.abstract_op_handler import CompiledOpHandler, OpHandler
from stimside.simulator_tableau import TablesideSimulator


class TablesideSampler(sinter.Sampler):

    def __init__(
        self,
        op_handler: OpHandler,
        batch_size: int = 1,
        dem_gen: (
            Callable[[stim.Circuit, NDArray[np.bool_]], stim.DetectorErrorModel]
            | stim.DetectorErrorModel
            | None
        ) = None,
        decoder: sinter.Decoder = sinter.BUILT_IN_DECODERS["pymatching"],
    ):
        self.op_handler = op_handler
        self.decoder: sinter.Decoder = decoder
        self.batch_size = batch_size
        self.dem_gen = dem_gen

    def compiled_sampler_for_task(self, task: sinter.Task) -> sinter.CompiledSampler:
        if task.circuit is None:
            raise ValueError(
                "FlipsideSampler requires a circuit in the task to compile a sampler."
            )
        if self.dem_gen is None:
            dem_gen = task.detector_error_model or task.circuit.detector_error_model()
        else:
            dem_gen = self.dem_gen

        return CompiledTablesideSampler(
            circuit=task.circuit,
            batch_size=self.batch_size,
            decoder=self.decoder,
            dem_gen=dem_gen,
            compiled_op_handler=self.op_handler.compile_op_handler(
                circuit=task.circuit,
                batch_size=1,
                # The op_handler only handles 1 shot,
                # But the simulator generates multiple shots with the modified circuit on demand
            ),
        )


class CompiledTablesideSampler(sinter.CompiledSampler):
    def __init__(
        self,
        circuit: stim.Circuit,
        decoder: sinter.Decoder,
        dem_gen: (
            Callable[[stim.Circuit, NDArray[np.bool_]], stim.DetectorErrorModel]
            | stim.DetectorErrorModel
        ),
        compiled_op_handler: CompiledOpHandler,
        batch_size: int,
    ):
        self.circuit = circuit
        self.tab_simulator = TablesideSimulator(
            circuit=circuit,
            compiled_op_handler=compiled_op_handler,
            batch_size=batch_size,
        )

        self.batch_size = batch_size
        self.dem_gen = dem_gen
        self.decoder = decoder
        self.compiled_op_handler = compiled_op_handler

        if isinstance(dem_gen, stim.DetectorErrorModel):
            self.compiled_decoder = decoder.compile_decoder_for_dem(dem=dem_gen)
        else:
            self.compiled_decoder = None

    def sample(self, suggested_shots: int) -> sinter.AnonTaskStats:
        stats = sinter.AnonTaskStats()

        while stats.shots < suggested_shots:
            # TODO: be nicer to sinter regarding batch_size vs shots
            # if the number of shots requested is wildly different from the batch_size
            # maybe do something more sensible than ignore or massively overdo it
            start_time = time.process_time()

            self.tab_simulator.clear()
            self.tab_simulator.run()

            det_and_obs_events = self.tab_simulator.get_detector_flips(
                append_observables=True
            )

            det_events = det_and_obs_events[:, : self.tab_simulator.num_detectors]
            # shape is (batch_len=1, detector_idxs)

            det_events_bit_packed = np.packbits(
                det_events, axis=len(det_events.shape) - 1, bitorder="little"
            )

            obs_flips = det_and_obs_events[:, self.tab_simulator.num_detectors :]
            # notice the transpose, shape is (batch_len, detector_idxs)
            actual_obs_flips = np.packbits(
                obs_flips, axis=len(obs_flips.shape) - 1, bitorder="little"
            )

            if callable(self.dem_gen):
                dem = self.dem_gen(
                    self.circuit, self.tab_simulator.final_measurement_records
                )
                compiled_decoder_local = self.decoder.compile_decoder_for_dem(dem=dem)
            elif self.compiled_decoder is None:
                raise ValueError(
                    "No compiled decoder available."
                    "dem_gen must be provided as a callable when initializing TablesideSampler."
                )
            else:
                compiled_decoder_local = self.compiled_decoder
            decoded_obs_flips = compiled_decoder_local.decode_shots_bit_packed(
                bit_packed_detection_event_data=det_events_bit_packed
            )

            # count a shot as an error if any of the observables was predicted wrong
            num_errors = np.count_nonzero(
                np.any(decoded_obs_flips != actual_obs_flips, axis=-1)
            )

            end_time = time.process_time()
            cpu_seconds = end_time - start_time

            stats += sinter.AnonTaskStats(
                shots=self.batch_size,
                errors=num_errors,
                discards=0,
                seconds=cpu_seconds,
            )
        return stats
