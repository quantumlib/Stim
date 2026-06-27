import io

from matplotlib import pyplot as plt

import sinter


def test_better_sorted_str_terms():
    f = sinter.better_sorted_str_terms
    assert f('everyone et al. 2020') == ('everyone et al', '.', ' ', 2020)
    assert f('a') == ('a',)
    assert f('abc') == ('abc',)
    assert f('a1b2') == ('a', 1, 'b', 2)
    assert f('a1.5b2') == ('a', 1.5, 'b', 2)
    assert f('a1.5.3b2') == ('a', (1, 5, 3), 'b', 2)
    assert f(1) < f(None)
    assert f(1) < f('2')
    assert f('2') > f(1)
    assert sorted([
        "planar d=10 r=30",
        "planar d=16 r=36",
        "planar d=4 r=12",
        "toric d=10 r=30",
        "toric d=18 r=54",
    ], key=f) == [
        "planar d=4 r=12",
        "planar d=10 r=30",
        "planar d=16 r=36",
        "toric d=10 r=30",
        "toric d=18 r=54",
    ]
    assert sorted([
        "a1",
        "1a",
    ], key=f) == [
        "1a",
        "a1",
    ]


def test_plotting_does_not_crash():
    data = io.StringIO()
    data.write("""  shots,errors,discards,seconds,decoder,strong_id,json_metadata
1000000, 837,0,36.6,pymatching,9f7e20c54fec45b6aef7491b774dd5c0a3b9a005aa82faf5b9c051d6e40d60a9,"{""d"":3,""p"":0.001}"
  53498,1099,0,6.52,pymatching,3f40432443a99b933fb548b831fb54e7e245d9d73a35c03ea5a2fb2ce270f8c8,"{""d"":3,""p"":0.005}"
  16269,1023,0,3.23,pymatching,17b2e0c99560d20307204494ac50e31b33e50721b4ebae99d9e3577ae7248874,"{""d"":3,""p"":0.01}"
1000000, 151,0,77.3,pymatching,e179a18739201250371ffaae0197d8fa19d26b58dfc2942f9f1c85568645387a,"{""d"":5,""p"":0.001}"
  11363,1068,0,12.5,pymatching,a4dec28934a033215ff1389651a26114ecc22016a6e122008830cf7dd04ba5ad,"{""d"":5,""p"":0.01}"
  61569,1001,0,24.5,pymatching,2fefcc356752482fb4c6d912c228f6d18762f5752796c668b6abeb7775f5de92,"{""d"":5,""p"":0.005}"
""")
    data.seek(0)
    stats = sinter.stats_from_csv_files(data)

    fig, ax = plt.subplots(1, 1)
    sinter.plot_error_rate(
        ax=ax,
        stats=stats,
        group_func=lambda e: f"Rotated Surface Code d={e.json_metadata['d']}",
        x_func=lambda e: e.json_metadata['p'],
        plot_args_func=lambda k, e: {'marker': "ov*sp^<>8PhH+xXDd|"[k]},
    )
    sinter.plot_error_rate(
        ax=ax,
        stats=stats,
        group_func=lambda e: f"Rotated Surface Code d={e.json_metadata['d']}",
        x_func=lambda e: e.json_metadata['p'],
        plot_args_func=lambda k, e: {'marker': "ov*sp^<>8PhH+xXDd|"[k]},
        failure_units_per_shot_func=lambda stats: stats.json_metadata['d'] * 3,
    )
    sinter.plot_error_rate(
        ax=ax,
        stats=stats,
        group_func=lambda e: f"Rotated Surface Code d={e.json_metadata['d']}",
        x_func=lambda e: e.json_metadata['p'],
        plot_args_func=lambda k, e: {'marker': "ov*sp^<>8PhH+xXDd|"[k]},
        failure_units_per_shot_func=lambda stats: stats.json_metadata['d'] * 3,
        highlight_max_likelihood_factor=None,
    )
    sinter.plot_discard_rate(
        ax=ax,
        stats=stats,
        group_func=lambda e: f"Rotated Surface Code d={e.json_metadata['d']}",
        x_func=lambda e: e.json_metadata['p'],
        plot_args_func=lambda k, e: {'marker': "ov*sp^<>8PhH+xXDd|"[k]},
    )
    sinter.plot_discard_rate(
        ax=ax,
        stats=stats,
        group_func=lambda e: f"Rotated Surface Code d={e.json_metadata['d']}",
        x_func=lambda e: e.json_metadata['p'],
        plot_args_func=lambda k, e: {'marker': "ov*sp^<>8PhH+xXDd|"[k]},
        highlight_max_likelihood_factor=None,
    )
    sinter.plot_discard_rate(
        ax=ax,
        stats=stats,
        group_func=lambda e: f"Rotated Surface Code d={e.json_metadata['d']}",
        x_func=lambda e: e.json_metadata['p'],
        plot_args_func=lambda k, e: {'marker': "ov*sp^<>8PhH+xXDd|"[k]},
        failure_units_per_shot_func=lambda stats: stats.json_metadata['d'] * 3,
    )


def test_group_by():
    assert sinter.group_by([1, 2, 3], key=lambda i: i == 2) == {False: [1, 3], True: [2]}
    assert sinter.group_by(range(10), key=lambda i: i % 3) == {0: [0, 3, 6, 9], 1: [1, 4, 7], 2: [2, 5, 8]}
    assert sinter.group_by([], key=lambda i: 0) == {}
    assert sinter.group_by([1, 2, 3, 1], key=lambda i: 0) == {0: [1, 2, 3, 1]}
