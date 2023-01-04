import contextlib
import io
import pathlib
import tempfile

from sinter._main import main


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


def test_order_flag():
    with tempfile.TemporaryDirectory() as d:
        d = pathlib.Path(d)
        with open(d / f'input.csv', 'w') as f:
            print("""
shots,errors,discards,seconds,decoder,   strong_id,json_metadata
1000, 100,   4,       2.0,    pymatching,deadbeef0,"{""d"":19}"
2000, 300,   3,       3.0,    pymatching,deadbeef1,"{""d"":9}"
3000, 200,   2000,    5.0,    pymatching,deadbeef2,"{""d"":200}"
4000, 100,   1,       7.0,    pymatching,deadbeef3,"{""d"":3}"
5000, 100,   0,       11,     pymatching,deadbeef4,"{""d"":5}"
            """.strip(), file=f)

        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            main(command_line_args=[
                "combine",
                "--order",
                "preserve",
                str(d / "input.csv"),
                str(d / "input.csv"),
            ])
        assert out.getvalue() == """     shots,    errors,  discards, seconds,decoder,strong_id,json_metadata
      2000,       200,         8,    4.00,pymatching,deadbeef0,"{""d"":19}"
      4000,       600,         6,    6.00,pymatching,deadbeef1,"{""d"":9}"
      6000,       400,      4000,    10.0,pymatching,deadbeef2,"{""d"":200}"
      8000,       200,         2,    14.0,pymatching,deadbeef3,"{""d"":3}"
     10000,       200,         0,    22.0,pymatching,deadbeef4,"{""d"":5}"
"""

        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            main(command_line_args=[
                "combine",
                "--order",
                "metadata",
                str(d / "input.csv"),
                str(d / "input.csv"),
            ])
        assert out.getvalue() == """     shots,    errors,  discards, seconds,decoder,strong_id,json_metadata
      8000,       200,         2,    14.0,pymatching,deadbeef3,"{""d"":3}"
     10000,       200,         0,    22.0,pymatching,deadbeef4,"{""d"":5}"
      4000,       600,         6,    6.00,pymatching,deadbeef1,"{""d"":9}"
      2000,       200,         8,    4.00,pymatching,deadbeef0,"{""d"":19}"
      6000,       400,      4000,    10.0,pymatching,deadbeef2,"{""d"":200}"
"""

        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            main(command_line_args=[
                "combine",
                "--order",
                "error",
                str(d / "input.csv"),
                str(d / "input.csv"),
            ])
        assert out.getvalue() == """     shots,    errors,  discards, seconds,decoder,strong_id,json_metadata
     10000,       200,         0,    22.0,pymatching,deadbeef4,"{""d"":5}"
      8000,       200,         2,    14.0,pymatching,deadbeef3,"{""d"":3}"
      2000,       200,         8,    4.00,pymatching,deadbeef0,"{""d"":19}"
      4000,       600,         6,    6.00,pymatching,deadbeef1,"{""d"":9}"
      6000,       400,      4000,    10.0,pymatching,deadbeef2,"{""d"":200}"
"""
