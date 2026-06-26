import sys

import stim
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
