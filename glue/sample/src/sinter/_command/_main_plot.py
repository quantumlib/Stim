import math
import sys
from typing import Any, Callable, Iterable, List, Optional, TYPE_CHECKING, Tuple, Union, Dict, Sequence, cast
import argparse

import matplotlib.pyplot as plt
import numpy as np

from sinter._command._main_combine import ExistingData
from sinter._plotting import plot_discard_rate, plot_custom
from sinter._plotting import plot_error_rate
from sinter._probability_util import shot_error_rate_to_piece_error_rate, Fit

if TYPE_CHECKING:
    import sinter


def parse_args(args: List[str]) -> Any:
    parser = argparse.ArgumentParser(description='Plot collected CSV data.',
                                     prog='sinter plot')
    parser.add_argument('--filter_func',
                        type=str,
                        default="True",
                        help='A python expression that determines whether a case is kept or not.\n'
                             'Values available to the python expression:\n'
                             '    metadata: The parsed value from the json_metadata for the data point.\n'
                             '    m: `m.key` is a shorthand for `metadata.get("key", None)`.\n'
                             '    decoder: The decoder that decoded the data for the data point.\n'
                             '    strong_id: The cryptographic hash of the case that was sampled for the data point.\n'
                             '    stat: The sinter.TaskStats object for the data point.\n'
                             'Expected expression type:\n'
                             '    Something that can be given to `bool` to get True or False.\n'
                             'Examples:\n'
                             '''    --filter_func "decoder=='pymatching'"\n'''
                             '''    --filter_func "0.001 < metadata['p'] < 0.005"\n''')
    parser.add_argument('--preprocess_stats_func',
                        type=str,
                        default=None,
                        help='An expression that operates on a `stats` value, returning a new list of stats to plot.\n'
                             'For example, this could double add a field to json_metadata or merge stats together.\n'
                             'Examples:\n'
                             '''    --preprocess_stats_func "[stat for stat in stats if stat.errors > 0]\n'''
                             '''    --preprocess_stats_func "[stat.with_edits(errors=stat.custom_counts['severe_errors']) for stat in stats]\n'''
                             '''    --preprocess_stats_func "__import__('your_custom_module').your_custom_function(stats)"\n'''
                             )
    parser.add_argument('--x_func',
                        type=str,
                        default="1",
                        help='A python expression that determines where points go on the x axis.\n'
                             'Values available to the python expression:\n'
                             '    metadata: The parsed value from the json_metadata for the data point.\n'
                             '    m: `m.key` is a shorthand for `metadata.get("key", None)`.\n'
                             '    decoder: The decoder that decoded the data for the data point.\n'
                             '    strong_id: The cryptographic hash of the case that was sampled for the data point.\n'
                             '    stat: The sinter.TaskStats object for the data point.\n'
                             'Expected expression type:\n'
                             '    Something that can be given to `float` to get a float.\n'
                             'Examples:\n'
                             '''    --x_func "metadata['p']"\n'''
                             '''    --x_func m.p\n'''
                             '''    --x_func "metadata['path'].split('/')[-1].split('.')[0]"\n'''
                        )
    parser.add_argument('--point_label_func',
                        type=str,
                        default="None",
                        help='A python expression that determines text to put next to data points.\n'
                             'Values available to the python expression:\n'
                             '    metadata: The parsed value from the json_metadata for the data point.\n'
                             '    m: `m.key` is a shorthand for `metadata.get("key", None)`.\n'
                             '    decoder: The decoder that decoded the data for the data point.\n'
                             '    strong_id: The cryptographic hash of the case that was sampled for the data point.\n'
                             '    stat: The sinter.TaskStats object for the data point.\n'
                             'Expected expression type:\n'
                             '    Something Falsy (no label), or something that can be given to `str` to get a string.\n'
                             'Examples:\n'
                             '''    --point_label_func "f'p={m.p}'"\n'''
                        )
    parser.add_argument('--y_func',
                        type=str,
                        default=None,
                        help='A python expression that determines where points go on the y axis.\n'
                             'This argument is not used by error rate or discard rate plots; only\n'
                             'by the "custom_y" type plot.'
                             'Values available to the python expression:\n'
                             '    metadata: The parsed value from the json_metadata for the data point.\n'
                             '    m: `m.key` is a shorthand for `metadata.get("key", None)`.\n'
                             '    decoder: The decoder that decoded the data for the data point.\n'
                             '    strong_id: The cryptographic hash of the case that was sampled for the data point.\n'
                             '    stat: The sinter.TaskStats object for the data point.\n'
                             'Expected expression type:\n'
                             '    A `sinter.Fit` specifying an uncertainty region,.\n'
                             '    or else something that can be given to `float` to get a float.\n'
                             'Examples:\n'
                             '''    --x_func "metadata['p']"\n'''
                             '''    --x_func "metadata['path'].split('/')[-1].split('.')[0]"\n'''
                        )
    parser.add_argument('--fig_size',
                        type=int,
                        nargs=2,
                        default=None,
                        help='Desired figure width and height in pixels.')
    parser.add_argument('--dpi',
                        type=float,
                        default=100,
                        help='Dots per inch. Determines resolution of the figure.')
    parser.add_argument('--group_func',
                        type=str,
                        default="'all data (use -group_func and -x_func to group into curves)'",
                        help='A python expression that determines how points are grouped into curves.\n'
                             'If this evaluates to a dict, different keys control different groupings (e.g. "color" and "marker")\n'
                             'Values available to the python expression:\n'
                             '    metadata: The parsed value from the json_metadata for the data point.\n'
                             '    m: `m.key` is a shorthand for `metadata.get("key", None)`.\n'
                             '    decoder: The decoder that decoded the data for the data point.\n'
                             '    strong_id: The cryptographic hash of the case that was sampled for the data point.\n'
                             '    stat: The sinter.TaskStats object for the data point.\n'
                             'Expected expression type:\n'
                             '    A dict, or something that can be given to `str` to get a useful string.\n'
                             'Recognized dict keys:\n'
                             '    "color": controls color grouping\n'
                             '    "marker": controls marker grouping\n'
                             '    "linestyle": controls linestyle grouping\n'
                             '    "order": controls ordering in the legend\n'
                             '    "label": the text shown in the legend\n'
                             'Examples:\n'
                             '''    --group_func "(decoder, m.d)"\n'''
                             '''    --group_func "{'color': decoder, 'marker': m.d, 'label': (decoder, m.d)}"\n'''
                             '''    --group_func "metadata['path'].split('/')[-2]"\n'''
                        )
    parser.add_argument('--failure_unit_name',
                        type=str,
                        default=None,
                        help='The unit of failure, typically either "shot" (the default) or "round".\n'
                             'If this argument is specified, --failure_units_per_shot_func must also be specified.\n'''
                        )
    parser.add_argument('--failure_units_per_shot_func',
                        type=str,
                        default=None,
                        help='A python expression that evaluates to the number of failure units there are per shot.\n'
                             'For example, if the failure unit is rounds, this should be an expression that returns\n'
                             'the number of rounds in a shot. Sinter has no way of knowing what you consider a round\n'
                             'to be, otherwise.\n'
                             '\n'
                             'This value is used to rescale the logical error rate plots. For example, if there are 4\n'
                             'failure units per shot then a shot error rate of 10%% corresponds to a unit failure rate\n'
                             'of 2.7129%%. The conversion formula (assuming less than 50%% error rates) is:\n'
                             '\n'
                             '    P_unit = 0.5 - 0.5 * (1 - 2 * P_shot)**(1/units_per_shot)\n'
                             '\n'
                             'Values available to the python expression:\n'
                             '    metadata: The parsed value from the json_metadata for the data point.\n'
                             '    m: `m.key` is a shorthand for `metadata.get("key", None)`.\n'
                             '    decoder: The decoder that decoded the data for the data point.\n'
                             '    strong_id: The cryptographic hash of the case that was sampled for the data point.\n'
                             '    stat: The sinter.TaskStats object for the data point.\n'
                             '\n'
                             'Expected expression type:\n'
                             '    float.\n'
                             '\n'
                             'Examples:\n'
                             '''    --failure_units_per_shot_func "metadata['rounds']"\n'''
                             '''    --failure_units_per_shot_func m.r\n'''
                             '''    --failure_units_per_shot_func "m.distance * 3"\n'''
                             '''    --failure_units_per_shot_func "10"\n'''
                        )
    parser.add_argument('--failure_values_func',
                        type=str,
                        default=None,
                        help='A python expression that evaluates to the number of independent ways a shot can fail.\n'
                             'For example, if a shot corresponds to a memory experiment preserving two observables,\n'
                             'then the failure unions is 2.\n'
                             '\n'
                             'This value is necessary to correctly rescale the logical error rate plots when using\n'
                             '--failure_values_func. By default it is assumed to be 1.\n'
                             '\n'
                             'Values available to the python expression:\n'
                             '    metadata: The parsed value from the json_metadata for the data point.\n'
                             '    m: `m.key` is a shorthand for `metadata.get("key", None)`.\n'
                             '    decoder: The decoder that decoded the data for the data point.\n'
                             '    strong_id: The cryptographic hash of the case that was sampled for the data point.\n'
                             '    stat: The sinter.TaskStats object for the data point.\n'
                             '\n'
                             'Expected expression type:\n'
                             '    float.\n'
                             '\n'
                             'Examples:\n'
                             '''    --failure_values_func "metadata['num_obs']"\n'''
                             '''    --failure_values_func "2"\n'''
                        )
    parser.add_argument('--plot_args_func',
                        type=str,
                        default='''{}''',
                        help='A python expression used to customize the look of curves.\n'
                             'Values available to the python expression:\n'
                             '    index: A unique integer identifying the curve.\n'
                             '    key: The group key (returned from --group_func) identifying the curve.\n'
                             '    stats: The list of sinter.TaskStats object in the group.\n'
                             '    metadata: (From one arbitrary data point in the group.) The parsed value from the json_metadata for the data point.\n'
                             '    m: `m.key` is a shorthand for `metadata.get("key", None)`.\n'
                             '    decoder: (From one arbitrary data point in the group.) The decoder that decoded the data for the data point.\n'
                             '    strong_id: (From one arbitrary data point in the group.) The cryptographic hash of the case that was sampled for the data point.\n'
                             '    stat: (From one arbitrary data point in the group.) The sinter.TaskStats object for the data point.\n'
                             'Expected expression type:\n'
                             '    A dictionary to give to matplotlib plotting functions as a **kwargs argument.\n'
                             'Examples:\n'
                             """    --plot_args_func "'''{'label': 'curve #' + str(index), 'linewidth': 5}'''"\n"""
                             """    --plot_args_func "'''{'marker': 'ov*sp^<>8PhH+xXDd|'[index %% 18]}'''"\n"""
                        )
    parser.add_argument('--in',
                        type=str,
                        nargs='+',
                        required=True,
                        help='Input files to get data from.')
    parser.add_argument('--type',
                        choices=['error_rate', 'discard_rate', 'custom_y'],
                        nargs='+',
                        default=(),
                        help='Picks the figures to include.')
    parser.add_argument('--out',
                        type=str,
                        default=None,
                        help='Write the plot to a file instead of showing it.\n'
                             '(Use --show to still show the plot.)')
    parser.add_argument('--xaxis',
                        type=str,
                        default='[log]',
                        help='Customize the X axis label. '
                             'Prefix [log] for logarithmic scale. '
                             'Prefix [sqrt] for square root scale.')
    parser.add_argument('--yaxis',
                        type=str,
                        default=None,
                        help='Customize the Y axis label. '
                             'Prefix [log] for logarithmic scale. '
                             'Prefix [sqrt] for square root scale.')
    parser.add_argument('--custom_error_count_keys',
                        type=str,
                        nargs='+',
                        default=None,
                        help="Replaces the stat's error count with one of its custom counts. Stats "
                             "without this count end up with an error count of 0. Adds the json "
                             "metadata field 'custom_error_count_key' to identify the custom count "
                             "used. Specifying multiple values turns each stat into multiple "
                             "stats.")
    parser.add_argument('--show',
                        action='store_true',
                        help='Displays the plot in a window even when --out is specified.')
    parser.add_argument('--xmin',
                        default=None,
                        type=float,
                        help='Forces the minimum value of the x axis.')
    parser.add_argument('--xmax',
                        default=None,
                        type=float,
                        help='Forces the maximum value of the x axis.')
    parser.add_argument('--ymin',
                        default=None,
                        type=float,
                        help='Forces the minimum value of the y axis.')
    parser.add_argument('--ymax',
                        default=None,
                        type=float,
                        help='Forces the maximum value of the y axis.')
    parser.add_argument('--title',
                        default=None,
                        type=str,
                        help='Sets the title of the plot.')
    parser.add_argument('--subtitle',
                        default=None,
                        type=str,
                        help='Sets the subtitle of the plot.\n'
                             '\n'
                             'Note: The pattern "{common}" will expand to text including\n'
                             'all json metadata values that are the same across all stats.')
    parser.add_argument('--highlight_max_likelihood_factor',
                        type=float,
                        default=1000,
                        help='The relative likelihood ratio that determines the color highlights around curves.\n'
                             'Set this to 1 or larger. Set to 1 to disable highlighting.')
    parser.add_argument('--line_fits',
                        action='store_true',
                        help='Adds dashed line fits to every curve.')

    a = parser.parse_args(args=args)
    if 'custom_y' in a.type and a.y_func is None:
        raise ValueError("--type custom_y requires --y_func.")
    if a.y_func is not None and a.type and 'custom_y' not in a.type:
        raise ValueError("--y_func only works with --type custom_y.")
    if (len(a.type) == 0 and a.y_func is not None) or list(a.type) == 'custom_y':
        if a.failure_units_per_shot_func is not None:
            raise ValueError("--failure_units_per_shot_func doesn't affect --type custom_y")
    if (a.failure_units_per_shot_func is not None) != (a.failure_unit_name is not None):
        raise ValueError("--failure_units_per_shot_func and --failure_unit_name can only be specified together.")
    if a.failure_values_func is not None and a.failure_units_per_shot_func is None:
            raise ValueError('Specified --failure_values_func without --failure_units_per_shot_func')
    if a.failure_units_per_shot_func is None:
        a.failure_units_per_shot_func = "1"
    if a.failure_values_func is None:
        a.failure_values_func = "1"
    if a.failure_unit_name is None:
        a.failure_unit_name = 'shot'

    def _compile_argument_into_func(arg_name: str, arg_val: Any = ()):
        if arg_val == ():
            arg_val = getattr(a, arg_name)
        raw_func = eval(compile(
            f'lambda *, stat, decoder, metadata, m, strong_id, sinter, math, np: {arg_val}',
            filename=f'{arg_name}:command_line_arg',
            mode='eval',
        ))
        import sinter
        return lambda stat: raw_func(
            sinter=sinter,
            math=math,
            np=np,
            stat=stat,
            decoder=stat.decoder,
            metadata=stat.json_metadata,
            m=_FieldToMetadataWrapper(stat.json_metadata),
            strong_id=stat.strong_id)

    a.preprocess_stats_func = None if a.preprocess_stats_func is None else eval(compile(
        f'lambda *, stats: {a.preprocess_stats_func}',
        filename='preprocess_stats_func:command_line_arg',
        mode='eval'))
    a.x_func = _compile_argument_into_func('x_func', a.x_func)
    if a.y_func is not None:
        a.y_func = _compile_argument_into_func('y_func')
    a.point_label_func = _compile_argument_into_func('point_label_func')
    a.group_func = _compile_argument_into_func('group_func')
    a.filter_func = _compile_argument_into_func('filter_func')
    a.failure_units_per_shot_func = _compile_argument_into_func('failure_units_per_shot_func')
    a.failure_values_func = _compile_argument_into_func('failure_values_func')
    raw_plot_args_func = eval(compile(
        f'lambda *, index, key, stats, stat, decoder, metadata, m, strong_id: {a.plot_args_func}',
        filename='plot_args_func:command_line_arg',
        mode='eval'))
    a.plot_args_func = lambda index, group_key, stats: raw_plot_args_func(
            index=index,
            key=group_key,
            stats=stats,
            stat=stats[0],
            decoder=stats[0].decoder,
            metadata=stats[0].json_metadata,
            m=_FieldToMetadataWrapper(stats[0].json_metadata),
            strong_id=stats[0].strong_id)
    return a


