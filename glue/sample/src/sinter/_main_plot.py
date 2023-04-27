import math
from typing import Any, Callable, Iterable, List, Optional, TYPE_CHECKING, Tuple, Union, Dict, Sequence, cast
import argparse

import matplotlib.pyplot as plt

from sinter import shot_error_rate_to_piece_error_rate
from sinter._main_combine import ExistingData
from sinter._plotting import plot_discard_rate, plot_custom
from sinter._plotting import plot_error_rate

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
                             '    decoder: The decoder that decoded the data for the data point.\n'
                             '    strong_id: The cryptographic hash of the case that was sampled for the data point.\n'
                             '    stat: The sinter.TaskStats object for the data point.\n'
                             'Expected expression type:\n'
                             '    Something that can be given to `bool` to get True or False.\n'
                             'Examples:\n'
                             '''    --filter_func "decoder=='pymatching'"\n'''
                             '''    --filter_func "0.001 < metadata['p'] < 0.005"\n''')
    parser.add_argument('--x_func',
                        type=str,
                        default="1",
                        help='A python expression that determines where points go on the x axis.\n'
                             'Values available to the python expression:\n'
                             '    metadata: The parsed value from the json_metadata for the data point.\n'
                             '    decoder: The decoder that decoded the data for the data point.\n'
                             '    strong_id: The cryptographic hash of the case that was sampled for the data point.\n'
                             '    stat: The sinter.TaskStats object for the data point.\n'
                             'Expected expression type:\n'
                             '    Something that can be given to `float` to get a float.\n'
                             'Examples:\n'
                             '''    --x_func "metadata['p']"\n'''
                             '''    --x_func "metadata['path'].split('/')[-1].split('.')[0]"\n'''
                        )
    parser.add_argument('--y_func',
                        type=str,
                        default=None,
                        help='A python expression that determines where points go on the y axis.\n'
                             'This argument is not used by error rate or discard rate plots; only\n'
                             'by the "custom_y" type plot.'
                             'Values available to the python expression:\n'
                             '    metadata: The parsed value from the json_metadata for the data point.\n'
                             '    decoder: The decoder that decoded the data for the data point.\n'
                             '    strong_id: The cryptographic hash of the case that was sampled for the data point.\n'
                             '    stat: The sinter.TaskStats object for the data point.\n'
                             'Expected expression type:\n'
                             '    Something that can be given to `float` to get a float.\n'
                             'Examples:\n'
                             '''    --x_func "metadata['p']"\n'''
                             '''    --x_func "metadata['path'].split('/')[-1].split('.')[0]"\n'''
                        )
    parser.add_argument('--fig_size',
                        type=int,
                        nargs=2,
                        default=None,
                        help='Desired figure width in pixels.')
    parser.add_argument('--fig_height',
                        type=int,
                        default=None,
                        help='Desired figure height in pixels.')
    parser.add_argument('--group_func',
                        type=str,
                        default="'all data (use -group_func and -x_func to group into curves)'",
                        help='A python expression that determines how points are grouped into curves.\n'
                             'Values available to the python expression:\n'
                             '    metadata: The parsed value from the json_metadata for the data point.\n'
                             '    decoder: The decoder that decoded the data for the data point.\n'
                             '    strong_id: The cryptographic hash of the case that was sampled for the data point.\n'
                             '    stat: The sinter.TaskStats object for the data point.\n'
                             'Expected expression type:\n'
                             '    Something that can be given to `str` to get a useful string.\n'
                             'Examples:\n'
                             '''    --group_func "(decoder, metadata['d'])"\n'''
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
                             'failure units per shot then a shot error rate of 10% corresponds to a unit failure rate\n'
                             'of 2.7129%. The conversion formula (assuming <50% error rates) is:\n'
                             '\n'
                             '    P_unit = 0.5 - 0.5 * (1 - 2 * P_shot)**(1/units_per_shot)\n'
                             '\n'
                             'Values available to the python expression:\n'
                             '    metadata: The parsed value from the json_metadata for the data point.\n'
                             '    decoder: The decoder that decoded the data for the data point.\n'
                             '    strong_id: The cryptographic hash of the case that was sampled for the data point.\n'
                             '    stat: The sinter.TaskStats object for the data point.\n'
                             '\n'
                             'Expected expression type:\n'
                             '    float.\n'
                             '\n'
                             'Examples:\n'
                             '''    --failure_units_per_shot_func "metadata['rounds']"\n'''
                             '''    --failure_units_per_shot_func "metadata['distance'] * 3"\n'''
                             '''    --failure_units_per_shot_func "10"\n'''
                        )
    parser.add_argument('--plot_args_func',
                        type=str,
                        default='''{'marker': 'ov*sp^<>8PhH+xXDd|'[index % 18]}''',
                        help='A python expression used to customize the look of curves.\n'
                             'Values available to the python expression:\n'
                             '    index: A unique integer identifying the curve.\n'
                             '    key: The group key (returned from --group_func) identifying the curve.\n'
                             '    stats: The list of sinter.TaskStats object in the group.\n'
                             '    metadata: (From one arbitrary data point in the group.) The parsed value from the json_metadata for the data point.\n'
                             '    decoder: (From one arbitrary data point in the group.) The decoder that decoded the data for the data point.\n'
                             '    strong_id: (From one arbitrary data point in the group.) The cryptographic hash of the case that was sampled for the data point.\n'
                             '    stat: (From one arbitrary data point in the group.) The sinter.TaskStats object for the data point.\n'
                             'Expected expression type:\n'
                             '    A dictionary to give to matplotlib plotting functions as a **kwargs argument.\n'
                             'Examples:\n'
                             """    --plot_args_func "'''{'label': 'curve #' + str(index), 'linewidth': 5}'''"\n"""
                             """    --plot_args_func "'''{'marker': 'ov*sp^<>8PhH+xXDd|'[index % 18]}'''"\n"""
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
                        help='Output file to write the plot to.\n'
                             'The file extension determines the type of image.\n'
                             'Either this or --show must be specified.')
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
    parser.add_argument('--show',
                        action='store_true',
                        help='Displays the plot in a window.\n'
                             'Either this or --out must be specified.')
    parser.add_argument('--ymin',
                        default=None,
                        type=float,
                        help='Sets the minimum value of the y axis (max always 1).')
    parser.add_argument('--title',
                        default=None,
                        type=str,
                        help='Sets the title of the plot.')
    parser.add_argument('--highlight_max_likelihood_factor',
                        type=float,
                        default=1000,
                        help='The relative likelihood ratio that determines the color highlights around curves.\n'
                             'Set this to 1 or larger. Set to 1 to disable highlighting.')

    a = parser.parse_args(args=args)
    if not a.show and a.out is None:
        raise ValueError("Must specify '--out file' or '--show'.")
    if 'custom_y' in a.type and a.y_func is None:
        raise ValueError("--type custom_y requires --y_func.")
    if a.y_func is not None and a.type and 'custom_y' not in a.type:
        raise ValueError("--y_func only works with --type custom_y.")
    if (len(a.type) == 0 and a.y_func is not None) or list(a.type) == 'custom_y':
        if a.failure_units_per_shot_func is not None:
            raise ValueError("--failure_units_per_shot_func doesn't affect --type custom_y")
    if (a.failure_units_per_shot_func is not None) != (a.failure_unit_name is not None):
        raise ValueError("--failure_units_per_shot_func and --failure_unit_name can only be specified together.")
    if a.failure_units_per_shot_func is None:
        a.failure_units_per_shot_func = "1"
    if a.failure_unit_name is None:
        a.failure_unit_name = 'shot'

    a.x_func = eval(compile(
        f'lambda *, stat, decoder, metadata, strong_id: {a.x_func}',
        filename='x_func:command_line_arg',
        mode='eval'))
    if a.y_func is not None:
        a.y_func = eval(compile(
            f'lambda *, stat, decoder, metadata, strong_id: {a.y_func}',
            filename='x_func:command_line_arg',
            mode='eval'))
    a.group_func = eval(compile(
        f'lambda *, stat, decoder, metadata, strong_id: {a.group_func}',
        filename='group_func:command_line_arg',
        mode='eval'))
    a.filter_func = eval(compile(
        f'lambda *, stat, decoder, metadata, strong_id: {a.filter_func}',
        filename='filter_func:command_line_arg',
        mode='eval'))
    a.failure_units_per_shot_func = eval(compile(
        f'lambda *, stat, decoder, metadata, strong_id: {a.failure_units_per_shot_func}',
        filename='failure_units_per_shot_func:command_line_arg',
        mode='eval'))
    a.plot_args_func = eval(compile(
        f'lambda *, index, key, stats, stat, decoder, metadata, strong_id: {a.plot_args_func}',
        filename='plot_args_func:command_line_arg',
        mode='eval'))
    return a


