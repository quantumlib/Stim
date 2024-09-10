import math
from typing import Callable, TypeVar, List, Any, Iterable, Optional, TYPE_CHECKING, Dict, Union, Literal, Tuple
from typing import Sequence
from typing import cast

import numpy as np

from sinter._probability_util import fit_binomial, shot_error_rate_to_piece_error_rate, Fit

if TYPE_CHECKING:
    import sinter
    import matplotlib.pyplot as plt


MARKERS: str = "ov*sp^<>8PhH+xXDd|" * 100
LINESTYLES: tuple[str, ...] = (
    'solid',
    'dotted',
    'dashed',
    'dashdot',
    'loosely dotted',
    'dotted',
    'densely dotted',
    'long dash with offset',
    'loosely dashed',
    'dashed',
    'densely dashed',
    'loosely dashdotted',
    'dashdotted',
    'densely dashdotted',
    'dashdotdotted',
    'loosely dashdotdotted',
    'densely dashdotdotted',
)
T = TypeVar('T')
TVal = TypeVar('TVal')
TKey = TypeVar('TKey')


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


class LooseCompare:
    def __init__(self, val: Any):
        self.val: Any = None

        self.val = val.val if isinstance(val, LooseCompare) else val

    def __lt__(self, other: Any) -> bool:
        other_val = other.val if isinstance(other, LooseCompare) else other
        if isinstance(self.val, (int, float)) and isinstance(other_val, (int, float)):
            return self.val < other_val
        if isinstance(self.val, (tuple, list)) and isinstance(other_val, (tuple, list)):
            return tuple(LooseCompare(e) for e in self.val) < tuple(LooseCompare(e) for e in other_val)
        return str(self.val) < str(other_val)

    def __gt__(self, other: Any) -> bool:
        other_val = other.val if isinstance(other, LooseCompare) else other
        if isinstance(self.val, (int, float)) and isinstance(other_val, (int, float)):
            return self.val > other_val
        if isinstance(self.val, (tuple, list)) and isinstance(other_val, (tuple, list)):
            return tuple(LooseCompare(e) for e in self.val) > tuple(LooseCompare(e) for e in other_val)
        return str(self.val) > str(other_val)

    def __str__(self) -> str:
        return str(self.val)

    def __eq__(self, other: Any) -> bool:
        if isinstance(other, LooseCompare):
            other_val = other.val
        else:
            other_val = other
        if isinstance(self.val, (int, float)) and isinstance(other_val, (int, float)):
            return self.val == other_val
        return str(self.val) == str(other_val)


def better_sorted_str_terms(val: Any) -> Any:
    """A function that orders "a10000" after "a9", instead of before.

    Normally, sorting strings sorts them lexicographically, treating numbers so
    that "1999999" ends up being less than "2". This method splits the string
    into a tuple of text pairs and parsed number parts, so that sorting by this
    key puts "2" before "1999999".

    Because this method is intended for use in plotting, where it's more
    important to see a bad result than to see nothing, it returns a type that
    tries to be comparable to everything.

    Args:
        val: The value to convert into a value with a better sorting order.

    Returns:
        A custom type of object with a better sorting order.

    Examples:
        >>> import sinter
        >>> items = [
        ...    "distance=199999, rounds=3",
        ...    "distance=2, rounds=3",
        ...    "distance=199999, rounds=199999",
        ...    "distance=2, rounds=199999",
        ... ]
        >>> for e in sorted(items, key=sinter.better_sorted_str_terms):
        ...    print(e)
        distance=2, rounds=3
        distance=2, rounds=199999
        distance=199999, rounds=3
        distance=199999, rounds=199999
    """
    if val is None:
        return 'None'
    if isinstance(val, tuple):
        return tuple(better_sorted_str_terms(e) for e in val)
    if not isinstance(val, str):
        return LooseCompare(val)
    terms = split_by(val, lambda c: c in '.0123456789')
    result = []
    for term in terms:
        term = ''.join(term)
        if '.' in term:
            try:
                term = float(term)
            except ValueError:
                try:
                    term = tuple(int(e) for e in term.split('.'))
                except ValueError:
                    pass
        else:
            try:
                term = int(term)
            except ValueError:
                pass
        result.append(term)
    if len(result) == 1 and isinstance(result[0], (int, float)):
        return LooseCompare(result[0])
    return tuple(LooseCompare(e) for e in result)


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
        >>> import sinter
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


