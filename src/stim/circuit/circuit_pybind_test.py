import sys

import stim


def test_reference_sample():
    circuit = stim.Circuit(
        """
        MPP X0*X1 Y0*Y1 Z0*Z1
        """
    )

    print("TEST PROGRESSION: E", file=sys.stderr)
    _ = circuit.reference_sample()

    print("TEST PROGRESSION: F", file=sys.stderr)
