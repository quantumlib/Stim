import numpy as np
import pytest

from stimside.util.alias_table import build_alias_table, sample_from_alias_table

class TestBuildAliasTable:
    def test_build_alias_table_len2(self):
        elements = [(1, 0.1), (2, 0.9)]
        expected_bases = np.array([1, 2])
        expected_aliases = np.array([2, 2])
        expected_thresholds = np.array([0.2, 1.0])
        got_bases, got_aliases, got_thresholds = build_alias_table(elements)
        assert (got_bases == expected_bases).all()
        assert (got_aliases == expected_aliases).all()
        assert np.isclose(got_thresholds, expected_thresholds).all()

    def test_build_alias_table_len4(self):
        elements = [(11, 0.1), (22, 0.2), (33, 0.3), (44, 0.4)]
        expected_bases = np.array([11, 22, 44, 33])
        expected_aliases = np.array([44, 33, 33, 33])
        expected_thresholds = np.array([0.1 / 0.25, 0.2 / 0.25, 1.0, 1.0])
        got_bases, got_aliases, got_thresholds = build_alias_table(elements)
        assert (got_bases == expected_bases).all()
        assert (got_aliases == expected_aliases).all()
        assert np.isclose(got_thresholds, expected_thresholds).all()

    def test_build_alias_table_uniform(self):
        elements = [(1, 0.25), (2, 0.25), (3, 0.25), (4, 0.25)]
        expected_bases = np.array([1, 2, 3, 4])
        expected_aliases = np.array([4, 4, 4, 4])
        expected_thresholds = np.array([1.0, 1.0, 1.0, 1.0])
        got_bases, got_aliases, got_thresholds = build_alias_table(elements)
        assert (got_bases == expected_bases).all()
        assert (got_aliases == expected_aliases).all()
        assert np.isclose(got_thresholds, expected_thresholds).all()

    def test_build_alias_table_string_dtype(self):
        elements = [("a", 0.01), ("b", 0.05), ("c", 0.2), ("d", 0.2), ("e", 0.54)]
        expected_bases = np.array(["a", "b", "c", "d", "e"])
        expected_aliases = np.array(["e", "e", "e", "e", "e"])
        expected_thresholds = np.array([0.01 / 0.2, 0.05 / 0.2, 1.0, 1.0, 1.0])
        got_bases, got_aliases, got_thresholds = build_alias_table(elements)
        assert (got_bases == expected_bases).all()
        assert (got_aliases == expected_aliases).all()
        assert np.isclose(got_thresholds, expected_thresholds).all()

    def test_build_alias_table_zero_probs(self):
        elements = [(1, 0.25), (2, 0.75), (3, 0.0), (4, 0.0)]
        expected_bases = np.array([1, 2])
        expected_aliases = np.array([2, 2])
        expected_thresholds = np.array([0.5, 1.0])
        got_bases, got_aliases, got_thresholds = build_alias_table(elements)
        print(got_bases, got_aliases, got_thresholds)
        assert (got_bases == expected_bases).all()
        assert (got_aliases == expected_aliases).all()
        assert np.isclose(got_thresholds, expected_thresholds).all()

    def test_build_alias_table_noncomplete(self):
        elements = [(1, 0.25), (2, 0.25)]
        with pytest.raises(ValueError, match="complete set of disjoint probabilities"):
            build_alias_table(elements)

    def test_build_alias_table_nonprobability(self):
        elements = [(1, 0.25), (2, 1.25)]
        with pytest.raises(ValueError, match="probabilities outside 0<=p<=1."):
            build_alias_table(elements)


