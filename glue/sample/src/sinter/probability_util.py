import math
from typing import Union, Callable, Sequence, Tuple

import numpy as np
from scipy.stats import linregress
from scipy.optimize import leastsq
from scipy.stats._stats_mstats_common import LinregressResult


def log_binomial(*, p: Union[float, np.ndarray], n: int, hits: int) -> Union[float, np.ndarray]:
    r"""Approximates $\ln(P(hits = B(n, p)))$; the natural logarithm of a binomial distribution.

    All computations are done in log space to ensure intermediate values can be represented as
    floating point numbers without underflowing to 0 or overflowing to infinity. This is necessary
    when computing likelihoods over many samples. For example, if 80% of a million samples are hits,
    the maximum likelihood estimate is p=0.8. But even this optimal estimate assigns a prior
    probability of roughly 10^-217322 for seeing *exactly* 80% hits out of a million (whereas the
    smallest representable double is roughly 10^-324).

    This method can be broadcast over multiple hypothesis probabilities.

    Args:
        p: The independent probability of a hit occurring for each sample. This can also be an array
            of probabilities, in which case the function is broadcast over the array.
        n: The number of samples that were taken.
        hits: The number of hits that were observed amongst the samples that were taken.

    Returns:
        $\ln(P(hits = B(n, p)))$
    """
    # Clamp probabilities into the valid [0, 1] range (in case float error put them outside it).
    p_clipped = np.clip(p, 0, 1)

    result = np.zeros(shape=p_clipped.shape, dtype=np.float32)
    misses = n - hits

    # Handle p=0 and p=1 cases separately, to avoid arithmetic warnings.
    if hits:
        result[p_clipped == 0] = -np.inf
    if misses:
        result[p_clipped == 1] = -np.inf

    # Multiply p**hits and (1-p)**misses onto the total, in log space.
    result[p_clipped != 0] += np.log(p_clipped[p_clipped != 0]) * hits
    result[p_clipped != 1] += np.log1p(-p_clipped[p_clipped != 1]) * misses

    # Multiply (n choose hits) onto the total, in log space.
    log_n_choose_hits = log_factorial(n) - log_factorial(misses) - log_factorial(hits)
    result += log_n_choose_hits

    return result


def log_factorial(n: int) -> float:
    r"""Approximates $\ln(n!)$; the natural logarithm of a factorial.

    Uses Stirling's approximation for large n.
    """
    if n < 20:
        return sum(math.log(k) for k in range(1, n + 1))
    return (n + 0.5) * math.log(n) - n + math.log(2 * np.pi) / 2


def binary_search(*, func: Callable[[int], float], min_x: int, max_x: int, target: float) -> int:
    """Performs an approximate granular binary search over a monotonically ascending function."""
    while max_x > min_x + 1:
        med_x = (min_x + max_x) // 2
        out = func(med_x)
        if out < target:
            min_x = med_x
        elif out > target:
            max_x = med_x
        else:
            return med_x
    fmax = func(max_x)
    fmin = func(min_x)
    dmax = 0 if fmax == target else fmax - target
    dmin = 0 if fmin == target else fmin - target
    return max_x if abs(dmax) < abs(dmin) else min_x


def binary_intercept(*, func: Callable[[float], float], start_x: float, step: float, target_y: float, atol: float) -> float:
    """Performs an approximate granular binary search over a monotonically ascending function."""
    start_y = func(start_x)
    if abs(start_y - target_y) <= atol:
        return start_x
    while (func(start_x + step) >= target_y) == (start_y >= target_y):
        step *= 2
        if np.isinf(step) or step == 0:
            raise ValueError("Failed.")
    xs = [start_x, start_x + step]
    min_x = min(xs)
    max_x = max(xs)
    increasing = func(min_x) < func(max_x)

    while True:
        med_x = (min_x + max_x) / 2
        med_y = func(med_x)
        if abs(med_y - target_y) <= atol:
            return med_x
        assert med_x not in [min_x, max_x]
        if (med_y < target_y) == increasing:
            min_x = med_x
        else:
            max_x = med_x


def least_squares_cost(*, xs: np.ndarray, ys: np.ndarray, intercept: float, slope: float) -> float:
    assert len(xs.shape) == 1
    assert xs.shape == ys.shape
    return np.sum((intercept + slope*xs - ys)**2)


