import dataclasses
import math
import pathlib
import stim
from typing import Any
from typing import Dict
from typing import Union, Callable, Sequence, Tuple, TYPE_CHECKING

import numpy as np

if TYPE_CHECKING:
    import sinter
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


def least_squares_through_point(*, xs: np.ndarray, ys: np.ndarray, required_x: float, required_y: float) -> 'LinregressResult':
    # Local import to reduce initial cost of importing sinter.
    from scipy.optimize import leastsq
    from scipy.stats._stats_mstats_common import LinregressResult

    xs2 = xs - required_x
    ys2 = ys - required_y

    def err(slope: float) -> float:
        return least_squares_cost(xs=xs2, ys=ys2, intercept=0, slope=slope)

    (best_slope,), _ = leastsq(func=err, x0=0.0)
    intercept = required_y - required_x * best_slope
    return LinregressResult(best_slope, intercept, None, None, None, intercept_stderr=False)


def least_squares_with_slope(*, xs: np.ndarray, ys: np.ndarray, required_slope: float) -> 'LinregressResult':
    def err(intercept: float) -> float:
        return least_squares_cost(xs=xs, ys=ys, intercept=intercept, slope=required_slope)

    # Local import to reduce initial cost of importing sinter.
    from scipy.optimize import leastsq
    from scipy.stats._stats_mstats_common import LinregressResult
    (best_intercept,), _ = leastsq(func=err, x0=0.0)
    return LinregressResult(required_slope, best_intercept, None, None, None, intercept_stderr=False)


@dataclasses.dataclass(frozen=True)
class Fit:
    """The result of a fitting process.

    Attributes:
        best: The max likelihood hypothesis. The hypothesis that had the lowest squared error,
            or the best fitting score.
        low: The hypothesis with the smallest parameter whose cost or score was still "close to"
            the cost of the best hypothesis. For example, this could be a hypothesis whose
            squared error was within some tolerance of the best fit's square error, or whose
            likelihood was within some maximum Bayes factor of the max likelihood hypothesis.
        high: The hypothesis with the larger parameter whose cost or score was still "close to"
            the cost of the best hypothesis. For example, this could be a hypothesis whose
            squared error was within some tolerance of the best fit's square error, or whose
            likelihood was within some maximum Bayes factor of the max likelihood hypothesis.
    """
    low: float
    best: float
    high: float


def fit_line_y_at_x(*,
                    xs: Sequence[float],
                    ys: Sequence[float],
                    target_x: float,
                    max_extra_squared_error: float) -> 'sinter.Fit':
    """Performs a line fit of the given points, focusing on the line's value for y at a given x coordinate.

    Finds the y value at the given x of the best fit, but also the minimum and maximum
    values for y at the given x amongst all possible line fits whose squared error cost
    is within the given `max_extra_squared_error` cost of the best fit.

    Args:
        xs: The x coordinates of points to fit.
        ys: The y coordinates of points to fit.
        target_x: The reported fit values are the value of y at this x coordinate.
        max_extra_squared_error: When computing the low and high fits, this is
            the maximum additional squared error that can be introduced by
            varying the slope away from the best fit.

    Returns:
        A sinter.Fit containing the best fit for y at the given x, as well as low
        and high fits that are as far as possible from the best fit while respective
        the given max_extra_squared_error.
    """

    # Local import to reduce initial cost of importing sinter.
    from scipy.stats import linregress

    xs = np.array(xs, dtype=np.float64)
    ys = np.array(ys, dtype=np.float64)
    fit = linregress(xs, ys)
    base_cost = least_squares_cost(xs=xs, ys=ys, intercept=fit.intercept, slope=fit.slope)
    base_y = float(fit.intercept + target_x * fit.slope)

    def cost_for_y(y2: float) -> float:
        fit2 = least_squares_through_point(xs=xs, ys=ys, required_x=target_x, required_y=y2)
        return least_squares_cost(xs=xs, ys=ys, intercept=fit2.intercept, slope=fit2.slope)

    low_y = binary_intercept(start_x=base_y, step=-1, target_y=base_cost + max_extra_squared_error, func=cost_for_y, atol=1e-5)
    high_y = binary_intercept(start_x=base_y, step=1, target_y=base_cost + max_extra_squared_error, func=cost_for_y, atol=1e-5)
    return Fit(low=low_y, best=base_y, high=high_y)


