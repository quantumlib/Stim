import contextlib
import io
import pathlib
import tempfile

import pytest
from sinter._command._main import main
from sinter._command._main_plot import _log_ticks, _sqrt_ticks


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
                "--in",
                str(d / "input.csv"),
                "--out",
                str(d / "output.png"),
                "--x_func",
                "int('a' in metadata['path'])",
                "--group_func",
                "decoder",
                "--ymin",
                "1e-3",
                "--title",
                "test_plot",
            ])
        assert (d / "output.png").exists()


def test_main_plot_2():
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
                "--in",
                str(d / "input.csv"),
                "--out",
                str(d / "output.png"),
                "--x_func",
                "int('a' in metadata['path'])",
                "--group_func",
                "decoder",
                "--plot_args_func",
                "{'lw': 2, 'color': 'r' if decoder == 'never' else 'b'}",
            ])
        assert (d / "output.png").exists()


def test_main_plot_failure_units():
    with tempfile.TemporaryDirectory() as d:
        d = pathlib.Path(d)
        with open(d / f'input.csv', 'w') as f:
            print("""
shots,errors,discards,seconds,decoder,strong_id,json_metadata
300,1,20,1.0,pymatching,f256bab362f516ebe4d59a08ae67330ff7771ff738757cd738f4b30605ddccf6,"{""r"":15,""d"":5}"
300,100,200,2.0,pymatching,f256bab362f516ebe4d59a08ae67330ff7771ff738757cd738f4b30605ddccf7,"{""r"":9,""d"":3}"
9,5,4,6.0,pymatching,5fe5a6cd4226b1a910d57e5479d1ba6572e0b3115983c9516360916d1670000f,"{""r"":6,""d"":2}"
            """.strip(), file=f)

        out = io.StringIO()
        with pytest.raises(ValueError,  match="specified together"):
            main(command_line_args=[
                "plot",
                "--in",
                str(d / "input.csv"),
                "--out",
                str(d / "output.png"),
                "--x_func",
                "metadata['d']",
                "--group_func",
                "decoder",
                "--failure_units_per_shot_func",
                "metadata['r']",
            ])
        with pytest.raises(ValueError,  match="specified together"):
            main(command_line_args=[
                "plot",
                "--in",
                str(d / "input.csv"),
                "--out",
                str(d / "output.png"),
                "--x_func",
                "metadata['d']",
                "--group_func",
                "decoder",
                "--failure_unit_name",
                "Rounds",
            ])
        assert not (d / "output.png").exists()
        with contextlib.redirect_stdout(out):
            main(command_line_args=[
                "plot",
                "--in",
                str(d / "input.csv"),
                "--out",
                str(d / "output.png"),
                "--x_func",
                "metadata['d']",
                "--group_func",
                "decoder",
                "--failure_units_per_shot_func",
                "metadata['r']",
                "--failure_unit_name",
                "Rounds",
            ])
        assert (d / "output.png").exists()


def test_main_plot_xaxis():
    with tempfile.TemporaryDirectory() as d:
        d = pathlib.Path(d)
        with open(d / f'input.csv', 'w') as f:
            print("""
shots,errors,discards,seconds,decoder,strong_id,json_metadata
300,1,20,1.0,pymatching,f256bab362f516ebe4d59a08ae67330ff7771ff738757cd738f4b30605ddccf6,"{""r"":15,""d"":5}"
300,100,200,2.0,pymatching,f256bab362f516ebe4d59a08ae67330ff7771ff738757cd738f4b30605ddccf7,"{""r"":9,""d"":3}"
9,5,4,6.0,pymatching,5fe5a6cd4226b1a910d57e5479d1ba6572e0b3115983c9516360916d1670000f,"{""r"":6,""d"":2}"
            """.strip(), file=f)

        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            main(command_line_args=[
                "plot",
                "--in",
                str(d / "input.csv"),
                "--out",
                str(d / "output.png"),
                "--x_func",
                "metadata['d']",
                "--xaxis",
                "[sqrt]distance root",
            ])
        assert (d / "output.png").exists()

        with contextlib.redirect_stdout(out):
            main(command_line_args=[
                "plot",
                "--in",
                str(d / "input.csv"),
                "--out",
                str(d / "output2.png"),
                "--x_func",
                "metadata['d']",
                "--xaxis",
                "[log]distance log",
            ])
        assert (d / "output2.png").exists()

        with contextlib.redirect_stdout(out):
            main(command_line_args=[
                "plot",
                "--in",
                str(d / "input.csv"),
                "--out",
                str(d / "output3.png"),
                "--x_func",
                "metadata['d']",
                "--xaxis",
                "distance raw",
            ])
        assert (d / "output3.png").exists()


