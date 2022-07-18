import numpy as np
import pytest
import stim

import sinter
from sinter._collection import post_selection_mask_from_4th_coord
from sinter._decoding import sample_decode


def test_decode_using_pymatching():
    circuit = stim.Circuit.generated('repetition_code:memory',
                                     rounds=3,
                                     distance=3,
                                     after_clifford_depolarization=0.05)
    result = sample_decode(
        circuit=circuit,
        decoder_error_model=circuit.detector_error_model(decompose_errors=True),
        num_shots=1000,
        decoder='pymatching',
    )
    assert result.discards == 0
    assert 1 <= result.errors <= 100
    assert result.shots == 1000


def test_pymatching_works_on_surface_code():
    circuit = stim.Circuit.generated(
        "surface_code:rotated_memory_x",
        distance=3,
        rounds=15,
        after_clifford_depolarization=0.001,
    )
    stats = sample_decode(
        num_shots=1000,
        circuit=circuit,
        decoder_error_model=circuit.detector_error_model(decompose_errors=True),
        decoder="pymatching",
    )
    assert 0 <= stats.errors <= 50


def test_decode_using_internal_decoder():
    pytest.importorskip('gqec')

    circuit = stim.Circuit.generated('repetition_code:memory',
                                     rounds=3,
                                     distance=3,
                                     after_clifford_depolarization=0.05)
    result = sample_decode(
        circuit=circuit,
        decoder_error_model=circuit.detector_error_model(decompose_errors=True),
        num_shots=1000,
        decoder='internal',
    )
    assert result.discards == 0
    assert 1 <= result.errors <= 100
    assert result.shots == 1000


def test_decode_using_internal_decoder_correlated():
    pytest.importorskip('gqec')

    circuit = stim.Circuit.generated('repetition_code:memory',
                                     rounds=3,
                                     distance=3,
                                     after_clifford_depolarization=0.05)
    result = sample_decode(
        circuit=circuit,
        decoder_error_model=circuit.detector_error_model(decompose_errors=True),
        num_shots=1000,
        decoder='internal_correlated',
    )
    assert result.discards == 0
    assert 1 <= result.errors <= 100
    assert result.shots == 1000


def test_empty():
    circuit = stim.Circuit()
    result = sample_decode(
        circuit=circuit,
        decoder_error_model=circuit.detector_error_model(decompose_errors=True),
        num_shots=1000,
        decoder='pymatching',
    )
    assert result.discards == 0
    assert result.shots == 1000
    assert result.errors == 0


def test_no_observables():
    c = stim.Circuit("""
        X_ERROR(0.1) 0
        M 0
        DETECTOR rec[-1]
    """)
    result = sample_decode(
        circuit=c,
        decoder_error_model=c.detector_error_model(decompose_errors=True),
        num_shots=1000,
        decoder='pymatching',
    )
    assert result.discards == 0
    assert result.shots == 1000
    assert result.errors == 0


def test_invincible_observables():
    c = stim.Circuit("""
        X_ERROR(0.1) 0
        M 0 1
        DETECTOR rec[-2]
        OBSERVABLE_INCLUDE(1) rec[-1]
    """)
    result = sample_decode(
        circuit=c,
        decoder_error_model=c.detector_error_model(decompose_errors=True),
        num_shots=1000,
        decoder='pymatching',
    )
    assert result.discards == 0
    assert result.shots == 1000
    assert result.errors == 0


@pytest.mark.parametrize("offset", range(8))
def test_observable_offsets_mod8(offset: int):
    c = stim.Circuit("""
        X_ERROR(0.1) 0
        MR 0
        DETECTOR rec[-1]
    """) * (8 + offset) + stim.Circuit("""
        X_ERROR(0.1) 0
        MR 0
        OBSERVABLE_INCLUDE(0) rec[-1]
    """)
    result = sample_decode(
        circuit=c,
        decoder_error_model=c.detector_error_model(decompose_errors=True),
        num_shots=1000,
        decoder='pymatching',
    )
    assert result.discards == 0
    assert result.shots == 1000
    assert 50 <= result.errors <= 150


def test_no_detectors():
    c = stim.Circuit("""
        X_ERROR(0.1) 0
        M 0
        OBSERVABLE_INCLUDE(0) rec[-1]
    """)
    result = sample_decode(
        circuit=c,
        decoder_error_model=c.detector_error_model(decompose_errors=True),
        num_shots=1000,
        decoder='pymatching',
    )
    assert result.discards == 0
    assert 50 <= result.errors <= 150


def test_no_detectors_with_post_mask():
    c = stim.Circuit("""
        X_ERROR(0.1) 0
        M 0
        OBSERVABLE_INCLUDE(0) rec[-1]
    """)
    result = sample_decode(
        circuit=c,
        decoder_error_model=c.detector_error_model(decompose_errors=True),
        post_mask=np.array([], dtype=np.uint8),
        num_shots=1000,
        decoder='pymatching',
    )
    assert result.discards == 0
    assert 50 <= result.errors <= 150


def test_post_selection():
    c = stim.Circuit("""
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
        circuit=c,
        decoder_error_model=c.detector_error_model(decompose_errors=True),
        post_mask=post_selection_mask_from_4th_coord(c),
        num_shots=2000,
        decoder='pymatching',
    )
    assert 1050 <= result.discards <= 1350
    assert 40 <= result.errors <= 160
