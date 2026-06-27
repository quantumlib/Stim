import math
from typing import Union

import numpy as np
import pytest

import sinter
from sinter._probability_util import (
    binary_search, log_binomial, log_factorial, fit_line_y_at_x, fit_line_slope,
    binary_intercept, least_squares_through_point, fit_binomial, shot_error_rate_to_piece_error_rate,
)
from sinter._probability_util import comma_separated_key_values


@pytest.mark.parametrize(
    "arg,result",
    {
        0: 0,
        1: 0,
        2: math.log(2),
        3: math.log(2) + math.log(3),
        # These values were taken from wolfram alpha:
        10: 15.1044125730755152952257093292510,
        100: 363.73937555556349014407999336965,
        1000: 5912.128178488163348878130886725,
        10000: 82108.9278368143534553850300635,
        100000: 1051299.2218991218651292781082,
    }.items(),
)
def test_log_factorial(arg, result):
    np.testing.assert_allclose(log_factorial(arg), result, rtol=1e-11)


@pytest.mark.parametrize(
    "n,p,hits,result",
    [
        (1, 0.5, 0, np.log(0.5)),
        (1, 0.5, 1, np.log(0.5)),
        (1, 0.1, 0, np.log(0.9)),
        (1, 0.1, 1, np.log(0.1)),
        (2, [0, 1, 0.1, 0.5], 0, [0, -np.inf, np.log(0.9 ** 2), np.log(0.25)]),
        (2, [0, 1, 0.1, 0.5], 1, [-np.inf, -np.inf, np.log(0.1 * 0.9 * 2), np.log(0.5)]),
        (2, [0, 1, 0.1, 0.5], 2, [-np.inf, 0, np.log(0.1 ** 2), np.log(0.25)]),
        # Magic number comes from PDF[BinomialDistribution[10^10, 10^-6], 10000] on wolfram alpha.
        (10 ** 10, 10 ** -6, 10 ** 4, np.log(0.0039893915536591)),
        # Corner cases.
        (1, 0.0, 0, 0),
        (1, 0.0, 1, -np.inf),
        (1, 1.0, 0, -np.inf),
        (1, 1.0, 1, 0),
        # Array broadcast.
        (2, np.array([0.0, 0.5, 1.0]), 0, np.array([0.0, np.log(0.25), -np.inf])),
        (2, np.array([0.0, 0.5, 1.0]), 1, np.array([-np.inf, np.log(0.5), -np.inf])),
        (2, np.array([0.0, 0.5, 1.0]), 2, np.array([-np.inf, np.log(0.25), 0.0])),
    ],
)
def test_log_binomial(
    n: int, p: Union[float, np.ndarray], hits: int, result: Union[float, np.ndarray]
) -> None:
    np.testing.assert_allclose(log_binomial(n=n, p=p, hits=hits), result, rtol=1e-2)


def test_binary_search():
    assert binary_search(func=lambda x: x**2, min_x=0, max_x=10**100, target=100.1) == 10
    assert binary_search(func=lambda x: x**2, min_x=0, max_x=10**100, target=100) == 10
    assert binary_search(func=lambda x: x**2, min_x=0, max_x=10**100, target=99.9) == 10
    assert binary_search(func=lambda x: x**2, min_x=0, max_x=10**100, target=90) == 9
    assert binary_search(func=lambda x: x**2, min_x=0, max_x=10**100, target=92) == 10
    assert binary_search(func=lambda x: x**2, min_x=0, max_x=10**100, target=-100) == 0
    assert binary_search(func=lambda x: x**2, min_x=0, max_x=10**100, target=10**300) == 10**100


def test_least_squares_through_point():
    fit = least_squares_through_point(
        xs=np.array([1, 2, 3]),
        ys=np.array([2, 3, 4]),
        required_x=1,
        required_y=2)
    np.testing.assert_allclose(fit.slope, 1)
    np.testing.assert_allclose(fit.intercept, 1)

    fit = least_squares_through_point(
        xs=np.array([1, 2, 3]),
        ys=np.array([2, 3, 4]),
        required_x=1,
        required_y=1)
    np.testing.assert_allclose(fit.slope, 1.6, rtol=1e-5)
    np.testing.assert_allclose(fit.intercept, -0.6, atol=1e-5)


def test_binary_intercept():
    t = binary_intercept(func=lambda x: x**2, start_x=5, step=1, target_y=82.3, atol=0.01)
    assert t > 0 and abs(t**2 - 82.3) <= 0.01
    t = binary_intercept(func=lambda x: -x**2, start_x=5, step=1, target_y=-82.3, atol=0.01)
    assert t > 0 and abs(t**2 - 82.3) <= 0.01
    t = binary_intercept(func=lambda x: x**2, start_x=0, step=-1, target_y=82.3, atol=0.01)
    assert t < 0 and abs(t**2 - 82.3) <= 0.01
    t = binary_intercept(func=lambda x: -x**2, start_x=0, step=-1, target_y=-82.3, atol=0.2)
    assert t < 0 and abs(t**2 - 82.3) <= 0.2


def test_fit_y_at_x():
    fit = fit_line_y_at_x(
        xs=[1, 2, 3],
        ys=[1, 5, 9],
        target_x=100,
        max_extra_squared_error=1,
    )
    assert 300 < fit.low < 390 < fit.best < 410 < fit.high < 500