def _log_ticks(
        min_v: float,
        max_v: float,
) -> Tuple[float, float, List[float], List[float]]:
    d0 = math.floor(math.log10(min_v) + 0.0001)
    d1 = math.ceil(math.log10(max_v) - 0.0001)
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
) -> Tuple[float, float]:
    vs = [
        v
        for stat in plotted_stats
        if (v := v_func(stat)) is not None
        if v > 0 or not want_positive
    ]

    min_v = min(vs, default=default_min)
    max_v = max(vs, default=default_max)
    if forced_min is not None:
        min_v = min_v
        max_v = max(min_v, max_v)
    if forced_max is not None:
        max_v = max_v
        min_v = min(min_v, max_v)
    if want_positive:
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
        forced_scale: Optional[str] = None,
        forced_min_v: Optional[float] = None,
        forced_max_v: Optional[float] = None,
        plotted_stats: Sequence['sinter.TaskStats'],
):
    if ax is None:
        return
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
    )

    if scale_name == 'linear':
        pass
    elif scale_name == 'log':
        set_scale('log')
        min_v, max_v, major_ticks, minor_ticks = _log_ticks(min_v, max_v)
        set_ticks(major_ticks)
        set_ticks(minor_ticks, minor=True)
        set_lim(min_v, max_v)
    elif scale_name == 'sqrt':
        from matplotlib.scale import FuncScale
        min_v, max_v, major_ticks, minor_ticks = _sqrt_ticks(min_v, max_v)
        set_lim(min_v, max_v)
        set_scale(FuncScale(ax, (lambda e: e**0.5, lambda e: e**2)))
        set_ticks(major_ticks)
        set_ticks(minor_ticks, minor=True)
    else:
        raise NotImplemented(f'{scale_name=}')


