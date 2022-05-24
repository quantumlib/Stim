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
import tempfile

import numpy as np
import stim


def test_convert_file_without_sweep_bits():
    converter = stim.Circuit('''
       X 0
       CNOT sweep[0] 0
       M 0
       DETECTOR rec[-1]
       OBSERVABLE_INCLUDE(0) rec[-1]
    ''').compile_m2d_converter()

    with tempfile.TemporaryDirectory() as d:
        with open(f"{d}/measurements.01", "w") as f:
            print("0", file=f)
            print("1", file=f)

        converter.convert_file(
            measurements_filepath=f"{d}/measurements.01",
            detection_events_filepath=f"{d}/detections.01",
            append_observables=False,
        )

        with open(f"{d}/detections.01", "r") as f:
            assert f.read() == "1\n0\n"

    with tempfile.TemporaryDirectory() as d:
        with open(f"{d}/measurements.b8", "wb") as f:
            f.write((0).to_bytes(1, 'big'))
            f.write((1).to_bytes(1, 'big'))
            f.write((0).to_bytes(1, 'big'))
            f.write((1).to_bytes(1, 'big'))
        with open(f"{d}/sweep.hits", "w") as f:
            print("", file=f)
            print("", file=f)
            print("0", file=f)
            print("0", file=f)

        converter.convert_file(
            measurements_filepath=f"{d}/measurements.b8",
            measurements_format="b8",
            sweep_bits_filepath=f"{d}/sweep.hits",
            sweep_bits_format="hits",
            detection_events_filepath=f"{d}/detections.dets",
            detection_events_format="dets",
            append_observables=True,
        )

        with open(f"{d}/detections.dets", "r") as f:
            assert f.read() == "shot D0 L0\nshot\nshot\nshot D0 L0\n"

        converter.convert_file(
            measurements_filepath=f"{d}/measurements.b8",
            measurements_format="b8",
            sweep_bits_filepath=f"{d}/sweep.hits",
            sweep_bits_format="hits",
            detection_events_filepath=f"{d}/detections.dets",
            detection_events_format="dets",
            obs_out_filepath=f"{d}/obs.hits",
            obs_out_format="hits",
        )

        with open(f"{d}/detections.dets", "r") as f:
            assert f.read() == "shot D0\nshot\nshot\nshot D0\n"
        with open(f"{d}/obs.hits", "r") as f:
            assert f.read() == "0\n\n\n0\n"


def test_convert():
    converter = stim.Circuit('''
       X 0
       CNOT sweep[0] 0
       M 0
       DETECTOR rec[-1]
       OBSERVABLE_INCLUDE(0) rec[-1]
    ''').compile_m2d_converter()

    result = converter.convert(
        measurements=np.array([[0], [1]], dtype=np.bool8),
        append_observables=False,
    )
    assert result.dtype == np.bool8
    assert result.shape == (2, 1)
    np.testing.assert_array_equal(result, [[1], [0]])

    result = converter.convert(
        measurements=np.array([[0], [1]], dtype=np.bool8),
        append_observables=True,
    )
    assert result.dtype == np.bool8
    assert result.shape == (2, 2)
    np.testing.assert_array_equal(result, [[1, 1], [0, 0]])

    result = converter.convert(
        measurements=np.array([[0], [1], [0], [1]], dtype=np.bool8),
        sweep_bits=np.array([[0], [0], [1], [1]], dtype=np.bool8),
        append_observables=True,
    )
    assert result.dtype == np.bool8
    assert result.shape == (4, 2)
    np.testing.assert_array_equal(result, [[1, 1], [0, 0], [0, 0], [1, 1]])
