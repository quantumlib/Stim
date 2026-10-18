import math
import random

import numpy as np
import pytest

from stimside.op_handlers.leakage_handlers.leakage_parameters import (
    LeakageTransition1Params,
    LeakageTransition2Params,
    LeakageTransitionZParams,
)
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

    def test_build_alias_table_total_off_by_rounding(self):
        # LEAKAGE_TRANSITION_2 (0.1, 2_U-->3_X) (0.13, 2_U-->3_Y) (0.11, 2_U-->3_Z) plus its
        # remainder 1 - sum: with Python >= 3.12's compensated sum() these total 1 - 1 ulp.
        # The other two total 1 - 1 ulp and 1 + 1 ulp in any Python.
        probs = [((3, "X"), 0.1), ((3, "Y"), 0.13), ((3, "Z"), 0.11)]
        for elements in (
            [*probs, ((2, "U"), 1 - sum(p for _, p in probs))],
            [(1, 0.5), (2, 0.5 - 2**-53)],
            [(1, 0.5), (2, 0.5 + 2**-52)],
        ):
            got = _implied_distribution(build_alias_table(elements))
            assert got == pytest.approx({_key(e): p for e, p in elements}, abs=1e-15)

    @pytest.mark.parametrize(
        "elements",
        [
            [(1, 0.7), (2, 0.7)],
            [(1, 0.5), (2, 0.49998)],
            [(1, 0.25), (2, 0.75002)],
            # Off 1 by more than rounding, though np.isclose to it.
            [(1, 0.25), (2, 0.750005)],
            [(1, 0.5), (2, 0.5 + 1e-9)],
            [(1, 0.5), (2, 0.5 - 1e-9)],
            [(1, 0.5), (2, float("nan"))],
        ],
    )
    def test_build_alias_table_total_off_by_more_than_rounding_raises(self, elements):
        with pytest.raises(ValueError, match="complete set of disjoint probabilities"):
            build_alias_table(elements)


def _key(element):
    """An element or table entry (an int, a str, or a tuple / numpy row of them) as a tuple of str."""
    return tuple(str(v) for v in np.atleast_1d(np.asarray(element, dtype=object)).tolist())


def _implied_distribution(table):
    """The distribution sample_from_alias_table draws from, keyed by _key."""
    bases, aliases, thresholds = table
    n = len(thresholds)
    dist = {}
    for base, alias, t in zip(bases, aliases, thresholds):
        dist[_key(base)] = dist.get(_key(base), 0.0) + t / n
        dist[_key(alias)] = dist.get(_key(alias), 0.0) + (1 - t) / n
    return dist


def test_transition_params_build_alias_tables_for_totals_off_1_by_rounding():
    # Randomized grid over LEAKAGE_TRANSITION_1 / _2 / _Z argument sets from one input state:
    # decimals (summing to <= 1, or to exactly 1 in decimal), floats (scaled to a total <= 1,
    # or normalized to 1), and totals above 1 by rounding (accepted), by more than rounding
    # though np.isclose to 1, or beyond np.isclose (both rejected with the validation error).
    # Each accepted set must sample transition i with p_i (p_i / total if total > 1) and stay
    # with max(0, 1 - total).
    rng = random.Random(2026)
    one_q = ["U", 2, 3, 4, 5, 6, 7, 8, 9]
    leg_out = ["U", "X", "Y", "Z", "D", 0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
    num_accepted = num_rejected_np_isclose = 0
    for trial in range(1400):
        mode = trial % 7
        k = rng.randint(2 if mode >= 4 else 1, 5)
        if mode == 0:
            d = rng.choice([2, 3, 4, 6])
            ps = [round(rng.random() / k, d) for _ in range(k)]
        elif mode in (1, 4, 5, 6):
            d = rng.choice([1, 2, 3, 4])
            cuts = sorted(rng.sample(range(1, 10**d), k - 1))
            ps = [(b - a) / 10**d for a, b in zip([0, *cuts], [*cuts, 10**d])]
            i = ps.index(max(ps))
            if mode == 4:
                for _ in range(rng.randint(1, 3)):
                    ps[i] = math.nextafter(ps[i], 2.0)
            if mode == 5:
                ps[i] += rng.choice([1e-12, 1e-9, 1e-6, 9e-6])
            if mode == 6:
                ps[0] += rng.choice([2e-5, 1e-3, 0.4])
        else:
            u = [rng.random() for _ in range(k)]
            scale = rng.random() / sum(u) if mode == 2 else 1 / sum(u)
            ps = [x * scale for x in u]
        kind = rng.choice(["LT1", "LT2", "LTZ"])
        if kind == "LT2":
            s0 = (rng.choice(one_q), rng.choice(one_q))
            outs = list(dict.fromkeys((rng.choice(leg_out), rng.choice(leg_out)) for _ in range(40)))
            outs = [o for o in outs if o != s0][:k]
            cls = LeakageTransition2Params
        else:
            states = one_q if kind == "LT1" else list(range(10))
            s0 = rng.choice(states)
            outs = rng.sample([s for s in states if s != s0], k)
            cls = LeakageTransition1Params if kind == "LT1" else LeakageTransitionZParams
        args = tuple((p, s0, out) for p, out in zip(ps, outs))
        total = sum(p for p, _, _ in args)
        if total > 1 and not np.isclose(total, 1):
            with pytest.raises(ValueError, match="total probability"):
                cls(args=args, from_tag="grid")
            continue
        if mode == 5:
            with pytest.raises(ValueError, match="total probability"):
                cls(args=args, from_tag="grid")
            num_rejected_np_isclose += 1
            continue
        got = _implied_distribution(cls(args=args, from_tag="grid").alias_table_by_input_state[s0])
        want = {_key(out): p / max(total, 1.0) for p, _, out in args}
        want[_key(s0)] = want.get(_key(s0), 0.0) + max(0.0, 1 - total)
        for key in set(got) | set(want):
            assert abs(got.get(key, 0.0) - want.get(key, 0.0)) <= 1e-12, (kind, args)
        num_accepted += 1
    assert num_accepted > 900 and num_rejected_np_isclose > 150


@pytest.mark.parametrize("kind", ["LT1", "LT2", "LTZ"])
def test_transition_params_validate_totals_above_1_with_the_alias_table_tolerance(kind):
    # A total above 1 passes the params' validation iff the alias table builds it: within 4 ulps
    # (of 1) per nonzero probability, so zero-probability transitions do not widen it. Beyond
    # that it gets the validation error, not the alias table's.
    cls, s0, outs = {
        "LT1": (LeakageTransition1Params, "U", [2, 3, 4, 5]),
        "LT2": (LeakageTransition2Params, ("U", "U"), [("U", "X"), ("U", "Y"), ("U", "Z"), (2, "U")]),
        "LTZ": (LeakageTransitionZParams, 0, [2, 3, 4, 5]),
    }[kind]
    for num_zeros in (0, 2):
        for ulps in (8, 9):  # two nonzero probabilities: up to 8 ulps is rounding
            ps = [0.5, 0.5 + ulps * math.ulp(1.0), *[0.0] * num_zeros]  # total exactly 1 + ulps ulps
            args = tuple((p, s0, out) for p, out in zip(ps, outs))
            if ulps == 8:
                got = _implied_distribution(cls(args=args, from_tag="t").alias_table_by_input_state[s0])
                want = {_key(out): p / sum(ps) for p, out in zip(ps, outs) if p}
                assert got == pytest.approx(want, abs=1e-15)
            else:
                with pytest.raises(ValueError, match="total probability 1.000000000000002>1"):
                    cls(args=args, from_tag="t")


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
