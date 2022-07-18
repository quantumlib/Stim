import pathlib

import numpy as np
import stim
import tempfile

import sinter


def test_predict_on_disk_no_postselect():
    with tempfile.TemporaryDirectory() as tmp_dir:
        tmp_dir = pathlib.Path(tmp_dir)
        dem_path = tmp_dir / 'dem'
        dets_path = tmp_dir / 'dets'
        obs_out_path = tmp_dir / 'obs'

        with open(dem_path, 'w') as f:
            print("""
                error(0.1) D0 L0
                detector(0, 0, 0, 1) D0
            """, file=f)
        with open(dets_path, 'w') as f:
            print("0", file=f)
            print("1", file=f)

        sinter.predict_on_disk(
            decoder='pymatching',
            dem_path=dem_path,
            dets_path=dets_path,
            dets_format='01',
            obs_out_path=obs_out_path,
            obs_out_format='01',
            postselect_detectors_with_non_zero_4th_coord=False,
            discards_out_path=None,
            discards_out_format=None,
        )

        with open(obs_out_path) as f:
            assert f.read() == '0\n1\n'


def test_predict_on_disk_yes_postselect():
    with tempfile.TemporaryDirectory() as tmp_dir:
        tmp_dir = pathlib.Path(tmp_dir)
        dem_path = tmp_dir / 'dem'
        dets_path = tmp_dir / 'dets'
        obs_out_path = tmp_dir / 'obs'
        discards_out_path = tmp_dir / 'discards'

        with open(dem_path, 'w') as f:
            print("""
                error(0.1) D0 L0
                detector(0, 0, 0, 1) D0
            """, file=f)
        with open(dets_path, 'w') as f:
            print("0", file=f)
            print("1", file=f)

        sinter.predict_on_disk(
            decoder='pymatching',
            dem_path=dem_path,
            dets_path=dets_path,
            dets_format='01',
            obs_out_path=obs_out_path,
            obs_out_format='01',
            postselect_detectors_with_non_zero_4th_coord=True,
            discards_out_path=discards_out_path,
            discards_out_format='01',
        )

        with open(obs_out_path) as f:
            assert f.read() == '0\n'
        with open(discards_out_path) as f:
            assert f.read() == '0\n1\n'


def test_predict_discards_bit_packed_none_postselected():
    dem = stim.DetectorErrorModel("""
        error(0.1) D0 L0
    """)
    actual = sinter.predict_discards_bit_packed(
        dem=dem,
        dets_bit_packed=np.packbits(np.array([
            [False],
            [True],
        ], dtype=np.bool8), bitorder='little', axis=1),
        postselect_detectors_with_non_zero_4th_coord=True,
    )
    np.testing.assert_array_equal(
        actual,
        [False, False],
    )


def test_predict_discards_bit_packed_some_postselected():
    dem = stim.DetectorErrorModel("""
        error(0.1) D0 L0
        detector(0, 0, 0, 1) D0
    """)
    actual = sinter.predict_discards_bit_packed(
        dem=dem,
        dets_bit_packed=np.packbits(np.array([
            [False],
            [True],
        ], dtype=np.bool8), bitorder='little', axis=1),
        postselect_detectors_with_non_zero_4th_coord=True,
    )
    np.testing.assert_array_equal(
        actual,
        [False, True],
    )


def test_predict_observables_bit_packed():
    dem = stim.DetectorErrorModel("""
        error(0.1) D0 L0
    """)

    actual = sinter.predict_observables_bit_packed(
        dem=dem,
        dets_bit_packed=np.packbits(np.array([
            [False],
            [True],
        ], dtype=np.bool8), bitorder='little', axis=1),
        decoder='pymatching',
    )
    np.testing.assert_array_equal(
        np.unpackbits(actual, bitorder='little', count=1, axis=1),
        [
            [False],
            [True],
        ],
    )


def test_predict_observables():
    dem = stim.DetectorErrorModel("""
        error(0.1) D0 L0
    """)

    actual = sinter.predict_observables(
        dem=dem,
        dets=np.packbits(np.array([
            [False],
            [True],
        ], dtype=np.bool8), bitorder='little', axis=1),
        decoder='pymatching',
        bit_pack_result=True,
    )
    np.testing.assert_array_equal(
        np.unpackbits(actual, bitorder='little', count=1, axis=1),
        [
            [False],
            [True],
        ],
    )

    actual = sinter.predict_observables(
        dem=dem,
        dets=np.array([
            [False],
            [True],
        ], dtype=np.bool8),
        decoder='pymatching',
        bit_pack_result=True,
    )
    np.testing.assert_array_equal(
        np.unpackbits(actual, bitorder='little', count=1, axis=1),
        [
            [False],
            [True],
        ],
    )

    actual = sinter.predict_observables(
        dem=dem,
        dets=np.packbits(np.array([
            [False],
            [True],
        ], dtype=np.bool8), bitorder='little', axis=1),
        decoder='pymatching',
        bit_pack_result=False,
    )
    np.testing.assert_array_equal(
        actual,
        [[False], [True]],
    )

    actual = sinter.predict_observables(
        dem=dem,
        dets=np.array([
            [False],
            [True],
        ], dtype=np.bool8),
        decoder='pymatching',
        bit_pack_result=False,
    )
    np.testing.assert_array_equal(
        actual,
        [[False], [True]],
    )

    actual = sinter.predict_observables(
        dem=dem,
        dets=np.packbits(np.array([
            [False],
            [True],
        ], dtype=np.bool8), bitorder='little', axis=1),
        decoder='pymatching',
    )
    np.testing.assert_array_equal(
        actual,
        [[False], [True]],
    )

    actual = sinter.predict_observables(
        dem=dem,
        dets=np.array([
            [False],
            [True],
        ], dtype=np.bool8),
        decoder='pymatching',
    )
    np.testing.assert_array_equal(
        actual,
        [[False], [True]],
    )
