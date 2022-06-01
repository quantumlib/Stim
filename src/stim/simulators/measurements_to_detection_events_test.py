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
import pytest
import stim


def test_convert_file_without_sweep_bits():
    converter = stim.Circuit('''
        X_ERROR(0.1) 0
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
       X_ERROR(0.1) 0
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


def test_convert_bit_packed():
    converter = stim.Circuit('''
       REPEAT 100 {
           X_ERROR(0.1) 0
           X 0
           MR 0
           DETECTOR rec[-1]
       }
    ''').compile_m2d_converter()

    measurements = np.array([[0] * 100, [1] * 100], dtype=np.bool8)
    expected_detections = np.array([[1] * 100, [0] * 100], dtype=np.bool8)
    measurements_bit_packed = np.packbits(measurements, axis=1, bitorder='little')
    expected_detections_packed = np.packbits(expected_detections, axis=1, bitorder='little')

    for m in measurements, measurements_bit_packed:
        result = converter.convert(
            measurements=m,
            append_observables=False,
        )
        assert result.dtype == np.bool8
        assert result.shape == (2, 100)
        np.testing.assert_array_equal(result, expected_detections)

        result = converter.convert(
            measurements=m,
            append_observables=False,
            bit_pack_result=True,
        )
        assert result.dtype == np.uint8
        assert result.shape == (2, 13)
        np.testing.assert_array_equal(result, expected_detections_packed)


def test_convert_bit_packed_swept():
    converter = stim.Circuit('''
       REPEAT 100 {
           CNOT sweep[0] 0
           X_ERROR(0.1) 0
           X 0
           MR 0
           DETECTOR rec[-1]
       }
    ''').compile_m2d_converter()

    measurements = np.array([[0] * 100, [1] * 100], dtype=np.bool8)
    sweeps = np.array([[1], [0]], dtype=np.bool8)
    expected_detections = np.array([[0] * 100, [0] * 100], dtype=np.bool8)

    measurements_packed = np.packbits(measurements, axis=1, bitorder='little')
    expected_detections_packed = np.packbits(expected_detections, axis=1, bitorder='little')
    sweeps_packed = np.packbits(sweeps, axis=1, bitorder='little')

    for m in measurements, measurements_packed:
        for s in sweeps, sweeps_packed:
            result = converter.convert(
                measurements=m,
                sweep_bits=s,
                append_observables=False,
            )
            assert result.dtype == np.bool8
            assert result.shape == (2, 100)
            np.testing.assert_array_equal(result, expected_detections)

            result = converter.convert(
                measurements=m,
                sweep_bits=s,
                append_observables=False,
                bit_pack_result=True,
            )
            assert result.dtype == np.uint8
            assert result.shape == (2, 13)
            np.testing.assert_array_equal(result, expected_detections_packed)


def test_convert_bit_packed_separate_observables():
    converter = stim.Circuit('''
       REPEAT 100 {
           X_ERROR(0.1) 0
           X 0
           MR 0
           DETECTOR rec[-1]
       }
       OBSERVABLE_INCLUDE(0) rec[-1]
       OBSERVABLE_INCLUDE(6) rec[-2]
       OBSERVABLE_INCLUDE(14) rec[-3]
    ''').compile_m2d_converter()

    measurements = np.array([[0] * 100, [1] * 100], dtype=np.bool8)
    expected_dets = np.array([[1] * 100, [0] * 100], dtype=np.bool8)
    expected_obs = np.array([[1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1], [0] * 15], dtype=np.bool8)
    measurements_bit_packed = np.packbits(measurements, axis=1, bitorder='little')
    expected_dets_packed = np.packbits(expected_dets, axis=1, bitorder='little')
    expected_obs_packed = np.packbits(expected_obs, axis=1, bitorder='little')

    for m in measurements, measurements_bit_packed:
        actual_dets, actual_obs = converter.convert(
            measurements=m,
            separate_observables=True,
        )
        assert actual_dets.dtype == actual_obs.dtype == np.bool8
        assert actual_dets.shape == (2, 100)
        assert actual_obs.shape == (2, 15)
        np.testing.assert_array_equal(actual_dets, expected_dets)
        np.testing.assert_array_equal(actual_obs, expected_obs)

        actual_dets, actual_obs = converter.convert(
            measurements=m,
            separate_observables=True,
            bit_pack_result=True,
        )
        assert actual_dets.dtype == actual_obs.dtype == np.uint8
        assert actual_dets.shape == (2, 13)
        assert actual_obs.shape == (2, 2)
        np.testing.assert_array_equal(actual_dets, expected_dets_packed)
        np.testing.assert_array_equal(actual_obs, expected_obs_packed)


def test_noiseless_conversion():
    converter = stim.Circuit('''
       MR 0
       DETECTOR rec[-1]
       X 0
       MR 0
       DETECTOR rec[-1]
       OBSERVABLE_INCLUDE(0) rec[-1]
    ''').compile_m2d_converter()

    measurements = np.array([[0, 0], [0, 1], [1, 0], [1, 1]], dtype=np.bool8)
    expected_dets = np.array([[0, 1], [0, 0], [1, 1], [1, 0]], dtype=np.bool8)
    expected_obs = np.array([[1], [0], [1], [0]], dtype=np.bool8)

    actual_dets, actual_obs = converter.convert(
        measurements=measurements,
        separate_observables=True,
    )
    assert actual_dets.dtype == actual_obs.dtype == np.bool8
    assert actual_dets.shape == (4, 2)
    assert actual_obs.shape == (4, 1)
    np.testing.assert_array_equal(actual_dets, expected_dets)
    np.testing.assert_array_equal(actual_obs, expected_obs)


def test_needs_append_or_separate():
    converter = stim.Circuit().compile_m2d_converter()
    ms = np.zeros(shape=(50, 0), dtype=np.bool8)
    with pytest.raises(ValueError, match="explicitly specify either separate"):
        converter.convert(measurements=ms)
    d1 = converter.convert(measurements=ms, append_observables=True)
    d2, d3 = converter.convert(measurements=ms, separate_observables=True)
    d4, d5 = converter.convert(measurements=ms, separate_observables=True, append_observables=True)
    np.testing.assert_array_equal(d1, d2)
    np.testing.assert_array_equal(d1, d3)
    np.testing.assert_array_equal(d1, d4)
    np.testing.assert_array_equal(d1, d5)


def test_anticommuting_pieces_combining_into_deterministic_observable():
    c = stim.Circuit('''
        MX 0
        OBSERVABLE_INCLUDE(0) rec[-1]
        MX 0
        OBSERVABLE_INCLUDE(0) rec[-1]
    ''').without_noise()
    m = c.compile_sampler().sample_bit_packed(shots=1000)
    det, obs = c.compile_m2d_converter().convert(measurements=m, separate_observables=True)
    np.testing.assert_array_equal(obs, obs * 0)
