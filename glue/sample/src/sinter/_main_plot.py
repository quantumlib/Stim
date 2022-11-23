from typing import Any, Callable, Iterable, List, Optional, TYPE_CHECKING, Tuple, Union, Dict, Sequence
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
                        help='Customize the X axis label. Prefix [log] for log scale.')
    parser.add_argument('--yaxis',
                        type=str,
                        default=None,
                        help='Customize the Y axis label. Prefix [log] for log scale.')
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


def _ax_log_helper(*, ax: Optional[plt.Axes], log_x: bool, log_y: bool):
    if ax is None:
        return
    if log_x and log_y:
        ax.loglog()
    elif log_y:
        ax.semilogy()
    elif log_x:
        ax.semilogx()


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

    want_log_x = xaxis.startswith('[log]')
    force_log_y = yaxis is not None and yaxis.startswith('[log]')
    force_not_log_y = yaxis is not None and not yaxis.startswith('[log]')
    _ax_log_helper(ax=ax_err, log_x=want_log_x, log_y=not force_not_log_y)
    _ax_log_helper(ax=ax_dis, log_x=want_log_x, log_y=force_log_y)
    _ax_log_helper(ax=ax_cus, log_x=want_log_x, log_y=force_log_y)
    if xaxis.startswith('[log]'):
        xaxis = xaxis[5:]
    if yaxis is not None and yaxis.startswith('[log]'):
        yaxis = yaxis[5:]

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
        if min_y is None:
            min_y = 1
            for stat in plotted_stats:
                if stat.shots <= stat.discards:
                    continue
                err_rate = stat.errors / (stat.shots - stat.discards)
                pieces = failure_units_per_shot_func(stat)
                err_rate = shot_error_rate_to_piece_error_rate(err_rate, pieces=pieces)
                if err_rate < min_y:
                    min_y = err_rate
            if not plotted_stats:
                min_y = 1e-4
            low_d = 4
            while 10**-low_d > min_y*0.9 and low_d < 10:
                low_d += 1
            min_y = 10**-low_d
        major_tick_steps = 1
        while 10**-major_tick_steps >= min_y * 0.1:
            major_tick_steps += 1
        ax_err.set_yticks([10**-d for d in range(major_tick_steps)])
        ax_err.set_yticks([b*10**-d for d in range(1, major_tick_steps) for b in range(2, 10)], minor=True)
        ax_err.set_ylim(min_y, 1e-0)
        if yaxis is not None and not include_custom_plot:
            ax_err.set_ylabel(yaxis)
        else:
            ax_err.set_ylabel(f"Logical Error Rate (per {failure_unit})")
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
        ax_dis.set_ylim(0, 1)
        ax_err.grid()
        if yaxis is not None and not include_custom_plot:
            ax_err.set_ylabel(yaxis)
        else:
            ax_dis.set_ylabel(f"Discard Rate (per {failure_unit}")
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
        ax_cus.grid()
        if yaxis is not None:
            ax_cus.set_ylabel(yaxis)
        else:
            ax_cus.set_ylabel('custom')
        ax_cus.legend()

    inferred_x_axis = xaxis if xaxis is not None else 'custom'
    for ax in [ax_err, ax_dis, ax_cus]:
        if ax is not None:
            ax.set_xlabel(inferred_x_axis)

    vs_suffix = ''
    if xaxis is not None:
        vs_suffix = f' vs {xaxis}'
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
