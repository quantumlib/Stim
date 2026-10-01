from __future__ import annotations

import os
import time

import numpy as np
import sinter  # type: ignore[import-untyped]
import stim  # type: ignore[import-untyped]

from stimside.op_handlers.abstract_op_handler import CompiledOpHandler, OpHandler
from stimside.sampler_tableau import (
    _CompiledSeedMixin,
    DemGenLike,
)
from stimside.simulator_coset import CosetsideSimulator
from stimside.util.reference_chp import CompiledReferenceCircuit


class CosetsideSampler(_CompiledSeedMixin, sinter.Sampler):
    """Sinter sampler for CosetsideSimulator with multi-shot CPU batching."""

    def __init__(
        self,
        op_handler: OpHandler[CosetsideSimulator],
        batch_size: int = 256,
        dem_gen: DemGenLike | None = None,
        decoder: sinter.Decoder | None = sinter.BUILT_IN_DECODERS["pymatching"],
        seed: int | None = None,
        decompose_errors: bool = False,
        reweight_only: bool = False,
    ) -> None:
        self.op_handler = op_handler
        self.batch_size = batch_size
        self.dem_gen = dem_gen
        self.decoder: sinter.Decoder | None = decoder
        self.seed = seed
        self.decompose_errors = decompose_errors
        self._init_pid = os.getpid()
        self._compile_count = 0

    def compiled_sampler_for_task(self, task: sinter.Task) -> sinter.CompiledSampler:
        if task.circuit is None:
            raise ValueError(
                "CosetsideSampler requires a circuit in the task to compile a sampler."
            )

        decoder = task.decoder or self.decoder
        if decoder is None:
            raise ValueError("CosetsideSampler requires a decoder to be specified.")
        if isinstance(decoder, str):
            decoder = sinter.BUILT_IN_DECODERS[decoder]

        if self.dem_gen is None:
            dem_gen = task.detector_error_model or task.circuit.detector_error_model(
                decompose_errors=self.decompose_errors
            )
        else:
            dem_gen = self.dem_gen

        compiled_ref = CompiledReferenceCircuit(task.circuit)
        return CompiledCosetsideSampler(
            circuit=task.circuit,
            batch_size=self.batch_size,
            decoder=decoder,
            dem_gen=dem_gen,
            compiled_op_handler=self.op_handler.compile_op_handler(
                circuit=task.circuit,
                batch_size=self.batch_size,
            ),
            compiled_ref=compiled_ref,
            seed=self._next_compiled_seed(),
        )


class CompiledCosetsideSampler(sinter.CompiledSampler):
    """Compiled sinter sampler executing B independent trajectories per batch in CosetsideSimulator."""

    def __init__(
        self,
        circuit: stim.Circuit,
        decoder: sinter.Decoder,
        dem_gen: DemGenLike,
        compiled_op_handler: CompiledOpHandler[CosetsideSimulator],
        batch_size: int,
        compiled_ref: CompiledReferenceCircuit | None = None,
        seed: int | None = None,
    ) -> None:
        self.circuit = circuit
        self.batch_size = batch_size
        self.dem_gen = dem_gen
        self.decoder = decoder
        self.compiled_op_handler = compiled_op_handler

        self.coset_simulator = CosetsideSimulator(
            circuit=circuit,
            compiled_op_handler=compiled_op_handler,
            batch_size=batch_size,
            compiled_ref=compiled_ref,
            seed=seed,
        )

        if isinstance(dem_gen, stim.DetectorErrorModel):
            self.compiled_decoder = decoder.compile_decoder_for_dem(dem=dem_gen)
        else:
            self.compiled_decoder = None

    def sample(self, suggested_shots: int) -> sinter.AnonTaskStats:
        stats = sinter.AnonTaskStats()

        while stats.shots < suggested_shots:
            start_time = time.process_time()

            self.coset_simulator.clear()
            self.coset_simulator.run()

            det_and_obs_events = self.coset_simulator.get_detector_flips(
                append_observables=True
            )
            det_events = det_and_obs_events[:, : self.coset_simulator.num_detectors]
            det_events_bit_packed = np.packbits(
                det_events, axis=len(det_events.shape) - 1, bitorder="little"
            )

            obs_flips = det_and_obs_events[:, self.coset_simulator.num_detectors :]
            actual_obs_flips = np.packbits(
                obs_flips, axis=len(obs_flips.shape) - 1, bitorder="little"
            )

            decoded_obs_flips = np.array([])
            if callable(self.dem_gen):
                records = self.coset_simulator.get_final_measurement_records()
                assert records is not None       
                for n in range(len(records)):
                    dem = self.dem_gen(
                        self.circuit,
                        records[n],
                    )
                    compiled_decoder = self.decoder.compile_decoder_for_dem(dem=dem)
                    decoded_obs_flips = np.append(
                        decoded_obs_flips,
                        self.compiled_decoder.decode_shots_bit_packed(
                            bit_packed_detection_event_data=np.array([det_events_bit_packed[n]])
                        ),
                        axis = 0,
                    )
            elif self.compiled_decoder is None:
                raise ValueError(
                    "No compiled decoder available. "
                    "dem_gen must be provided when initializing CosetsideSampler."
                )
            else:
                decoded_obs_flips = self.compiled_decoder.decode_shots_bit_packed(
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
