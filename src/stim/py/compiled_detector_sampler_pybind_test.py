# Copyright 2021 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
import pathlib

import tempfile

import numpy as np
import stim


def test_compiled_detector_sampler_sample():
    c = stim.Circuit()
    c.append_operation("X_ERROR", [0], 1)
    c.append_operation("M", [0, 1, 2])
    c.append_operation("DETECTOR", [stim.target_rec(-3), stim.target_rec(-2)])
    c.append_operation("DETECTOR", [stim.target_rec(-3), stim.target_rec(-1)])
    c.append_operation("DETECTOR", [stim.target_rec(-1), stim.target_rec(-2)])
    c.append_from_stim_program_text("""
        OBSERVABLE_INCLUDE(0) rec[-3]
        OBSERVABLE_INCLUDE(3) rec[-2] rec[-1]
    """)
    c.append_operation("OBSERVABLE_INCLUDE", [stim.target_rec(-2)], 0)
    np.testing.assert_array_equal(
        c.compile_detector_sampler().sample(5),
        np.array([
            [1, 1, 0],
            [1, 1, 0],
            [1, 1, 0],
            [1, 1, 0],
            [1, 1, 0],
        ], dtype=np.uint8))
    np.testing.assert_array_equal(
        c.compile_detector_sampler().sample(5, prepend_observables=True),
        np.array([
            [1, 0, 0, 0, 1, 1, 0],
            [1, 0, 0, 0, 1, 1, 0],
            [1, 0, 0, 0, 1, 1, 0],
            [1, 0, 0, 0, 1, 1, 0],
            [1, 0, 0, 0, 1, 1, 0],
        ], dtype=np.uint8))
    np.testing.assert_array_equal(
        c.compile_detector_sampler().sample(5, append_observables=True),
        np.array([
            [1, 1, 0, 1, 0, 0, 0],
            [1, 1, 0, 1, 0, 0, 0],
            [1, 1, 0, 1, 0, 0, 0],
            [1, 1, 0, 1, 0, 0, 0],
            [1, 1, 0, 1, 0, 0, 0],
        ], dtype=np.uint8))
    np.testing.assert_array_equal(
        c.compile_detector_sampler().sample(5, append_observables=True, prepend_observables=True),
        np.array([
            [1, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0],
            [1, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0],
            [1, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0],
            [1, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0],
            [1, 0, 0, 0, 1, 1, 0, 1, 0, 0, 0],
        ], dtype=np.uint8))
    np.testing.assert_array_equal(
        c.compile_detector_sampler().sample_bit_packed(5),
        np.array([
            [0b011],
            [0b011],
            [0b011],
            [0b011],
            [0b011],
        ], dtype=np.uint8))

    with tempfile.TemporaryDirectory() as d:
        path = f"{d}/tmp.dat"
        c.compile_detector_sampler().sample_write(5, filepath=path, format='b8')
        with open(path, 'rb') as f:
            assert f.read() == b'\x03' * 5

        c.compile_detector_sampler().sample_write(5, filepath=path, format='01', prepend_observables=True)
        with open(path, 'r') as f:
            assert f.readlines() == ['1000110\n'] * 5

        c.compile_detector_sampler().sample_write(5, filepath=path, format='01', append_observables=True)
        with open(path, 'r') as f:
            assert f.readlines() == ['1101000\n'] * 5


def test_write_obs_file():
    c = stim.Circuit("""
        X_ERROR(1) 1
        MR 0 1
        DETECTOR rec[-2]
        DETECTOR rec[-2]
        DETECTOR rec[-2]
        DETECTOR rec[-1]
        DETECTOR rec[-2]
        DETECTOR rec[-2]
        OBSERVABLE_INCLUDE(1) rec[-1]
        OBSERVABLE_INCLUDE(2) rec[-2]
    """)
    r: stim.CompiledDetectorSampler = c.compile_detector_sampler()
    with tempfile.TemporaryDirectory() as d:
        d = pathlib.Path(d)
        r.sample_write(
            shots=100,
            filepath=str(d / 'det'),
            format='dets',
            obs_out_filepath=str(d / 'obs'),
            obs_out_format='hits',
        )
        with open(d / 'det') as f:
            assert f.read() == 'shot D3\n' * 100
        with open(d / 'obs') as f:
            assert f.read() == '1\n' * 100