def _log_ticks(
        min_v: float,
        max_v: float,
) -> Tuple[float, float, List[float], List[float]]:
    d0 = math.floor(math.log10(min_v) + 0.0001)
    d1 = math.ceil(math.log10(max_v) - 0.0001)
    if d1 == d0:
        d1 += 1
        d0 -= 1
    return (
        10**d0,
        10**d1,
        [10**k for k in range(d0, d1 + 1)],
        [d*10**k for k in range(d0, d1) for d in range(2, 10)],
    )


def _sqrt_ticks(
        min_v: float,
        max_v: float,
) -> Tuple[float, float, List[float], List[float]]:
    if max_v == min_v:
        max_v *= 2
        min_v /= 2
    if max_v == min_v:
        max_v = 1
        min_v = 0
    d = max_v - min_v
    step = 10**math.floor(math.log10(d))
    small_step = step / 10

    start_k = math.floor(min_v / step)
    end_k = math.ceil(max_v / step) + 1
    major_ticks = [step * k for k in range(start_k, end_k)]
    if len(major_ticks) < 5:
        step /= 2
        start_k = math.floor(min_v / step)
        end_k = math.ceil(max_v / step) + 1
        major_ticks = [step * k for k in range(start_k, end_k)]

    small_start_k = math.floor(major_ticks[0] / small_step)
    small_end_k = math.ceil(major_ticks[-1] / small_step) + 1
    minor_ticks = [small_step * k for k in range(small_start_k, small_end_k)]

    return (
        major_ticks[0],
        major_ticks[-1],
        major_ticks,
        minor_ticks,
    )


