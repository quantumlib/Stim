from typing import Tuple
import pathlib

import stim
import numpy as np

from sinter._sampling_sampler_class import Sampler, CompiledSampler


class StimCompiledDetectorSampler(CompiledSampler):
    def __init__(self, circuit: stim.Circuit):
        self.sampler = circuit.compile_detector_sampler()

    def sample_detectors_bit_packed(
        self,
        *,
        shots: int,
    ) -> Tuple[np.ndarray, np.ndarray]:
        return self.sampler.sample(shots, separate_observables=True, bit_packed=True)


class StimDetectorSampler(Sampler):
    """Use `stim.CompiledDetectorSampler` to sample detectors from a circuit."""
    def compile_sampler_for_circuit(self, *, circuit: stim.Circuit) -> CompiledSampler:
        return StimCompiledDetectorSampler(circuit)

    def sample_detectors_via_files(
        self,
        *,
        shots: int,
        circuit_path: pathlib.Path,
        dets_b8_out_path: pathlib.Path,
        obs_flips_b8_out_path: pathlib.Path,
        tmp_dir: pathlib.Path,
    ) -> None:
        circuit = stim.Circuit.from_file(circuit_path)
        sampler = circuit.compile_detector_sampler()
        sampler.sample_write(
            shots,
            filepath=str(dets_b8_out_path),
            format="b8",
            obs_out_filepath=str(obs_flips_b8_out_path),
            obs_out_format="b8",
        )
