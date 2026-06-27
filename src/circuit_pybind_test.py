import sys

import stim


def test_reference_sample():
    print("TEST PROGRESSION: E", file=sys.stderr)
    _ = stim.test()

    print("TEST PROGRESSION: F", file=sys.stderr)
