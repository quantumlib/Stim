import argparse
import collections
from typing import List, Any, Tuple, Iterable, DefaultDict

import matplotlib.colors as mcolors
import matplotlib.pyplot as plt

from simmer import CaseStats
from simmer.main_combine import ExistingData
from simmer.probability_util import binominal_relative_likelihood_range

MARKERS = "ov*sp^<>8PhH+xXDd|" * 100
COLORS = list(mcolors.TABLEAU_COLORS) * 3


def parse_args(args: List[str]) -> Any:
    parser = argparse.ArgumentParser(description='Plot collected CSV data.',
                                     prog='simmer plot')
    parser.add_argument('-x_func',
                        type=str,
                        default="1",
                        help='A python expression that transforms a `name` into an x coordinate.\n'
                             'Used to spread data points over the x axis.')
    parser.add_argument('-group_func',
                        type=str,
                        default="'use -group_func and -x_func to customize'",
                        help='A python expression that transforms a `name` into a group key.\n'
                             'Data points with the same group end up in the same curve.\n'
                             'The group key is used as the name of the curve in the legend.')
    parser.add_argument('-in',
                        type=str,
                        nargs='+',
                        required=True,
                        help='Input files to get data from.')
    parser.add_argument('-out',
                        type=str,
                        default=None,
                        help='Output file to write the plot to.\n'
                             'The file extension determines the type of image.\n'
                             'Either this or -show must be specified.')
    parser.add_argument('-show',
                        action='store_true',
                        help='Displays the plot in a window.\n'
                             'Either this or -out must be specified.')
    parser.add_argument('-likelihood_ratio',
                        type=float,
                        default=1e-3,
                        help='The relative likelihood ratio that determines the color highlights around curves.\n'
                             'Setting this outside (0, 1) disables showing highlights.')

    a = parser.parse_args(args=args)
    if not a.show and a.out is None:
        raise ValueError("Must specify '-out file' or '-show'.")
    a.x_func = eval('lambda name: ' + a.x_func)
    a.group_func = eval('lambda name: ' + a.group_func)
    return a


def plot_case_stats(
        *,
        curves: Iterable[Tuple[Any, Iterable[Tuple[float, CaseStats]]]],
        highlight_likelihood_ratio: float = 1e-3) -> Tuple[plt.Figure, plt.Axes, plt.Axes]:
    fig: plt.Figure
    ax1: plt.Axes
    ax2: plt.Axes
    fig, (ax1, ax2) = plt.subplots(1, 2)
    ax1.set_title('Error Rates')
    ax1.set_ylim(1e-5, 1e-0)
    ax1.loglog()
    ax1.grid()

    ax2.set_title('Discard Rates')
    ax2.set_ylim(0, 1)
    ax2.semilogx()
    ax2.set_yticks([p*0.1 for p in range(11)])
    ax2.set_yticklabels([str(p * 10) + '%' for p in range(11)])
    ax2.grid()

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
            num_kept = stats.num_shots - stats.num_discards
            if num_kept:
                xs1.append(x)
                ys1.append(stats.num_errors / num_kept)
                if 0 < highlight_likelihood_ratio < 1:
                    low, high = binominal_relative_likelihood_range(num_shots=num_kept,
                                                                    num_hits=stats.num_errors,
                                                                    likelihood_ratio=highlight_likelihood_ratio)
                    ys1_low.append(low)
                    ys1_high.append(high)
            if stats.num_shots:
                xs2.append(x)
                ys2.append(stats.num_discards / stats.num_shots)
                if 0 < highlight_likelihood_ratio < 1:
                    low, high = binominal_relative_likelihood_range(num_shots=stats.num_shots,
                                                                    num_hits=stats.num_discards,
                                                                    likelihood_ratio=highlight_likelihood_ratio)
                    ys2_low.append(low)
                    ys2_high.append(high)
        ax1.plot(xs1, ys1, label=group, marker=MARKERS[group_index], color=COLORS[group_index], zorder=100)
        ax2.plot(xs2, ys2, label=group, marker=MARKERS[group_index], color=COLORS[group_index], zorder=100)
        if 0 < highlight_likelihood_ratio < 1:
            ax1.fill_between(xs1, ys1_low, ys1_high, color=COLORS[group_index], alpha=0.25)
            ax2.fill_between(xs2, ys2_low, ys2_high, color=COLORS[group_index], alpha=0.25)

    ax1.legend()
    ax2.legend()
    return fig, ax1, ax2


def main_plot(*, command_line_args: List[str]):
    args = parse_args(command_line_args)
    total = ExistingData()
    for file in getattr(args, 'in'):
        total += ExistingData.from_file(file)

    groups: DefaultDict[Any, List[Tuple[float, CaseStats]]] = collections.defaultdict(list)
    for (name, strong_id), stats in total.data.items():
        g = args.group_func(name)
        x = float(args.x_func(name))
        groups[g].append((x, stats))

    sorted_groups = [
        (key, groups[key])
        for key in sorted(groups)
    ]
    fig, _, _ = plot_case_stats(
        curves=sorted_groups,
        highlight_likelihood_ratio=args.likelihood_ratio,
    )
    if args.out is not None:
        fig.savefig(args.out)
    if args.show:
        plt.show()
