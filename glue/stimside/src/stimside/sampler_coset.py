from __future__ import annotations

import os
import time

import numpy as np
import sinter  # type: ignore[import-untyped]
import stim  # type: ignore[import-untyped]

from stimside.dem_generators.leakage_decoder import (
    CompiledLeakageDecoder,
    LeakageDecoderLike,
    as_leakage_decoder,
)
from stimside.op_handlers.abstract_op_handler import CompiledOpHandler, OpHandler
from stimside.sampler_tableau import _CompiledSeedMixin
from stimside.simulator_coset import CosetsideSimulator
from stimside.util.reference_chp import CompiledReferenceCircuit


class CosetsideSampler(_CompiledSeedMixin, sinter.Sampler):
    """Sinter sampler for CosetsideSimulator with multi-shot CPU batching."""

    def __init__(
        self,
        op_handler: OpHandler[CosetsideSimulator],
        dem_decoder: LeakageDecoderLike,
        batch_size: int = 256,
        seed: int | None = None,
    ) -> None:
        dem_decoder = as_leakage_decoder(dem_decoder, "CosetsideSampler")
        self.op_handler = op_handler
        self.dem_decoder = dem_decoder
        self.batch_size = batch_size
        self.seed = seed
        self._init_pid = os.getpid()
        self._compile_count = 0

    def compiled_sampler_for_task(self, task: sinter.Task) -> sinter.CompiledSampler:
        if task.circuit is None:
            raise ValueError(
                "CosetsideSampler requires a circuit in the task to compile a sampler."
            )
        compiled_ref = CompiledReferenceCircuit(task.circuit)
        compiled_op_handler = self.op_handler.compile_op_handler(
            circuit=task.circuit,
            batch_size=self.batch_size,
        )
        seed = self._next_compiled_seed()
        return CompiledCosetsideSampler(
            circuit=task.circuit,
            compiled_op_handler=compiled_op_handler,
            batch_size=self.batch_size,
            compiled_dem_decoder=self.dem_decoder.compile_for_task(task),
            needs_records=self.dem_decoder.needs_records,
            needs_leakage_events=self.dem_decoder.needs_leakage_events,
            compiled_ref=compiled_ref,
            seed=seed,
        )


class CompiledCosetsideSampler(sinter.CompiledSampler):
    """Compiled sinter sampler executing B independent trajectories per batch in CosetsideSimulator."""

    def __init__(
        self,
        circuit: stim.Circuit,
        compiled_op_handler: CompiledOpHandler[CosetsideSimulator],
        batch_size: int,
        compiled_dem_decoder: CompiledLeakageDecoder,
        *,
        needs_records: bool,
        needs_leakage_events: bool,
        compiled_ref: CompiledReferenceCircuit | None = None,
        seed: int | None = None,
    ) -> None:
        self.circuit = circuit
        self.batch_size = batch_size
        self.compiled_dem_decoder = compiled_dem_decoder
        self.compiled_op_handler = compiled_op_handler
        self.needs_records = needs_records
        self.needs_leakage_events = needs_leakage_events

        self.simulator = CosetsideSimulator(
            circuit=circuit,
            compiled_op_handler=compiled_op_handler,
            batch_size=batch_size,
            compiled_ref=compiled_ref,
            seed=seed,
            record_leakage_events=needs_leakage_events,
        )

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

            decoded_obs_flips = self.compiled_dem_decoder.decode_shots_bit_packed(
                bit_packed_detection_event_data=det_events_bit_packed,
                records=(
                    self.simulator.get_final_measurement_records()
                    if self.needs_records
                    else None
                ),
                leakage_events=(
                    self.simulator.get_leakage_events()
                    if self.needs_leakage_events
                    else None
                ),
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
