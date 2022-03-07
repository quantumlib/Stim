import collections

import pytest
import stim

import simmer


def test_iter_collect():
    result = collections.defaultdict(simmer.CaseStats)
    for case, stats in simmer.iter_collect(
        num_workers=2,
        num_todo=4,
        iter_todo=(
            simmer.CollectionCaseTracker(
                circuit=stim.Circuit.generated(
                    'repetition_code:memory',
                    rounds=3,
                    distance=3,
                    after_clifford_depolarization=p),
                name=str(p),
                post_mask=None,
                strong_id="fake",
                start_batch_size=100,
                max_batch_size=1000,
                max_errors=100,
                max_shots=1000,
                decoder='pymatching',
            )
            for p in [0.01, 0.02, 0.03, 0.04]
        ),
        max_shutdown_wait_seconds=10,
        print_progress=False,
    ):
        result[case.name] += stats
    assert len(result) == 4
    for k, v in result.items():
        assert v.num_shots >= 1000 or v.num_errors >= 100
        assert v.num_discards == 0
    assert result["0.01"].num_errors <= 10
    assert result["0.02"].num_errors <= 30
    assert result["0.03"].num_errors <= 70
    assert 1 <= result["0.04"].num_errors <= 100


def test_iter_collect_list():
    result = collections.defaultdict(simmer.CaseStats)
    for case, stats in simmer.iter_collect(
        num_workers=2,
        iter_todo=[
            simmer.CollectionCaseTracker(
                circuit=stim.Circuit.generated(
                    'repetition_code:memory',
                    rounds=3,
                    distance=3,
                    after_clifford_depolarization=p),
                name=str(p),
                post_mask=None,
                strong_id="fake",
                start_batch_size=100,
                max_batch_size=1000,
                max_errors=100,
                max_shots=1000,
                decoder='pymatching',
            )
            for p in [0.01, 0.02, 0.03, 0.04]
        ],
        max_shutdown_wait_seconds=10,
        print_progress=False,
    ):
        result[case.name] += stats
    assert len(result) == 4
    for k, v in result.items():
        assert v.num_shots >= 1000 or v.num_errors >= 100
        assert v.num_discards == 0
    assert result["0.01"].num_errors <= 10
    assert result["0.02"].num_errors <= 30
    assert result["0.03"].num_errors <= 70
    assert 1 <= result["0.04"].num_errors <= 100


def test_iter_collect_worker_fails():
    with pytest.raises(RuntimeError, match="Job failed"):
        _ = list(simmer.iter_collect(
            num_workers=1,
            num_todo=1,
            iter_todo=iter([
                simmer.CollectionCaseTracker(
                    circuit=stim.Circuit.generated('repetition_code:memory', rounds=3, distance=3),
                    name="!",
                    post_mask=None,
                    strong_id="fake",
                    start_batch_size=1,
                    max_batch_size=1,
                    max_errors=1,
                    max_shots=1,
                    decoder='NOT A VALID DECODER',
                ),
            ]),
            max_shutdown_wait_seconds=10,
            print_progress=False,
        ))