def test_main_plot_custom_y_func():
    with tempfile.TemporaryDirectory() as d:
        d = pathlib.Path(d)
        with open(d / f'input.csv', 'w') as f:
            print("""
shots,errors,discards,seconds,decoder,strong_id,json_metadata
300,1,20,1.0,pymatching,f256bab362f516ebe4d59a08ae67330ff7771ff738757cd738f4b30605ddccf6,"{""r"":15,""d"":5}"
300,100,200,2.0,pymatching,f256bab362f516ebe4d59a08ae67330ff7771ff738757cd738f4b30605ddccf7,"{""r"":9,""d"":3}"
9,5,4,6.0,pymatching,5fe5a6cd4226b1a910d57e5479d1ba6572e0b3115983c9516360916d1670000f,"{""r"":6,""d"":2}"
            """.strip(), file=f)

        out = io.StringIO()
        with pytest.raises(AttributeError,  match="secondsX"):
            main(command_line_args=[
                "plot",
                "--in",
                str(d / "input.csv"),
                "--out",
                str(d / "output.png"),
                "--x_func",
                "metadata['d']",
                "--group_func",
                "decoder",
                "--y_func",
                "stat.secondsX",
                "--yaxis",
                "test axis"
            ])
        assert not (d / "output.png").exists()
        with contextlib.redirect_stdout(out):
            main(command_line_args=[
                "plot",
                "--in",
                str(d / "input.csv"),
                "--out",
                str(d / "output.png"),
                "--x_func",
                "metadata['d']",
                "--group_func",
                "decoder",
                "--y_func",
                "stat.seconds",
                "--yaxis",
                "test axis"
            ])
        assert (d / "output.png").exists()


def test_log_ticks():
    assert _log_ticks(12, 499) == (
        10,
        1000,
        [10, 100, 1000],
        [20, 30, 40, 50, 60, 70, 80, 90, 200, 300, 400, 500, 600, 700, 800, 900],
    )

    assert _log_ticks(1.2, 4.9) == (
        1,
        10,
        [1, 10],
        [2, 3, 4, 5, 6, 7, 8, 9],
    )


def test_sqrt_ticks():
    assert _sqrt_ticks(12, 499) == (
        0,
        500,
        [0, 100, 200, 300, 400, 500],
        [10*k for k in range(51)],
    )

    assert _sqrt_ticks(105, 499) == (
        100,
        500,
        [100, 200, 300, 400, 500],
        [10*k for k in range(10, 51)],
    )

    assert _sqrt_ticks(305, 590) == (
        300,
        600,
        [300, 350, 400, 450, 500, 550, 600],
        [10*k for k in range(30, 61)],
    )

    assert _sqrt_ticks(305000, 590000) == (
        300000,
        600000,
        [300000, 350000, 400000, 450000, 500000, 550000, 600000],
        [10000*k for k in range(30, 61)],
    )


def test_main_plot_degenerate_data():
    with tempfile.TemporaryDirectory() as d:
        d = pathlib.Path(d)
        with open(d / f'input.csv', 'w') as f:
            print("""
                shots,errors,discards,seconds,decoder,strong_id,json_metadata
                  100,     0,       0,   1.00,magical,000000000,"{}"
            """.strip(), file=f)

        main(command_line_args=[
            "plot",
            "--in",
            str(d / "input.csv"),
            "--out",
            str(d / "output.png"),
        ])
        assert (d / "output.png").exists()


def test_main_plot_degenerate_data_sqrt_axis():
    with tempfile.TemporaryDirectory() as d:
        d = pathlib.Path(d)
        with open(d / f'input.csv', 'w') as f:
            print("""
                shots,errors,discards,seconds,decoder,strong_id,json_metadata
                  100,     0,       0,   1.00,magical,000000000,"{}"
            """.strip(), file=f)

        main(command_line_args=[
            "plot",
            "--in",
            str(d / "input.csv"),
            "--out",
            str(d / "output.png"),
            "--xaxis",
            "[sqrt]x",
        ])
        assert (d / "output.png").exists()


