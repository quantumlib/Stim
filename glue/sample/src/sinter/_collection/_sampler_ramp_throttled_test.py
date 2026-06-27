from unittest import mock

import pytest

import sinter
import stim
from sinter._collection._sampler_ramp_throttled import (
    CompiledRampThrottledSampler,
    RampThrottledSampler,
)
from sinter._data import AnonTaskStats, Task


class MockSampler(sinter.Sampler, sinter.CompiledSampler):
    """Mock sampler that tracks `suggested_shots` parameter in `sample` calls."""

    def __init__(self):
        self.calls = []

    def compiled_sampler_for_task(self, task: Task) -> sinter.CompiledSampler:
        return self

    def sample(self, suggested_shots: int) -> AnonTaskStats:
        self.calls.append(suggested_shots)
        return AnonTaskStats(
            shots=suggested_shots,
            errors=1,
            seconds=0.001 * suggested_shots,  # Simulate time proportional to shots
        )


@pytest.fixture
def mock_sampler():
    return MockSampler()


def test_initial_batch_size(mock_sampler):
    """Test that the sampler starts with a batch size of 1."""
    sampler = CompiledRampThrottledSampler(
        sub_sampler=mock_sampler,
        target_batch_seconds=1.0,
        max_batch_shots=1024,
    )

    # First call should use batch_size=1
    sampler.sample(100)
    assert mock_sampler.calls[0] == 1


def test_batch_size_ramps_up(mock_sampler):
    """Test that the batch size increases when execution is fast."""
    sampler = CompiledRampThrottledSampler(
        sub_sampler=mock_sampler,
        target_batch_seconds=1.0,
        max_batch_shots=1024,
    )

    # Mock time.monotonic to simulate fast execution
    # two calls per sample for tic/toc
    with mock.patch(
        "time.monotonic", side_effect=[0.0, 0.001, 0.02, 0.021, 0.03, 0.031]
    ):
        sampler.sample(100)  # First call, batch_size=1
        sampler.sample(100)  # Should double 4 times to 16
        sampler.sample(100)  # Should double 4 times again but hit limit of 100

    assert mock_sampler.calls == [1, 16, 100]


def test_batch_size_decreases(mock_sampler):
    """Test that the batch size decreases when execution is slow."""
    sampler = CompiledRampThrottledSampler(
        sub_sampler=mock_sampler,
        target_batch_seconds=0.1,
        max_batch_shots=1024,
    )

    # Set initial batch size higher for this test
    sampler.batch_shots = 64

    # Mock time.monotonic to simulate slow execution (>1.3x target)
    with mock.patch("time.monotonic", side_effect=[0.0, 0.15, 0.5, 0.65]):
        sampler.sample(100)  # First call, batch_size=64
        sampler.sample(100)  # Should halve to 32

    assert mock_sampler.calls == [64, 32]


def test_respects_max_batch_shots(mock_sampler):
    """Test that the batch size never exceeds max_batch_shots."""
    sampler = CompiledRampThrottledSampler(
        sub_sampler=mock_sampler,
        target_batch_seconds=1.0,
        max_batch_shots=16,  # Small max for testing
    )

    # Set initial batch size close to max
    sampler.batch_shots = 8

    # Mock time.monotonic to simulate very fast execution
    # two calls per sample for tic/toc
    with mock.patch(
        "time.monotonic", side_effect=[0.0, 0.001, 0.02, 0.021, 0.03, 0.031]
    ):
        sampler.sample(100)  # First call, batch_size=8
        sampler.sample(100)  # Should double to 16
        sampler.sample(100)  # Should stay at 16 (max)

    assert mock_sampler.calls == [8, 16, 16]


def test_respects_max_shots_parameter(mock_sampler):
    """Test that the sampler respects the max_shots parameter."""
    sampler = CompiledRampThrottledSampler(
        sub_sampler=mock_sampler,
        target_batch_seconds=1.0,
        max_batch_shots=1024,
    )

    # Set batch size higher than max_shots
    sampler.batch_shots = 100

    # Call with max_shots=10
    sampler.sample(10)

    # Should only request 10 shots, not 100
    assert mock_sampler.calls[0] == 10


def test_sub_sampler_parameter_pass_through(mock_sampler):
    """Test that parameters are passed through to compiled sub sampler."""
    factory = RampThrottledSampler(
        sub_sampler=mock_sampler,
        target_batch_seconds=0.5,
        max_batch_shots=512,
    )

    task = Task(circuit=stim.Circuit(), decoder="test")
    compiled = factory.compiled_sampler_for_task(task)

    assert isinstance(compiled, CompiledRampThrottledSampler)
    assert compiled.target_batch_seconds == 0.5
    assert compiled.max_batch_shots == 512
    assert compiled.batch_shots == 1  # Initial batch size
