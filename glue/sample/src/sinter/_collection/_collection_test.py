import collections
import math
import pathlib
import sys
import tempfile
import time

import pytest
import stim

import sinter


def test_iter_collect():
    result = collections.defaultdict(sinter.AnonTaskStats)
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
                collection_options=sinter.CollectionOptions(
                    max_shots=1000,
                    max_errors=100,
                    start_batch_size=100,
                    max_batch_size=1000,
                ),
            )
            for p in [0.01, 0.02, 0.03, 0.04]
        ],
    ):
        for stats in sample.new_stats:
            result[stats.json_metadata['p']] += stats.to_anon_stats()
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
                collection_options=sinter.CollectionOptions(
                    max_shots=1000,
                    max_errors=100,
                    start_batch_size=100,
                    max_batch_size=1000,
                ),
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


def test_collect_from_paths():
    with tempfile.TemporaryDirectory() as d:
        d = pathlib.Path(d)
        tasks = []
        for p in [0.01, 0.02, 0.03, 0.04]:
            path = d / f'tmp{p}.stim'
            stim.Circuit.generated(
                'repetition_code:memory',
                rounds=3,
                distance=3,
                after_clifford_depolarization=p,
            ).to_file(path)
            tasks.append(sinter.Task(
                circuit_path=path,
                decoder='pymatching',
                json_metadata={'p': p},
                collection_options=sinter.CollectionOptions(
                    max_shots=1000,
                    max_errors=100,
                    start_batch_size=100,
                    max_batch_size=1000,
                ),
            ))

        results = sinter.collect(
            num_workers=2,
            tasks=tasks
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


class AlternatingPredictionsDecoder(sinter.Decoder):
    def decode_via_files(self,
                         *,
                         num_shots: int,
                         num_dets: int,
                         num_obs: int,
                         dem_path: pathlib.Path,
                         dets_b8_in_path: pathlib.Path,
                         obs_predictions_b8_out_path: pathlib.Path,
                         tmp_dir: pathlib.Path,
                       ) -> None:
        bytes_per_shot = (num_obs + 7) // 8
        with open(obs_predictions_b8_out_path, 'wb') as f:
            for k in range(num_shots):
                f.write((k % 3 == 0).to_bytes(length=bytes_per_shot, byteorder='little'))


def test_collect_custom_decoder():
    results = sinter.collect(
        num_workers=2,
        tasks=[
            sinter.Task(
                circuit=stim.Circuit("""
                    M(0.1) 0
                    DETECTOR rec[-1]
                    OBSERVABLE_INCLUDE(0) rec[-1]
                """),
                json_metadata=None,
            )
        ],
        max_shots=10000,
        decoders=['alternate'],
        custom_decoders={'alternate': AlternatingPredictionsDecoder()},
    )
    assert len(results) == 1
    assert results[0].shots == 10000
    assert 2500 < results[0].errors < 4000


def test_iter_collect_list():
    result = collections.defaultdict(sinter.AnonTaskStats)
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
                collection_options=sinter.CollectionOptions(
                    max_errors=100,
                    max_shots=1000,
                    start_batch_size=100,
                    max_batch_size=1000,
                ),
            )
            for p in [0.01, 0.02, 0.03, 0.04]
        ],
    ):
        for stats in sample.new_stats:
            result[stats.json_metadata['p']] += stats.to_anon_stats()
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
            decoders=['NOT A VALID DECODER'],
            num_workers=1,
            tasks=iter([
                sinter.Task(
                    circuit=stim.Circuit.generated('repetition_code:memory', rounds=3, distance=3),
                    collection_options=sinter.CollectionOptions(
                        max_errors=1,
                        max_shots=1,
                    ),
                ),
            ]),
        ))


class FixedSizeSampler(sinter.Sampler, sinter.CompiledSampler):
    def compiled_sampler_for_task(self, task: sinter.Task) -> sinter.CompiledSampler:
        return self

    def sample(self, suggested_shots: int) -> 'sinter.AnonTaskStats':
        return sinter.AnonTaskStats(
            shots=1024,
            errors=5,
        )


def test_fixed_size_sampler():
    results = sinter.collect(
        num_workers=2,
        tasks=[
            sinter.Task(
                circuit=stim.Circuit(),
                decoder='fixed_size_sampler',
                json_metadata={},
                collection_options=sinter.CollectionOptions(
                    max_shots=100_000,
                    max_errors=1_000,
                ),
            )
        ],
        custom_decoders={'fixed_size_sampler': FixedSizeSampler()}
    )
    assert 100_000 <= results[0].shots <= 100_000 + 3000


class MockTimingSampler(sinter.Sampler, sinter.CompiledSampler):
    def compiled_sampler_for_task(self, task: sinter.Task) -> sinter.CompiledSampler:
        return self

    def sample(self, suggested_shots: int) -> 'sinter.AnonTaskStats':
        actual_shots = -(-suggested_shots // 1024) * 1024
        time.sleep(actual_shots * 0.00001)
        return sinter.AnonTaskStats(
            shots=actual_shots,
            errors=5,
            seconds=actual_shots * 0.00001,
        )


def test_mock_timing_sampler():
    results = sinter.collect(
        num_workers=12,
        tasks=[
            sinter.Task(
                circuit=stim.Circuit(),
                decoder='MockTimingSampler',
                json_metadata={},
            )
        ],
        max_shots=1_000_000,
        max_errors=10_000,
        custom_decoders={'MockTimingSampler': MockTimingSampler()},
    )
    assert 1_000_000 <= results[0].shots <= 1_000_000 + 12000