def _pick_min_max(
        *,
        plotted_stats: Sequence['sinter.TaskStats'],
        v_func: Callable[['sinter.TaskStats'], Optional[float]],
        default_min: float,
        default_max: float,
        forced_min: Optional[float],
        forced_max: Optional[float],
        want_positive: bool,
        want_strictly_positive: bool,
) -> Tuple[float, float]:
    assert default_max >= default_min
    vs = []
    for stat in plotted_stats:
        v = v_func(stat)
        if isinstance(v, (int, float)):
            vs.append(v)
        elif isinstance(v, Fit):
            for e in [v.low, v.best, v.high]:
                if e is not None:
                    vs.append(e)
        elif v is None:
            pass
        else:
            raise NotImplementedError(f'{v=}')
    if want_positive:
        vs = [v for v in vs if v > 0]

    min_v = min(vs, default=default_min)
    max_v = max(vs, default=default_max)
    if forced_min is not None:
        min_v = forced_min
        max_v = max(min_v, max_v)
    if forced_max is not None:
        max_v = forced_max
        min_v = min(min_v, max_v)
    if want_positive:
        assert min_v >= 0
    if want_strictly_positive:
        assert min_v > 0
    assert max_v >= min_v

    return min_v, max_v


def _set_axis_scale_label_ticks(
        *,
        ax: Optional[plt.Axes],
        y_not_x: bool,
        axis_label: str,
        default_scale: str = 'linear',
        default_min_v: float = 0,
        default_max_v: float = 0,
        v_func: Callable[['sinter.TaskStats'], Optional[float]],
        forced_min_v: Optional[float] = None,
        forced_max_v: Optional[float] = None,
        plotted_stats: Sequence['sinter.TaskStats'],
) -> Optional[str]:
    if ax is None:
        return None
    set_scale = ax.set_yscale if y_not_x else ax.set_xscale
    set_label = ax.set_ylabel if y_not_x else ax.set_xlabel
    set_lim = cast(Callable[[Optional[float], Optional[float]], None], ax.set_ylim if y_not_x else ax.set_xlim)
    set_ticks = ax.set_yticks if y_not_x else ax.set_xticks

    if axis_label.startswith('[') and ']' in axis_label:
        axis_split = axis_label.index(']')
        scale_name = axis_label[1:axis_split]
        axis_label = axis_label[axis_split + 1:]
    else:
        scale_name = default_scale
    set_label(axis_label)

    min_v, max_v = _pick_min_max(
        plotted_stats=plotted_stats,
        v_func=v_func,
        default_min=default_min_v,
        default_max=default_max_v,
        forced_min=forced_min_v,
        forced_max=forced_max_v,
        want_positive=scale_name != 'linear',
        want_strictly_positive=scale_name == 'log',
    )

    if scale_name == 'linear':
        set_lim(min_v, max_v)
    elif scale_name == 'log':
        set_scale('log')
        min_v, max_v, major_ticks, minor_ticks = _log_ticks(min_v, max_v)
        if forced_min_v is not None:
            min_v = forced_min_v
        if forced_max_v is not None:
            max_v = forced_max_v
        set_ticks(major_ticks)
        set_ticks(minor_ticks, minor=True)
        set_lim(min_v, max_v)
    elif scale_name == 'sqrt':
        from matplotlib.scale import FuncScale
        min_v, max_v, major_ticks, minor_ticks = _sqrt_ticks(min_v, max_v)
        if forced_min_v is not None:
            min_v = forced_min_v
        if forced_max_v is not None:
            max_v = forced_max_v
        set_scale(FuncScale(ax, (lambda e: e**0.5, lambda e: e**2)))
        set_ticks(major_ticks)
        set_ticks(minor_ticks, minor=True)
        set_lim(min_v, max_v)
    else:
        raise NotImplemented(f'{scale_name=}')
    return scale_name


