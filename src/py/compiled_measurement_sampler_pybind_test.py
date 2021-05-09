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

import numpy as np
import stim


def test_compiled_measurement_sampler_sample():
    c = stim.Circuit()
    c.append_operation("X", [1])
    c.append_operation("M", [0, 1, 2, 3])
    np.testing.assert_array_equal(
        c.compile_sampler().sample(5),
        np.array([
            [0, 1, 0, 0],
            [0, 1, 0, 0],
            [0, 1, 0, 0],
            [0, 1, 0, 0],
            [0, 1, 0, 0],
        ], dtype=np.uint8))
    np.testing.assert_array_equal(
        c.compile_sampler().sample_bit_packed(5),
        np.array([
            [0b00010],
            [0b00010],
            [0b00010],
            [0b00010],
            [0b00010],
        ], dtype=np.uint8))


def test_measurements_vs_resets():
    assert not np.any(stim.Circuit("""
        RX 0
        RY 1
        RZ 2
        H 0
        H_YZ 1
        M 0 1 2
    """).compile_sampler().sample(shots=100))

    assert not np.any(stim.Circuit("""
        H 0
        H_YZ 1
        MRX 0
        MRY 1
        MRZ 2
        H 0
        H_YZ 1
        M 0 1 2
    """).compile_sampler().sample(shots=100))

    assert not np.any(stim.Circuit("""
        H 0
        H_YZ 1
        MRX 0
        MRY 1
        MRZ 2
    """).compile_sampler().sample(shots=100))
