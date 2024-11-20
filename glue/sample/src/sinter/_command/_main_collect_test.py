import collections
import pathlib
import tempfile

import stim

import pytest
import sinter
from sinter._command._main import main
from sinter._command._main_combine import ExistingData
from sinter._plotting import split_by


def test_split_by():
    assert split_by('abcdefcccghi', lambda e: e == 'c') == [list('ab'), list('c'), list('def'), list('ccc'), list('ghi')]


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
            "--circuits",
            str(d / "3.stim"),
            str(d / "5.stim"),
            str(d / "7.stim"),
            "--max_shots",
            "1000",
            "--max_errors",
            "10",
            "--decoders",
            "pymatching",
            "--processes",
            "4",
            "--quiet",
            "--save_resume_filepath",
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
            "--circuits",
            str(d / "3.stim"),
            str(d / "5.stim"),
            str(d / "7.stim"),
            "--max_shots",
            "1000",
            "--max_errors",
            "10",
            "--decoders",
            "pymatching",
            "--processes",
            "4",
            "--quiet",
            "--save_resume_filepath",
            str(d / "out.csv"),
        ])
        with open(d / "out.csv") as f:
            contents2 = f.read()
        assert contents1 == contents2

        # No more work when existing work is sufficient.
        main(command_line_args=[
            "collect",
            "--circuits",
            str(d / "3.stim"),
            str(d / "5.stim"),
            str(d / "7.stim"),
            "--max_shots",
            "1000",
            "--max_errors",
            "10",
            "--decoders",
            "pymatching",
            "--processes",
            "4",
            "--quiet",
            "--existing_data_filepaths",
            str(d / "out.csv"),
            "--save_resume_filepath",
            str(d / "out2.csv"),
        ])
        data2 = ExistingData.from_file(d / "out2.csv").data
        assert len(data2) == 0


class AlternatingPredictionsDecoder(sinter.Decoder):
    def decode_via_files(self,
                         *,
                         num_shots: int,
                         num_dets: int,
                         num_obs: int,
                         dem_path: pathlib.Path,
                         dets_b8_in_path: pathlib.Path,
                         obs_predictions_b8_out_path: pathlib.Path,
                         tmp_dir: pathlib.Path,
                       ) -> None:
        bytes_per_shot = (num_obs + 7) // 8
        with open(obs_predictions_b8_out_path, 'wb') as f:
            for k in range(num_shots):
                f.write((k % 3 == 0).to_bytes(length=bytes_per_shot, byteorder='little'))


def _make_custom_decoders():
    return {'alternate': AlternatingPredictionsDecoder()}


def test_main_collect_with_custom_decoder():
    with tempfile.TemporaryDirectory() as d:
        d = pathlib.Path(d)
        with open(d / f'tmp.stim', 'w') as f:
            print("""
                M(0.1) 0
                DETECTOR rec[-1]
                OBSERVABLE_INCLUDE(0) rec[-1]
            """, file=f)

        with pytest.raises(ValueError, match="Not a recognized decoder"):
            main(command_line_args=[
                "collect",
                "--circuits",
                str(d / "tmp.stim"),
                "--max_shots",
                "1000",
                "--decoders",
                "NOTEXIST",
                "--custom_decoders_module_function",
                "sinter._command._main_collect_test:_make_custom_decoders",
                "--processes",
                "2",
                "--quiet",
                "--save_resume_filepath",
                str(d / "out.csv"),
            ])

        # Collects requested stats.
        main(command_line_args=[
            "collect",
            "--circuits",
            str(d / "tmp.stim"),
            "--max_shots",
            "1000",
            "--decoders",
            "alternate",
            "--custom_decoders_module_function",
            "sinter._command._main_collect_test:_make_custom_decoders",
            "--processes",
            "2",
            "--quiet",
            "--save_resume_filepath",
            str(d / "out.csv"),
        ])
        data = ExistingData.from_file(d / "out.csv").data
        assert len(data) == 1
        v, = data.values()
        assert v.shots == 1000
        assert 50 < v.errors < 500
        assert v.discards == 0


