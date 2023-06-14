import pathlib
import tempfile
from typing import Optional

import numpy as np
import pytest
import stim

from sinter._collection import post_selection_mask_from_4th_coord
from sinter._decoding import sample_decode
from sinter._decoding_all_built_in_decoders import BUILT_IN_DECODERS

_DECODER_KEY_AND_PACKAGE = [
    ('vacuous', 'sinter'),
    ('pymatching', 'pymatching'),
    ('fusion_blossom', 'fusion_blossom'),
    ('internal', 'gqec'),
    ('internal_correlated', 'gqec'),
    ('internal_2', 'gqec'),
    ('internal_correlated_2', 'gqec'),
]

DECODER_CASES = [
    (a, b, c)
    for a, b in _DECODER_KEY_AND_PACKAGE
    for c in [None, True]
]


@pytest.mark.parametrize('decoder,required_import,force_streaming', DECODER_CASES)
def test_decode_repetition_code(decoder: str, required_import: str, force_streaming: Optional[bool]):
    pytest.importorskip(required_import)

    circuit = stim.Circuit.generated('repetition_code:memory',
                                     rounds=3,
                                     distance=3,
                                     after_clifford_depolarization=0.05)
    result = sample_decode(
        circuit_obj=circuit,
        circuit_path=None,
        dem_obj=circuit.detector_error_model(decompose_errors=True),
        dem_path=None,
        num_shots=1000,
        decoder=decoder,
    )
    assert result.discards == 0
    if decoder != 'vacuous':
        assert 1 <= result.errors <= 100
    assert result.shots == 1000


@pytest.mark.parametrize('decoder,required_import,force_streaming', DECODER_CASES)
def test_decode_surface_code(decoder: str, required_import: str, force_streaming: Optional[bool]):
    pytest.importorskip(required_import)

    circuit = stim.Circuit.generated(
        "surface_code:rotated_memory_x",
        distance=3,
        rounds=15,
        after_clifford_depolarization=0.001,
    )
    stats = sample_decode(
        num_shots=1000,
        circuit_obj=circuit,
        circuit_path=None,
        dem_obj=circuit.detector_error_model(decompose_errors=True),
        dem_path=None,
        decoder=decoder,
        __private__unstable__force_decode_on_disk=force_streaming,
    )
    if decoder != 'vacuous':
        assert 0 <= stats.errors <= 50


@pytest.mark.parametrize('decoder,required_import,force_streaming', DECODER_CASES)
def test_empty(decoder: str, required_import: str, force_streaming: Optional[bool]):
    pytest.importorskip(required_import)
    circuit = stim.Circuit()
    result = sample_decode(
        circuit_obj=circuit,
        circuit_path=None,
        dem_obj=circuit.detector_error_model(decompose_errors=True),
        dem_path=None,
        num_shots=1000,
        decoder=decoder,
        __private__unstable__force_decode_on_disk=force_streaming,
    )
    assert result.discards == 0
    assert result.shots == 1000
    assert result.errors == 0


@pytest.mark.parametrize('decoder,required_import,force_streaming', DECODER_CASES)
def test_no_observables(decoder: str, required_import: str, force_streaming: Optional[bool]):
    pytest.importorskip(required_import)
    circuit = stim.Circuit("""
        X_ERROR(0.1) 0
        M 0
        DETECTOR rec[-1]
    """)
    result = sample_decode(
        circuit_obj=circuit,
        circuit_path=None,
        dem_obj=circuit.detector_error_model(decompose_errors=True),
        dem_path=None,
        num_shots=1000,
        decoder=decoder,
        __private__unstable__force_decode_on_disk=force_streaming,
    )
    assert result.discards == 0
    assert result.shots == 1000
    assert result.errors == 0


@pytest.mark.parametrize('decoder,required_import,force_streaming', DECODER_CASES)
def test_invincible_observables(decoder: str, required_import: str, force_streaming: Optional[bool]):
    pytest.importorskip(required_import)
    circuit = stim.Circuit("""
        X_ERROR(0.1) 0
        M 0 1
        DETECTOR rec[-2]
        OBSERVABLE_INCLUDE(1) rec[-1]
    """)
    result = sample_decode(
        circuit_obj=circuit,
        circuit_path=None,
        dem_obj=circuit.detector_error_model(decompose_errors=True),
        dem_path=None,
        num_shots=1000,
        decoder=decoder,
        __private__unstable__force_decode_on_disk=force_streaming,
    )
    assert result.discards == 0
    assert result.shots == 1000
    assert result.errors == 0


