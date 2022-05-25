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

import doctest
import importlib
import pytest
import types

import stim
import re


def test_version():
    assert re.match(r"^\d\.\d+", stim.__version__)


def test_targets():
    t = stim.target_x(5)
    assert isinstance(t, stim.GateTarget)
    assert t.is_x_target and t.value == 5
    assert not t.is_y_target

    t = stim.target_y(6)
    assert isinstance(t, stim.GateTarget)
    assert t.is_y_target and t.value == 6

    t = stim.target_z(5)
    assert isinstance(t, stim.GateTarget)
    assert t.is_z_target and t.value == 5

    t = stim.target_inv(5)
    assert isinstance(t, stim.GateTarget)
    assert t.is_inverted_result_target and t.value == 5

    t = stim.target_rec(-5)
    assert isinstance(t, stim.GateTarget)
    assert t.is_measurement_record_target and not t.is_inverted_result_target and t.value == -5

    t = stim.target_sweep_bit(4)
    assert isinstance(t, stim.GateTarget)
    assert t.is_sweep_bit_target and not t.is_inverted_result_target and t.value == 4

    t = stim.target_combiner()
    assert isinstance(t, stim.GateTarget)


def test_gate_data():
    data = stim._UNSTABLE_raw_gate_data()
    assert len(data) >= 69
    assert data["CX"]["name"] == "CX"
    assert data["X"]["unitary_matrix"] == [[0, 1], [1, 0]]


def test_format_data():
    format_data = stim._UNSTABLE_raw_format_data()
    assert len(format_data) >= 6

    # Check that example code has needed imports.
    for k, v in format_data.items():
        exec(v["parse_example"], {}, {})
        exec(v["save_example"], {}, {})

    # Collect sample methods.
    ctx = {}
    for k, v in format_data.items():
        exec(v["parse_example"], {}, ctx)
        exec(v["save_example"], {}, ctx)

    # Manually test example save/parse methods.
    data = [[0, 1, 1, 0], [0, 0, 0, 0]]
    saved = "0110\n0000\n"
    assert ctx["save_01"](data) == saved
    assert ctx["parse_01"](saved) == data

    saved = b"\x01\x00\x01\x04"
    assert ctx["save_r8"](data) == saved
    assert ctx["parse_r8"](saved, bits_per_shot=4) == data

    saved = b"\x06\x00"
    assert ctx["save_b8"](data) == saved
    assert ctx["parse_b8"](saved, bits_per_shot=4) == data

    saved = "1,2\n\n"
    assert ctx["save_hits"](data) == saved
    assert ctx["parse_hits"](saved, bits_per_shot=4) == data

    saved = "shot D1 L0\nshot\n"
    assert ctx["save_dets"](data, num_detectors=2, num_observables=2) == saved
    assert ctx["parse_dets"](saved, num_detectors=2, num_observables=2) == data

    big_data = [data[0]] + [data[1]] * 36 + [[1, 0, 1, 0]] * 4 + [data[1]] * (64 - 41)
    saved = (b"\x00\x00\x00\x00\xe0\x01\x00\x00"
             b"\x01\x00\x00\x00\x00\x00\x00\x00"
             b"\x01\x00\x00\x00\xe0\x01\x00\x00"
             b"\x00\x00\x00\x00\x00\x00\x00\x00")
    assert ctx["save_ptb64"](big_data) == saved
    assert ctx["parse_ptb64"](saved, bits_per_shot=4) == big_data

    # Check that python examples in help strings are correct.
    for k, v in format_data.items():
        mod = types.ModuleType('stim_test_fake')
        mod.f = lambda: 5
        mod.__test__ = {"f": mod.f}
        mod.f.__doc__ = v["help"]
        assert doctest.testmod(mod).failed == 0, k


def test_main():
    with tempfile.TemporaryDirectory() as d:
        p = pathlib.Path(d) / 'tmp'
        assert stim.main(command_line_args=[
            "gen",
            "--code=repetition_code",
            "--task=memory",
            "--rounds=1000",
            "--distance=2",
            f"--out={p}"
        ]) == 0
        with open(p) as f:
            assert "Generated repetition_code" in f.read()
