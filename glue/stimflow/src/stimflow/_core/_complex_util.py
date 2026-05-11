from __future__ import annotations

from collections.abc import Callable, Iterable
from typing import Any, cast, TypeVar


def complex_key(c: complex) -> Any:
    return c.real != int(c.real), c.real, c.imag


def sorted_complex(values: Iterable[complex]) -> list[complex]:
    return sorted(values, key=complex_key)


def min_max_complex(
    coords: Iterable[complex], *, default: complex | None = None
) -> tuple[complex, complex]:
    """Computes the bounding box of a collection of complex numbers.

    Args:
        coords: The complex numbers to place a bounding box around.
        default: If no elements are included, the bounding box will cover this
            single value when the collection of complex numbers is empty. If
            this argument isn't set (or is set to None), an exception will be
            raised instead when given an empty collection.

    Returns:
        A pair of complex values (c_min, c_max) where c_min's real component
        where c_min is the minimum corner of the bounding box and c_max is the
        maximum corner of the bounding box.
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
    """
    result = sorted(vals, key=cast(Any, key))
    n = len(result)
    skipped = 0
    k = 0
    while k + 1 < n:
        if result[k] == result[k + 1]:
            skipped += 2
            k += 2
        else:
            result[k - skipped] = result[k]
            k += 1
    if k < n:
        result[k - skipped] = result[k]
    while skipped:
        result.pop()
        skipped -= 1
    return result
