import pathlib
import tempfile

from sinter._command._main import main


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
            "--dets",
            str(d / "input.dets"),
            "--dem",
            str(d / "input.dem"),
            "--decoder",
            "pymatching",
            "--dets_format",
            "dets",
            "--obs_out",
            str(d / "output.01"),
            "--obs_out_format",
            "01",
        ])
        with open(d / 'output.01') as f:
            assert f.read() == '1\n0\n'
