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
import io
import pathlib
import sys
import tempfile
from typing import cast

import stim
import pytest
import numpy as np


def test_reference_sample():
    print("TEST PROGRESSION: A", file=sys.stderr)
    circuit = stim.Circuit(
        """
        H 0
        CNOT 0 1
    """
    )
    print("TEST PROGRESSION: B", file=sys.stderr)
    ref = circuit.reference_sample()
    print("TEST PROGRESSION: C", file=sys.stderr)
    assert len(ref) == 0
    print("TEST PROGRESSION: D", file=sys.stderr)
    circuit = stim.Circuit(
        """
        H 0 1
        CX 0 2 1 3
        MPP X0*X1 Y0*Y1 Z0*Z1
        """
    )
    print("TEST PROGRESSION: E", file=sys.stderr)
    np.testing.assert_array_equal(circuit.reference_sample(), circuit.reference_sample())
    print("TEST PROGRESSION: F", file=sys.stderr)
    assert np.sum(circuit.reference_sample()) % 2 == 1
    print("TEST PROGRESSION: G", file=sys.stderr)
    circuit.append("X", (i for i in range(0, 100, 2)))
    print("TEST PROGRESSION: H", file=sys.stderr)
    circuit.append("M", (i for i in range(100)))
    print("TEST PROGRESSION: I", file=sys.stderr)
    ref_sample = circuit.reference_sample(bit_packed=True)
    print("TEST PROGRESSION: J", file=sys.stderr)
    unpacked = np.unpackbits(ref_sample, bitorder="little")
    print("TEST PROGRESSION: K", file=sys.stderr)
    expected = circuit.reference_sample(bit_packed=False)
    print("TEST PROGRESSION: L", file=sys.stderr)
    expected_padded = np.zeros_like(unpacked)
    print("TEST PROGRESSION: M", file=sys.stderr)
    expected_padded[:len(expected)] = expected
    print("TEST PROGRESSION: N", file=sys.stderr)
    np.testing.assert_array_equal(unpacked, expected_padded)
    print("TEST PROGRESSION: O", file=sys.stderr)