class _FrozenDict:
    def __init__(self, v: dict):
        self._v = dict(v)
        self._eq = frozenset(v.items())
        self._hash = hash(self._eq)

        terms = []
        for k in sorted(self._v.keys(), key=lambda e: (e != 'sort', e)):
            terms.append(k)
            terms.append(better_sorted_str_terms(self._v[k])
        )
        self._order = tuple(terms)

    def __eq__(self, other):
        if isinstance(other, _FrozenDict):
            return self._eq == other._eq
        return NotImplemented

    def __lt__(self, other):
        if isinstance(other, _FrozenDict):
            return self._order < other._order
        return NotImplemented

    def __ne__(self, other):
        return not (self == other)

    def __hash__(self):
        return self._hash

    def __getitem__(self, item):
        return self._v[item]

    def get(self, item, alternate = None):
        return self._v.get(item, alternate)

    def __str__(self):
        return " ".join(str(v) for _, v in sorted(self._v.items()))


def plot_discard_rate(
        *,
        ax: 'plt.Axes',
        stats: 'Iterable[sinter.TaskStats]',
        x_func: Callable[['sinter.TaskStats'], Any],
        failure_units_per_shot_func: Callable[['sinter.TaskStats'], Any] = lambda _: 1,
        group_func: Callable[['sinter.TaskStats'], TCurveId] = lambda _: None,
        filter_func: Callable[['sinter.TaskStats'], Any] = lambda _: True,
        plot_args_func: Callable[[int, TCurveId, List['sinter.TaskStats']], Dict[str, Any]] = lambda index, group_key, group_stats: dict(),
        highlight_max_likelihood_factor: Optional[float] = 1e3,
        point_label_func: Callable[['sinter.TaskStats'], Any] = lambda _: None,
) -> None:
    """Plots discard rates in curves with uncertainty highlights.

    Args:
        ax: The plt.Axes to plot onto. For example, the `ax` value from `fig, ax = plt.subplots(1, 1)`.
        stats: The collected statistics to plot.
        x_func: The X coordinate to use for each stat's data point. For example, this could be
            `x_func=lambda stat: stat.json_metadata['physical_error_rate']`.
        failure_units_per_shot_func: How many discard chances there are per shot. This rescales what the
            discard rate means. By default, it is the discard rate per shot, but this allows
            you to instead make it the discard rate per round. For example, if the metadata
            associated with a shot has a field 'r' which is the number of rounds, then this can be
            achieved with `failure_units_per_shot_func=lambda stats: stats.metadata['r']`.
        group_func: Optional. When specified, multiple curves will be plotted instead of one curve.
            The statistics are grouped into curves based on whether or not they get the same result
            out of this function. For example, this could be `group_func=lambda stat: stat.decoder`.
            If the result of the function is a dictionary, then optional keys in the dictionary will
            also control the plotting of each curve. Available keys are:
                'label': the label added to the legend for the curve
                'color': the color used for plotting the curve
                'marker': the marker used for the curve
                'linestyle': the linestyle used for the curve
                'sort': the order in which the curves will be plotted and added to the legend
            e.g. if two curves (with different resulting dictionaries from group_func) share the same
            value for key 'marker', they will be plotted with the same marker.
            Colors, markers and linestyles are assigned in order, sorted by the values for those keys.
        filter_func: Optional. When specified, some curves will not be plotted.
            The statistics are filtered and only plotted if filter_func(stat) returns True.
            For example, `filter_func=lambda s: s.json_metadata['basis'] == 'x'` would plot only stats
            where the saved metadata indicates the basis was 'x'.
        plot_args_func: Optional. Specifies additional arguments to give the underlying calls to
            `plot` and `fill_between` used to do the actual plotting. For example, this can be used
            to specify markers and colors. Takes the index of the curve in sorted order and also a
            curve_id (these will be 0 and None respectively if group_func is not specified). For example,
            this could be:

                plot_args_func=lambda index, curve_id: {'color': 'red'
                                                        if curve_id == 'pymatching'
                                                        else 'blue'}

        highlight_max_likelihood_factor: Controls how wide the uncertainty highlight region around curves is.
            Must be 1 or larger. Hypothesis probabilities at most that many times as unlikely as the max likelihood
            hypothesis will be highlighted.
        point_label_func: Optional. Specifies text to draw next to data points.
    """
    if highlight_max_likelihood_factor is None:
        highlight_max_likelihood_factor = 1

    def y_func(stat: 'sinter.TaskStats') -> Union[float, 'sinter.Fit']:
        result = fit_binomial(
            num_shots=stat.shots,
            num_hits=stat.discards,
            max_likelihood_factor=highlight_max_likelihood_factor,
        )

        pieces = failure_units_per_shot_func(stat)
        result = Fit(
            low=shot_error_rate_to_piece_error_rate(result.low, pieces=pieces),
            best=shot_error_rate_to_piece_error_rate(result.best, pieces=pieces),
            high=shot_error_rate_to_piece_error_rate(result.high, pieces=pieces),
        )

        if highlight_max_likelihood_factor == 1:
            return result.best
        return result

    plot_custom(
        ax=ax,
        stats=stats,
        x_func=x_func,
        y_func=y_func,
        group_func=group_func,
        filter_func=filter_func,
        plot_args_func=plot_args_func,
        point_label_func=point_label_func,
    )


