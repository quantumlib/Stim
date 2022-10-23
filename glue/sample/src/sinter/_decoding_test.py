import numpy as np
import pytest
import stim

from sinter._collection import post_selection_mask_from_4th_coord
from sinter._decoding import sample_decode

DECODER_PACKAGES = [
    ('pymatching', 'pymatching'),
    ('fusion_blossom', 'fusion_blossom'),
    ('internal', 'gqec'),
    ('internal_correlated', 'gqec'),
    ('internal_2', 'gqec'),
    ('internal_correlated_2', 'gqec'),
]


@pytest.mark.parametrize('decoder,required_import', DECODER_PACKAGES)
def test_decode_repetition_code(decoder: str, required_import: str):
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
    assert 1 <= result.errors <= 100
    assert result.shots == 1000


@pytest.mark.parametrize('decoder,required_import', DECODER_PACKAGES)
def test_decode_surface_code(decoder: str, required_import: str):
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
    )
    assert 0 <= stats.errors <= 50


@pytest.mark.parametrize('decoder,required_import', DECODER_PACKAGES)
def test_empty(decoder: str, required_import: str):
    pytest.importorskip(required_import)
    circuit = stim.Circuit()
    result = sample_decode(
        circuit_obj=circuit,
        circuit_path=None,
        dem_obj=circuit.detector_error_model(decompose_errors=True),
        dem_path=None,
        num_shots=1000,
        decoder=decoder,
    )
    assert result.discards == 0
    assert result.shots == 1000
    assert result.errors == 0


@pytest.mark.parametrize('decoder,required_import', DECODER_PACKAGES)
def test_no_observables(decoder: str, required_import: str):
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
    )
    assert result.discards == 0
    assert result.shots == 1000
    assert result.errors == 0


@pytest.mark.parametrize('decoder,required_import', DECODER_PACKAGES)
def test_invincible_observables(decoder: str, required_import: str):
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
    )
    assert result.discards == 0
    assert result.shots == 1000
    assert result.errors == 0


@pytest.mark.parametrize('decoder,required_import,offset', [(a, b, c) for a, b in DECODER_PACKAGES for c in range(8)])
def test_observable_offsets_mod8(decoder: str, required_import: str, offset: int):
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
    )
    assert result.discards == 0
    assert result.shots == 1000
    assert 50 <= result.errors <= 150


@pytest.mark.parametrize('decoder,required_import', DECODER_PACKAGES)
def test_no_detectors(decoder: str, required_import: str):
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
    )
    assert result.discards == 0
    assert 50 <= result.errors <= 150


@pytest.mark.parametrize('decoder,required_import', DECODER_PACKAGES)
def test_no_detectors_with_post_mask(decoder: str, required_import: str):
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
    )
    assert result.discards == 0
    assert 50 <= result.errors <= 150


@pytest.mark.parametrize('decoder,required_import', DECODER_PACKAGES)
def test_post_selection(decoder: str, required_import: str):
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
    )
    assert 1050 <= result.discards <= 1350
    assert 40 <= result.errors <= 160
