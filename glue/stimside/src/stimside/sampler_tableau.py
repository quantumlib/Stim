from __future__ import annotations

import math
import os
import time
from typing import Any, Callable, Sequence, TypeAlias
import warnings

import numpy as np
from numpy.typing import NDArray
import sinter  # type: ignore[import-untyped]
import stim  # type: ignore[import-untyped]

from stimside.op_handlers.abstract_op_handler import CompiledOpHandler, OpHandler
from stimside.simulator_tableau import TablesideSimulator

# `dem_gen` accepted by the Tableside/Flipside/Cosetside samplers.
DemGenLike: TypeAlias = (
    Callable[
        [stim.Circuit, NDArray[np.bool_]],
        stim.DetectorErrorModel
        | Sequence[stim.DetectorErrorModel]
        | NDArray[np.float64]
        | Sequence[NDArray[np.float64]],
    ]
    | stim.DetectorErrorModel
)

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
        batch_size: int = 1,
        dem_gen: DemGenLike | None = None,
        decoder: sinter.Decoder | None = sinter.BUILT_IN_DECODERS["pymatching"],
        seed: int | None = None,
        decompose_errors: bool = False,
    ):
        self.op_handler = op_handler
        self.decoder: sinter.Decoder | None = decoder
        self.batch_size = batch_size
        self.dem_gen = dem_gen
        self.seed = seed
        self.decompose_errors = decompose_errors
        self._init_pid = os.getpid()
        self._compile_count = 0

    def compiled_sampler_for_task(self, task: sinter.Task) -> sinter.CompiledSampler:
        if task.circuit is None:
            raise ValueError(
                "TablesideSampler requires a circuit in the task to compile a sampler."
            )
        decoder = task.decoder or self.decoder
        if decoder is None:
            raise ValueError("TablesideSampler requires a decoder to be specified.")
        if isinstance(decoder, str):
            decoder = sinter.BUILT_IN_DECODERS[decoder]

        if self.dem_gen is None:
            dem_gen = task.detector_error_model or task.circuit.detector_error_model(
                decompose_errors=self.decompose_errors
            )
        else:
            dem_gen = self.dem_gen

        return CompiledTablesideSampler(
            circuit=task.circuit,
            batch_size=self.batch_size,
            decoder=decoder,
            dem_gen=dem_gen,
            compiled_op_handler=self.op_handler.compile_op_handler(
                circuit=task.circuit,
                batch_size=self.batch_size,
            ),
            seed=self._next_compiled_seed(),
        )


class CompiledTablesideSampler(sinter.CompiledSampler):
    def __init__(
        self,
        circuit: stim.Circuit,
        decoder: sinter.Decoder,
        dem_gen: DemGenLike,
        compiled_op_handler: CompiledOpHandler,
        batch_size: int,
        seed: int | None = None,
    ):
        self.circuit = circuit
        self.batch_size = batch_size
        self.decoder = decoder
        self.dem_gen = dem_gen
        self.compiled_op_handler = compiled_op_handler
        self.tab_simulator = TablesideSimulator(
            circuit=circuit,
            compiled_op_handler=compiled_op_handler,
            batch_size=batch_size,
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

            self.tab_simulator.clear()
            self.tab_simulator.run()

            # With batch_size > 1, detection events are sampled on their own unless
            # the measurement records were sampled first; dem_gen needs the records
            # of the very shots that get decoded.
            records = (
                self.tab_simulator.get_final_measurement_records()
                if callable(self.dem_gen)
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

            if callable(self.dem_gen):
                assert records is not None
                dem = self.dem_gen(
                    self.circuit,
                    records[0],
                )
                self.compiled_decoder = self.decoder.compile_decoder_for_dem(dem=dem)
                decoded_obs_flips = self.compiled_decoder.decode_shots_bit_packed(
                    bit_packed_detection_event_data=det_events_bit_packed
                )
            elif self.compiled_decoder is None:
                raise ValueError(
                    "No compiled decoder available."
                    "dem_gen must be provided as a callable when initializing TablesideSampler."
                )
            else:
                decoded_obs_flips = self.compiled_decoder.decode_shots_bit_packed(
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
