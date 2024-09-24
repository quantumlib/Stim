import os

import pathlib
import tempfile
from typing import Dict, List, Optional, Tuple

import numpy as np
import pytest

import sinter
import stim

from sinter._collection import post_selection_mask_from_4th_coord
from sinter._decoding._decoding_all_built_in_decoders import BUILT_IN_DECODERS
from sinter._decoding._decoding import sample_decode
from sinter._decoding._decoding_vacuous import VacuousDecoder


def get_test_decoders() -> Tuple[List[str], Dict[str, sinter.Decoder]]:
    available_decoders = list(BUILT_IN_DECODERS.keys())
    custom_decoders = {}
    try:
        import pymatching
    except ImportError:
        available_decoders.remove('pymatching')
    try:
        import fusion_blossom
    except ImportError:
        available_decoders.remove('fusion_blossom')
    try:
        import mwpf
    except ImportError:
        available_decoders.remove('hypergraph_union_find')
        available_decoders.remove('mw_parity_factor')

    e = os.environ.get('SINTER_PYTEST_CUSTOM_DECODERS')
    if e is not None:
        for term in e.split(';'):
            module, method = term.split(':')
            for name, obj in getattr(__import__(module), method)().items():
                custom_decoders[name] = obj
                available_decoders.append(name)

    available_decoders.append("also_vacuous")
    custom_decoders["also_vacuous"] = VacuousDecoder()
    return available_decoders, custom_decoders

TEST_DECODER_NAMES, TEST_CUSTOM_DECODERS = get_test_decoders()

DECODER_CASES = [
    (decoder, force_streaming)
    for decoder in TEST_DECODER_NAMES
    for force_streaming in [None, True]
]


@pytest.mark.parametrize('decoder,force_streaming', DECODER_CASES)
def test_decode_repetition_code(decoder: str, force_streaming: Optional[bool]):
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
        __private__unstable__force_decode_on_disk=force_streaming,
        custom_decoders=TEST_CUSTOM_DECODERS,
    )
    assert result.discards == 0
    if 'vacuous' not in decoder:
        assert 1 <= result.errors <= 100
    assert result.shots == 1000


@pytest.mark.parametrize('decoder,force_streaming', DECODER_CASES)
def test_decode_surface_code(decoder: str, force_streaming: Optional[bool]):
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
        custom_decoders=TEST_CUSTOM_DECODERS,
    )
    if 'vacuous' not in decoder:
        assert 0 <= stats.errors <= 50


@pytest.mark.parametrize('decoder,force_streaming', DECODER_CASES)
def test_empty(decoder: str, force_streaming: Optional[bool]):
    circuit = stim.Circuit()
    result = sample_decode(
        circuit_obj=circuit,
        circuit_path=None,
        dem_obj=circuit.detector_error_model(decompose_errors=True),
        dem_path=None,
        num_shots=1000,
        decoder=decoder,
        __private__unstable__force_decode_on_disk=force_streaming,
        custom_decoders=TEST_CUSTOM_DECODERS,
    )
    assert result.discards == 0
    assert result.shots == 1000
    assert result.errors == 0


@pytest.mark.parametrize('decoder,force_streaming', DECODER_CASES)
def test_no_observables(decoder: str, force_streaming: Optional[bool]):
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
        custom_decoders=TEST_CUSTOM_DECODERS,
    )
    assert result.discards == 0
    assert result.shots == 1000
    assert result.errors == 0


@pytest.mark.parametrize('decoder,force_streaming', DECODER_CASES)
def test_invincible_observables(decoder: str, force_streaming: Optional[bool]):
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
        custom_decoders=TEST_CUSTOM_DECODERS,
    )
    assert result.discards == 0
    assert result.shots == 1000
    assert result.errors == 0


