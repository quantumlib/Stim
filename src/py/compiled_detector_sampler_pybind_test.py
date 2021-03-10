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
