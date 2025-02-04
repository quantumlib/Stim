import collections

import numpy as np

from sinter._decoding._stim_then_decode_sampler import \
    classify_discards_and_errors


def test_classify_discards_and_errors():
    assert classify_discards_and_errors(
        actual_obs=np.array([
            [1, 2],
            [2, 2],
            [3, 2],
            [4, 3],
            [1, 3],
            [0, 3],
            [0, 3],
        ], dtype=np.uint8),
        predictions=np.array([
            [1, 2],
            [2, 2],
            [3, 2],
            [4, 3],
            [1, 3],
            [0, 3],
            [0, 3],
        ], dtype=np.uint8),
        postselected_observables_mask=None,
        out_count_observable_error_combos=None,
        num_obs=16,
    ) == (0, 0)

    assert classify_discards_and_errors(
        actual_obs=np.array([
            [1, 2],
            [2, 2],
            [3, 2],
            [4, 3],
            [1, 3],
            [0, 3],
            [0, 3],
        ], dtype=np.uint8),
        predictions=np.array([
            [0, 0],
            [2, 2],
            [3, 2],
            [4, 1],
            [1, 3],
            [0, 3],
            [0, 3],
        ], dtype=np.uint8),
        postselected_observables_mask=None,
        out_count_observable_error_combos=None,
        num_obs=16,
    ) == (0, 2)

    assert classify_discards_and_errors(
        actual_obs=np.array([
            [1, 2],
            [2, 2],
            [3, 2],
            [4, 3],
            [1, 3],
            [0, 3],
            [0, 3],
        ], dtype=np.uint8),
        predictions=np.array([
            [0, 0, 0],
            [2, 2, 0],
            [3, 2, 0],
            [4, 1, 0],
            [1, 3, 0],
            [0, 3, 0],
            [0, 3, 0],
        ], dtype=np.uint8),
        postselected_observables_mask=None,
        out_count_observable_error_combos=None,
        num_obs=16,
    ) == (0, 2)

    assert classify_discards_and_errors(
        actual_obs=np.array([
            [1, 2],
            [2, 2],
            [3, 2],
            [4, 3],
            [1, 3],
            [0, 3],
            [0, 3],
        ], dtype=np.uint8),
        predictions=np.array([
            [0, 0, 0],
            [2, 2, 1],
            [3, 2, 0],
            [4, 1, 0],
            [1, 3, 0],
            [0, 3, 0],
            [0, 3, 0],
        ], dtype=np.uint8),
        postselected_observables_mask=None,
        out_count_observable_error_combos=None,
        num_obs=16,
    ) == (1, 2)

    assert classify_discards_and_errors(
        actual_obs=np.array([
            [1, 2],
            [2, 2],
            [3, 2],
            [4, 3],
            [1, 3],
            [0, 3],
            [0, 3],
        ], dtype=np.uint8),
        predictions=np.array([
            [0, 0, 1],
            [2, 2, 0],
            [3, 2, 0],
            [4, 1, 0],
            [1, 3, 0],
            [0, 3, 0],
            [0, 3, 0],
        ], dtype=np.uint8),
        postselected_observables_mask=None,
        out_count_observable_error_combos=None,
        num_obs=16,
    ) == (1, 1)

    assert classify_discards_and_errors(
        actual_obs=np.array([
            [1, 2],
            [2, 2],
            [3, 2],
            [4, 3],
            [1, 3],
            [0, 3],
            [0, 3],
        ], dtype=np.uint8),
        predictions=np.array([
            [0, 0, 1],
            [2, 2, 1],
            [3, 2, 0],
            [4, 1, 0],
            [1, 3, 0],
            [0, 3, 0],
            [0, 3, 0],
        ], dtype=np.uint8),
        postselected_observables_mask=None,
        out_count_observable_error_combos=None,
        num_obs=16,
    ) == (2, 1)

    assert classify_discards_and_errors(
        actual_obs=np.array([
            [1, 2],
            [2, 2],
            [3, 2],
            [4, 3],
            [1, 3],
            [2, 3],
            [1, 3],
        ], dtype=np.uint8),
        predictions=np.array([
            [0, 0, 1],
            [2, 2, 1],
            [3, 2, 0],
            [4, 1, 0],
            [1, 3, 0],
            [0, 3, 0],
            [0, 3, 0],
        ], dtype=np.uint8),
        postselected_observables_mask=np.array([1, 0]),
        out_count_observable_error_combos=None,
        num_obs=16,
    ) == (3, 2)

    counter = collections.Counter()
    assert classify_discards_and_errors(
        actual_obs=np.array([
            [1, 2],
            [1, 2],
        ], dtype=np.uint8),
        predictions=np.array([
            [1, 0],
            [1, 2],
        ], dtype=np.uint8),
        postselected_observables_mask=np.array([1, 0]),
        out_count_observable_error_combos=counter,
        num_obs=13,
    ) == (0, 1)
    assert counter == collections.Counter(["obs_mistake_mask=_________E___"])