class TestSampleFromAliasTable:
    def test_threshold_behavior(self):
        num_samples = 100
        np_rng = np.random.default_rng(seed=0)

        # Test case 1: Thresholds = 0 (should always pick alias)
        bases_0 = np.array([1, 2, 3])
        aliases_0 = np.array([4, 5, 6])
        thresholds_0 = np.array([0.0, 0.0, 0.0])
        samples_0 = sample_from_alias_table(num_samples, bases_0, aliases_0, thresholds_0, np_rng)
        # Since rands are in [0, 1), they will never be < 0.
        # So it should always pick the alias.
        assert all(s in aliases_0 for s in samples_0)
        assert not any(s in bases_0 for s in samples_0)

        # Test case 2: Thresholds = 1 (should always pick base)
        bases_1 = np.array([1, 2, 3])
        aliases_1 = np.array([4, 5, 6])
        thresholds_1 = np.array([1.0, 1.0, 1.0])
        samples_1 = sample_from_alias_table(num_samples, bases_1, aliases_1, thresholds_1, np_rng)
        # Since rands are in [0, 1), they will always be < 1.
        # So it should always pick the base.
        assert all(s in bases_1 for s in samples_1)
        assert not any(s in aliases_1 for s in samples_1)

    def test_output_shape(self):
        num_samples = 50
        np_rng = np.random.default_rng(seed=1)
        thresholds = np.array([0.5, 0.5])

        # Test with 1D elements
        bases_1d = np.array([1, 2])
        aliases_1d = np.array([3, 4])
        samples_1d = sample_from_alias_table(num_samples, bases_1d, aliases_1d, thresholds, np_rng)
        assert samples_1d.shape == (num_samples,)

        # Test with 2D elements
        bases_2d = np.array([[1, 1], [2, 2]])
        aliases_2d = np.array([[3, 3], [4, 4]])
        samples_2d = sample_from_alias_table(num_samples, bases_2d, aliases_2d, thresholds, np_rng)
        assert samples_2d.shape == (num_samples, 2)

        # Test with 3D elements
        bases_3d = np.array([[[1, 1], [1, 1]], [[2, 2], [2, 2]]])
        aliases_3d = np.array([[[3, 3], [3, 3]], [[4, 4], [4, 4]]])
        samples_3d = sample_from_alias_table(num_samples, bases_3d, aliases_3d, thresholds, np_rng)
        assert samples_3d.shape == (num_samples, 2, 2)

    def test_statistical_correctness(self):
        num_samples = 10000
        np_rng = np.random.default_rng(seed=2)

        # 50/50 split between two outcomes
        elements = [(0, 0.5), (1, 0.5)]
        bases, aliases, thresholds = build_alias_table(elements)

        samples = sample_from_alias_table(num_samples, bases, aliases, thresholds, np_rng)

        # Check the distribution
        counts = np.bincount(samples)
        assert len(counts) == 2
        # Check if the count of 0s is close to 5000
        assert 4800 < counts[0] < 5200
        # Check if the count of 1s is close to 5000
        assert 4800 < counts[1] < 5200

        # Test with a more complex distribution
        elements = [(0, 0.1), (1, 0.2), (2, 0.7)]
        bases, aliases, thresholds = build_alias_table(elements)
        samples = sample_from_alias_table(num_samples, bases, aliases, thresholds, np_rng)
        counts = np.bincount(samples)
        assert len(counts) == 3
        assert 800 < counts[0] < 1200  # 1000 +/- 200
        assert 1800 < counts[1] < 2200  # 2000 +/- 200
        assert 6800 < counts[2] < 7200  # 7000 +/- 200

    def test_arbitrary_object_sampling(self):
        num_samples = 100
        np_rng = np.random.default_rng(seed=3)

        elements = [("a", 0.5), (1, 0.5)]
        bases, aliases, thresholds = build_alias_table(elements)

        samples = sample_from_alias_table(num_samples, bases, aliases, thresholds, np_rng)

        # Check that the output contains the expected elements
        assert all(s in ["a", 1] for s in samples)
        # Check that both types are present
        assert any(isinstance(s, str) for s in samples)
        assert any(isinstance(s, int) for s in samples)
        # Check that the dtype=object
        assert samples.dtype == object

    def test_edge_cases(self):
        np_rng = np.random.default_rng(seed=4)

        # Test with num_samples = 0
        bases = np.array([1, 2])
        aliases = np.array([3, 4])
        thresholds = np.array([0.5, 0.5])
        samples = sample_from_alias_table(0, bases, aliases, thresholds, np_rng)
        assert samples.shape == (0,)

        # Test with a single entry table
        elements = [(1, 1.0)]
        bases, aliases, thresholds = build_alias_table(elements)
        samples = sample_from_alias_table(100, bases, aliases, thresholds, np_rng)
        assert all(s == 1 for s in samples)

    def test_determinism(self):
        num_samples = 100
        elements = [(0, 0.25), (1, 0.75)]
        bases, aliases, thresholds = build_alias_table(elements)

        # Create two separate generators with the same seed
        np_rng_1 = np.random.default_rng(seed=5)
        np_rng_2 = np.random.default_rng(seed=5)

        samples_1 = sample_from_alias_table(num_samples, bases, aliases, thresholds, np_rng_1)
        samples_2 = sample_from_alias_table(num_samples, bases, aliases, thresholds, np_rng_2)

        assert np.array_equal(samples_1, samples_2)