def test_main_collect_post_select_observables():
    with tempfile.TemporaryDirectory() as d:
        d = pathlib.Path(d)
        with open(d / f'circuit.stim', 'w') as f:
            print("""
                M(0.125) 0 1
                OBSERVABLE_INCLUDE(0) rec[-1]
                OBSERVABLE_INCLUDE(11) rec[-1] rec[-2]
            """, file=f)

        # Collects requested stats.
        main(command_line_args=[
            "collect",
            "--postselected_observables_predicate",
            "index == 11",
            "--circuits",
            str(d / "circuit.stim"),
            "--max_shots",
            "10000",
            "--max_errors",
            "10000",
            "--decoders",
            "pymatching",
            "--processes",
            "4",
            "--quiet",
            "--save_resume_filepath",
            str(d / "out.csv"),
        ])
        data = sinter.stats_from_csv_files(d / "out.csv")
        assert len(data) == 1
        stats, = data
        assert stats.shots == 10000
        assert 0.21875 - 0.1 < stats.discards / stats.shots < 0.21875 + 0.1
        assert 0.015625 - 0.01 <= stats.errors / (stats.shots - stats.discards) <= 0.015625 + 0.02


def test_main_collect_comma_separated_key_values():
    with tempfile.TemporaryDirectory() as d:
        d = pathlib.Path(d)
        paths = []
        for distance in [3, 5, 7]:
            c = stim.Circuit.generated(
                'repetition_code:memory',
                rounds=3,
                distance=distance,
                after_clifford_depolarization=0.02)
            path = d / f'd={distance},p=0.02,r=3.0,c=rep_code.stim'
            paths.append(str(path))
            with open(path, 'w') as f:
                print(c, file=f)

        # Collects requested stats.
        main(command_line_args=[
            "collect",
            "--circuits",
            *paths,
            "--max_shots",
            "1000",
            "--metadata_func",
            "sinter.comma_separated_key_values(path)",
            "--max_errors",
            "10",
            "--decoders",
            "pymatching",
            "--processes",
            "4",
            "--quiet",
            "--save_resume_filepath",
            str(d / "out.csv"),
        ])
        data = sinter.stats_from_csv_files(d / "out.csv")
        seen_metadata = frozenset(repr(e.json_metadata) for e in data)
        assert seen_metadata == frozenset([
            "{'c': 'rep_code', 'd': 3, 'p': 0.02, 'r': 3.0}",
            "{'c': 'rep_code', 'd': 5, 'p': 0.02, 'r': 3.0}",
            "{'c': 'rep_code', 'd': 7, 'p': 0.02, 'r': 3.0}",
        ])


def test_main_collect_count_observable_error_combos():
    with tempfile.TemporaryDirectory() as d:
        d = pathlib.Path(d)
        with open(d / 'a=3.stim', 'w') as f:
            print("""
                X_ERROR(0.1) 0
                X_ERROR(0.2) 1
                M 0 1
                OBSERVABLE_INCLUDE(0) rec[-1]
                OBSERVABLE_INCLUDE(1) rec[-2]
            """, file=f)

        # Collects requested stats.
        main(command_line_args=[
            "collect",
            "--circuits",
            str(d / 'a=3.stim'),
            "--max_shots",
            "100000",
            "--metadata_func",
            "sinter.comma_separated_key_values(path)",
            "--max_errors",
            "10000",
            "--decoders",
            "pymatching",
            "--count_observable_error_combos",
            "--processes",
            "4",
            "--quiet",
            "--save_resume_filepath",
            str(d / "out.csv"),
        ])
        data = sinter.stats_from_csv_files(d / "out.csv")
        assert len(data) == 1
        item, = data
        assert set(item.custom_counts.keys()) == {"obs_mistake_mask=E_", "obs_mistake_mask=_E", "obs_mistake_mask=EE"}
        assert 0.1*0.8 - 0.01 < item.custom_counts['obs_mistake_mask=_E'] / item.shots < 0.1*0.8 + 0.01
        assert 0.9*0.2 - 0.01 < item.custom_counts['obs_mistake_mask=E_'] / item.shots < 0.9*0.2 + 0.01
        assert 0.1*0.2 - 0.01 < item.custom_counts['obs_mistake_mask=EE'] / item.shots < 0.1*0.2 + 0.01


def test_main_collect_count_detection_events():
    with tempfile.TemporaryDirectory() as d:
        d = pathlib.Path(d)
        with open(d / 'a=3.stim', 'w') as f:
            print("""
                X_ERROR(0.1) 0
                X_ERROR(0.2) 1
                M 0 1
                OBSERVABLE_INCLUDE(0) rec[-1]
                OBSERVABLE_INCLUDE(1) rec[-2]
                DETECTOR rec[-2]
            """, file=f)

        # Collects requested stats.
        main(command_line_args=[
            "collect",
            "--circuits",
            str(d / 'a=3.stim'),
            "--max_shots",
            "100000",
            "--metadata_func",
            "sinter.comma_separated_key_values(path)",
            "--decoders",
            "pymatching",
            "--count_detection_events",
            "--processes",
            "4",
            "--quiet",
            "--save_resume_filepath",
            str(d / "out.csv"),
        ])
        data = sinter.stats_from_csv_files(d / "out.csv")
        assert len(data) == 1
        item, = data
        assert set(item.custom_counts.keys()) == {"detection_events", "detectors_checked"}
        assert item.custom_counts['detectors_checked'] == 100000
        assert 100000 * 0.1 * 0.5 < item.custom_counts['detection_events'] < 100000 * 0.1 * 1.5


