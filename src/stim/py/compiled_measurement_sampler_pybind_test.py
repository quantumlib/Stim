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
        ], dtype=np.bool_))
    np.testing.assert_array_equal(
        c.compile_sampler().sample(5, bit_packed=True),
        np.array([
            [0b00010],
            [0b00010],
            [0b00010],
            [0b00010],
            [0b00010],
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


def test_sample_write():
    c = stim.Circuit("""
        X 0 4 5
        M 0 1 2 3 4 5 6
    """)
    with tempfile.TemporaryDirectory() as d:
        path = f"{d}/tmp.dat"
        c.compile_sampler().sample_write(5, filepath=path, format='b8')
        with open(path, 'rb') as f:
            assert f.read() == b'\x31' * 5

        c.compile_sampler().sample_write(5, filepath=path, format='01')
        with open(path, 'r') as f:
            assert f.readlines() == ['1000110\n'] * 5


def test_skip_reference_sample():
    np.testing.assert_array_equal(
        stim.Circuit("X 0\nM 0").compile_sampler().sample(1),
        [[True]],
    )
    np.testing.assert_array_equal(
        stim.Circuit("X 0\nM 0").compile_sampler(skip_reference_sample=False).sample(1),
        [[True]],
    )
    np.testing.assert_array_equal(
        stim.Circuit("X 0\nM 0").compile_sampler(skip_reference_sample=True).sample(1),
        [[False]],
    )
    np.testing.assert_array_equal(
        stim.Circuit("X_ERROR(1) 0\nM 0").compile_sampler(skip_reference_sample=False).sample(1),
        [[True]],
    )
    np.testing.assert_array_equal(
        stim.Circuit("X_ERROR(1) 0\nM 0").compile_sampler(skip_reference_sample=True).sample(1),
        [[True]],
    )

def test_reference_sample_init():
    np.testing.assert_array_equal(
        stim.Circuit("X 0\nM 0").compile_sampler(reference_sample=None).sample(1),
        [[True]],
    )
    circuit = stim.Circuit("X 0\nM 0")
    ref_sample = np.array([False])
    np.testing.assert_array_equal(
        circuit.compile_sampler(reference_sample=ref_sample).sample(1),
        [[False]],
    )
    ref_sample = circuit.reference_sample()
    np.testing.assert_array_equal(
        circuit.compile_sampler(reference_sample=ref_sample).sample(1),
        [[True]],
    )
    circuit = stim.Circuit("X_ERROR(1) 0\n M 0")
    ref_sample = np.array([False])
    np.testing.assert_array_equal(
        circuit.compile_sampler(reference_sample=ref_sample).sample(1),
        [[True]],
    )
    ref_sample = circuit.reference_sample()
    np.testing.assert_array_equal(
        circuit.compile_sampler(reference_sample=ref_sample).sample(1),
        [[True]],
    )
    with pytest.raises(ValueError):
        circuit.compile_sampler(reference_sample=ref_sample, skip_reference_sample=True)
    circuit = stim.Circuit("H 0\n X 1\n CNOT 0 1\n H 0 1\n MPP X0*X1")
    ref_sample = circuit.reference_sample()
    np.testing.assert_array_equal(
        circuit.compile_sampler(reference_sample=ref_sample, seed=0).sample(10),
        circuit.compile_sampler(reference_sample=None, seed=0).sample(10),
    )
    circuit = stim.Circuit("H 0\n X 1\n CNOT 0 1\n H 0 1 2\n MPP Y1*Y2 X1*X2 Z1*Z2")
    ref_sample = circuit.reference_sample()
    np.testing.assert_array_equal(
        circuit.compile_sampler(reference_sample=ref_sample, seed=0).sample(11),
        circuit.compile_sampler(seed=0).sample(11),
    )


def test_repr():
    assert repr(stim.Circuit("""
        X 0
        M 0
    """).compile_sampler()) == """stim.CompiledMeasurementSampler(stim.Circuit('''
    X 0
    M 0
'''))"""

    assert repr(stim.Circuit("""
        X 0
        M 0
    """).compile_sampler(skip_reference_sample=True)) == """stim.CompiledMeasurementSampler(stim.Circuit('''
    X 0
    M 0
'''), skip_reference_sample=True)"""


def test_circuit_sampler_actually_fills_array():
    circuit = stim.Circuit('''
       X_ERROR(1) 0
       M 0
       DETECTOR rec[-1]
    ''')
    sampler = circuit.compile_detector_sampler()
    measure_data = sampler.sample(shots=10000)
    assert np.all(measure_data)
