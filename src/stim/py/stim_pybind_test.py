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

import numpy as np
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
    data = stim.gate_data()
    assert len(data) == 69
    assert data["CX"].name == "CX"
    assert data["CX"].aliases == ["CNOT", "CX", "ZCX"]
    assert data["X"].is_unitary
    assert "CNOT" not in data


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


def test_main_write_to_file():
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


def test_main_redirects_stdout(capsys):
    assert stim.main(command_line_args=[
        "gen",
        "--code=repetition_code",
        "--task=memory",
        "--rounds=1000",
        "--distance=2",
    ]) == 0
    captured = capsys.readouterr()
    assert "Generated repetition_code" in captured.out
    assert captured.err == ""


def test_main_redirects_stderr(capsys):
    assert stim.main(command_line_args=[
        "gen",
        "--code=XXXXX",
        "--task=memory",
        "--rounds=1000",
        "--distance=2",
    ]) == 1
    captured = capsys.readouterr()
    assert captured.out == ""
    assert "Unrecognized value 'XXXXX'" in captured.err


def test_target_methods_accept_gate_targets():
    assert stim.target_inv(stim.GateTarget(5)) == stim.target_inv(5)
    assert stim.target_inv(stim.target_inv(5)) == stim.GateTarget(5)
    assert stim.target_inv(stim.target_x(5)) == stim.target_x(5, invert=True)
    assert stim.target_inv(stim.target_y(5)) == stim.target_y(5, invert=True)
    assert stim.target_inv(stim.target_z(5)) == stim.target_z(5, invert=True)

    assert stim.target_x(stim.GateTarget(5)) == stim.target_x(5)
    assert stim.target_x(stim.target_inv(stim.GateTarget(5))) == stim.target_x(5, invert=True)
    assert stim.target_x(stim.GateTarget(5), invert=True) == stim.target_x(5, invert=True)
    assert stim.target_x(stim.target_inv(stim.GateTarget(5)), invert=True) == stim.target_x(5)

    assert stim.target_y(stim.GateTarget(5)) == stim.target_y(5)
    assert stim.target_y(stim.target_inv(stim.GateTarget(5))) == stim.target_y(5, invert=True)
    assert stim.target_y(stim.GateTarget(5), invert=True) == stim.target_y(5, invert=True)
    assert stim.target_y(stim.target_inv(stim.GateTarget(5)), invert=True) == stim.target_y(5)

    assert stim.target_z(stim.GateTarget(5)) == stim.target_z(5)
    assert stim.target_z(stim.target_inv(stim.GateTarget(5))) == stim.target_z(5, invert=True)
    assert stim.target_z(stim.GateTarget(5), invert=True) == stim.target_z(5, invert=True)
    assert stim.target_z(stim.target_inv(stim.GateTarget(5)), invert=True) == stim.target_z(5)

    with pytest.raises(ValueError):
        stim.target_inv(stim.target_sweep_bit(4))

    with pytest.raises(ValueError):
        stim.target_inv(stim.target_rec(-4))

    with pytest.raises(ValueError):
        stim.target_x(stim.target_sweep_bit(4))

    with pytest.raises(ValueError):
        stim.target_y(stim.target_sweep_bit(4))

    with pytest.raises(ValueError):
        stim.target_z(stim.target_sweep_bit(4))


def test_target_pauli():
    assert stim.target_pauli(2, "I") == stim.GateTarget(2)
    assert stim.target_pauli(2, "X") == stim.target_x(2)
    assert stim.target_pauli(2, "Y") == stim.target_y(2)
    assert stim.target_pauli(2, "Z") == stim.target_z(2)
    assert stim.target_pauli(5, "x") == stim.target_x(5)
    assert stim.target_pauli(2, "y") == stim.target_y(2)
    assert stim.target_pauli(2, "z") == stim.target_z(2)
    assert stim.target_pauli(2, 0) == stim.GateTarget(2)
    assert stim.target_pauli(2, 1) == stim.target_x(2)
    assert stim.target_pauli(2, 2) == stim.target_y(2)
    assert stim.target_pauli(2, 3) == stim.target_z(2)
    assert stim.target_pauli(2, 3, True) == stim.target_z(2, True)
    assert stim.target_pauli(qubit_index=2, pauli=3, invert=True) == stim.target_z(2, True)
    assert stim.target_pauli(5, np.array([2], dtype=np.uint8)[0]) == stim.target_y(5)
    assert stim.target_pauli(5, np.array([2], dtype=np.uint32)[0]) == stim.target_y(5)
    assert stim.target_pauli(5, np.array([2], dtype=np.int16)[0]) == stim.target_y(5)

    with pytest.raises(ValueError, match="too large"):
        stim.target_pauli(2**31, 'X')
    with pytest.raises(ValueError, match="Expected pauli"):
        stim.target_pauli(5, 'F')
    with pytest.raises(ValueError, match="Expected pauli"):
        stim.target_pauli(5, np.array([257], dtype=np.uint32)[0])


def test_target_combined_paulis():
    assert stim.target_combined_paulis(stim.PauliString("XYZ")) == [
        stim.target_x(0),
        stim.target_combiner(),
        stim.target_y(1),
        stim.target_combiner(),
        stim.target_z(2),
    ]

    assert stim.target_combined_paulis(stim.PauliString("X"), True) == [
        stim.target_x(0, True),
    ]

    assert stim.target_combined_paulis(stim.PauliString("-XYIZ")) == [
        stim.target_x(0, invert=True),
        stim.target_combiner(),
        stim.target_y(1),
        stim.target_combiner(),
        stim.target_z(3),
    ]

    assert stim.target_combined_paulis(stim.PauliString("-XYIZ"), True) == [
        stim.target_x(0),
        stim.target_combiner(),
        stim.target_y(1),
        stim.target_combiner(),
        stim.target_z(3),
    ]

    assert stim.target_combined_paulis([stim.target_x(5), stim.target_z(9)]) == [
        stim.target_x(5),
        stim.target_combiner(),
        stim.target_z(9),
    ]

    assert stim.target_combined_paulis([stim.target_x(5, True), stim.target_z(9)]) == [
        stim.target_x(5, True),
        stim.target_combiner(),
        stim.target_z(9),
    ]
    assert stim.target_combined_paulis([stim.target_x(5), stim.target_z(9, True)]) == [
        stim.target_x(5, True),
        stim.target_combiner(),
        stim.target_z(9),
    ]
    assert stim.target_combined_paulis([stim.target_x(5), stim.target_z(9)], True) == [
        stim.target_x(5, True),
        stim.target_combiner(),
        stim.target_z(9),
    ]
    assert stim.target_combined_paulis([stim.target_y(4)]) == [
        stim.target_y(4),
    ]

    with pytest.raises(ValueError, match="Expected a pauli string"):
        stim.target_combined_paulis([stim.target_rec(-2)])
    with pytest.raises(ValueError, match="Expected a pauli string"):
        stim.target_combined_paulis([object()])
    with pytest.raises(ValueError, match="Identity pauli product"):
        stim.target_combined_paulis([])
    with pytest.raises(ValueError, match="Identity pauli product"):
        stim.target_combined_paulis(stim.PauliString(0))
    with pytest.raises(ValueError, match="Identity pauli product"):
        stim.target_combined_paulis(stim.PauliString(10))
    with pytest.raises(ValueError, match="Imaginary"):
        stim.target_combined_paulis(stim.PauliString("iX"))
