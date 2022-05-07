import argparse
from typing import Callable, List, Any, Tuple, Iterable, Optional, TYPE_CHECKING, Union

import matplotlib.pyplot as plt

from sinter.plotting import DataPointId
from sinter.plotting import plot_discard_rate, plot_error_rate
from sinter.task_summary import TaskSummary
from sinter.main_combine import ExistingData

if TYPE_CHECKING:
    import sinter


def parse_args(args: List[str]) -> Any:
    parser = argparse.ArgumentParser(description='Plot collected CSV data.',
                                     prog='sinter plot')
    parser.add_argument('-filter_func',
                        type=str,
                        default="True",
                        help='A python expression that determines whether a case is kept or not.\n'
                             'Available values:\n'
                             '    metadata: The parsed value from the json_metadata column.\n'
                             '    decoder: The decoder that decoded the case.\n'
                             '    strong_id: The cryptographic hash of the case that was sampled.\n'
                             'Expected type:\n'
                             '    Something that can be given to `bool` to get True or False.\n'
                             'Examples:\n'
                             '''    -filter_func "decoder=='pymatching'"\n'''
                             '''    -filter_func "0.001 < metadata['p'] < 0.005"\n''')
    parser.add_argument('-x_func',
                        type=str,
                        default="1",
                        help='A python expression that determines where points go on the x axis.\n'
                             'Available values:\n'
                             '    metadata: The parsed value from the json_metadata column.\n'
                             '    decoder: The decoder that decoded the case.\n'
                             '    strong_id: The cryptographic hash of the case that was sampled.\n'
                             'Expected type:\n'
                             '    Something that can be given to `float` to get a float.\n'
                             'Examples:\n'
                             '''    -x_func "metadata['p']"\n'''
                             '''    -x_func "metadata['path'].split('/')[-1].split('.')[0]"\n'''
                        )
    parser.add_argument('-fig_size',
                        type=int,
                        nargs=2,
                        default=None,
                        help='Desired figure width in pixels.')
    parser.add_argument('-fig_height',
                        type=int,
                        default=None,
                        help='Desired figure height in pixels.')
    parser.add_argument('-group_func',
                        type=str,
                        default="'all data (use -group_func and -x_func to group into curves)'",
                        help='A python expression that determines how points are grouped into curves.\n'
                             'Available values:\n'
                             '    metadata: The parsed value from the json_metadata column.\n'
                             '    decoder: The decoder that decoded the case.\n'
                             '    strong_id: The cryptographic hash of the case that was sampled.\n'
                             'Expected type:\n'
                             '    Something that can be given to `str` to get a useful string.\n'
                             'Examples:\n'
                             '''    -group_func "(decoder, metadata['d'])"\n'''
                             '''    -group_func "metadata['path'].split('/')[-2]"\n'''
                        )
    parser.add_argument('-in',
                        type=str,
                        nargs='+',
                        required=True,
                        help='Input files to get data from.')
    parser.add_argument('-type',
                        choices=['error_rate', 'discard_rate'],
                        nargs='+',
                        default=(),
                        help='Picks the figures to include.')
    parser.add_argument('-out',
                        type=str,
                        default=None,
                        help='Output file to write the plot to.\n'
                             'The file extension determines the type of image.\n'
                             'Either this or -show must be specified.')
    parser.add_argument('-xaxis',
                        type=str,
                        default='[log]',
                        help='Customize the X axis title. Prefix [log] for log scale.')
    parser.add_argument('-show',
                        action='store_true',
                        help='Displays the plot in a window.\n'
                             'Either this or -out must be specified.')
    parser.add_argument('-highlight_likelihood_ratio',
                        type=float,
                        default=1e-3,
                        help='The relative likelihood ratio that determines the color highlights around curves.\n'
                             'Setting this outside (0, 1) disables showing highlights.')

    a = parser.parse_args(args=args)
    if not a.show and a.out is None:
        raise ValueError("Must specify '-out file' or '-show'.")
    a.x_func = eval(compile(
        'lambda *, decoder, metadata, strong_id: ' + a.x_func,
        filename='x_func:command_line_arg',
        mode='eval'))
    a.group_func = eval(compile(
        'lambda *, decoder, metadata, strong_id: ' + a.group_func,
        filename='group_func:command_line_arg',
        mode='eval'))
    a.filter_func = eval(compile(
        'lambda *, decoder, metadata, strong_id: ' + a.filter_func,
        filename='filter_func:command_line_arg',
        mode='eval'))
    return a


