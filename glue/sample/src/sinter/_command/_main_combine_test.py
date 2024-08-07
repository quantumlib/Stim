import contextlib
import io
import pathlib
import tempfile

from sinter._command._main import main


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
        assert out.getvalue() == """     shots,    errors,  discards, seconds,decoder,strong_id,json_metadata,custom_counts
       600,       101,       220,    3.00,pymatching,f256bab362f516ebe4d59a08ae67330ff7771ff738757cd738f4b30605ddccf6,"{""path"":""a.stim""}",
         9,         5,         4,    6.00,pymatching,5fe5a6cd4226b1a910d57e5479d1ba6572e0b3115983c9516360916d1670000f,"{""path"":""b.stim""}",
"""

        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            main(command_line_args=[
                "combine",
                str(d / "input.csv"),
                str(d / "input.csv"),
            ])
        assert out.getvalue() == """     shots,    errors,  discards, seconds,decoder,strong_id,json_metadata,custom_counts
      1200,       202,       440,    6.00,pymatching,f256bab362f516ebe4d59a08ae67330ff7771ff738757cd738f4b30605ddccf6,"{""path"":""a.stim""}",
        18,        10,         8,    12.0,pymatching,5fe5a6cd4226b1a910d57e5479d1ba6572e0b3115983c9516360916d1670000f,"{""path"":""b.stim""}",
"""


def test_main_combine_legacy_custom_counts():
    with tempfile.TemporaryDirectory() as d:
        d = pathlib.Path(d)
        with open(d / f'old.csv', 'w') as f:
            print("""
shots,errors,discards,seconds,decoder,strong_id,json_metadata
100,1,20,1.0,pymatching,abc123,"{""path"":""a.stim""}"
            """.strip(), file=f)
        with open(d / f'new.csv', 'w') as f:
            print("""
shots,errors,discards,seconds,decoder,strong_id,json_metadata,custom_counts
300,1,20,1.0,pymatching,abc123,"{""path"":""a.stim""}","{""x"":2}"
300,1,20,1.0,pymatching,abc123,"{""path"":""a.stim""}","{""y"":3}"
            """.strip(), file=f)

        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            main(command_line_args=[
                "combine",
                str(d / "old.csv"),
                str(d / "new.csv"),
            ])
        assert out.getvalue() == """     shots,    errors,  discards, seconds,decoder,strong_id,json_metadata,custom_counts
       700,         3,        60,    3.00,pymatching,abc123,"{""path"":""a.stim""}","{""x"":2,""y"":3}"
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
        assert out.getvalue() == """     shots,    errors,  discards, seconds,decoder,strong_id,json_metadata,custom_counts
      2000,       200,         8,    4.00,pymatching,deadbeef0,"{""d"":19}",
      4000,       600,         6,    6.00,pymatching,deadbeef1,"{""d"":9}",
      6000,       400,      4000,    10.0,pymatching,deadbeef2,"{""d"":200}",
      8000,       200,         2,    14.0,pymatching,deadbeef3,"{""d"":3}",
     10000,       200,         0,    22.0,pymatching,deadbeef4,"{""d"":5}",
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
        assert out.getvalue() == """     shots,    errors,  discards, seconds,decoder,strong_id,json_metadata,custom_counts
      8000,       200,         2,    14.0,pymatching,deadbeef3,"{""d"":3}",
     10000,       200,         0,    22.0,pymatching,deadbeef4,"{""d"":5}",
      4000,       600,         6,    6.00,pymatching,deadbeef1,"{""d"":9}",
      2000,       200,         8,    4.00,pymatching,deadbeef0,"{""d"":19}",
      6000,       400,      4000,    10.0,pymatching,deadbeef2,"{""d"":200}",
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
        assert out.getvalue() == """     shots,    errors,  discards, seconds,decoder,strong_id,json_metadata,custom_counts
     10000,       200,         0,    22.0,pymatching,deadbeef4,"{""d"":5}",
      8000,       200,         2,    14.0,pymatching,deadbeef3,"{""d"":3}",
      2000,       200,         8,    4.00,pymatching,deadbeef0,"{""d"":19}",
      4000,       600,         6,    6.00,pymatching,deadbeef1,"{""d"":9}",
      6000,       400,      4000,    10.0,pymatching,deadbeef2,"{""d"":200}",
"""


def test_order_custom_counts():
    with tempfile.TemporaryDirectory() as d:
        d = pathlib.Path(d)
        with open(d / f'input.csv', 'w') as f:
            print("""
shots,errors,discards,seconds,decoder,   strong_id,json_metadata,custom_counts
1000, 100,   4,       2.0,    pymatching,deadbeef0,[],"{""d4"":3,""d2"":30}"
            """.strip(), file=f)

        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            main(command_line_args=[
                "combine",
                str(d / "input.csv"),
            ])
        assert out.getvalue() == """     shots,    errors,  discards, seconds,decoder,strong_id,json_metadata,custom_counts
      1000,       100,         4,    2.00,pymatching,deadbeef0,[],"{""d2"":30,""d4"":3}"
"""