def test_cpu_pin():
    with tempfile.TemporaryDirectory() as d:
        d = pathlib.Path(d)
        with open(d / 'a=3.stim', 'w') as f:
            print("""
                X_ERROR(0.1) 0
                X_ERROR(0.2) 1
                M 0 1
                OBSERVABLE_INCLUDE(0) rec[-1]
                OBSERVABLE_INCLUDE(1) rec[-2]
                DETECTOR rec[-2]
            """, file=f)

        # Collects requested stats.
        main(command_line_args=[
            "collect",
            "--circuits",
            str(d / 'a=3.stim'),
            "--max_shots",
            "100000",
            "--metadata_func",
            "auto",
            "--decoders",
            "pymatching",
            "--count_detection_events",
            "--processes",
            "4",
            "--quiet",
            "--save_resume_filepath",
            str(d / "out.csv"),
            "--allowed_cpu_affinity_ids",
            "0",
            "range(1, 9, 2)",
            "[4, 20]"
        ])
        data = sinter.stats_from_csv_files(d / "out.csv")
        assert len(data) == 1
        item, = data
        assert set(item.custom_counts.keys()) == {"detection_events", "detectors_checked"}
        assert item.custom_counts['detectors_checked'] == 100000
        assert 100000 * 0.1 * 0.5 < item.custom_counts['detection_events'] < 100000 * 0.1 * 1.5


def test_custom_error_stopping_count():
    with tempfile.TemporaryDirectory() as d:
        d = pathlib.Path(d)
        stim.Circuit.generated(
            'repetition_code:memory',
            rounds=25,
            distance=5,
            after_clifford_depolarization=0.1,
        ).to_file(d / 'a=3.stim')

        # Collects requested stats.
        main(command_line_args=[
            "collect",
            "--circuits",
            str(d / 'a=3.stim'),
            "--max_shots",
            "100_000_000_000_000",
            "--quiet",
            "--max_errors",
            "1_000_000",
            "--metadata_func",
            "auto",
            "--decoders",
            "vacuous",
            "--count_detection_events",
            "--processes",
            "4",
            "--save_resume_filepath",
            str(d / "out.csv"),
            "--custom_error_count_key",
            "detection_events",
        ])
        data = sinter.stats_from_csv_files(d / "out.csv")
        assert len(data) == 1
        item, = data
        # Would normally need >1_000_000 shots to see 1_000_000 errors.
        assert item.shots < 100_000
        assert item.errors < 90_000
        assert item.custom_counts['detection_events'] > 1_000_000


def test_auto_processes():
    with tempfile.TemporaryDirectory() as d:
        d = pathlib.Path(d)
        stim.Circuit.generated(
            'repetition_code:memory',
            rounds=5,
            distance=3,
            after_clifford_depolarization=0.1,
        ).to_file(d / 'a=3.stim')

        # Collects requested stats.
        main(command_line_args=[
            "collect",
            "--circuits",
            str(d / 'a=3.stim'),
            "--max_shots",
            "200",
            "--quiet",
            "--metadata_func",
            "auto",
            "--decoders",
            "vacuous",
            "--processes",
            "auto",
            "--save_resume_filepath",
            str(d / "out.csv"),
        ])
        data = sinter.stats_from_csv_files(d / "out.csv")
        assert len(data) == 1


def test_implicit_auto_processes():
    with tempfile.TemporaryDirectory() as d:
        d = pathlib.Path(d)
        stim.Circuit.generated(
            'repetition_code:memory',
            rounds=5,
            distance=3,
            after_clifford_depolarization=0.1,
        ).to_file(d / 'a=3.stim')

        # Collects requested stats.
        main(command_line_args=[
            "collect",
            "--circuits",
            str(d / 'a=3.stim'),
            "--max_shots",
            "200",
            "--quiet",
            "--metadata_func",
            "auto",
            "--decoders",
            "perfectionist",
            "--save_resume_filepath",
            str(d / "out.csv"),
        ])
        data = sinter.stats_from_csv_files(d / "out.csv")
        assert len(data) == 1
        assert data[0].discards > 0
