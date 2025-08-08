import collections

import numpy as np
import stim

from sinter._data import Task
from sinter._decoding._decoding_vacuous import VacuousDecoder
from sinter._decoding._stim_then_decode_sampler import \
    classify_discards_and_errors, _CompiledStimThenDecodeSampler


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

def test_detector_post_selection():
    circuit = stim.Circuit("""
        X_ERROR(1) 0
        M 0
        DETECTOR rec[-1]
    """)
    sampler = _CompiledStimThenDecodeSampler(
        decoder=VacuousDecoder(),
        task = Task(
            circuit=circuit,
            detector_error_model=circuit.detector_error_model(),
            postselection_mask=np.array([1], dtype=np.uint8),
        ),
        count_observable_error_combos=False,
        count_detection_events=False,
        tmp_dir=None
    )
    result = sampler.sample(max_shots=1)
    assert result.discards == 1