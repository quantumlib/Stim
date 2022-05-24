import contextlib
import io
import pathlib
import tempfile

import stim

from sinter.main import main
from sinter.main_combine import ExistingData
from sinter.plotting import better_sorted_str_terms, split_by


def test_split_by():
    assert split_by('abcdefcccghi', lambda e: e == 'c') == [list('ab'), list('c'), list('def'), list('ccc'), list('ghi')]


def test_better_sorted_str_terms():
    assert better_sorted_str_terms('a') == ('a',)
    assert better_sorted_str_terms('abc') == ('abc',)
    assert better_sorted_str_terms('a1b2') == ('a', 1, 'b', 2)
    assert better_sorted_str_terms('a1.5b2') == ('a', 1.5, 'b', 2)
    assert better_sorted_str_terms('a1.5.3b2') == ('a', (1, 5, 3), 'b', 2)
    assert sorted([
        "planar d=10 r=30",
        "planar d=16 r=36",
        "planar d=4 r=12",
        "toric d=10 r=30",
        "toric d=18 r=54",
    ], key=better_sorted_str_terms) == [
        "planar d=4 r=12",
        "planar d=10 r=30",
        "planar d=16 r=36",
        "toric d=10 r=30",
        "toric d=18 r=54",
    ]


def test_main_collect():
    with tempfile.TemporaryDirectory() as d:
        d = pathlib.Path(d)
        for distance in [3, 5, 7]:
            c = stim.Circuit.generated(
                'repetition_code:memory',
                rounds=3,
                distance=distance,
                after_clifford_depolarization=0.02)
            with open(d / f'{distance}.stim', 'w') as f:
                print(c, file=f)

        # Collects requested stats.
        main(command_line_args=[
            "collect",
            "-circuits",
            str(d / "3.stim"),
            str(d / "5.stim"),
            str(d / "7.stim"),
            "-max_shots",
            "1000",
            "-max_errors",
            "10",
            "-decoders",
            "pymatching",
            "-processes",
            "4",
            "-quiet",
            "-save_resume_filepath",
            str(d / "out.csv"),
        ])
        data = ExistingData.from_file(d / "out.csv").data
        assert len(data) == 3
        for k, v in data.items():
            assert v.discards == 0
            assert v.errors <= 50
            assert v.shots >= 1000

        # No more work when existing stats at merge location are sufficient.
        with open(d / "out.csv") as f:
            contents1 = f.read()
        main(command_line_args=[
            "collect",
            "-circuits",
            str(d / "3.stim"),
            str(d / "5.stim"),
            str(d / "7.stim"),
            "-max_shots",
            "1000",
            "-max_errors",
            "10",
            "-decoders",
            "pymatching",
            "-processes",
            "4",
            "-quiet",
            "-save_resume_filepath",
            str(d / "out.csv"),
        ])
        with open(d / "out.csv") as f:
            contents2 = f.read()
        assert contents1 == contents2

        # No more work when existing work is sufficient.
        main(command_line_args=[
            "collect",
            "-circuits",
            str(d / "3.stim"),
            str(d / "5.stim"),
            str(d / "7.stim"),
            "-max_shots",
            "1000",
            "-max_errors",
            "10",
            "-decoders",
            "pymatching",
            "-processes",
            "4",
            "-quiet",
            "-existing_data_filepaths",
            str(d / "out.csv"),
            "-save_resume_filepath",
            str(d / "out2.csv"),
        ])
        data2 = ExistingData.from_file(d / "out2.csv").data
        assert len(data2) == 0


def test_main_combine():
    with tempfile.TemporaryDirectory() as d:
        d = pathlib.Path(d)
        with open(d / f'input.csv', 'w') as f:
            print("""
shots,errors,discards,seconds,decoder,strong_id,json_metadata
300,1,20,1.0,pymatching,f256bab362f516ebe4d59a08ae67330ff7771ff738757cd738f4b30605ddccf6,"{""path"":""a.stim""}"
300,100,200,2.0,pymatching,f256bab362f516ebe4d59a08ae67330ff7771ff738757cd738f4b30605ddccf6,"{""path"":""a.stim""}"
9,5,4,6.0,pymatching,5fe5a6cd4226b1a910d57e5479d1ba6572e0b3115983c9516360916d1670000f,"{""path"":""b.stim""}"
            """.strip(), file=f)

        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            main(command_line_args=[
                "combine",
                str(d / "input.csv"),
            ])
        assert out.getvalue() == """     shots,    errors,  discards, seconds,decoder,strong_id,json_metadata
       600,       101,       220,    3.00,pymatching,f256bab362f516ebe4d59a08ae67330ff7771ff738757cd738f4b30605ddccf6,"{""path"":""a.stim""}"
         9,         5,         4,    6.00,pymatching,5fe5a6cd4226b1a910d57e5479d1ba6572e0b3115983c9516360916d1670000f,"{""path"":""b.stim""}"
"""

        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            main(command_line_args=[
                "combine",
                str(d / "input.csv"),
                str(d / "input.csv"),
            ])
        assert out.getvalue() == """     shots,    errors,  discards, seconds,decoder,strong_id,json_metadata
      1200,       202,       440,    6.00,pymatching,f256bab362f516ebe4d59a08ae67330ff7771ff738757cd738f4b30605ddccf6,"{""path"":""a.stim""}"
        18,        10,         8,    12.0,pymatching,5fe5a6cd4226b1a910d57e5479d1ba6572e0b3115983c9516360916d1670000f,"{""path"":""b.stim""}"
"""


def test_main_plot():
    with tempfile.TemporaryDirectory() as d:
        d = pathlib.Path(d)
        with open(d / f'input.csv', 'w') as f:
            print("""
shots,errors,discards,seconds,decoder,strong_id,json_metadata
300,1,20,1.0,pymatching,f256bab362f516ebe4d59a08ae67330ff7771ff738757cd738f4b30605ddccf6,"{""path"":""a.stim""}"
300,100,200,2.0,pymatching,f256bab362f516ebe4d59a08ae67330ff7771ff738757cd738f4b30605ddccf6,"{""path"":""a.stim""}"
9,5,4,6.0,pymatching,5fe5a6cd4226b1a910d57e5479d1ba6572e0b3115983c9516360916d1670000f,"{""path"":""b.stim""}"
            """.strip(), file=f)

        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            main(command_line_args=[
                "plot",
                "-in",
                str(d / "input.csv"),
                "-out",
                str(d / "output.png"),
                "-x_func",
                "int('a' in metadata['path'])",
                "-group_func",
                "decoder",
            ])
        assert (d / "output.png").exists()


def test_main_predict():
    with tempfile.TemporaryDirectory() as d:
        d = pathlib.Path(d)
        with open(d / f'input.dets', 'w') as f:
            print("""
shot D0
shot
            """, file=f)
        with open(d / f'input.dem', 'w') as f:
            print("""
error(0.1) D0 L0
            """, file=f)

        main(command_line_args=[
            "predict",
            "-dets",
            str(d / "input.dets"),
            "-dem",
            str(d / "input.dem"),
            "-decoder",
            "pymatching",
            "-dets_format",
            "dets",
            "-out",
            str(d / "output.01"),
            "-out_format",
            "01",
        ])
        with open(d / 'output.01') as f:
            assert f.read() == '1\n0\n'
