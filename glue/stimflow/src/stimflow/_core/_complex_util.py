from __future__ import annotations

from collections.abc import Callable, Iterable
from typing import Any, cast, TypeVar


def sorted_complex(values: Iterable[complex]) -> list[complex]:
    """Sorts complex numbers by real then imaginary coordinate.

    Args:
        values: The complex numbers to sort.

    Returns:
        The sorted list.

    Examples:
        >>> import stimflow as sf
        >>> sf.sorted_complex([0, 1, 1j, 1 + 1j])
        [0, 1j, 1, (1+1j)]
    """
    return sorted(values, key=lambda e: (e.real, e.imag))


def min_max_complex(
    coords: Iterable[complex], *, default: complex | None = None
) -> tuple[complex, complex]:
    """Computes the bounding box of a collection of complex numbers.

    Args:
        coords: The complex numbers to place a bounding box around.
        default: If no elements are included, the returned minimum and maximum
            will be equal to this value. If this argument isn't set (or is set to None),
            an exception will be raised instead when given an empty collection. The
            default value is not used when coords is not empty.

    Returns:
        A pair of complex values (c_min, c_max) where c_min is the minimum corner of
        the bounding box and c_max is the maximum corner of the bounding box.

    Raises:
        ValueError:
            An empty list of coords was given, and a default value wasn't specified.
    Examples:
        >>> import stimflow as sf
        >>> sf.min_max_complex([1+2j, 2+1j])
        ((1+1j), (2+2j))
        >>> sf.min_max_complex([1+2j, 2+1j, 1+3j])
        ((1+1j), (2+3j))
        >>> sf.min_max_complex([], default=4+3j)
        ((4+3j), (4+3j))
        >>> sf.min_max_complex([1])
        ((1+0j), (1+0j))
        >>> sf.min_max_complex([1, 3, 2])
        ((1+0j), (3+0j))
        >>> sf.min_max_complex([2j, 1j, 3j])
        (1j, 3j)
    """
    coords = list(coords)
    if not coords and default is not None:
        return default, default
    real = [c.real for c in coords]
    imag = [c.imag for c in coords]
    min_r = min(real)
    min_i = min(imag)
    max_r = max(real)
    max_i = max(imag)
    return min_r + min_i * 1j, max_r + max_i * 1j


TItem = TypeVar("TItem")


def xor_sorted(vals: Iterable[TItem], *, key: Callable[[TItem], Any] | None = None) -> list[TItem]:
    """Sorts items and then cancels pairs of equal items.

    An item will be in the result once if it appeared an odd number of times.
    An item won't be in the result if it appeared an even number of times.

    Args:
        vals: The items to sort.
        key: An optional key function, mapping the items to keys that determine the
            sorted order. Unequal items with the same key don't cancel.

    Examples:
        >>> import stimflow as sf
        >>> sf.xor_sorted([1])
        [1]
        >>> sf.xor_sorted([1, 1])
        []
        >>> sf.xor_sorted([1, 1, 1])
        [1]
        >>> sf.xor_sorted([1, 1, 1, 1])
        []
        >>> sf.xor_sorted([3, 1, 2, 1])
        [2, 3]
        >>> sf.xor_sorted([3, 1, 2, 1, 3])
        [2]
        >>> sf.xor_sorted([5, 4, 3, 2, 1, 4])
        [1, 2, 3, 5]
        >>> sf.xor_sorted([*range(10), *range(2, 6)])
        [0, 1, 6, 7, 8, 9]
        >>> sf.xor_sorted([61, 91, 83, 72, 61], key=lambda e: e % 10)
        [91, 72, 83]
    """
    kept = set()
    for v in vals:
        if v in kept:
            kept.remove(v)
        else:
            kept.add(v)

    seen = set()
    filtered = []
    for v in vals:
        if v in kept and v not in seen:
            seen.add(v)
            filtered.append(v)

    return sorted(filtered, key=cast(Any, key))