def _common_json_properties(stats: List['sinter.TaskStats']) -> Dict[str, Any]:
    vals = {}
    for stat in stats:
        if isinstance(stat.json_metadata, dict):
            for k in stat.json_metadata:
                vals[k] = set()
    for stat in stats:
        if isinstance(stat.json_metadata, dict):
            for k in vals:
                v = stat.json_metadata.get(k)
                if v is None or isinstance(v, (float, str, int)):
                    vals[k].add(v)
    if 'decoder' not in vals:
        vals['decoder'] = set()
        for stat in stats:
            vals['decoder'].add(stat.decoder)
    return {k: next(iter(v)) for k, v in vals.items() if len(v) == 1}


def _plot_helper(
    *,
    samples: Union[Iterable['sinter.TaskStats'], ExistingData],
    group_func: Callable[['sinter.TaskStats'], Any],
    filter_func: Callable[['sinter.TaskStats'], Any],
    preprocess_stats_func: Optional[Callable],
    failure_units_per_shot_func: Callable[['sinter.TaskStats'], Any],
    failure_values_func: Callable[['sinter.TaskStats'], Any],
    x_func: Callable[['sinter.TaskStats'], Any],
    y_func: Optional[Callable[['sinter.TaskStats'], Any]],
    failure_unit: str,
    plot_types: Sequence[str],
    highlight_max_likelihood_factor: Optional[float],
    xaxis: str,
    yaxis: Optional[str],
    min_y: Optional[float],
    max_y: Optional[float],
    max_x: Optional[float],
    min_x: Optional[float],
    title: Optional[str],
    subtitle: Optional[str],
    fig_size: Optional[Tuple[int, int]],
    plot_args_func: Callable[[int, Any, List['sinter.TaskStats']], Dict[str, Any]],
    line_fits: bool,
    point_label_func: Callable[['sinter.TaskStats'], Any] = lambda _: None,
    dpi: float,
) -> Tuple[plt.Figure, List[plt.Axes]]:
    if isinstance(samples, ExistingData):
        total = samples
    else:
        total = ExistingData()
        for sample in samples:
            total.add_sample(sample)
    total.data = {k: v
                  for k, v in total.data.items()
                  if bool(filter_func(v))}

    if preprocess_stats_func is not None:
        processed_stats = preprocess_stats_func(stats=list(total.data.values()))
        total.data = {}
        for stat in processed_stats:
            total.add_sample(stat)

    if not plot_types:
        if y_func is not None:
            plot_types = ['custom_y']
        else:
            plot_types = ['error_rate']
            if any(s.discards for s in total.data.values()):
                plot_types.append('discard_rate')
    include_error_rate_plot = 'error_rate' in plot_types
    include_discard_rate_plot = 'discard_rate' in plot_types
    include_custom_plot = 'custom_y' in plot_types
    num_plots = include_error_rate_plot + include_discard_rate_plot + include_custom_plot

    fig: plt.Figure
    ax_err: Optional[plt.Axes] = None
    ax_dis: Optional[plt.Axes] = None
    ax_cus: Optional[plt.Axes] = None
    fig, axes = plt.subplots(1, num_plots)
    if num_plots == 1:
        axes = [axes]
    axes = list(axes)
    pop_axes = list(axes)
    if include_custom_plot:
        ax_cus = pop_axes.pop()
    if include_discard_rate_plot:
        ax_dis = pop_axes.pop()
    if include_error_rate_plot:
        ax_err = pop_axes.pop()
    assert not pop_axes

    plotted_stats: List['sinter.TaskStats'] = [
        stat
        for stat in total.data.values()
    ]

    def stat_to_err_rate(stat: 'sinter.TaskStats') -> Optional[float]:
        if stat.shots <= stat.discards:
            return None
        err_rate = stat.errors / (stat.shots - stat.discards)
        pieces = failure_units_per_shot_func(stat)
        return shot_error_rate_to_piece_error_rate(err_rate, pieces=pieces)

    x_scale_name: Optional[str] = None
    for ax in [ax_err, ax_dis, ax_cus]:
        v = _set_axis_scale_label_ticks(
            ax=ax,
            y_not_x=False,
            axis_label=xaxis,
            default_scale='linear',
            default_min_v=1,
            default_max_v=10,
            forced_max_v=max_x,
            forced_min_v=min_x,
            plotted_stats=plotted_stats,
            v_func=x_func,
        )
        x_scale_name = x_scale_name or v

    y_scale_name: Optional[str] = None
    if ax_err is not None:
        y_scale_name = y_scale_name or _set_axis_scale_label_ticks(
            ax=ax_err,
            y_not_x=True,
            axis_label=f"Logical Error Rate (per {failure_unit})" if yaxis is None else yaxis,
            default_scale='log',
            forced_max_v=max_y if max_y is not None else 1 if min_y is None or 1 > min_y else None,
            default_min_v=1e-4,
            default_max_v=1,
            forced_min_v=min_y,
            plotted_stats=plotted_stats,
            v_func=stat_to_err_rate,
        )
        assert x_scale_name is not None
        assert y_scale_name is not None
        plot_error_rate(
            ax=ax_err,
            stats=plotted_stats,
            group_func=group_func,
            x_func=x_func,
            failure_units_per_shot_func=failure_units_per_shot_func,
            failure_values_func=failure_values_func,
            highlight_max_likelihood_factor=highlight_max_likelihood_factor,
            plot_args_func=plot_args_func,
            line_fits=None if not line_fits else (x_scale_name, y_scale_name),
            point_label_func=point_label_func,
        )
        ax_err.grid(which='major', color='#000000')
        ax_err.grid(which='minor', color='#DDDDDD')
        ax_err.legend()

    if ax_dis is not None:
        plot_discard_rate(
            ax=ax_dis,
            stats=plotted_stats,
            group_func=group_func,
            failure_units_per_shot_func=failure_units_per_shot_func,
            x_func=x_func,
            highlight_max_likelihood_factor=highlight_max_likelihood_factor,
            plot_args_func=plot_args_func,
            point_label_func=point_label_func,
        )
        ax_dis.set_yticks([p / 10 for p in range(11)], labels=[f'{10*p}%' for p in range(11)])
        ax_dis.set_ylim(0, 1)
        ax_dis.grid(which='major', color='#000000')
        ax_dis.grid(which='minor', color='#DDDDDD')
        if yaxis is not None and not include_custom_plot and ax_err is None:
            ax_dis.set_ylabel(yaxis)
        else:
            ax_dis.set_ylabel(f"Discard Rate (per {failure_unit})")
        ax_dis.legend()

    if ax_cus is not None:
        assert y_func is not None
        y_scale_name = y_scale_name or _set_axis_scale_label_ticks(
            ax=ax_cus,
            y_not_x=True,
            axis_label='custom' if yaxis is None else yaxis,
            default_scale='linear',
            default_min_v=1e-4,
            default_max_v=1,
            plotted_stats=plotted_stats,
            v_func=y_func,
            forced_min_v=min_y,
            forced_max_v=max_y,
        )
        plot_custom(
            ax=ax_cus,
            stats=plotted_stats,
            x_func=x_func,
            y_func=y_func,
            group_func=group_func,
            plot_args_func=plot_args_func,
            line_fits=None if not line_fits else (x_scale_name, y_scale_name),
            point_label_func=point_label_func,
        )
        ax_cus.grid(which='major', color='#000000')
        ax_cus.grid(which='minor', color='#DDDDDD')
        ax_cus.legend()

    stripped_xaxis = xaxis
    if stripped_xaxis is not None:
        if stripped_xaxis.startswith('[') and ']' in stripped_xaxis:
            stripped_xaxis = stripped_xaxis[stripped_xaxis.index(']') + 1:]

    vs_suffix = ''
    if stripped_xaxis is not None:
        vs_suffix = f' vs {stripped_xaxis}'
    if ax_err is not None:
        ax_err.set_title(f'Logical Error Rate per {failure_unit}{vs_suffix}')
        if title is not None:
            ax_err.set_title(title)
    if ax_dis is not None:
        ax_dis.set_title(f'Discard Rate per {failure_unit}{vs_suffix}')
    if ax_cus is not None:
        if title is not None:
            ax_cus.set_title(title)
        else:
            ax_cus.set_title(f'Custom Plot')
    if subtitle is not None:
        if '{common}' in subtitle:
            auto_subtitle = ', '.join(f'{k}={v}' for k, v in sorted(_common_json_properties(plotted_stats).items()))
            subtitle = subtitle.replace('{common}', auto_subtitle)
        for ax in axes:
            ax.set_title(ax.title.get_text() + '\n' + subtitle)

    if fig_size is None:
        fig.set_dpi(dpi)
        fig.set_size_inches(1000 * num_plots / dpi, 1000 / dpi)
    else:
        w, h = fig_size
        fig.set_dpi(dpi)
        fig.set_size_inches(w / dpi, h / dpi)
    fig.tight_layout()
    axs = [e for e in [ax_err, ax_dis] if e is not None]
    return fig, axs


