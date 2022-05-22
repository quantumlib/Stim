import collections
import dataclasses
from typing import Callable, TypeVar, List, Any, Tuple, Iterable, DefaultDict, Optional, TYPE_CHECKING, Dict

import matplotlib.colors as mcolors
import matplotlib.pyplot as plt

from sinter.anon_task_stats import AnonTaskStats
from sinter.probability_util import binomial_relative_likelihood_range
from sinter.task_summary import TaskSummary

if TYPE_CHECKING:
    import sinter


MARKERS: str = "ov*sp^<>8PhH+xXDd|" * 100
COLORS: Tuple[str, ...] = tuple(mcolors.TABLEAU_COLORS.keys()) * 3


T = TypeVar('T')


def split_by(vs: Iterable[T], key_func: Callable[[T], Any]) -> List[List[T]]:
    cur_key: Any = None
    out: List[List[T]] = []
    buf: List[T] = []
    for item in vs:
        key = key_func(item)
        if key != cur_key:
            cur_key = key
            if buf:
                out.append(buf)
                buf = []
        buf.append(item)
    if buf:
        out.append(buf)
    return out


def better_sorted_str_terms(val: Any) -> Any:
    if isinstance(val, tuple):
        return tuple(better_sorted_str_terms(e) for e in val)
    if not isinstance(val, str):
        return val
    terms = split_by(val, lambda c: c in '.0123456789')
    result = []
    for term in terms:
        term = ''.join(term)
        if '.' in term:
            try:
                term = float(term)
            except ValueError:
                term = tuple(int(e) for e in term.split('.'))
        else:
            try:
                term = int(term)
            except ValueError:
                pass
        result.append(term)
    return tuple(result)


TVal = TypeVar('TVal')
TKey = TypeVar('TKey')


def group_by(items: Iterable[TVal],
             *,
             key: Callable[[TVal], TKey],
             ) -> Dict[TKey, List[TVal]]:
    """Groups items based on whether they produce the same key from a function.

    Args:
        items: The items to group.
        key: Items that produce the same value from this function get grouped together.

    Returns:
        A dictionary mapping outputs that were produced by the grouping function to
        the list of items that produced that output.

    Examples:
        >>> sinter.group_by([1, 2, 3], key=lambda i: i == 2)
        {False: [1, 3], True: [2]}

        >>> sinter.group_by(range(10), key=lambda i: i % 3)
        {0: [0, 3, 6, 9], 1: [1, 4, 7], 2: [2, 5, 8]}
    """

    result: Dict[TKey, List[TVal]] = {}

    for item in items:
        curve_id = key(item)
        result.setdefault(curve_id, []).append(item)

    return result


TCurveId = TypeVar('TCurveId')


def plot_discard_rate(
        *,
        ax: plt.Axes,
        stats: 'Iterable[sinter.TaskStats]',
        x_func: Callable[['sinter.TaskStats'], float],
        curve_func: Callable[['sinter.TaskStats'], TCurveId] = lambda _: None,
        plot_args_func: Callable[[int, TCurveId], Dict[str, Any]] = lambda _: {},
        highlight_likelihood_ratio: Optional[float] = 1e-3,
) -> None:
    """Plots discard rates in curves with uncertainty highlights.

    Args:
        ax: The plt.Axes to plot onto. For example, the `ax` value from `fig, ax = plt.subplots(1, 1)`.
        stats: The collected statistics to plot.
        x_func: The X coordinate to use for each stat's data point. For example, this could be
            `x_func=lambda stat: stat.json_metadata['physical_error_rate']`.
        curve_func: Optional. When specified, multiple curves will be plotted instead of one curve.
            The statistics are grouped into curves based on whether or not they get the same result
            out of this function. For example, this could be `curve_func=lambda stat: stat.decoder`.
        plot_args_func: Optional. Specifies additional arguments to give the the underlying calls to
            `plot` and `fill_between` used to do the actual plotting. For example, this can be used
            to specify markers and colors. Takes the index of the curve in sorted order and also a
            curve_id (these will be 0 and None respectively if curve_func is not specified). For example,
            this could be:

                plot_args_func=lambda index, curve_id: {'color': 'red'
                                                        if curve_id == 'pymatching'
                                                        else 'blue'}

        highlight_likelihood_ratio: Controls how wide the uncertainty highlight region around curves is.
            Set to this a value between 0 and 1, and the hypothesis probabilities at least that many times
            as likely as the max likelihood hypothesis will be highlighted.
    """

    curve_groups = group_by(stats, key=curve_func)
    for k, curve_id in enumerate(sorted(curve_groups.keys(), key=better_sorted_str_terms)):
        stats = sorted(curve_groups[curve_id], key=x_func)

        xs = []
        ys = []
        ys_low = []
        ys_high = []
        for stat in stats:
            x = x_func(stat)
            if stat.shots:
                xs.append(x)
                ys.append(stat.discards / stat.shots)
                if 0 < highlight_likelihood_ratio < 1:
                    low, high = binomial_relative_likelihood_range(
                        num_shots=stat.shots,
                        num_hits=stat.discards,
                        likelihood_ratio=highlight_likelihood_ratio)
                    ys_low.append(low)
                    ys_high.append(high)

        kwargs = dict(plot_args_func(k, curve_id))
        if 'label' not in kwargs and curve_id is not None:
            kwargs['label'] = str(curve_id)
        ax.plot(xs, ys, **kwargs)
        if 0 < highlight_likelihood_ratio < 1:
            if 'zorder' not in kwargs:
                kwargs['zorder'] = 0
            if 'alpha' not in kwargs:
                kwargs['alpha'] = 1
            kwargs['zorder'] -= 100
            kwargs['alpha'] *= 0.25
            if 'marker' in kwargs:
                del kwargs['marker']
            if 'linestyle' in kwargs:
                del kwargs['linestyle']
            del kwargs['label']
            ax.fill_between(xs, ys_low, ys_high, **kwargs)


