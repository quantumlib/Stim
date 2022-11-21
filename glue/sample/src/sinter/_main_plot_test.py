import contextlib
import io
import pathlib
import tempfile

import pytest
from sinter._main import main


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