def fit_line_slope(*,
              xs: Sequence[float],
              ys: Sequence[float],
              max_extra_squared_error: float) -> 'sinter.Fit':
    """Performs a line fit of the given points, focusing on the line's slope.

    Finds the slope of the best fit, but also the minimum and maximum slopes
    for line fits whose squared error cost is within the given `max_extra_squared_error`
    cost of the best fit.

    Args:
        xs: The x coordinates of points to fit.
        ys: The y coordinates of points to fit.
        max_extra_squared_error: When computing the low and high fits, this is
            the maximum additional squared error that can be introduced by
            varying the slope away from the best fit.

    Returns:
        A sinter.Fit containing the best fit, as well as low and high fits that
        are as far as possible from the best fit while respective the given
        max_extra_squared_error.
    """
    # Local import to reduce initial cost of importing sinter.
    from scipy.stats import linregress

    xs = np.array(xs, dtype=np.float64)
    ys = np.array(ys, dtype=np.float64)
    fit = linregress(xs, ys)
    base_cost = least_squares_cost(xs=xs, ys=ys, intercept=fit.intercept, slope=fit.slope)

    def cost_for_slope(slope: float) -> float:
        fit2 = least_squares_with_slope(xs=xs, ys=ys, required_slope=slope)
        return least_squares_cost(xs=xs, ys=ys, intercept=fit2.intercept, slope=fit2.slope)

    low_slope = binary_intercept(start_x=fit.slope, step=-1, target_y=base_cost + max_extra_squared_error, func=cost_for_slope, atol=1e-5)
    high_slope = binary_intercept(start_x=fit.slope, step=1, target_y=base_cost + max_extra_squared_error, func=cost_for_slope, atol=1e-5)
    return Fit(low=low_slope, best=fit.slope, high=high_slope)


def fit_binomial(
        *,
        num_shots: int,
        num_hits: int,
        max_likelihood_factor: float) -> 'sinter.Fit':
    """Compute the (min, max) probabilities within the given ratio of the max likelihood hypothesis.

    Args:
        num_shots: The number of samples that were taken.
        num_hits: The number of hits that were seen in the samples.
        max_likelihood_factor: The maximum Bayes factor between the low,high hypotheses and
            the best hypothesis (the max likelihood hypothesis). This value should be
            larger than 1 (as opposed to between 0 and 1).

    Returns:
        A `sinter.Fit` containing the max likelihood h
        A (min_hypothesis_probability, max_hypothesis_probability) tuple.
    """
    if max_likelihood_factor < 1:
        raise ValueError(f'max_likelihood_factor={max_likelihood_factor} < 1')
    if num_shots == 0:
        return Fit(low=0, high=1, best=0.5)
    log_max_likelihood = log_binomial(p=num_hits / num_shots, n=num_shots, hits=num_hits)
    target_log_likelihood = log_max_likelihood - math.log(max_likelihood_factor)
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
    return Fit(best=num_hits / num_shots, low=low / num_shots, high=high / num_shots)


def shot_error_rate_to_piece_error_rate(shot_error_rate: float, *, pieces: int) -> float:
    """Convert from total error rate to per-piece error rate.

    Works by assuming pieces fail independently and a shot fails if an any single
    piece fails.
    """
    if not (0 <= shot_error_rate <= 1):
        raise ValueError(f'not (0 <= shot_error_rate={shot_error_rate} <= 1)')
    if pieces == 1:
        return shot_error_rate
    if shot_error_rate > 0.5:
        return 1 - shot_error_rate_to_piece_error_rate(1 - shot_error_rate, pieces=pieces)
    assert 0 <= shot_error_rate <= 0.5
    randomize_rate = 2*shot_error_rate
    round_randomize_rate = 1 - (1 - randomize_rate)**(1 / pieces)
    round_error_rate = round_randomize_rate / 2

    if round_error_rate == 0:
        # The intermediate numbers got too small. Fallback to division approximation.
        return shot_error_rate / pieces

    return round_error_rate


def comma_separated_key_values(path: str) -> Dict[str, Any]:
    name = pathlib.Path(path).name
    if '.' in name:
        name = name[:name.rindex('.')]
    result = {}
    for term in name.split(','):
        parts = term.split('=')
        if len(parts) != 2:
            raise ValueError(f"Expected a path with a filename containing comma-separated key=value terms like 'a=2,b=3.stim', but got {path!r}.")
        k, v = parts
        try:
            v = int(v)
        except ValueError:
            try:
                v = float(v)
            except ValueError:
                pass
        result[k] = v
    return result
