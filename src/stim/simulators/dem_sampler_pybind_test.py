import numpy as np
import pathlib
import pytest
import stim
import tempfile


@pytest.mark.parametrize("bit_packed", [False, True])
def test_dem_sampler_sample(bit_packed: bool):
    noisy_dem = stim.DetectorErrorModel("""
        error(0.125) D0
        error(0.25) D1
    """)
    noisy_sampler = noisy_dem.compile_sampler()
    det_data, obs_data, err_data = noisy_sampler.sample(shots=100, return_errors=True, bit_packed=bit_packed)
    replay_det_data, replay_obs_data, _ = noisy_sampler.sample(shots=100, recorded_errors_to_replay=err_data, bit_packed=bit_packed)
    np.testing.assert_array_equal(det_data, replay_det_data)
    np.testing.assert_array_equal(obs_data, replay_obs_data)


def test_dem_sampler_sampler_write():
    dem = stim.DetectorErrorModel('''
       error(0) D0
       error(0) D1
       error(0) D0
       error(1) D1 D2 L0
       error(0) D0
    ''')
    sampler = dem.compile_sampler()
    with tempfile.TemporaryDirectory() as d:
        d = pathlib.Path(d)
        sampler.sample_write(
            shots=1,
            det_out_file=d / 'dets.01',
            det_out_format='01',
            obs_out_file=d / 'obs.01',
            obs_out_format='01',
            err_out_file=d / 'err.hits',
            err_out_format='hits',
        )
        with open(d / 'dets.01') as f:
            assert f.read() == "011\n"
        with open(d / 'obs.01') as f:
            assert f.read() == "1\n"
        with open(d / 'err.hits') as f:
            assert f.read() == "3\n"

        sampler = stim.DetectorErrorModel('''
           error(1) D0  # this should be overridden by the replay.
           error(1) D1
           error(1) D0
           error(1) D1 D2 L0
           error(1) D0
        ''').compile_sampler()
        sampler.sample_write(
            shots=1,
            det_out_file=d / 'dets.01',
            det_out_format='01',
            obs_out_file=d / 'obs.01',
            obs_out_format='01',
            err_out_file=d / 'err2.01',
            err_out_format='01',
            replay_err_in_file=d / 'err.hits',
            replay_err_in_format='hits',
        )
        with open(d / 'dets.01') as f:
            assert f.read() == "011\n"
        with open(d / 'obs.01') as f:
            assert f.read() == "1\n"
        with open(d / 'err.hits') as f:
            assert f.read() == "3\n"
        with open(d / 'err2.01') as f:
            assert f.read() == "00010\n"