class _FieldToMetadataWrapper:
    def __init__(self, d: Dict):
        self.__private_d = d

    def __getattr__(self, item):
        if isinstance(self.__private_d, dict):
            return self.__private_d.get(item, None)
        return None


def main_plot(*, command_line_args: List[str]):
    args = parse_args(command_line_args)
    total = ExistingData()
    for file in getattr(args, 'in'):
        total += ExistingData.from_file(file)

    if args.custom_error_count_keys:
        seen_keys = {k for stat in total.data.values() for k in stat.custom_counts}
        missing = []
        for k in args.custom_error_count_keys:
            if k not in seen_keys:
                missing.append(k)
        if missing:
            print("Warning: the following custom error count keys didn't appear in any statistic:", file=sys.stderr)
            for k in sorted(missing):
                print(f'    {k!r}', file=sys.stderr)
            print("Here are the keys that do appear:", file=sys.stderr)
            for k in sorted(seen_keys):
                print(f'    {k!r}', file=sys.stderr)

        total.data = {
            s.strong_id: s
            for v in total.data.values()
            for s in v._split_custom_counts(args.custom_error_count_keys)
        }

    fig, _ = _plot_helper(
        samples=total,
        group_func=args.group_func,
        x_func=args.x_func,
        point_label_func=args.point_label_func,
        y_func=args.y_func,
        filter_func=args.filter_func,
        failure_units_per_shot_func=args.failure_units_per_shot_func,
        failure_values_func=args.failure_values_func,
        plot_args_func=args.plot_args_func,
        failure_unit=args.failure_unit_name,
        plot_types=args.type,
        xaxis=args.xaxis,
        yaxis=args.yaxis,
        fig_size=args.fig_size,
        min_y=args.ymin,
        max_y=args.ymax,
        max_x=args.xmax,
        min_x=args.xmin,
        highlight_max_likelihood_factor=args.highlight_max_likelihood_factor,
        title=args.title,
        subtitle=args.subtitle,
        line_fits=args.line_fits,
        preprocess_stats_func=args.preprocess_stats_func,
        dpi=args.dpi,
    )
    if args.out is not None:
        fig.savefig(args.out, dpi=args.dpi)
    if args.show or args.out is None:
        plt.show()
