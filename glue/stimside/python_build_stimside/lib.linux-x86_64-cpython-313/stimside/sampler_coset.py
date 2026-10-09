from __future__ import annotations

import time
from typing import Callable

import numpy as np
from numpy.typing import NDArray
import sinter  # type: ignore[import-untyped]
import stim  # type: ignore[import-untyped]

from stimside.op_handlers.abstract_op_handler import CompiledOpHandler, OpHandler
from stimside.simulator_coset import CosetsideSimulator
from stimside.util.reference_chp import CompiledReferenceCircuit


class CosetsideSampler(sinter.Sampler):
    """Sinter sampler for CosetsideSimulator with multi-shot CPU batching."""

    def __init__(
        self,
        op_handler: OpHandler[CosetsideSimulator],
        batch_size: int = 256,
        dem_gen: (
            Callable[[stim.Circuit, NDArray[np.bool_]], stim.DetectorErrorModel]
            | stim.DetectorErrorModel
            | None
        ) = None,
        decoder: sinter.Decoder = sinter.BUILT_IN_DECODERS["pymatching"],
    ) -> None:
        self.op_handler = op_handler
        self.batch_size = batch_size
        self.dem_gen = dem_gen
        self.decoder: sinter.Decoder = decoder

    def compiled_sampler_for_task(self, task: sinter.Task) -> sinter.CompiledSampler:
        if task.circuit is None:
            raise ValueError(
                "CosetsideSampler requires a circuit in the task to compile a sampler."
            )
        if self.dem_gen is None:
            dem_gen = task.detector_error_model or task.circuit.detector_error_model()
        else:
            dem_gen = self.dem_gen

        compiled_ref = CompiledReferenceCircuit(task.circuit)
        return CompiledCosetsideSampler(
            circuit=task.circuit,
            batch_size=self.batch_size,
            decoder=self.decoder,
            dem_gen=dem_gen,
            compiled_op_handler=self.op_handler.compile_op_handler(
                circuit=task.circuit,
                batch_size=self.batch_size,
            ),
            compiled_ref=compiled_ref,
        )


class CompiledCosetsideSampler(sinter.CompiledSampler):
    """Compiled sinter sampler executing B independent trajectories per batch in CosetsideSimulator."""

    def __init__(
        self,
        circuit: stim.Circuit,
        decoder: sinter.Decoder,
        dem_gen: (
            Callable[[stim.Circuit, NDArray[np.bool_]], stim.DetectorErrorModel]
            | stim.DetectorErrorModel
        ),
        compiled_op_handler: CompiledOpHandler[CosetsideSimulator],
        batch_size: int,
        compiled_ref: CompiledReferenceCircuit | None = None,
    ) -> None:
        self.circuit = circuit
        self.batch_size = batch_size
        self.dem_gen = dem_gen
        self.decoder = decoder
        self.compiled_op_handler = compiled_op_handler

        self.simulator = CosetsideSimulator(
            circuit=circuit,
            compiled_op_handler=compiled_op_handler,
            batch_size=batch_size,
            compiled_ref=compiled_ref,
        )

        if isinstance(dem_gen, stim.DetectorErrorModel):
            self.compiled_decoder = decoder.compile_decoder_for_dem(dem=dem_gen)
        else:
            self.compiled_decoder = None

    def sample(self, suggested_shots: int) -> sinter.AnonTaskStats:
        stats = sinter.AnonTaskStats()

        while stats.shots < suggested_shots:
            start_time = time.process_time()

            self.simulator.clear()
            self.simulator.run()

            det_and_obs_events = self.simulator.get_detector_flips(
                append_observables=True
            )
            det_events = det_and_obs_events[:, : self.simulator.num_detectors]
            det_events_bit_packed = np.packbits(
                det_events, axis=len(det_events.shape) - 1, bitorder="little"
            )

            obs_flips = det_and_obs_events[:, self.simulator.num_detectors :]
            actual_obs_flips = np.packbits(
                obs_flips, axis=len(obs_flips.shape) - 1, bitorder="little"
            )

            if callable(self.dem_gen):
                dem = self.dem_gen(
                    self.circuit, self.simulator.get_final_measurement_records()
                )
                compiled_decoder_local = self.decoder.compile_decoder_for_dem(
                    dem=dem
                )
            elif self.compiled_decoder is None:
                raise ValueError(
                    "No compiled decoder available. "
                    "dem_gen must be provided when initializing CosetsideSampler."
                )
            else:
                compiled_decoder_local = self.compiled_decoder

            decoded_obs_flips = compiled_decoder_local.decode_shots_bit_packed(
                bit_packed_detection_event_data=det_events_bit_packed
            )

            num_errors = int(
                np.count_nonzero(
                    np.any(decoded_obs_flips != actual_obs_flips, axis=-1)
                )
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
