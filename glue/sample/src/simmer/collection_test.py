import collections

import pytest
import stim

import simmer


def test_iter_collect():
    result = collections.defaultdict(simmer.CaseStats)
    for case, stats in simmer.iter_collect(
        num_workers=2,
        start_batch_size=100,
        max_batch_size=1000,
        goals=[
            simmer.CaseGoal(
                case=simmer.CaseExecutable(
                    circuit=stim.Circuit.generated(
                        'repetition_code:memory',
                        rounds=3,
                        distance=3,
                        after_clifford_depolarization=p),
                    decoder='pymatching',
                    custom={'p': p},
                ),
                max_shots=1000,
                max_errors=100,
            )
            for p in [0.01, 0.02, 0.03, 0.04]
        ],
        max_shutdown_wait_seconds=10,
        print_progress=False,
    ):
        result[case.custom['p']] += stats
    assert len(result) == 4
    for k, v in result.items():
        assert v.shots >= 1000 or v.errors >= 100
        assert v.discards == 0
    assert result[0.01].errors <= 10
    assert result[0.02].errors <= 30
    assert result[0.03].errors <= 70
    assert 1 <= result[0.04].errors <= 100


def test_iter_collect_list():
    result = collections.defaultdict(simmer.CaseStats)
    for case, stats in simmer.iter_collect(
        num_workers=2,
        start_batch_size=100,
        max_batch_size=1000,
        goals=[
            simmer.CaseGoal(
                case=simmer.CaseExecutable(
                    circuit=stim.Circuit.generated(
                        'repetition_code:memory',
                        rounds=3,
                        distance=3,
                        after_clifford_depolarization=p),
                    decoder='pymatching',
                    custom={'p': p},
                ),
                max_errors=100,
                max_shots=1000,
            )
            for p in [0.01, 0.02, 0.03, 0.04]
        ],
        max_shutdown_wait_seconds=10,
        print_progress=False,
    ):
        result[case.custom['p']] += stats
    assert len(result) == 4
    for k, v in result.items():
        assert v.shots >= 1000 or v.errors >= 100
        assert v.discards == 0
    assert result[0.01].errors <= 10
    assert result[0.02].errors <= 30
    assert result[0.03].errors <= 70
    assert 1 <= result[0.04].errors <= 100


def test_iter_collect_worker_fails():
    with pytest.raises(RuntimeError, match="Worker failed"):
        _ = list(simmer.iter_collect(
            num_workers=1,
            goals=iter([
                simmer.CaseGoal(
                    case=simmer.CaseExecutable(
                        circuit=stim.Circuit.generated('repetition_code:memory', rounds=3, distance=3),
                        decoder='NOT A VALID DECODER',
                    ),
                    max_errors=1,
                    max_shots=1,
                ),
            ]),
            start_batch_size=1,
            max_batch_size=1,
            max_shutdown_wait_seconds=10,
            print_progress=False,
        ))