def plot(
    *,
    samples: Union[Iterable['sinter.TaskStats'], ExistingData],
    group_func: Callable[[TaskSummary], Any],
    filter_func: Callable[[TaskSummary], Any] = lambda e: True,
    x_func: Callable[[TaskSummary], Any],
    include_discard_rate_plot: bool = False,
    include_error_rate_plot: bool = False,
    highlight_likelihood_ratio: Optional[float] = 1e-3,
    xaxis: str,
    fig_size: Optional[Tuple[int, int]] = None,
) -> Tuple[plt.Figure, List[plt.Axes]]:
    if isinstance(samples, ExistingData):
        total = samples
    else:
        total = ExistingData()
        for sample in samples:
            total.add_sample(sample)
    total.data = {k: v
                  for k, v in total.data.items()
                  if bool(filter_func(v.to_task_summary()))}

    if not include_error_rate_plot and not include_discard_rate_plot:
        include_error_rate_plot = True
        include_discard_rate_plot = any(s.discards for s in total.data.values())

    fig: plt.Figure
    ax_err: Optional[plt.Axes]
    ax_dis: Optional[plt.Axes]
    if include_error_rate_plot and include_discard_rate_plot:
        fig, (ax_err, ax_dis) = plt.subplots(1, 2)
    elif include_error_rate_plot:
        fig, ax_err = plt.subplots(1, 1)
        ax_dis = None
    elif include_discard_rate_plot:
        fig, ax_dis = plt.subplots(1, 1)
        ax_err = None
    else:
        raise NotImplementedError()

    def curve_func(summary: TaskSummary) -> Optional[DataPointId]:
        if not filter_func(summary):
            return None
        return DataPointId(
            curve_label=str(group_func(summary)),
            x=float(x_func(summary)),
        )

    if ax_err is not None:
        plot_error_rate(
            ax=ax_err,
            samples=total.data.values(),
            curve_func=curve_func,
            highlight_likelihood_ratio=highlight_likelihood_ratio,
            xaxis=xaxis,
        )
    if ax_dis is not None:
        plot_discard_rate(
            ax=ax_dis,
            samples=total.data.values(),
            curve_func=curve_func,
            highlight_likelihood_ratio=highlight_likelihood_ratio,
            xaxis=xaxis,
        )

    if fig_size is None:
        fig.set_size_inches(10 * (include_discard_rate_plot + include_error_rate_plot), 10)
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

    fig, _ = plot(
        samples=total,
        group_func=lambda summary: args.group_func(
            decoder=summary.decoder,
            metadata=summary.json_metadata,
            strong_id=summary.strong_id),
        x_func=lambda summary: args.x_func(
            decoder=summary.decoder,
            metadata=summary.json_metadata,
            strong_id=summary.strong_id),
        filter_func=lambda summary: args.filter_func(
            decoder=summary.decoder,
            metadata=summary.json_metadata,
            strong_id=summary.strong_id),
        include_discard_rate_plot='discard_rate' in args.type,
        include_error_rate_plot='error_rate' in args.type,
        xaxis=args.xaxis,
        fig_size=args.fig_size,
        highlight_likelihood_ratio=args.highlight_likelihood_ratio,
    )
    if args.out is not None:
        fig.savefig(args.out)
    if args.show:
        plt.show()
