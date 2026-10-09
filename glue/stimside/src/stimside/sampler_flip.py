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
from stimside.simulator_flip import FlipsideSimulator


def _reject_loss_oracle(needs_leakage_events: bool) -> None:
    if needs_leakage_events:
        raise ValueError(
            "FlipsideSampler cannot run a dem_decoder that needs the shots' leakage events "
            "(e.g. MarginalLeakageDemGenerator(loss_oracle=True)): FlipsideSimulator does "
            "not record them. Use TablesideSampler or CosetsideSampler."
        )


class FlipsideSampler(_CompiledSeedMixin, sinter.Sampler):

    def __init__(
        self,
        op_handler: OpHandler,
        dem_decoder: LeakageDecoderLike,
        batch_size: int = 2**10,
        seed: int | None = None,
    ):
        dem_decoder = as_leakage_decoder(dem_decoder, "FlipsideSampler")
        _reject_loss_oracle(dem_decoder.needs_leakage_events)
        self.op_handler = op_handler
        self.dem_decoder = dem_decoder
        self.batch_size = batch_size
        self.seed = seed
        self._init_pid = os.getpid()
        self._compile_count = 0

    def compiled_sampler_for_task(self, task: sinter.Task) -> sinter.CompiledSampler:
        if task.circuit is None:
            raise ValueError(
                "FlipsideSampler requires a circuit in the task to compile a sampler."
            )
        compiled_op_handler = self.op_handler.compile_op_handler(
            circuit=task.circuit, batch_size=self.batch_size
        )
        seed = self._next_compiled_seed()
        return CompiledFlipsideSampler(
            circuit=task.circuit,
            compiled_op_handler=compiled_op_handler,
            batch_size=self.batch_size,
            compiled_dem_decoder=self.dem_decoder.compile_for_task(task),
            needs_records=self.dem_decoder.needs_records,
            needs_leakage_events=self.dem_decoder.needs_leakage_events,
            seed=seed,
        )


class CompiledFlipsideSampler(sinter.CompiledSampler):
    def __init__(
        self,
        circuit: stim.Circuit,
        compiled_op_handler: CompiledOpHandler,
        batch_size: int,
        compiled_dem_decoder: CompiledLeakageDecoder,
        *,
        needs_records: bool,
        needs_leakage_events: bool = False,
        seed: int | None = None,
    ):
        _reject_loss_oracle(needs_leakage_events)
        self.circuit = circuit
        self.batch_size = batch_size
        self.compiled_dem_decoder = compiled_dem_decoder
        self.compiled_op_handler = compiled_op_handler
        self.needs_records = needs_records
        self.simulator = FlipsideSimulator(
            circuit=circuit,
            batch_size=batch_size,
            compiled_op_handler=compiled_op_handler,
            seed=seed,
        )

    def sample(self, suggested_shots: int) -> sinter.AnonTaskStats:
        start_time = time.process_time()

        shots_taken = 0
        num_errors = 0
        while shots_taken < suggested_shots:
            self.simulator.clear()
            self.simulator.run()

            det_events_bit_packed = self.simulator.get_detector_flips(bit_packed=True)
            actual_obs_flips = self.simulator.get_observable_flips(bit_packed=True)

            decoded_obs_flips = self.compiled_dem_decoder.decode_shots_bit_packed(
                bit_packed_detection_event_data=det_events_bit_packed,
                records=(
                    self.simulator.get_final_measurement_records()
                    if self.needs_records
                    else None
                ),
            )

            # count a shot as an error if any of the observables was predicted wrong
            num_errors += int(
                np.count_nonzero(
                    np.any(decoded_obs_flips != actual_obs_flips, axis=-1)
                )
            )
            shots_taken += self.simulator.batch_size

        end_time = time.process_time()
        seconds = end_time - start_time

        return sinter.AnonTaskStats(
            shots=shots_taken, errors=num_errors, discards=0, seconds=seconds
        )
