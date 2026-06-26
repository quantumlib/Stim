import sys

import stim
import numpy as np


def test_reference_sample():
    circuit = stim.Circuit(
        """
        H 0 1
        CX 0 2 1 3
        MXX 0 1
        MYY 0 1
        MZZ 0 1
        """
    )

    print("TEST PROGRESSION: E", file=sys.stderr)
    _ = circuit.reference_sample()
    # np.testing.assert_array_equal(circuit.reference_sample(), circuit.reference_sample())

    print("TEST PROGRESSION: F", file=sys.stderr)
