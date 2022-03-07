import pathlib
import tempfile

import stim

from simmer.main import main
from simmer.main_combine import ExistingData


def test_main():
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
            "-merge_data_location",
            str(d / "out.csv"),
        ])
        data = ExistingData.from_file(d / "out.csv").data
        assert len(data) == 3
        for k, v in data.items():
            assert v.num_discards == 0
            assert v.num_errors <= 50
            assert v.num_shots >= 1000

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
            "-merge_data_location",
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
            "-existing_data_location",
            str(d / "out.csv"),
            "-merge_data_location",
            str(d / "out2.csv"),
        ])
        data2 = ExistingData.from_file(d / "out2.csv").data
        assert len(data2) == 0
