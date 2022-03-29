import argparse
import collections
from typing import Callable, TypeVar
from typing import List, Any, Tuple, Iterable, DefaultDict
from typing import Optional
from typing import Union

import matplotlib.colors as mcolors
import matplotlib.pyplot as plt

import simmer
from simmer.case_stats import CaseStats
from simmer.case_summary import CaseSummary
from simmer.main_combine import ExistingData
from simmer.probability_util import binominal_relative_likelihood_range

MARKERS = "ov*sp^<>8PhH+xXDd|" * 100
COLORS = list(mcolors.TABLEAU_COLORS) * 3


def parse_args(args: List[str]) -> Any:
    parser = argparse.ArgumentParser(description='Plot collected CSV data.',
                                     prog='simmer plot')
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


def better_sorted_str_terms(val: str) -> Tuple[Any, ...]:
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


def plot_case_stats(
        *,
        curves: Iterable[Tuple[Any, Iterable[Tuple[float, CaseStats]]]],
        highlight_likelihood_ratio: Optional[float],
        xaxis: str,
        include_error_rate_plot: bool,
        include_discard_rate_plot: bool,
    ) -> Tuple[plt.Figure, Optional[plt.Axes], Optional[plt.Axes]]:
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

    if ax_err is not None:
        ax_err.set_ylabel('Logical Error Probability (per shot)')
        ax_err.grid()

    if ax_dis is not None:
        ax_dis.set_ylim(0, 1)
        ax_dis.set_ylabel('Discard Probability (per shot)')
        ax_dis.set_yticks([p*0.1 for p in range(11)])
        ax_dis.set_yticklabels([str(p * 10) + '%' for p in range(11)])
        ax_dis.grid()

    if xaxis.startswith('[log]'):
        if ax_err is not None:
            ax_err.loglog()
        if ax_dis is not None:
            ax_dis.semilogx()
        xaxis = xaxis[5:]
    else:
        ax_err.semilogy()
    if xaxis:
        if ax_err is not None:
            ax_err.set_xlabel(xaxis)
            ax_err.set_title('Logical Error Rate vs ' + xaxis)
        if ax_dis is not None:
            ax_dis.set_xlabel(xaxis)
            ax_dis.set_title('Discard Rate vs ' + xaxis)
    else:
        if ax_err is not None:
            ax_err.set_title('Logical Error Rates')
        if ax_dis is not None:
            ax_dis.set_title('Discard Rates')
    all_ys1 = []

    for group_index, (group, x_stats) in enumerate(curves):
        xs1 = []
        ys1 = []
        ys1_low = []
        ys1_high = []
        xs2 = []
        ys2 = []
        ys2_low = []
        ys2_high = []
        for x, stats in sorted(x_stats, key=lambda e: e[0]):
            num_kept = stats.shots - stats.discards
            if num_kept:
                xs1.append(x)
                ys1.append(stats.errors / num_kept)
                if 0 < highlight_likelihood_ratio < 1:
                    low, high = binominal_relative_likelihood_range(num_shots=num_kept,
                                                                    num_hits=stats.errors,
                                                                    likelihood_ratio=highlight_likelihood_ratio)
                    ys1_low.append(low)
                    ys1_high.append(high)
            if stats.shots and highlight_likelihood_ratio is not None:
                xs2.append(x)
                ys2.append(stats.discards / stats.shots)
                if 0 < highlight_likelihood_ratio < 1:
                    low, high = binominal_relative_likelihood_range(num_shots=stats.shots,
                                                                    num_hits=stats.discards,
                                                                    likelihood_ratio=highlight_likelihood_ratio)
                    ys2_low.append(low)
                    ys2_high.append(high)
        all_ys1 += ys1
        all_ys1 += ys1_low
        all_ys1 += ys1_high
        if ax_err is not None:
            ax_err.plot(xs1, ys1, label=group, marker=MARKERS[group_index], color=COLORS[group_index], zorder=100)
        if ax_dis is not None:
            ax_dis.plot(xs2, ys2, label=group, marker=MARKERS[group_index], color=COLORS[group_index], zorder=100)
        if 0 < highlight_likelihood_ratio < 1:
            if ax_err is not None:
                ax_err.fill_between(xs1, ys1_low, ys1_high, color=COLORS[group_index], alpha=0.25)
            if ax_dis is not None:
                ax_dis.fill_between(xs2, ys2_low, ys2_high, color=COLORS[group_index], alpha=0.25)

    if ax_err is not None:
        min_y = min((y for y in all_ys1 if y > 0), default=0.002)
        low_d = 4
        while 10**-low_d > min_y and low_d < 10:
            low_d += 1
        ax_err.set_ylim(10**-low_d, 1e-0)

    if ax_err is not None:
        ax_err.legend()
    if ax_dis is not None:
        ax_dis.legend()
    return fig, ax_err, ax_dis


def plot(
    *,
    samples: Union[Iterable[simmer.SampleStats], ExistingData],
    group_func: Callable[[CaseSummary], Any],
    filter_func: Callable[[CaseSummary], Any],
    x_func: Callable[[CaseSummary], Any],
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
                  if bool(filter_func(v.to_case_summary()))}

    groups: DefaultDict[Any, List[Tuple[float, CaseStats]]] = collections.defaultdict(list)
    has_discards = False
    for sample in total.data.values():
        if sample.discards:
            has_discards = True
        g = str(group_func(sample.to_case_summary()))
        x = float(x_func(sample.to_case_summary()))
        groups[g].append((x, sample.to_case_stats()))

    sorted_groups = [
        (key, groups[key])
        for key in sorted(groups, key=lambda g: better_sorted_str_terms(g))
    ]
    if not include_error_rate_plot and not include_discard_rate_plot:
        include_error_rate_plot = True
        include_discard_rate_plot = has_discards
    fig, ax1, ax2 = plot_case_stats(
        curves=sorted_groups,
        highlight_likelihood_ratio=highlight_likelihood_ratio,
        xaxis=xaxis,
        include_error_rate_plot=include_error_rate_plot,
        include_discard_rate_plot=include_discard_rate_plot,
    )
    if fig_size is None:
        fig.set_size_inches(10 * (include_discard_rate_plot + include_error_rate_plot), 10)
        fig.set_dpi(100)
    else:
        w, h = fig_size
        fig.set_size_inches(w / 100, h / 100)
        fig.set_dpi(100)
    fig.tight_layout()
    axs = [e for e in [ax1, ax2] if e is not None]
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
