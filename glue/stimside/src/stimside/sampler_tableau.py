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
from stimside.simulator_tableau import TablesideSimulator


class _CompiledSeedMixin:
    """`seed` for the first compile in the creating process; a derived per-compile/per-process seed after."""

    seed: int | None

    def _next_compiled_seed(self) -> int | None:
        if self.seed is None:
            return None
        idx = getattr(self, "_compile_count", 0)
        init_pid = getattr(self, "_init_pid", os.getpid())
        cur_pid = os.getpid()
        self._compile_count = idx + 1
        if idx == 0 and cur_pid == init_pid:
            return int(self.seed)
        pid_delta = (cur_pid - init_pid) & 0xFFFFFFFF
        ss = np.random.SeedSequence([int(self.seed), int(idx), int(pid_delta)])
        return int(ss.generate_state(1, dtype=np.uint32)[0] & 0x3FFFFFFF)


class TablesideSampler(_CompiledSeedMixin, sinter.Sampler):

    def __init__(
        self,
        op_handler: OpHandler,
        dem_decoder: LeakageDecoderLike,
        batch_size: int = 1,
        seed: int | None = None,
    ):
        dem_decoder = as_leakage_decoder(dem_decoder, "TablesideSampler")
        self.op_handler = op_handler
        self.dem_decoder = dem_decoder
        self.batch_size = batch_size
        self.seed = seed
        self._init_pid = os.getpid()
        self._compile_count = 0

    def compiled_sampler_for_task(self, task: sinter.Task) -> sinter.CompiledSampler:
        if task.circuit is None:
            raise ValueError(
                "TablesideSampler requires a circuit in the task to compile a sampler."
            )
        compiled_op_handler = self.op_handler.compile_op_handler(
            circuit=task.circuit,
            batch_size=self.batch_size,
        )
        seed = self._next_compiled_seed()
        return CompiledTablesideSampler(
            circuit=task.circuit,
            compiled_op_handler=compiled_op_handler,
            batch_size=self.batch_size,
            compiled_dem_decoder=self.dem_decoder.compile_for_task(task),
            needs_records=self.dem_decoder.needs_records,
            needs_leakage_events=self.dem_decoder.needs_leakage_events,
            seed=seed,
        )


class CompiledTablesideSampler(sinter.CompiledSampler):
    def __init__(
        self,
        circuit: stim.Circuit,
        compiled_op_handler: CompiledOpHandler,
        batch_size: int,
        compiled_dem_decoder: CompiledLeakageDecoder,
        *,
        needs_records: bool,
        needs_leakage_events: bool,
        seed: int | None = None,
    ):
        self.circuit = circuit
        self.tab_simulator = TablesideSimulator(
            circuit=circuit,
            compiled_op_handler=compiled_op_handler,
            batch_size=batch_size,
            seed=seed,
            record_leakage_events=needs_leakage_events,
        )

        self.batch_size = batch_size
        self.compiled_dem_decoder = compiled_dem_decoder
        self.compiled_op_handler = compiled_op_handler
        self.needs_records = needs_records
        self.needs_leakage_events = needs_leakage_events

    def sample(self, suggested_shots: int) -> sinter.AnonTaskStats:
        stats = sinter.AnonTaskStats()

        while stats.shots < suggested_shots:
            start_time = time.process_time()

            self.tab_simulator.clear()
            self.tab_simulator.run()

            # With batch_size > 1, detection events are sampled on their own unless
            # the measurement records were sampled first; the dem_decoder needs the
            # records of the very shots that get decoded.
            records = (
                self.tab_simulator.get_final_measurement_records()
                if self.needs_records
                else None
            )
            det_and_obs_events = self.tab_simulator.get_detector_flips(
                append_observables=True
            )

            det_events = det_and_obs_events[:, : self.tab_simulator.num_detectors]
            det_events_bit_packed = np.packbits(
                det_events, axis=len(det_events.shape) - 1, bitorder="little"
            )

            obs_flips = det_and_obs_events[:, self.tab_simulator.num_detectors :]
            actual_obs_flips = np.packbits(
                obs_flips, axis=len(obs_flips.shape) - 1, bitorder="little"
            )

            decoded_obs_flips = self.compiled_dem_decoder.decode_shots_bit_packed(
                bit_packed_detection_event_data=det_events_bit_packed,
                records=records,
                leakage_events=(
                    self.tab_simulator.get_leakage_events()
                    if self.needs_leakage_events
                    else None
                ),
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