def test_fit_slope():
    fit = fit_line_slope(
        xs=[1, 2, 3],
        ys=[1, 5, 9],
        max_extra_squared_error=1,
    )
    np.testing.assert_allclose(fit.best, 4)
    assert 3 < fit.low < 3.5 < fit.best < 4.5 < fit.high < 5


def test_fit_binomial_shrink_towards_half():
    with pytest.raises(ValueError, match='max_likelihood_factor'):
        fit_binomial(num_shots=10 ** 5, num_hits=10 ** 5 / 2, max_likelihood_factor=0.1)

    fit = fit_binomial(num_shots=10 ** 5, num_hits=10 ** 5 / 2, max_likelihood_factor=1e3)
    np.testing.assert_allclose(
        (fit.low, fit.best, fit.high),
        (0.494122, 0.5, 0.505878),
        rtol=1e-4,
    )
    fit = fit_binomial(num_shots=10 ** 4, num_hits=10 ** 4 / 2, max_likelihood_factor=1e3)
    np.testing.assert_allclose(
        (fit.low, fit.best, fit.high),
        (0.481422, 0.5, 0.518578),
        rtol=1e-4,
    )
    fit = fit_binomial(num_shots=10 ** 4, num_hits=10 ** 4 / 2, max_likelihood_factor=1e2)
    np.testing.assert_allclose(
        (fit.low, fit.best, fit.high),
        (0.48483, 0.5, 0.51517),
        rtol=1e-4,
    )
    fit = fit_binomial(num_shots=1000, num_hits=500, max_likelihood_factor=1e3)
    np.testing.assert_allclose(
        (fit.low, fit.best, fit.high),
        (0.44143, 0.5, 0.55857),
        rtol=1e-4,
    )
    fit = fit_binomial(num_shots=100, num_hits=50, max_likelihood_factor=1e3)
    np.testing.assert_allclose(
        (fit.low, fit.best, fit.high),
        (0.3204, 0.5, 0.6796),
        rtol=1e-4,
    )


@pytest.mark.parametrize("n,c,factor", [
    (100, 50, 1e1),
    (100, 50, 1e2),
    (100, 50, 1e3),
    (1000, 500, 1e3),
    (10**6, 100, 1e3),
    (10**6, 100, 1e2),
])
def test_fit_binomial_vs_log_binomial(n: int, c: int, factor: float):
    fit = fit_binomial(num_shots=n, num_hits=n - c, max_likelihood_factor=factor)
    a = fit.low
    b = fit.high

    raw = log_binomial(p=(n - c) / n, n=n, hits=n - c)
    low = log_binomial(p=a, n=n, hits=n - c)
    high = log_binomial(p=b, n=n, hits=n - c)

    np.testing.assert_allclose(
        fit.best,
        (n - c) / n,
        rtol=1e-4,
    )

    np.testing.assert_allclose(
        np.exp(raw - low),
        factor,
        rtol=1e-2,
    )
    np.testing.assert_allclose(
        np.exp(raw - high),
        factor,
        rtol=1e-2,
    )


def test_comma_separated_key_values():
    d = comma_separated_key_values("folder/a=2,b=3.0,c=test.stim")
    assert d == {
        'a': 2,
        'b': 3.0,
        'c': 'test',
    }
    assert type(d['a']) == int
    assert type(d['b']) == float
    with pytest.raises(ValueError, match='separated'):
        comma_separated_key_values("folder/a,b=3.0,c=test.stim")


def test_shot_error_rate_to_piece_error_rate():
    np.testing.assert_allclose(
        shot_error_rate_to_piece_error_rate(
            shot_error_rate=0.2 * (1 - 0.2) * 2,
            pieces=2,
        ),
        0.2,
        rtol=1e-5)

    np.testing.assert_allclose(
        shot_error_rate_to_piece_error_rate(
            shot_error_rate=0.001 * (1 - 0.001) * 2,
            pieces=2,
        ),
        0.001,
        rtol=1e-5)

    np.testing.assert_allclose(
        shot_error_rate_to_piece_error_rate(
            shot_error_rate=0.001 * (1 - 0.001)**2 * 3 + 0.001**3,
            pieces=3,
        ),
        0.001,
        rtol=1e-5)

    # Extremely low error rates.
    np.testing.assert_allclose(
        shot_error_rate_to_piece_error_rate(
            shot_error_rate=1e-100,
            pieces=100,
        ),
        1e-102,
        rtol=1e-5)


def test_shot_error_rate_to_piece_error_rate_unions():
    np.testing.assert_allclose(
        shot_error_rate_to_piece_error_rate(
            shot_error_rate=0.75,
            pieces=1,
            values=2,
        ),
        0.75,
        rtol=1e-5)

    np.testing.assert_allclose(
        shot_error_rate_to_piece_error_rate(
            shot_error_rate=0.2,
            pieces=1,
            values=2,
        ),
        0.2,
        rtol=1e-5)

    np.testing.assert_allclose(
        shot_error_rate_to_piece_error_rate(
            shot_error_rate=0.001,
            pieces=1000000,
            values=2,
        ),
        0.001 / 1000000,
        rtol=1e-2)

    np.testing.assert_allclose(
        shot_error_rate_to_piece_error_rate(
            shot_error_rate=0.0975,
            pieces=10,
            values=2,
        ),
        0.010453280306648605,
        rtol=1e-5)


def test_fit_repr():
    v = sinter.Fit(low=0.25, best=1, high=10)
    assert eval(repr(v), {"sinter": sinter}) == v