@pytest.mark.parametrize('decoder,force_streaming,offset', [(a, b, c) for a, b in DECODER_CASES for c in range(8)])
def test_observable_offsets_mod8(decoder: str, force_streaming: bool, offset: int):
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
        custom_decoders=TEST_CUSTOM_DECODERS,
    )
    assert result.discards == 0
    assert result.shots == 1000
    assert 50 <= result.errors <= 150


@pytest.mark.parametrize('decoder,force_streaming', DECODER_CASES)
def test_no_detectors(decoder: str, force_streaming: Optional[bool]):
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
        custom_decoders=TEST_CUSTOM_DECODERS,
    )
    assert result.discards == 0
    assert 50 <= result.errors <= 150


@pytest.mark.parametrize('decoder,force_streaming', DECODER_CASES)
def test_no_detectors_with_post_mask(decoder: str, force_streaming: Optional[bool]):
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
        custom_decoders=TEST_CUSTOM_DECODERS,
    )
    assert result.discards == 0
    assert 50 <= result.errors <= 150


@pytest.mark.parametrize('decoder,force_streaming', DECODER_CASES)
def test_post_selection(decoder: str, force_streaming: Optional[bool]):
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
        custom_decoders=TEST_CUSTOM_DECODERS,
    )
    assert 1050 <= result.discards <= 1350
    if 'vacuous' not in decoder:
        assert 40 <= result.errors <= 160


@pytest.mark.parametrize('decoder,force_streaming', DECODER_CASES)
def test_observable_post_selection(decoder: str, force_streaming: Optional[bool]):
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
        custom_decoders=TEST_CUSTOM_DECODERS,
    )
    np.testing.assert_allclose(result.discards / result.shots, 0.2, atol=0.1)
    if 'vacuous' not in decoder:
        np.testing.assert_allclose(result.errors / (result.shots - result.discards), 0.1, atol=0.05)


@pytest.mark.parametrize('decoder,force_streaming', DECODER_CASES)
def test_error_splitting(decoder: str, force_streaming: Optional[bool]):
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
        custom_decoders=TEST_CUSTOM_DECODERS,
    )
    assert result.discards == 0
    assert set(result.custom_counts.keys()) == {'obs_mistake_mask=E_', 'obs_mistake_mask=_E', 'obs_mistake_mask=EE'}
    if 'vacuous' not in decoder:
        np.testing.assert_allclose(result.errors / result.shots, 1 - 0.8 * 0.9, atol=0.05)
        np.testing.assert_allclose(result.custom_counts['obs_mistake_mask=E_'] / result.shots, 0.1 * 0.2, atol=0.05)
        np.testing.assert_allclose(result.custom_counts['obs_mistake_mask=_E'] / result.shots, 0.1 * 0.8, atol=0.05)
        np.testing.assert_allclose(result.custom_counts['obs_mistake_mask=EE'] / result.shots, 0.9 * 0.2, atol=0.05)


@pytest.mark.parametrize('decoder,force_streaming', DECODER_CASES)
def test_detector_counting(decoder: str, force_streaming: Optional[bool]):
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
        custom_decoders=TEST_CUSTOM_DECODERS,
    )
    assert result.discards == 0
    assert result.custom_counts['detectors_checked'] == 20000
    assert 0.3 * 10000 * 0.5 <= result.custom_counts['detection_events'] <= 0.3 * 10000 * 2.0
    assert set(result.custom_counts.keys()) == {'detectors_checked', 'detection_events'}


@pytest.mark.parametrize('decoder,force_streaming', DECODER_CASES)
def test_decode_fails_correctly(decoder: str, force_streaming: Optional[bool]):
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

        if 'vacuous' not in decoder:
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


@pytest.mark.parametrize('decoder', TEST_DECODER_NAMES)
def test_full_scale(decoder: str):
    result, = sinter.collect(
        num_workers=2,
        tasks=[sinter.Task(circuit=stim.Circuit())],
        decoders=[decoder],
        max_shots=1000,
        custom_decoders=TEST_CUSTOM_DECODERS,
    )
    assert result.discards == 0
    assert result.shots == 1000
    assert result.errors == 0