@pytest.mark.parametrize('decoder,required_import,force_streaming,offset', [(a, b, c, d) for a, b, c in DECODER_CASES for d in range(8)])
def test_observable_offsets_mod8(decoder: str, required_import: str, force_streaming: bool, offset: int):
    pytest.importorskip(required_import)
    circuit = stim.Circuit("""
        X_ERROR(0.1) 0
        MR 0
        DETECTOR rec[-1]
    """) * (8 + offset) + stim.Circuit("""
        X_ERROR(0.1) 0
        MR 0
        OBSERVABLE_INCLUDE(0) rec[-1]
    """)
    result = sample_decode(
        circuit_obj=circuit,
        circuit_path=None,
        dem_obj=circuit.detector_error_model(decompose_errors=True),
        dem_path=None,
        num_shots=1000,
        decoder=decoder,
        __private__unstable__force_decode_on_disk=force_streaming,
    )
    assert result.discards == 0
    assert result.shots == 1000
    assert 50 <= result.errors <= 150


@pytest.mark.parametrize('decoder,required_import,force_streaming', DECODER_CASES)
def test_no_detectors(decoder: str, required_import: str, force_streaming: Optional[bool]):
    pytest.importorskip(required_import)
    circuit = stim.Circuit("""
        X_ERROR(0.1) 0
        M 0
        OBSERVABLE_INCLUDE(0) rec[-1]
    """)
    result = sample_decode(
        circuit_obj=circuit,
        circuit_path=None,
        dem_obj=circuit.detector_error_model(decompose_errors=True),
        dem_path=None,
        num_shots=1000,
        decoder=decoder,
        __private__unstable__force_decode_on_disk=force_streaming,
    )
    assert result.discards == 0
    assert 50 <= result.errors <= 150


@pytest.mark.parametrize('decoder,required_import,force_streaming', DECODER_CASES)
def test_no_detectors_with_post_mask(decoder: str, required_import: str, force_streaming: Optional[bool]):
    pytest.importorskip(required_import)
    circuit = stim.Circuit("""
        X_ERROR(0.1) 0
        M 0
        OBSERVABLE_INCLUDE(0) rec[-1]
    """)
    result = sample_decode(
        circuit_obj=circuit,
        circuit_path=None,
        dem_obj=circuit.detector_error_model(decompose_errors=True),
        dem_path=None,
        post_mask=np.array([], dtype=np.uint8),
        num_shots=1000,
        decoder=decoder,
        __private__unstable__force_decode_on_disk=force_streaming,
    )
    assert result.discards == 0
    assert 50 <= result.errors <= 150


@pytest.mark.parametrize('decoder,required_import,force_streaming', DECODER_CASES)
def test_post_selection(decoder: str, required_import: str, force_streaming: Optional[bool]):
    pytest.importorskip(required_import)
    circuit = stim.Circuit("""
        X_ERROR(0.6) 0
        M 0
        DETECTOR(2, 0, 0, 1) rec[-1]
        OBSERVABLE_INCLUDE(0) rec[-1]

        X_ERROR(0.5) 1
        M 1
        DETECTOR(1, 0, 0) rec[-1]
        OBSERVABLE_INCLUDE(0) rec[-1]
        
        X_ERROR(0.1) 2
        M 2
        OBSERVABLE_INCLUDE(0) rec[-1]
    """)
    result = sample_decode(
        circuit_obj=circuit,
        circuit_path=None,
        dem_obj=circuit.detector_error_model(decompose_errors=True),
        dem_path=None,
        post_mask=post_selection_mask_from_4th_coord(circuit),
        num_shots=2000,
        decoder=decoder,
        __private__unstable__force_decode_on_disk=force_streaming,
    )
    assert 1050 <= result.discards <= 1350
    if decoder != 'vacuous':
        assert 40 <= result.errors <= 160


@pytest.mark.parametrize('decoder,required_import,force_streaming', DECODER_CASES)
def test_observable_post_selection(decoder: str, required_import: str, force_streaming: Optional[bool]):
    pytest.importorskip(required_import)
    circuit = stim.Circuit("""
        X_ERROR(0.1) 0
        X_ERROR(0.2) 1
        M 0 1
        OBSERVABLE_INCLUDE(0) rec[-1]
        OBSERVABLE_INCLUDE(1) rec[-1] rec[-2]
    """)
    result = sample_decode(
        circuit_obj=circuit,
        circuit_path=None,
        dem_obj=circuit.detector_error_model(decompose_errors=True),
        dem_path=None,
        post_mask=None,
        postselected_observable_mask=np.array([1], dtype=np.uint8),
        num_shots=10000,
        decoder=decoder,
        __private__unstable__force_decode_on_disk=force_streaming,
    )
    np.testing.assert_allclose(result.discards / result.shots, 0.2, atol=0.1)
    if decoder != 'vacuous':
        np.testing.assert_allclose(result.errors / (result.shots - result.discards), 0.1, atol=0.05)


