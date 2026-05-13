import pytest

import stimflow


def test_sorted_complex():
    assert stimflow.sorted_complex([1, 2j, 2, 1 + 2j]) == [2j, 1, 1 + 2j, 2]


def test_min_max_complex():
    with pytest.raises(ValueError):
        stimflow.min_max_complex([])
    assert stimflow.min_max_complex([], default=0) == (0, 0)
    assert stimflow.min_max_complex([], default=1 + 2j) == (1 + 2j, 1 + 2j)
    assert stimflow.min_max_complex([1j], default=0) == (1j, 1j)
    assert stimflow.min_max_complex([1j, 2]) == (0, 2 + 1j)
    assert stimflow.min_max_complex([1j + 1, 2]) == (1, 2 + 1j)


def test_xor_sorted():
    assert stimflow.xor_sorted([]) == []
    assert stimflow.xor_sorted([2]) == [2]
    assert stimflow.xor_sorted([2, 3]) == [2, 3]
    assert stimflow.xor_sorted([3, 2]) == [2, 3]
    assert stimflow.xor_sorted([2, 2]) == []
    assert stimflow.xor_sorted([2, 2, 2]) == [2]
    assert stimflow.xor_sorted([2, 2, 2, 2]) == []
    assert stimflow.xor_sorted([2, 2, 3]) == [3]
    assert stimflow.xor_sorted([3, 2, 2]) == [3]
    assert stimflow.xor_sorted([2, 3, 2]) == [3]
    assert stimflow.xor_sorted([2, 3, 3]) == [2]
    assert stimflow.xor_sorted([2, 3, 5, 7, 11, 13, 5]) == [2, 3, 7, 11, 13]
