import time

import numpy as np
import sinter  # type: ignore[import-untyped]
import stim  # type: ignore[import-untyped]

from stimside.op_handlers.abstract_op_handler import CompiledOpHandler, OpHandler
from stimside.simulator_flip import FlipsideSimulator


class FlipsideSampler(sinter.Sampler):

    def __init__(
        self,
        op_handler: OpHandler,
        batch_size: int = 2**10,
        decoder: sinter.Decoder | None = sinter.BUILT_IN_DECODERS["pymatching"],
        seed: int | None = None,
    ):
        self.op_handler = op_handler
        self.batch_size = batch_size
        self.decoder: sinter.Decoder | None = decoder
        self.seed = seed

    def compiled_sampler_for_task(self, task: sinter.Task) -> sinter.CompiledSampler:
        if task.circuit is None:
            raise ValueError(
                "FlipsideSampler requires a circuit in the task to compile a sampler."
            )

        decoder = task.decoder or self.decoder
        if decoder is None:
            raise ValueError("FlipsideSampler requires a decoder to be specified.")
        if isinstance(decoder, str):
            decoder = sinter.BUILT_IN_DECODERS[decoder]

        dem = task.detector_error_model or task.circuit.detector_error_model()

        return CompiledFlipsideSampler(
            circuit=task.circuit,
            batch_size=self.batch_size,
            compiled_decoder=decoder.compile_decoder_for_dem(dem=dem),
            compiled_op_handler=self.op_handler.compile_op_handler(
                circuit=task.circuit, batch_size=self.batch_size
            ),
            seed=self.seed,
        )


SinterSamplerFlip = FlipsideSampler


class CompiledFlipsideSampler(sinter.CompiledSampler):
    def __init__(
        self,
        circuit: stim.Circuit,
        compiled_decoder: sinter.CompiledDecoder,
        compiled_op_handler: CompiledOpHandler,
        batch_size: int,
        seed: int | None = None,
    ):
        self.circuit = circuit
        self.simulator = FlipsideSimulator(
            circuit=circuit,
            batch_size=batch_size,
            compiled_op_handler=compiled_op_handler,
            seed=seed,
        )
        self.compiled_decoder = compiled_decoder
        self.compiled_op_handler = compiled_op_handler

    def sample(self, suggested_shots: int) -> sinter.AnonTaskStats:
        start_time = time.process_time()

        shots_taken = 0
        num_errors = 0
        while shots_taken < suggested_shots:
            self.simulator.clear()
            self.simulator.run()

            det_events_bit_packed = self.simulator.get_detector_flips(bit_packed=True)
            actual_obs_flips = self.simulator.get_observable_flips(bit_packed=True)

            decoded_obs_flips = self.compiled_decoder.decode_shots_bit_packed(
                bit_packed_detection_event_data=det_events_bit_packed
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