def test_failure_values_func():
    with tempfile.TemporaryDirectory() as d:
        d = pathlib.Path(d)
        with open(d / f'input.csv', 'w') as f:
            print("""
                shots,errors,discards,seconds,decoder,strong_id,json_metadata
                 1000,   400,       0,   1.00,magical,000000001,"{""f"":1}"
                 1000,   400,       0,   1.00,magical,000000002,"{""f"":2}"
                 1000,   400,       0,   1.00,magical,000000003,"{""f"":3}"
                 1000,   400,       0,   1.00,magical,000000005,"{""f"":5}"
            """.strip(), file=f)

        main(command_line_args=[
            "plot",
            "--in",
            str(d / "input.csv"),
            "--out",
            str(d / "output.png"),
            "--xaxis",
            "values",
            "--x_func",
            "metadata['f']",
            "--subtitle",
            "test",
            "--failure_values_func",
            "metadata['f']",
            "--failure_units_per_shot_func",
            "100",
            "--failure_unit_name",
            "rounds",
        ])
        assert (d / "output.png").exists()


def test_m_fields():
    with tempfile.TemporaryDirectory() as d:
        d = pathlib.Path(d)
        with open(d / f'input.csv', 'w') as f:
            print("""
                shots,errors,discards,seconds,decoder,strong_id,json_metadata
                 1000,   400,       0,   1.00,magical,000000001,"{""f"":1}"
                 1000,   400,       0,   1.00,magical,000000002,"{""f"":2}"
                 1000,   400,       0,   1.00,magical,000000003,"{""f"":3}"
                 1000,   400,       0,   1.00,magical,000000005,"{""f"":5}"
            """.strip(), file=f)

        main(command_line_args=[
            "plot",
            "--in",
            str(d / "input.csv"),
            "--out",
            str(d / "output.png"),
            "--xaxis",
            "values",
            "--x_func",
            "m.f",
            "--group_func",
            "m.g",
            "--subtitle",
            "test",
        ])
        assert (d / "output.png").exists()


def test_split_custom_counts():
    with tempfile.TemporaryDirectory() as d:
        d = pathlib.Path(d)
        with open(d / f'input.csv', 'w') as f:
            print("""
                shots,errors,discards,seconds,decoder,strong_id,json_metadata,custom_counts
                 1000,   400,       0,   1.00,magical,000000001,"{""f"":1}",
                 1000,   400,       0,   1.00,magical,000000002,"{""f"":2}","{""a"":3}"
                 1000,   400,       0,   1.00,magical,000000003,"{""f"":3}","{""b"":3,""c"":4}"
                 1000,   400,       0,   1.00,magical,000000005,"{""f"":5}",
            """.strip(), file=f)

        main(command_line_args=[
            "plot",
            "--in",
            str(d / "input.csv"),
            "--out",
            str(d / "output.png"),
            "--xaxis",
            "values",
            "--x_func",
            "m.f",
            "--group_func",
            "m.g",
            "--subtitle",
            "test",
            "--custom_error_count_keys",
            "a",
            "b",
        ])
        assert (d / "output.png").exists()


def test_line_fits():
    with tempfile.TemporaryDirectory() as d:
        d = pathlib.Path(d)
        with open(d / f'input.csv', 'w') as f:
            print("""
                shots,errors,discards,seconds,decoder,strong_id,json_metadata,custom_counts
                 1000,   400,       0,   1.00,magical,000000001,"{""a"":1,""b"":1}",
                 1000,   400,       0,   1.00,magical,000000002,"{""a"":2,""b"":1}"
            """.strip(), file=f)

        main(command_line_args=[
            "plot",
            "--in",
            str(d / "input.csv"),
            "--out",
            str(d / "output.png"),
            "--xaxis",
            "values",
            "--x_func",
            "m.a",
            "--group_func",
            "m.b",
            "--xmin",
            "0",
            "--xmax",
            "10",
            "--line_fits"
        ])
        assert (d / "output.png").exists()
