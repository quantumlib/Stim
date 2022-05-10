import collections

import pytest
import stim

import sinter
from sinter.anon_task_stats import AnonTaskStats


def test_iter_collect():
    result = collections.defaultdict(AnonTaskStats)
    for sample in sinter.iter_collect(
        num_workers=2,
        tasks=[
            sinter.Task(
                circuit=stim.Circuit.generated(
                    'repetition_code:memory',
                    rounds=3,
                    distance=3,
                    after_clifford_depolarization=p),
                decoder='pymatching',
                json_metadata={'p': p},
                max_shots=1000,
                max_errors=100,
                start_batch_size=100,
                max_batch_size=1000,
            )
            for p in [0.01, 0.02, 0.03, 0.04]
        ],
        print_progress=False,
    ):
        result[sample.json_metadata['p']] += sample.to_anon_stats()
    assert len(result) == 4
    for k, v in result.items():
        assert v.shots >= 1000 or v.errors >= 100
        assert v.discards == 0
    assert result[0.01].errors <= 10
    assert result[0.02].errors <= 30
    assert result[0.03].errors <= 70
    assert 1 <= result[0.04].errors <= 100


def test_collect():
    results = sinter.collect(
        num_workers=2,
        tasks=[
            sinter.Task(
                circuit=stim.Circuit.generated(
                    'repetition_code:memory',
                    rounds=3,
                    distance=3,
                    after_clifford_depolarization=p),
                decoder='pymatching',
                json_metadata={'p': p},
                max_shots=1000,
                max_errors=100,
                start_batch_size=100,
                max_batch_size=1000,
            )
            for p in [0.01, 0.02, 0.03, 0.04]
        ]
    )
    probabilities = [e.json_metadata['p'] for e in results]
    assert len(probabilities) == len(set(probabilities))
    d = {e.json_metadata['p']: e for e in results}
    assert len(d) == 4
    for k, v in d.items():
        assert v.shots >= 1000 or v.errors >= 100
        assert v.discards == 0
    assert d[0.01].errors <= 10
    assert d[0.02].errors <= 30
    assert d[0.03].errors <= 70
    assert 1 <= d[0.04].errors <= 100


def test_iter_collect_list():
    result = collections.defaultdict(AnonTaskStats)
    for sample in sinter.iter_collect(
        num_workers=2,
        tasks=[
            sinter.Task(
                circuit=stim.Circuit.generated(
                    'repetition_code:memory',
                    rounds=3,
                    distance=3,
                    after_clifford_depolarization=p),
                decoder='pymatching',
                json_metadata={'p': p},
                max_errors=100,
                max_shots=1000,
                start_batch_size=100,
                max_batch_size=1000,
            )
            for p in [0.01, 0.02, 0.03, 0.04]
        ],
        print_progress=False,
    ):
        result[sample.json_metadata['p']] += sample.to_anon_stats()
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
        _ = list(sinter.iter_collect(
            num_workers=1,
            tasks=iter([
                sinter.Task(
                    circuit=stim.Circuit.generated('repetition_code:memory', rounds=3, distance=3),
                    decoder='NOT A VALID DECODER',
                    max_errors=1,
                    max_shots=1,
                ),
            ]),
            print_progress=False,
        ))