@pytest.mark.parametrize('decoder,required_import,force_streaming', DECODER_CASES)
def test_error_splitting(decoder: str, required_import: str, force_streaming: Optional[bool]):
    pytest.importorskip(required_import)
    circuit = stim.Circuit("""
        X_ERROR(0.1) 0
        X_ERROR(0.2) 1
        M 0 1
        OBSERVABLE_INCLUDE(0) rec[-1]
        OBSERVABLE_INCLUDE(1) rec[-1] rec[-2]
    """)
    result = sample_decode(
        circuit_obj=circuit,
        circuit_path=None,
        dem_obj=circuit.detector_error_model(decompose_errors=True),
        dem_path=None,
        post_mask=None,
        num_shots=10000,
        decoder=decoder,
        count_observable_error_combos=True,
        __private__unstable__force_decode_on_disk=force_streaming,
    )
    assert result.discards == 0
    assert set(result.custom_counts.keys()) == {'obs_mistake_mask=E_', 'obs_mistake_mask=_E', 'obs_mistake_mask=EE'}
    if decoder != 'vacuous':
        np.testing.assert_allclose(result.errors / result.shots, 1 - 0.8 * 0.9, atol=0.05)
        np.testing.assert_allclose(result.custom_counts['obs_mistake_mask=E_'] / result.shots, 0.1 * 0.2, atol=0.05)
        np.testing.assert_allclose(result.custom_counts['obs_mistake_mask=_E'] / result.shots, 0.1 * 0.8, atol=0.05)
        np.testing.assert_allclose(result.custom_counts['obs_mistake_mask=EE'] / result.shots, 0.9 * 0.2, atol=0.05)


@pytest.mark.parametrize('decoder,required_import,force_streaming', DECODER_CASES)
def test_detector_counting(decoder: str, required_import: str, force_streaming: Optional[bool]):
    pytest.importorskip(required_import)
    circuit = stim.Circuit("""
        X_ERROR(0.1) 0
        X_ERROR(0.2) 1
        M 0 1
        DETECTOR rec[-1]
        DETECTOR rec[-2]
        OBSERVABLE_INCLUDE(0) rec[-1]
        OBSERVABLE_INCLUDE(1) rec[-1] rec[-2]
    """)
    result = sample_decode(
        circuit_obj=circuit,
        circuit_path=None,
        dem_obj=circuit.detector_error_model(decompose_errors=True),
        dem_path=None,
        post_mask=None,
        num_shots=10000,
        decoder=decoder,
        count_detection_events=True,
        __private__unstable__force_decode_on_disk=force_streaming,
    )
    assert result.discards == 0
    assert result.custom_counts['detectors_checked'] == 20000
    assert 0.3 * 10000 * 0.5 <= result.custom_counts['detection_events'] <= 0.3 * 10000 * 2.0
    assert set(result.custom_counts.keys()) == {'detectors_checked', 'detection_events'}


@pytest.mark.parametrize('decoder,required_import,force_streaming', DECODER_CASES)
def test_decode_fails_correctly(decoder: str, required_import: str, force_streaming: Optional[bool]):
    pytest.importorskip(required_import)

    decoder_obj = BUILT_IN_DECODERS.get(decoder)
    with tempfile.TemporaryDirectory() as d:
        d = pathlib.Path(d)
        circuit = stim.Circuit("""
            REPEAT 9 {
                MR(0.001) 0
                DETECTOR rec[-1]
                OBSERVABLE_INCLUDE(0) rec[-1]
            }
        """)
        dem = circuit.detector_error_model()
        circuit.to_file(d / 'circuit.stim')
        dem.to_file(d / 'dem.dem')
        with open(d / 'bad_dets.b8', 'wb') as f:
            f.write(b'!')

        if decoder != 'vacuous':
            with pytest.raises(Exception):
                decoder_obj.decode_via_files(
                    num_shots=1,
                    num_dets=dem.num_detectors,
                    num_obs=dem.num_observables,
                    dem_path=d / 'dem.dem',
                    dets_b8_in_path=d / 'bad_dets.b8',
                    obs_predictions_b8_out_path=d / 'predict.b8',
                    tmp_dir=d,
                )
