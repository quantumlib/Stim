import dataclasses
import math
import pathlib
from typing import Any, Dict, Union, Callable, Sequence, TYPE_CHECKING, overload
from typing import Optional

import numpy as np

if TYPE_CHECKING:
    import sinter

    # Go on a magical journey looking for scipy's linear regression type.
    try:
        from scipy.stats._stats_py import LinregressResult
    except ImportError:
        try:
            from scipy.stats._stats_mstats_common import LinregressResult
        except ImportError:
            from scipy.stats import linregress
            LinregressResult = type(linregress([0, 1], [0, 1]))


def log_binomial(*, p: Union[float, np.ndarray], n: int, hits: int) -> np.ndarray:
    r"""Approximates the natural log of a binomial distribution's probability.

    When working with large binomials, it's often necessary to work in log space
    to represent the result. For example, suppose that out of two million
    samples 200_000 are hits. The maximum likelihood estimate is p=0.2. Even if
    this is the true probability, the chance of seeing *exactly* 20% hits out of
    a million shots is roughly 10^-217322. Whereas the smallest representable
    double is roughly 10^-324. But ln(10^-217322) ~= -500402.4 is representable.

    This method evaluates $\ln(P(hits = B(n, p)))$, with all computations done
    in log space to ensure intermediate values can be represented as floating
    point numbers without underflowing to 0 or overflowing to infinity. This
    method can be broadcast over multiple hypothesis probabilities by giving a
    numpy array for `p` instead of a single float.

    Args:
        p: The hypotehsis probability. The independent probability of a hit
            occurring for each sample. This can also be an array of
            probabilities, in which case the function is broadcast over the
            array.
        n: The number of samples that were taken.
        hits: The number of hits that were observed amongst the samples that
            were taken.

    Returns:
        $\ln(P(hits = B(n, p)))$

    Examples:
        >>> import sinter
        >>> sinter.log_binomial(p=0.5, n=100, hits=50)
        array(-2.5308762, dtype=float32)
        >>> sinter.log_binomial(p=0.2, n=1_000_000, hits=1_000)
        array(-216626.97, dtype=float32)
        >>> sinter.log_binomial(p=0.1, n=1_000_000, hits=1_000)
        array(-99654.86, dtype=float32)
        >>> sinter.log_binomial(p=0.01, n=1_000_000, hits=1_000)
        array(-6742.573, dtype=float32)
        >>> sinter.log_binomial(p=[0.01, 0.1, 0.2], n=1_000_000, hits=1_000)
        array([  -6742.573,  -99654.86 , -216626.97 ], dtype=float32)
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
    result[p_clipped != 0] += np.log(p_clipped[p_clipped != 0]) * float(hits)
    result[p_clipped != 1] += np.log1p(-p_clipped[p_clipped != 1]) * float(misses)

    # Multiply (n choose hits) onto the total, in log space.
    log_n_choose_hits = log_factorial(n) - log_factorial(misses) - log_factorial(hits)
    result += log_n_choose_hits

    return result


def log_factorial(n: int) -> float:
    r"""Approximates $\ln(n!)$; the natural logarithm of a factorial.

    Args:
        n: The input to the factorial.

    Returns:
        Evaluates $ln(n!)$ using `math.lgamma(n+1)`.

    Examples:
        >>> import sinter
        >>> sinter.log_factorial(0)
        0.0
        >>> sinter.log_factorial(1)
        0.0
        >>> sinter.log_factorial(2)
        0.693147180559945
        >>> sinter.log_factorial(100)
        363.73937555556347
    """
    return math.lgamma(n + 1)


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
    from scipy.stats import linregress

    # HACK: get scipy's linear regression result type
    LinregressResult = type(linregress([0, 1], [0, 1]))

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

    # HACK: get scipy's linear regression result type
    from scipy.stats import linregress
    LinregressResult = type(linregress([0, 1], [0, 1]))

    (best_intercept,), _ = leastsq(func=err, x0=0.0)
    return LinregressResult(required_slope, best_intercept, None, None, None, intercept_stderr=False)


@dataclasses.dataclass(frozen=True)
class Fit:
    """The result of a fitting process.

    Attributes:
        low: The hypothesis with the smallest parameter whose cost or score was
            still "close to" the cost of the best hypothesis. For example, this
            could be a hypothesis whose squared error was within some tolerance
            of the best fit's square error, or whose likelihood was within some
            maximum Bayes factor of the max likelihood hypothesis.
        best: The max likelihood hypothesis. The hypothesis that had the lowest
            squared error, or the best fitting score.
        high: The hypothesis with the larger parameter whose cost or score was
            still "close to" the cost of the best hypothesis. For example, this
            could be a hypothesis whose squared error was within some tolerance
            of the best fit's square error, or whose likelihood was within some
            maximum Bayes factor of the max likelihood hypothesis.
    """
    low: Optional[float]
    best: Optional[float]
    high: Optional[float]

    def __repr__(self) -> str:
        return f'sinter.Fit(low={self.low!r}, best={self.best!r}, high={self.high!r})'


def fit_line_y_at_x(*,
                    xs: Sequence[float],
                    ys: Sequence[float],
                    target_x: float,
                    max_extra_squared_error: float) -> 'sinter.Fit':
    """Performs a line fit, focusing on the line's y coord at a given x coord.

    Finds the y value at the given x of the best fit, but also the minimum and
    maximum values for y at the given x amongst all possible line fits whose
    squared error cost is within the given `max_extra_squared_error` cost of the
    best fit.

    Args:
        xs: The x coordinates of points to fit.
        ys: The y coordinates of points to fit.
        target_x: The fit values are the value of y at this x coordinate.
        max_extra_squared_error: When computing the low and high fits, this is
            the maximum additional squared error that can be introduced by
            varying the slope away from the best fit.

    Returns:
        A sinter.Fit containing the best fit for y at the given x, as well as
        low and high fits that are as far as possible from the best fit while
        respecting the given max_extra_squared_error.

    Examples:
        >>> import sinter
        >>> sinter.fit_line_y_at_x(
        ...     xs=[1, 2, 3],
        ...     ys=[10, 12, 14],
        ...     target_x=4,
        ...     max_extra_squared_error=1,
        ... )
        sinter.Fit(low=14.47247314453125, best=16.0, high=17.52752685546875)
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
    for line fits whose squared error cost is within the given
    `max_extra_squared_error` cost of the best fit.

    Note that the extra squared error is computed while including a specific
    offset of some specific line. So the low/high estimates are for specific
    lines, not for the general class of lines with a given slope, adding
    together the contributions of all lines in that class.

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

    Examples:
        >>> import sinter
        >>> sinter.fit_line_slope(
        ...     xs=[1, 2, 3],
        ...     ys=[10, 12, 14],
        ...     max_extra_squared_error=1,
        ... )
        sinter.Fit(low=1.2928924560546875, best=2.0, high=2.7071075439453125)
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
    return Fit(low=float(low_slope), best=float(fit.slope), high=float(high_slope))