def _plot_helper(
    *,
    samples: Union[Iterable['sinter.TaskStats'], ExistingData],
    group_func: Callable[['sinter.TaskStats'], Any],
    filter_func: Callable[['sinter.TaskStats'], Any],
    failure_units_per_shot_func: Callable[['sinter.TaskStats'], Any],
    x_func: Callable[['sinter.TaskStats'], Any],
    y_func: Optional[Callable[['sinter.TaskStats'], Any]],
    failure_unit: str,
    plot_types: Sequence[str],
    highlight_max_likelihood_factor: Optional[float],
    xaxis: str,
    yaxis: Optional[str],
    min_y: Optional[float],
    title: Optional[str],
    fig_size: Optional[Tuple[int, int]],
    plot_args_func: Callable[[int, Any, List['sinter.TaskStats']], Dict[str, Any]],
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
    if include_custom_plot:
        ax_cus = axes.pop()
    if include_discard_rate_plot:
        ax_dis = axes.pop()
    if include_error_rate_plot:
        ax_err = axes.pop()

    plotted_stats: List['sinter.TaskStats'] = [
        stat
        for stat in total.data.values()
        if filter_func(stat)
    ]

    def stat_to_err_rate(stat: 'sinter.TaskStats') -> Optional[float]:
        if stat.shots <= stat.discards:
            return None
        err_rate = stat.errors / (stat.shots - stat.discards)
        pieces = failure_units_per_shot_func(stat)
        return shot_error_rate_to_piece_error_rate(err_rate, pieces=pieces)

    for ax in [ax_err, ax_dis, ax_cus]:
        _set_axis_scale_label_ticks(
            ax=ax,
            y_not_x=False,
            axis_label=xaxis,
            default_scale='linear',
            default_min_v=1,
            default_max_v=10,
            forced_max_v=None,
            forced_min_v=None,
            plotted_stats=plotted_stats,
            v_func=x_func,
        )

    if ax_err is not None:
        plot_error_rate(
            ax=ax_err,
            stats=plotted_stats,
            group_func=group_func,
            x_func=x_func,
            failure_units_per_shot_func=failure_units_per_shot_func,
            highlight_max_likelihood_factor=highlight_max_likelihood_factor,
            plot_args_func=plot_args_func,
        )
        _set_axis_scale_label_ticks(
            ax=ax_err,
            y_not_x=True,
            axis_label=f"Logical Error Rate (per {failure_unit})" if yaxis is None else yaxis,
            default_scale='log',
            forced_max_v=1,
            default_min_v=1e-4,
            plotted_stats=plotted_stats,
            v_func=stat_to_err_rate,
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
        plot_custom(
            ax=ax_cus,
            stats=plotted_stats,
            x_func=x_func,
            y_func=y_func,
            group_func=group_func,
            filter_func=filter_func,
            plot_args_func=plot_args_func,
        )
        _set_axis_scale_label_ticks(
            ax=ax_cus,
            y_not_x=True,
            axis_label='custom' if yaxis is None else yaxis,
            default_scale='linear',
            default_min_v=1e-4,
            default_max_v=0,
            plotted_stats=plotted_stats,
            v_func=y_func,
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

    if fig_size is None:
        fig.set_size_inches(10 * num_plots, 10)
        fig.set_dpi(100)
    else:
        w, h = fig_size
        fig.set_size_inches(w / 100, h / 100)
        fig.set_dpi(100)
    fig.tight_layout()
    axs = [e for e in [ax_err, ax_dis] if e is not None]
    return fig, axs


def main_plot(*, command_line_args: List[str]):
    args = parse_args(command_line_args)
    total = ExistingData()
    for file in getattr(args, 'in'):
        total += ExistingData.from_file(file)

    fig, _ = _plot_helper(
        samples=total,
        group_func=lambda stat: args.group_func(
            stat=stat,
            decoder=stat.decoder,
            metadata=stat.json_metadata,
            strong_id=stat.strong_id),
        x_func=lambda stat: args.x_func(
            stat=stat,
            decoder=stat.decoder,
            metadata=stat.json_metadata,
            strong_id=stat.strong_id),
        y_func=None if args.y_func is None else lambda stat: args.y_func(
            stat=stat,
            decoder=stat.decoder,
            metadata=stat.json_metadata,
            strong_id=stat.strong_id),
        filter_func=lambda stat: args.filter_func(
            stat=stat,
            decoder=stat.decoder,
            metadata=stat.json_metadata,
            strong_id=stat.strong_id),
        failure_units_per_shot_func=lambda stat: args.failure_units_per_shot_func(
            stat=stat,
            decoder=stat.decoder,
            metadata=stat.json_metadata,
            strong_id=stat.strong_id),
        plot_args_func=lambda index, group_key, stats: args.plot_args_func(
            index=index,
            key=group_key,
            stats=stats,
            stat=stats[0],
            decoder=stats[0].decoder,
            metadata=stats[0].json_metadata,
            strong_id=stats[0].strong_id),
        failure_unit=args.failure_unit_name,
        plot_types=args.type,
        xaxis=args.xaxis,
        yaxis=args.yaxis,
        fig_size=args.fig_size,
        min_y=args.ymin,
        highlight_max_likelihood_factor=args.highlight_max_likelihood_factor,
        title=args.title,
    )
    if args.out is not None:
        fig.savefig(args.out)
    if args.show:
        plt.show()
