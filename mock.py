import sys

import stim
import numpy as np


def test_x():
    print("TEST PROGRESSION: START", file=sys.stderr)
    circuit = stim.Circuit(
        """
        H 0 1
        CX 0 2 1 3
        MPP X0*X1 Y0*Y1 Z0*Z1
        """
    )
    print("TEST PROGRESSION: A", file=sys.stderr)
    aa = circuit.reference_sample()
    print("TEST PROGRESSION: B", file=sys.stderr)
    bb = circuit.reference_sample()
    print("TEST PROGRESSION: C", file=sys.stderr)
    np.testing.assert_array_equal(aa, bb)
    print("TEST PROGRESSION: D", file=sys.stderr)


if __name__ == '__main__':
    test_x()