def least_squares_through_point(*, xs: np.ndarray, ys: np.ndarray, required_x: float, required_y: float) -> LinregressResult:
    xs2 = xs - required_x
    ys2 = ys - required_y

    def err(slope: float) -> float:
        return least_squares_cost(xs=xs2, ys=ys2, intercept=0, slope=slope)

    (best_slope,), _ = leastsq(func=err, x0=0.0)
    intercept = required_y - required_x * best_slope
    return LinregressResult(best_slope, intercept, None, None, None, intercept_stderr=False)


def least_squares_with_slope(*, xs: np.ndarray, ys: np.ndarray, required_slope: float) -> LinregressResult:
    def err(intercept: float) -> float:
        return least_squares_cost(xs=xs, ys=ys, intercept=intercept, slope=required_slope)

    (best_intercept,), _ = leastsq(func=err, x0=0.0)
    return LinregressResult(required_slope, best_intercept, None, None, None, intercept_stderr=False)


def least_squares_output_range(*,
                               xs: Sequence[float],
                               ys: Sequence[float],
                               target_x: float,
                               cost_increase: float) -> Tuple[float, float, float]:
    xs = np.array(xs, dtype=np.float64)
    ys = np.array(ys, dtype=np.float64)
    fit = linregress(xs, ys)
    base_cost = least_squares_cost(xs=xs, ys=ys, intercept=fit.intercept, slope=fit.slope)
    base_y = float(fit.intercept + target_x * fit.slope)

    def cost_for_y(y2: float) -> float:
        fit2 = least_squares_through_point(xs=xs, ys=ys, required_x=target_x, required_y=y2)
        return least_squares_cost(xs=xs, ys=ys, intercept=fit2.intercept, slope=fit2.slope)

    low_y = binary_intercept(start_x=base_y, step=-1, target_y=base_cost + cost_increase, func=cost_for_y, atol=1e-5)
    high_y = binary_intercept(start_x=base_y, step=1, target_y=base_cost + cost_increase, func=cost_for_y, atol=1e-5)
    return low_y, base_y, high_y


def least_squares_slope_range(*,
                              xs: Sequence[float],
                              ys: Sequence[float],
                              cost_increase: float) -> Tuple[float, float, float]:
    xs = np.array(xs, dtype=np.float64)
    ys = np.array(ys, dtype=np.float64)
    fit = linregress(xs, ys)
    base_cost = least_squares_cost(xs=xs, ys=ys, intercept=fit.intercept, slope=fit.slope)

    def cost_for_slope(slope: float) -> float:
        fit2 = least_squares_with_slope(xs=xs, ys=ys, required_slope=slope)
        return least_squares_cost(xs=xs, ys=ys, intercept=fit2.intercept, slope=fit2.slope)

    low_slope = binary_intercept(start_x=fit.slope, step=-1, target_y=base_cost + cost_increase, func=cost_for_slope, atol=1e-5)
    high_slope = binary_intercept(start_x=fit.slope, step=1, target_y=base_cost + cost_increase, func=cost_for_slope, atol=1e-5)
    return low_slope, fit.slope, high_slope


def binomial_relative_likelihood_range(
        *,
        num_shots: int,
        num_hits: int,
        likelihood_ratio: float) -> Tuple[float, float]:
    """Compute the (min, max) probabilities within the given ratio of the max likelihood hypothesis.

    Args:
        num_shots: The number of samples that were taken.
        num_hits: The number of hits that were seen in the samples.
        likelihood_ratio: Should be larger than 1. The maximum Bayes factor between hypotheses and
            the max likelihood hypothesis.

    Returns:
        A (min_hypothesis_probability, max_hypothesis_probability) tuple.
    """
    if num_shots == 0:
        return 0, 1
    log_max_likelihood = log_binomial(p=num_hits / num_shots, n=num_shots, hits=num_hits)
    target_log_likelihood = log_max_likelihood + math.log(likelihood_ratio)
    acc = 100
    low = binary_search(
        func=lambda exp_err: log_binomial(p=exp_err / (acc * num_shots), n=num_shots, hits=num_hits),
        target=target_log_likelihood,
        min_x=0,
        max_x=num_hits * acc) / acc
    high = binary_search(
        func=lambda exp_err: -log_binomial(p=exp_err / (acc * num_shots), n=num_shots, hits=num_hits),
        target=-target_log_likelihood,
        min_x=num_hits * acc,
        max_x=num_shots * acc) / acc
    return low / num_shots, high / num_shots
