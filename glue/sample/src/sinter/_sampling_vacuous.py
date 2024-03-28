from typing import Tuple
import pathlib

import stim
import numpy as np

from sinter._sampling_sampler_class import Sampler, CompiledSampler


class VacuousCompiledSampler(CompiledSampler):
    def __init__(self, detectors_shape: int, obs_shape: int):
        self.detectors_shape = detectors_shape
        self.obs_shape = obs_shape

    def sample_detectors_bit_packed(
        self,
        *,
        shots: int,
    ) -> Tuple[np.ndarray, np.ndarray]:
        return np.zeros(shape=(shots, self.detectors_shape), dtype=np.uint8), np.zeros(
            shape=(shots, self.obs_shape), dtype=np.uint8
        )


class VacuousSampler(Sampler):
    """An example sampler that always sample zero-valued detectors and zero-valued observables."""
    def compile_sampler_for_circuit(self, *, circuit: stim.Circuit) -> CompiledSampler:
        return VacuousCompiledSampler(
            (circuit.num_detectors + 7) // 8, (circuit.num_observables + 7) // 8
        )

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
        num_detectors = circuit.num_detectors
        num_obs = circuit.num_observables
        with open(dets_b8_out_path, "wb") as f:
            f.write(b"\0" * (num_detectors * shots))
        with open(obs_flips_b8_out_path, "wb") as f:
            f.write(b"\0" * (num_obs * shots))