def fit_binomial(
        *,
        num_shots: int,
        num_hits: int,
        max_likelihood_factor: float) -> 'sinter.Fit':
    """Determine hypothesis probabilities compatible with the given hit ratio.

    The result includes the best fit (the max likelihood hypothis) as well as
    the smallest and largest probabilities whose likelihood is within the given
    factor of the maximum likelihood hypothesis.

    Args:
        num_shots: The number of samples that were taken.
        num_hits: The number of hits that were seen in the samples.
        max_likelihood_factor: The maximum Bayes factor between the low/high
            hypotheses and the best hypothesis (the max likelihood hypothesis).
            This value should be larger than 1 (as opposed to between 0 and 1).

    Returns:
        A `sinter.Fit` with the low, best, and high hypothesis probabilities.

    Examples:
        >>> import sinter
        >>> sinter.fit_binomial(
        ...     num_shots=100_000_000,
        ...     num_hits=2,
        ...     max_likelihood_factor=1000,
        ... )
        sinter.Fit(low=2e-10, best=2e-08, high=1.259e-07)
        >>> sinter.fit_binomial(
        ...     num_shots=10,
        ...     num_hits=5,
        ...     max_likelihood_factor=9,
        ... )
        sinter.Fit(low=0.202, best=0.5, high=0.798)
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


@overload
def shot_error_rate_to_piece_error_rate(shot_error_rate: float, *, pieces: float, values: float = 1) -> float:
    pass
@overload
def shot_error_rate_to_piece_error_rate(shot_error_rate: 'sinter.Fit', *, pieces: float, values: float = 1) -> 'sinter.Fit':
    pass
def shot_error_rate_to_piece_error_rate(shot_error_rate: Union[float, 'sinter.Fit'], *, pieces: float, values: float = 1) -> Union[float, 'sinter.Fit']:
    """Convert from total error rate to per-piece error rate.

    Args:
        shot_error_rate: The rate at which shots fail. If this is set to a sinter.Fit,
            the conversion broadcasts over the low,best,high of the fit.
        pieces: The number of xor-pieces we want to subdivide each shot into,
            as if each piece was an independent chance for the shot to fail and
            the total chance of a shot failing was the xor of each piece
            failing.
        values: The number of or-pieces each shot's failure is being formed out
            of.

    Returns:
        Let N = `pieces` (number of rounds)
        Let V = `values` (number of observables)
        Let S = `shot_error_rate`
        Let R = the returned result

        R satisfies the following property. Let X be the probability of each
        observable flipping, each round. R will be the probability that any of
        the observables is flipped after 1 round, given this X. X is chosen to
        satisfy the following condition. If a Bernoulli distribution with
        probability X is sampled V*N times, and the results grouped into V
        groups of N, and each group is reduced to a single value using XOR, and
        then the reduced group values are reduced to a single final value using
        OR, then this final value will be True with probability S.

        Or, in other words, if a shot consists of N rounds which V independent
        observables must survive, then R is like the per-round failure for
        any of the observables.

    Examples:
        >>> import sinter
        >>> sinter.shot_error_rate_to_piece_error_rate(
        ...     shot_error_rate=0.1,
        ...     pieces=2,
        ... )
        0.05278640450004207
        >>> sinter.shot_error_rate_to_piece_error_rate(
        ...     shot_error_rate=0.05278640450004207,
        ...     pieces=1 / 2,
        ... )
        0.10000000000000003
        >>> sinter.shot_error_rate_to_piece_error_rate(
        ...     shot_error_rate=1e-9,
        ...     pieces=100,
        ... )
        1.000000082740371e-11
        >>> sinter.shot_error_rate_to_piece_error_rate(
        ...     shot_error_rate=0.6,
        ...     pieces=10,
        ...     values=2,
        ... )
        0.12052311142021144
    """

    if isinstance(shot_error_rate, Fit):
        return Fit(
            low=shot_error_rate_to_piece_error_rate(shot_error_rate=shot_error_rate.low, pieces=pieces, values=values),
            best=shot_error_rate_to_piece_error_rate(shot_error_rate=shot_error_rate.best, pieces=pieces, values=values),
            high=shot_error_rate_to_piece_error_rate(shot_error_rate=shot_error_rate.high, pieces=pieces, values=values),
        )

    if not (0 <= shot_error_rate <= 1):
        raise ValueError(f'need (0 <= shot_error_rate={shot_error_rate} <= 1)')
    if pieces <= 0:
        raise ValueError('need pieces > 0')
    if not isinstance(pieces, (int, float)):
        raise ValueError('need isinstance(pieces, (int, float)')
    if not isinstance(values, (int, float)):
        raise ValueError('need isinstance(values, (int, float)')
    if pieces == 1:
        return shot_error_rate
    if values != 1:
        p = 1 - (1 - shot_error_rate)**(1 / values)
        p = shot_error_rate_to_piece_error_rate(p, pieces=pieces)
        return 1 - (1 - p)**values

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
    """Converts paths like 'folder/d=5,r=3.stim' into dicts like {'d':5,'r':3}.

    On the command line, specifying `--metadata_func auto` results in this
    method being used to extra metadata from the circuit file paths. Integers
    and floats will be parsed into their values, instead of being stored as
    strings.

    Args:
        path: A file path where the name of the file has a series of terms like
            'a=b' separated by commas and ending in '.stim'.

    Returns:
        A dictionary from named keys to parsed values.

    Examples:
        >>> import sinter
        >>> sinter.comma_separated_key_values("folder/d=5,r=3.5,x=abc.stim")
        {'d': 5, 'r': 3.5, 'x': 'abc'}
    """
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
