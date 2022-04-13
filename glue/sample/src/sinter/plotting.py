import collections
import dataclasses
from typing import Callable, TypeVar, List, Any, Tuple, Iterable, DefaultDict, Optional, TYPE_CHECKING

import matplotlib.colors as mcolors
import matplotlib.pyplot as plt

from sinter.anon_task_stats import AnonTaskStats
from sinter.probability_util import binominal_relative_likelihood_range
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


@dataclasses.dataclass
class DataPointId:
    curve_label: str
    x: float
    curve_appearance_id: Optional[Any] = None
    hidden: bool = False

    def effective_appearance_id(self) -> Any:
        if self.curve_appearance_id is not None:
            return self.curve_appearance_id
        return self.curve_label


@dataclasses.dataclass(frozen=True)
class CurveStats:
    label: str
    marker: str
    color: str
    xs: Tuple[float, ...]
    stats: Tuple[AnonTaskStats, ...]
    hidden: bool

    @staticmethod
    def from_samples(samples: Iterable['sinter.TaskStats'],
                     *,
                     curve_func: Callable[[TaskSummary], Optional[DataPointId]],
                     ) -> List['CurveStats']:
        pairs: DefaultDict[Any, List[Tuple[float, AnonTaskStats]]] = collections.defaultdict(list)

        seen_appearance_ids = set()
        for sample in samples:
            curve = curve_func(sample.to_case_summary())
            if curve is not None:
                aid = curve.effective_appearance_id()
                seen_appearance_ids.add(aid)
                key = curve.curve_label, aid, curve.hidden
                stats = sample.to_case_stats()
                pairs[key].append((curve.x, stats))

        appearance_id_to_int = {
            k: i
            for i, k in enumerate(sorted(
                seen_appearance_ids,
                key=better_sorted_str_terms,
            ))
        }

        out = []
        for key in sorted(pairs.keys(), key=better_sorted_str_terms):
            val = pairs[key]
            label, appearance_id, hidden = key
            xs = []
            stats = []
            for x, s in sorted(val, key=lambda e: e[0]):
                xs.append(x)
                stats.append(s)
            i = appearance_id_to_int[appearance_id]
            out.append(CurveStats(
                label=label,
                marker=MARKERS[i % len(MARKERS)],
                color=COLORS[i % len(COLORS)],
                xs=tuple(xs),
                stats=tuple(stats),
                hidden=hidden,
            ))

        return out


def plot_discard_rate(
        *,
        ax: plt.Axes,
        samples: 'Iterable[sinter.TaskStats]',
        curve_func: Callable[[TaskSummary], Optional[DataPointId]],
        highlight_likelihood_ratio: Optional[float] = 1e-3,
        xaxis: str,
) -> None:

    ax.set_ylim(0, 1)
    ax.set_ylabel('Discard Probability (per shot)')
    ax.set_yticks([p*0.1 for p in range(11)])
    ax.set_yticklabels([str(p * 10) + '%' for p in range(11)])
    ax.grid()

    if xaxis.startswith('[log]'):
        ax.semilogx()
        xaxis = xaxis[5:]
    if xaxis:
        ax.set_xlabel(xaxis)
        ax.set_title('Discard Rate vs ' + xaxis)
    else:
        ax.set_title('Discard Rates')

    curves = CurveStats.from_samples(samples, curve_func=curve_func)
    for curve in curves:
        if curve.hidden:
            continue
        xs = []
        ys = []
        ys_low = []
        ys_high = []
        for x, stats in zip(curve.xs, curve.stats):
            if stats.shots and highlight_likelihood_ratio is not None:
                xs.append(x)
                ys.append(stats.discards / stats.shots)
                if 0 < highlight_likelihood_ratio < 1:
                    low, high = binominal_relative_likelihood_range(
                        num_shots=stats.shots,
                        num_hits=stats.discards,
                        likelihood_ratio=highlight_likelihood_ratio)
                    ys_low.append(low)
                    ys_high.append(high)
        ax.plot(xs,
                ys,
                label=curve.label,
                marker=curve.marker,
                color=curve.color,
                zorder=100)
        if 0 < highlight_likelihood_ratio < 1:
            ax.fill_between(xs,
                            ys_low,
                            ys_high,
                            color=curve.color,
                            alpha=0.25)

    ax.legend()


def plot_error_rate(
        *,
        ax: plt.Axes,
        samples: 'Iterable[sinter.TaskStats]',
        curve_func: Callable[[TaskSummary], Optional[DataPointId]],
        highlight_likelihood_ratio: Optional[float] = 1e-3,
        xaxis: str = '',
) -> None:
    ax.set_ylabel('Logical Error Probability (per shot)')
    ax.grid()

    if xaxis.startswith('[log]'):
        ax.loglog()
        xaxis = xaxis[5:]
    else:
        ax.semilogy()
    if xaxis:
        ax.set_xlabel(xaxis)
        ax.set_title('Logical Error Rate vs ' + xaxis)
    else:
        ax.set_title('Logical Error Rates')
    all_ys = []

    curves = CurveStats.from_samples(samples, curve_func=curve_func)
    for curve in curves:
        xs = []
        ys = []
        ys_low = []
        ys_high = []
        for x, stats in zip(curve.xs, curve.stats):
            num_kept = stats.shots - stats.discards
            if num_kept:
                xs.append(x)
                ys.append(stats.errors / num_kept)
                if 0 < highlight_likelihood_ratio < 1:
                    low, high = binominal_relative_likelihood_range(num_shots=num_kept,
                                                                    num_hits=stats.errors,
                                                                    likelihood_ratio=highlight_likelihood_ratio)
                    ys_low.append(low)
                    ys_high.append(high)
        all_ys += ys
        all_ys += ys_low
        all_ys += ys_high
        if not curve.hidden:
            ax.plot(xs,
                    ys,
                    label=curve.label,
                    marker=curve.marker,
                    color=curve.color,
                    zorder=100)
            if 0 < highlight_likelihood_ratio < 1:
                ax.fill_between(xs,
                                ys_low,
                                ys_high,
                                color=curve.color,
                                alpha=0.25)

    min_y = min((y for y in all_ys if y > 0), default=0.002)
    low_d = 4
    while 10**-low_d > min_y and low_d < 10:
        low_d += 1
    ax.set_ylim(10**-low_d, 1e-0)
    ax.legend()