def plot_error_rate(
        *,
        ax: 'plt.Axes',
        stats: 'Iterable[sinter.TaskStats]',
        x_func: Callable[['sinter.TaskStats'], Any],
        failure_units_per_shot_func: Callable[['sinter.TaskStats'], Any] = lambda _: 1,
        failure_values_func: Callable[['sinter.TaskStats'], Any] = lambda _: 1,
        group_func: Callable[['sinter.TaskStats'], TCurveId] = lambda _: None,
        filter_func: Callable[['sinter.TaskStats'], Any] = lambda _: True,
        plot_args_func: Callable[[int, TCurveId, List['sinter.TaskStats']], Dict[str, Any]] = lambda index, group_key, group_stats: dict(),
        highlight_max_likelihood_factor: Optional[float] = 1e3,
        line_fits: Optional[Tuple[Literal['linear', 'log', 'sqrt'], Literal['linear', 'log', 'sqrt']]] = None,
        point_label_func: Callable[['sinter.TaskStats'], Any] = lambda _: None,
) -> None:
    """Plots error rates in curves with uncertainty highlights.

    Args:
        ax: The plt.Axes to plot onto. For example, the `ax` value from `fig, ax = plt.subplots(1, 1)`.
        stats: The collected statistics to plot.
        x_func: The X coordinate to use for each stat's data point. For example, this could be
            `x_func=lambda stat: stat.json_metadata['physical_error_rate']`.
        failure_units_per_shot_func: How many error chances there are per shot. This rescales what the
            logical error rate means. By default, it is the logical error rate per shot, but this allows
            you to instead make it the logical error rate per round. For example, if the metadata
            associated with a shot has a field 'r' which is the number of rounds, then this can be
            achieved with `failure_units_per_shot_func=lambda stats: stats.metadata['r']`.
        failure_values_func: How many independent ways there are for a shot to fail, such as
            the number of independent observables in a memory experiment. This affects how the failure
            units rescaling plays out (e.g. with 1 independent failure the "center" of the conversion
            is at 50% whereas for 2 independent failures the "center" is at 75%).
        group_func: Optional. When specified, multiple curves will be plotted instead of one curve.
            The statistics are grouped into curves based on whether or not they get the same result
            out of this function. For example, this could be `group_func=lambda stat: stat.decoder`.
            If the result of the function is a dictionary, then optional keys in the dictionary will
            also control the plotting of each curve. Available keys are:
                'label': the label added to the legend for the curve
                'color': the color used for plotting the curve
                'marker': the marker used for the curve
                'linestyle': the linestyle used for the curve
                'sort': the order in which the curves will be plotted and added to the legend
            e.g. if two curves (with different resulting dictionaries from group_func) share the same
            value for key 'marker', they will be plotted with the same marker.
            Colors, markers and linestyles are assigned in order, sorted by the values for those keys.
        filter_func: Optional. When specified, some curves will not be plotted.
            The statistics are filtered and only plotted if filter_func(stat) returns True.
            For example, `filter_func=lambda s: s.json_metadata['basis'] == 'x'` would plot only stats
            where the saved metadata indicates the basis was 'x'.
        plot_args_func: Optional. Specifies additional arguments to give the underlying calls to
            `plot` and `fill_between` used to do the actual plotting. For example, this can be used
            to specify markers and colors. Takes the index of the curve in sorted order and also a
            curve_id (these will be 0 and None respectively if group_func is not specified). For example,
            this could be:

                plot_args_func=lambda index, curve_id: {'color': 'red'
                                                        if curve_id == 'pymatching'
                                                        else 'blue'}

        highlight_max_likelihood_factor: Controls how wide the uncertainty highlight region around curves is.
            Must be 1 or larger. Hypothesis probabilities at most that many times as unlikely as the max likelihood
            hypothesis will be highlighted.
        line_fits: Defaults to None. Set this to a tuple (x_scale, y_scale) to include a dashed line
            fit to every curve. The scales determine how to transform the coordinates before
            performing the fit, and can be set to 'linear', 'sqrt', or 'log'.
        point_label_func: Optional. Specifies text to draw next to data points.
    """
    if highlight_max_likelihood_factor is None:
        highlight_max_likelihood_factor = 1
    if not (highlight_max_likelihood_factor >= 1):
        raise ValueError(f"not (highlight_max_likelihood_factor={highlight_max_likelihood_factor} >= 1)")

    def y_func(stat: 'sinter.TaskStats') -> Union[float, 'sinter.Fit']:
        result = fit_binomial(
            num_shots=stat.shots - stat.discards,
            num_hits=stat.errors,
            max_likelihood_factor=highlight_max_likelihood_factor,
        )

        pieces = failure_units_per_shot_func(stat)
        values = failure_values_func(stat)
        result = Fit(
            low=shot_error_rate_to_piece_error_rate(result.low, pieces=pieces, values=values),
            best=shot_error_rate_to_piece_error_rate(result.best, pieces=pieces, values=values),
            high=shot_error_rate_to_piece_error_rate(result.high, pieces=pieces, values=values),
        )

        if stat.errors == 0:
            result = Fit(low=result.low, high=result.high, best=float('nan'))

        if highlight_max_likelihood_factor == 1:
            return result.best
        return result

    plot_custom(
        ax=ax,
        stats=stats,
        x_func=x_func,
        y_func=y_func,
        group_func=group_func,
        filter_func=filter_func,
        plot_args_func=plot_args_func,
        line_fits=line_fits,
        point_label_func=point_label_func,
    )


