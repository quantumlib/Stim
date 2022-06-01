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
import pytest as pytest
import tempfile

import numpy as np
import stim


def test_read_shot_data_file_01():
    with tempfile.TemporaryDirectory() as d:
        path = str(pathlib.Path(d) / 'file.01')
        with open(path, 'w') as f:
            print('0001110011', file=f)
            print('0000000000', file=f)

        with pytest.raises(ValueError, match='expected data length'):
            stim.read_shot_data_file(
                path=path,
                format='01',
                num_measurements=9,
            )

        with pytest.raises(ValueError, match='ended in middle'):
            stim.read_shot_data_file(
                path=path,
                format='01',
                num_measurements=11,
            )

        result = stim.read_shot_data_file(
            path=path,
            format='01',
            num_measurements=10,
        )
        assert result.dtype == np.bool8
        assert result.shape == (2, 10)
        np.testing.assert_array_equal(
            result,
            [
                [0, 0, 0, 1, 1, 1, 0, 0, 1, 1],
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
            ])

        result = stim.read_shot_data_file(
            path=path,
            format='01',
            num_measurements=10,
            bit_pack=True,
        )
        assert result.dtype == np.uint8
        assert result.shape == (2, 2)
        np.testing.assert_array_equal(
            result,
            [
                [0x38, 0x03],
                [0x00, 0x00],
            ])


def test_read_shot_data_file_dets():
    with tempfile.TemporaryDirectory() as d:
        path = str(pathlib.Path(d) / 'file.01')
        with open(path, 'w') as f:
            print('shot', file=f)
            print('shot D0 D5 L0', file=f)

        with pytest.raises(ValueError, match='D0'):
            stim.read_shot_data_file(
                path=path,
                format='dets',
                num_measurements=9,
            )

        with pytest.raises(ValueError, match='D5'):
            stim.read_shot_data_file(
                path=path,
                format='dets',
                num_measurements=0,
                num_detectors=5,
                num_observables=2,
            )

        result = stim.read_shot_data_file(
            path=path,
            format='dets',
            num_measurements=0,
            num_detectors=10,
            num_observables=2,
        )

        assert result.dtype == np.bool8
        assert result.shape == (2, 12)
        np.testing.assert_array_equal(
            result,
            [
                [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
                [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0],
            ])


def test_write_shot_data_file_01():
    with tempfile.TemporaryDirectory() as d:
        path = str(pathlib.Path(d) / 'file.01')

        with pytest.raises(ValueError, match='4 bits per shot.+bool8'):
            stim.write_shot_data_file(
                data=np.array([
                    [0, 1, 0],
                    [0, 1, 1],
                ], dtype=np.bool8),
                path=path,
                format='01',
                num_measurements=4,
            )

        with pytest.raises(ValueError, match='4 bits per shot.+uint8'):
            stim.write_shot_data_file(
                data=np.array([
                    [0, 1, 0],
                    [0, 1, 1],
                ], dtype=np.uint8),
                path=path,
                format='01',
                num_measurements=4,
            )

        with pytest.raises(ValueError, match='num_measurements and'):
            stim.write_shot_data_file(
                data=np.array([
                    [0, 1, 0],
                    [0, 1, 1],
                ], dtype=np.bool8),
                path=path,
                format='01',
                num_measurements=1,
                num_detectors=2,
            )

        stim.write_shot_data_file(
            data=np.array([
                [0, 1, 0],
                [0, 1, 1],
            ], dtype=np.bool8),
            path=path,
            format='01',
            num_measurements=3,
        )
        with open(path) as f:
            assert f.read() == '010\n011\n'

        stim.write_shot_data_file(
            data=np.array([
                [0x38, 0x03],
                [0x00, 0x00],
            ], dtype=np.uint8),
            path=path,
            format='01',
            num_measurements=13,
        )
        with open(path) as f:
            assert f.read() == '0001110011000\n0000000000000\n'