def plot_error_rate(
        *,
        ax: plt.Axes,
        stats: 'Iterable[sinter.TaskStats]',
        x_func: Callable[['sinter.TaskStats'], float],
        curve_func: Callable[['sinter.TaskStats'], TCurveId] = lambda _: None,
        plot_args_func: Callable[[int, TCurveId], Dict[str, Any]] = lambda _k, _c: {'marker': MARKERS[_k]},
        highlight_likelihood_ratio: Optional[float] = 1e-3,
) -> None:
    """Plots error rates in curves with uncertainty highlights.

    Args:
        ax: The plt.Axes to plot onto. For example, the `ax` value from `fig, ax = plt.subplots(1, 1)`.
        stats: The collected statistics to plot.
        x_func: The X coordinate to use for each stat's data point. For example, this could be
            `x_func=lambda stat: stat.json_metadata['physical_error_rate']`.
        curve_func: Optional. When specified, multiple curves will be plotted instead of one curve.
            The statistics are grouped into curves based on whether or not they get the same result
            out of this function. For example, this could be `curve_func=lambda stat: stat.decoder`.
        plot_args_func: Optional. Specifies additional arguments to give the the underlying calls to
            `plot` and `fill_between` used to do the actual plotting. For example, this can be used
            to specify markers and colors. Takes the index of the curve in sorted order and also a
            curve_id (these will be 0 and None respectively if curve_func is not specified). For example,
            this could be:

                plot_args_func=lambda index, curve_id: {'color': 'red'
                                                        if curve_id == 'pymatching'
                                                        else 'blue'}

        highlight_likelihood_ratio: Controls how wide the uncertainty highlight region around curves is.
            Set to this a value between 0 and 1, and the hypothesis probabilities at least that many times
            as likely as the max likelihood hypothesis will be highlighted.
    """
    curve_groups = group_by(stats, key=curve_func)
    for k, curve_id in enumerate(sorted(curve_groups.keys(), key=better_sorted_str_terms)):
        stats = sorted(curve_groups[curve_id], key=x_func)

        xs = []
        ys = []
        xs_range = []
        ys_low = []
        ys_high = []
        for stat in stats:
            num_kept = stat.shots - stat.discards
            if num_kept == 0:
                continue
            x = x_func(stat)
            if stat.errors:
                xs.append(x)
                ys.append(stat.errors / num_kept)
            if 0 < highlight_likelihood_ratio < 1:
                xs_range.append(x)
                low, high = binomial_relative_likelihood_range(
                    num_shots=num_kept,
                    num_hits=stat.errors,
                    likelihood_ratio=highlight_likelihood_ratio,
                )
                ys_low.append(low)
                ys_high.append(high)

        kwargs = dict(plot_args_func(k, curve_id))
        if 'label' not in kwargs and curve_id is not None:
            kwargs['label'] = str(curve_id)
        ax.plot(xs, ys, **kwargs)
        if 0 < highlight_likelihood_ratio < 1:
            if 'zorder' not in kwargs:
                kwargs['zorder'] = 0
            if 'alpha' not in kwargs:
                kwargs['alpha'] = 1
            kwargs['zorder'] -= 100
            kwargs['alpha'] *= 0.25
            if 'marker' in kwargs:
                del kwargs['marker']
            if 'linestyle' in kwargs:
                del kwargs['linestyle']
            del kwargs['label']
            ax.fill_between(xs_range, ys_low, ys_high, **kwargs)