def _rescale(v: Sequence[float], scale: str, invert: bool) -> np.ndarray:
    if scale == 'linear':
        return np.array(v)
    elif scale == 'log':
        return np.exp(v) if invert else np.log(v)
    elif scale == 'sqrt':
        return np.array(v)**2 if invert else np.sqrt(v)
    else:
        raise NotImplementedError(f'{scale=}')


def plot_custom(
        *,
        ax: 'plt.Axes',
        stats: 'Iterable[sinter.TaskStats]',
        x_func: Callable[['sinter.TaskStats'], Any],
        y_func: Callable[['sinter.TaskStats'], Union['sinter.Fit', float, int]],
        group_func: Callable[['sinter.TaskStats'], TCurveId] = lambda _: None,
        point_label_func: Callable[['sinter.TaskStats'], Any] = lambda _: None,
        filter_func: Callable[['sinter.TaskStats'], Any] = lambda _: True,
        plot_args_func: Callable[[int, TCurveId, List['sinter.TaskStats']], Dict[str, Any]] = lambda index, group_key, group_stats: dict(),
        line_fits: Optional[Tuple[Literal['linear', 'log', 'sqrt'], Literal['linear', 'log', 'sqrt']]] = None,
) -> None:
    """Plots error rates in curves with uncertainty highlights.

    Args:
        ax: The plt.Axes to plot onto. For example, the `ax` value from `fig, ax = plt.subplots(1, 1)`.
        stats: The collected statistics to plot.
        x_func: The X coordinate to use for each stat's data point. For example, this could be
            `x_func=lambda stat: stat.json_metadata['physical_error_rate']`.
        y_func: The Y value to use for each stat's data point. This can be a float or it can be a
            sinter.Fit value, in which case the curve will follow the fit.best value and a
            highlighted area will be shown from fit.low to fit.high.
        group_func: Optional. When specified, multiple curves will be plotted instead of one curve.
            The statistics are grouped into curves based on whether or not they get the same result
            out of this function. For example, this could be `group_func=lambda stat: stat.decoder`.
            If the result of the function is a dictionary, then optional keys in the dictionary will
            also control the plotting of each curve. Available keys are:
                'label': the label added to the legend for the curve
                'color': the color used for plotting the curve
                'marker': the marker used for the curve
                'linestyle': the linestyle used for the curve
                'sort': the order in which the curves will be plotted and added to the legend
            e.g. if two curves (with different resulting dictionaries from group_func) share the same
            value for key 'marker', they will be plotted with the same marker.
            Colors, markers and linestyles are assigned in order, sorted by the values for those keys.
        point_label_func: Optional. Specifies text to draw next to data points.
        filter_func: Optional. When specified, some curves will not be plotted.
            The statistics are filtered and only plotted if filter_func(stat) returns True.
            For example, `filter_func=lambda s: s.json_metadata['basis'] == 'x'` would plot only stats
            where the saved metadata indicates the basis was 'x'.
        plot_args_func: Optional. Specifies additional arguments to give the underlying calls to
            `plot` and `fill_between` used to do the actual plotting. For example, this can be used
            to specify markers and colors. Takes the index of the curve in sorted order and also a
            curve_id (these will be 0 and None respectively if group_func is not specified). For example,
            this could be:

                plot_args_func=lambda index, group_key, group_stats: {
                    'color': (
                        'red'
                        if group_key == 'decoder=pymatching p=0.001'
                        else 'blue'
                    ),
                }
        line_fits: Defaults to None. Set this to a tuple (x_scale, y_scale) to include a dashed line
            fit to every curve. The scales determine how to transform the coordinates before
            performing the fit, and can be set to 'linear', 'sqrt', or 'log'.
    """

    def group_dict_func(item: 'sinter.TaskStats') -> _FrozenDict:
        e = group_func(item)
        return _FrozenDict(e if isinstance(e, dict) else {'label': str(e)})

    # Backwards compatibility to when the group stats argument wasn't present.
    import inspect
    if len(inspect.signature(plot_args_func).parameters) == 2:
        old_plot_args_func = cast(Callable[[int, TCurveId], Any], plot_args_func)
        plot_args_func = lambda a, b, _: old_plot_args_func(a, b)

    filtered_stats: List['sinter.TaskStats'] = [
        stat
        for stat in stats
        if filter_func(stat)
    ]

    curve_groups = group_by(filtered_stats, key=group_dict_func)
    colors = {
        k: f'C{i}'
        for i, k in enumerate(sorted({g.get('color', g) for g in curve_groups.keys()}, key=better_sorted_str_terms))
    }
    markers = {
        k: MARKERS[i % len(MARKERS)]
        for i, k in enumerate(sorted({g.get('marker', g) for g in curve_groups.keys()}, key=better_sorted_str_terms))
    }
    linestyles = {
        k: LINESTYLES[i % len(LINESTYLES)]
        for i, k in enumerate(sorted({g.get('linestyle', None) for g in curve_groups.keys()}, key=better_sorted_str_terms))
    }

    def sort_key(a: Any) -> Any:
        if isinstance(a, _FrozenDict):
            return a.get('sort', better_sorted_str_terms(a))
        return better_sorted_str_terms(a)

    for k, group_key in enumerate(sorted(curve_groups.keys(), key=sort_key)):
        group = curve_groups[group_key]
        group = sorted(group, key=x_func)
        color = colors[group_key.get('color', group_key)]
        marker = markers[group_key.get('marker', group_key)]
        linestyle = linestyles[group_key.get('linestyle', None)]
        label = str(group_key.get('label', group_key))
        xs_label: list[float] = []
        ys_label: list[float] = []
        vs_label: list[float] = []
        xs_best: list[float] = []
        ys_best: list[float] = []
        xs_low_high: list[float] = []
        ys_low: list[float] = []
        ys_high: list[float] = []
        for item in group:
            x = x_func(item)
            y = y_func(item)
            point_label = point_label_func(item)
            if isinstance(y, Fit):
                if y.low is not None and y.high is not None and not math.isnan(y.low) and not math.isnan(y.high):
                    xs_low_high.append(x)
                    ys_low.append(y.low)
                    ys_high.append(y.high)
                if y.best is not None and not math.isnan(y.best):
                    ys_best.append(y.best)
                    xs_best.append(x)

                if point_label:
                    cy = None
                    for e in [y.best, y.high, y.low]:
                        if e is not None and not math.isnan(e):
                            cy = e
                            break
                    if cy is not None:
                        xs_label.append(x)
                        ys_label.append(cy)
                        vs_label.append(point_label)
            elif not math.isnan(y):
                xs_best.append(x)
                ys_best.append(y)
                if point_label:
                    xs_label.append(x)
                    ys_label.append(y)
                    vs_label.append(point_label)
        args = dict(plot_args_func(k, group_func(group[0]), group))
        if 'linestyle' not in args:
            args['linestyle'] = linestyle
        if 'marker' not in args:
            args['marker'] = marker
        if 'color' not in args:
            args['color'] = color
        if 'label' not in args:
            args['label'] = label
        ax.plot(xs_best, ys_best, **args)
        for x, y, lbl in zip(xs_label, ys_label, vs_label):
            if lbl:
                ax.annotate(lbl, (x, y))
        if len(xs_low_high) > 1:
            ax.fill_between(xs_low_high, ys_low, ys_high, color=args['color'], alpha=0.2, zorder=-100)
        elif len(xs_low_high) == 1:
            l, = ys_low
            h, = ys_high
            m = (l + h) / 2
            ax.errorbar(xs_low_high, [m], yerr=([m - l], [h - m]), marker='', elinewidth=1, ecolor=color, capsize=5)

        if line_fits is not None and len(set(xs_best)) >= 2:
            x_scale, y_scale = line_fits
            fit_xs = _rescale(xs_best, x_scale, False)
            fit_ys = _rescale(ys_best, y_scale, False)

            from scipy.stats import linregress
            line_fit = linregress(fit_xs, fit_ys)

            x0 = fit_xs[0]
            x1 = fit_xs[-1]
            dx = x1 - x0
            x0 -= dx*10
            x1 += dx*10
            if x0 < 0 <= fit_xs[0] > x0 and x_scale == 'sqrt':
                x0 = 0

            out_xs = np.linspace(x0, x1, 1000)
            out_ys = out_xs * line_fit.slope + line_fit.intercept
            out_xs = _rescale(out_xs, x_scale, True)
            out_ys = _rescale(out_ys, y_scale, True)

            line_fit_kwargs = args.copy()
            line_fit_kwargs.pop('marker', None)
            line_fit_kwargs.pop('label', None)
            line_fit_kwargs['linestyle'] = '--'
            line_fit_kwargs.setdefault('linewidth', 1)
            line_fit_kwargs['linewidth'] /= 2
            ax.plot(out_xs, out_ys, **line_fit_kwargs)
