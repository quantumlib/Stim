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
import pytest


def test_basics():
    p = stim.Flow()
    assert p.input_copy() == stim.PauliString(0)
    assert p.output_copy() == stim.PauliString(0)
    assert p.measurements_copy() == []
    assert str(p) == "1 -> 1"
    assert repr(p) == 'stim.Flow("1 -> 1")'

    p = stim.Flow(
        input=stim.PauliString("XX"),
        output=stim.PauliString("-YYZ"),
        measurements=[-1, 2, 3],
    )
    assert p.input_copy() == stim.PauliString("XX")
    assert p.output_copy() == stim.PauliString("-YYZ")
    assert p.measurements_copy() == [-1, 2, 3]
    assert str(p) == "XX -> -YYZ xor rec[-1] xor rec[2] xor rec[3]"
    assert repr(p) == 'stim.Flow("XX -> -YYZ xor rec[-1] xor rec[2] xor rec[3]")'

    p = stim.Flow("-X1*Z2 -> Y3 xor rec[-1]")
    assert p.input_copy() == stim.PauliString("-_XZ")
    assert p.output_copy() == stim.PauliString("___Y")
    assert p.measurements_copy() == [-1]
    assert str(p) == "-_XZ -> ___Y xor rec[-1]"
    assert repr(p) == 'stim.Flow("-_XZ -> ___Y xor rec[-1]")'

    p = stim.Flow("X20 -> Y xor rec[-1]")
    assert p.input_copy() == stim.PauliString("X20")
    assert p.output_copy() == stim.PauliString("Y")
    assert p.measurements_copy() == [-1]
    assert str(p) == "X20 -> Y0 xor rec[-1]"
    assert repr(p) == 'stim.Flow("X20 -> Y0 xor rec[-1]")'

    p = stim.Flow("X20*I21 -> Y xor rec[-1]")
    assert p.input_copy() == stim.PauliString("____________________X_")
    assert p.output_copy() == stim.PauliString("Y")
    assert p.measurements_copy() == [-1]
    assert str(p) == "____________________X_ -> Y xor rec[-1]"
    assert repr(p) == 'stim.Flow("____________________X_ -> Y xor rec[-1]")'

    p = stim.Flow("iX -> iY")
    assert p.input_copy() == stim.PauliString("X")
    assert p.output_copy() == stim.PauliString("Y")
    assert p.measurements_copy() == []

    p = stim.Flow(input=stim.PauliString("iX"), output=stim.PauliString("iY"))
    assert p.input_copy() == stim.PauliString("X")
    assert p.output_copy() == stim.PauliString("Y")
    assert p.measurements_copy() == []

    with pytest.raises(ValueError, match="Anti-Hermitian"):
        stim.Flow("iX -> Y")
    with pytest.raises(ValueError, match="Anti-Hermitian"):
        stim.Flow(input=stim.PauliString("iX"), output=stim.PauliString("Y"))


def test_equality():
    assert not (stim.Flow() == None)
    assert not (stim.Flow() == "other object")
    assert not (stim.Flow() == object())
    assert stim.Flow() != None
    assert stim.Flow() != "other object"
    assert stim.Flow() != object()

    assert stim.Flow('X -> Y') == stim.Flow('X -> Y')
    assert stim.Flow('X -> X') != stim.Flow('X -> Y')
    assert not (stim.Flow('X -> Y') != stim.Flow('X -> Y'))
    assert not (stim.Flow('X -> Y') == stim.Flow('X -> X'))

    assert stim.Flow("X -> X xor rec[-1]") == stim.Flow("X -> X xor rec[-1]")
    assert stim.Flow("X -> X xor rec[-1]") != stim.Flow("Y -> X xor rec[-1]")
    assert stim.Flow("X -> X xor rec[-1]") != stim.Flow("X -> Y xor rec[-1]")
    assert stim.Flow("X -> X xor rec[-1]") != stim.Flow("X -> X xor rec[-2]")


@pytest.mark.parametrize("value", [
    stim.Flow(),
    stim.Flow("X -> Y xor rec[-1]"),
    stim.Flow("X -> 1"),
    stim.Flow("-X -> Y"),
    stim.Flow("X -> -Y"),
    stim.Flow("-X -> -Y"),
    stim.Flow("1 -> X"),
    stim.Flow("X__________________ -> ________Y"),
])
def test_repr(value):
    assert eval(repr(value), {'stim': stim}) == value
    assert repr(eval(repr(value), {'stim': stim})) == repr(value)


def test_obs_flows():
    assert stim.Circuit("""
        OBSERVABLE_INCLUDE(1) X2
    """).has_flow(stim.Flow("X2 -> obs[1]"))
    assert not stim.Circuit("""
        OBSERVABLE_INCLUDE(1) !X2
    """).has_flow(stim.Flow("X2 -> obs[1]"))
    assert stim.Circuit("""
        OBSERVABLE_INCLUDE(1) !X2
    """).has_flow(stim.Flow("X2 -> obs[1]"), unsigned=True)
    assert stim.Circuit("""
        OBSERVABLE_INCLUDE(1) !X2
    """).has_flow(stim.Flow("-X2 -> obs[1]"))
    assert not stim.Circuit("""
        OBSERVABLE_INCLUDE(1) X2
    """).has_flow(stim.Flow("Y2 -> obs[1]"))
    assert not stim.Circuit("""
        OBSERVABLE_INCLUDE(1) X2
    """).has_flow(stim.Flow("Y2 -> obs[1]"), unsigned=True)


def test_obs_include_pauli_terms_sensitivity():
    _, obs = stim.Circuit("""
        OBSERVABLE_INCLUDE(0) X0
        X_ERROR(0.5) 0
        OBSERVABLE_INCLUDE(0) X0
    """).compile_detector_sampler().sample(shots=1024, separate_observables=True)
    assert np.count_nonzero(obs) == 0

    _, obs = stim.Circuit("""
        OBSERVABLE_INCLUDE(0) X0
        OBSERVABLE_INCLUDE(1) Y0
        OBSERVABLE_INCLUDE(2) Z0
        X_ERROR(0.5) 0
        OBSERVABLE_INCLUDE(0) X0
        OBSERVABLE_INCLUDE(1) Y0
        OBSERVABLE_INCLUDE(2) Z0
    """).compile_detector_sampler().sample(shots=1024, separate_observables=True)
    xs, ys, zs = np.count_nonzero(obs, axis=0)
    assert xs == 0
    assert 256 <= ys <= 768
    assert zs == ys

    _, obs = stim.Circuit("""
        OBSERVABLE_INCLUDE(0) X0
        OBSERVABLE_INCLUDE(1) Y0
        OBSERVABLE_INCLUDE(2) Z0
        Y_ERROR(0.5) 0
        OBSERVABLE_INCLUDE(0) X0
        OBSERVABLE_INCLUDE(1) Y0
        OBSERVABLE_INCLUDE(2) Z0
    """).compile_detector_sampler().sample(shots=1024, separate_observables=True)
    xs, ys, zs = np.count_nonzero(obs, axis=0)
    assert ys == 0
    assert 256 <= xs <= 768
    assert zs == xs

    _, obs = stim.Circuit("""
        OBSERVABLE_INCLUDE(0) X0
        OBSERVABLE_INCLUDE(1) Y0
        OBSERVABLE_INCLUDE(2) Z0
        Z_ERROR(0.5) 0
        OBSERVABLE_INCLUDE(0) X0
        OBSERVABLE_INCLUDE(1) Y0
        OBSERVABLE_INCLUDE(2) Z0
    """).compile_detector_sampler().sample(shots=1024, separate_observables=True)
    xs, ys, zs = np.count_nonzero(obs, axis=0)
    assert zs == 0
    assert 256 <= ys <= 768
    assert xs == ys


def test_flow_canonicalization():
    assert stim.Flow(measurements=[4, 0, 4]) == stim.Flow(measurements=[0])
    assert stim.Flow(included_observables=[4, 0, 4]) == stim.Flow(included_observables=[0])


def test_flow_multiplication():
    assert stim.Flow("XYZ -> 1") * stim.Flow("1 -> XYZ") == stim.Flow("XYZ -> XYZ")
    assert stim.Flow("XX_ -> 1") * stim.Flow("_XX -> 1") == stim.Flow("X_X -> 1")
    assert stim.Flow("1 -> XX_") * stim.Flow("1 -> _XX") == stim.Flow("1 -> X_X")
    assert stim.Flow("1 -> rec[-1] xor rec[-3]") * stim.Flow("1 -> rec[-1] xor rec[-2]") == stim.Flow("1 -> rec[-2] xor rec[-3]")
    assert stim.Flow("1 -> obs[1] xor obs[3]") * stim.Flow("1 -> obs[1] xor obs[2]") == stim.Flow("1 -> obs[2] xor obs[3]")
    assert stim.Flow("X -> X") * stim.Flow("Z -> Z") == stim.Flow("Y -> Y")
    assert stim.Flow("1 -> XX") * stim.Flow("1 -> ZZ") == stim.Flow("1 -> -YY")
    assert stim.Flow("1 -> obs[1]") * stim.Flow("1 -> obs[1]") == stim.Flow("1 -> 1")
    assert stim.Flow("1 -> rec[1]") * stim.Flow("1 -> rec[1]") == stim.Flow("1 -> 1")
    with pytest.raises(ValueError, match="anticommute"):
        _ = stim.Flow("1 -> X") * stim.Flow("1 -> Y")
    with pytest.raises(ValueError, match="anticommute"):
        _ = stim.Flow("1 -> Y") * stim.Flow("1 -> X")
