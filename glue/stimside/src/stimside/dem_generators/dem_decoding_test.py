"""C++ vs Python graph-like DEM decomposition: identical output DEMs."""

import numpy as np
import pytest
import sinter  # type: ignore[import-untyped]
import stim  # type: ignore[import-untyped]

from stimside.dem_generators import dem_decoding
from stimside.dem_generators.dem_decoding import (
    _decompose_dem_graphlike,
    _decompose_dem_graphlike_cpp,
    _decompose_dem_graphlike_py,
    decode_with_generated_dems,
)
from stimside.dem_generators.dem_generator_marginal import MarginalLeakageDemGenerator
from stimside.dem_generators.dem_generator_marginal_test import (
    _make_surface_code_leakage_circuit,
)


def _assert_identical(dem, base_dem=None):
    py = _decompose_dem_graphlike_py(dem, base_dem)
    cpp = _decompose_dem_graphlike_cpp(dem, base_dem)
    assert str(cpp) == str(py)
    assert cpp == py


def _random_dem_text(rng, num_dets, num_obs, num_lines, max_comp_dets):
    lines = [f"detector({d % 7}, {d // 7}) D{d}" for d in range(0, num_dets, 3)]
    lines.append(f"logical_observable L{num_obs - 1}")
    for _ in range(num_lines):
        comps = []
        for _ in range(int(rng.choice([1, 1, 1, 2, 3]))):
            k = int(rng.integers(0, min(max_comp_dets, num_dets) + 1))
            dets = rng.choice(num_dets, size=k, replace=bool(rng.random() < 0.2))
            obs = [o for o in rng.choice(num_obs, size=2) if rng.random() < 0.3]
            targets = [f"D{d}" for d in dets] + [f"L{o}" for o in obs]
            if targets:
                comps.append(" ".join(targets))
        if not comps:
            continue
        p = float(rng.choice([0.0, 0.5, rng.random() * 0.1, rng.random() * 1e-6]))
        tag = "[some_tag]" if rng.random() < 0.2 else ""
        lines.append(f"error{tag}({p!r}) " + " ^ ".join(comps))
    return "\n".join(lines)


@pytest.mark.parametrize("seed", range(40))
def test_random_dems_decompose_identically(seed):
    rng = np.random.default_rng(seed)
    num_dets = int(rng.integers(3, 40))
    dem = stim.DetectorErrorModel(
        _random_dem_text(rng, num_dets, int(rng.integers(1, 90)), 60, 7)
    )
    _assert_identical(dem)
    base = stim.DetectorErrorModel(
        _random_dem_text(rng, num_dets, 3, 20, 2)
    )
    _assert_identical(dem, base)


def test_repeat_blocks_shift_detectors_and_coords_decompose_identically():
    dem = stim.DetectorErrorModel(
        """
        detector(1, 2) D0
        error(0.1) D0 D1
        error(0.01) D1
        repeat 3 {
            error(0.125) D0 D1 D2 D3 L0
            error(0.2) D2 D3
            detector(0, 0, 1) D2
            shift_detectors(0, 0, 1) 2
        }
        error(0.3) D0 D1 D2 L1 ^ D5
        """
    )
    _assert_identical(dem)
    _assert_identical(dem, stim.DetectorErrorModel("error(0.1) D0 D2 L0\nerror(0.1) D1 D3"))


def test_empty_known_set_and_observable_only_components():
    _assert_identical(stim.DetectorErrorModel("error(0.1) D0 D1 D2 D3 D4 L3"))
    _assert_identical(
        stim.DetectorErrorModel("error(0.1) D0 D1 D2 ^ L2\nerror(0.2) L1\nerror(0) D5 D6 D7 D8")
    )


@pytest.mark.parametrize("n", [17, 20, 64, 70])
def test_large_hyperedges_decompose_identically(n):
    # A path of known edges; the hyperedge's L0 is carried by no known edge, so the
    # exact search fails and the cover search (n <= 16) / greedy peeling is used.
    lines = [f"error(0.01) D{d} D{d + 1}" for d in range(0, n - 1, 3)]
    lines += [f"error(0.01) D{d}" for d in range(1, n, 5)]
    lines.append("error(0.1) " + " ".join(f"D{d}" for d in range(n)) + " L0")
    lines.append("error(0.1) " + " ".join(f"D{d}" for d in range(0, n, 2)) + " L100")
    _assert_identical(stim.DetectorErrorModel("\n".join(lines)))


def test_cover_search_path_decomposes_identically():
    for n in (3, 5, 9, 16):
        lines = [f"error(0.01) D{d} D{d + 1} L{d % 2}" for d in range(0, n - 1, 2)]
        lines.append("error(0.1) " + " ".join(f"D{d}" for d in range(n)) + " L7")
        _assert_identical(stim.DetectorErrorModel("\n".join(lines)))


@pytest.mark.parametrize("reweight_only", [False, True])
def test_surface_code_per_shot_dems_decompose_identically(reweight_only):
    circuit = _make_surface_code_leakage_circuit(distance=3, rounds=3)
    gen = MarginalLeakageDemGenerator(reweight_only=reweight_only)
    rng = np.random.default_rng(5)
    records = rng.random((24, circuit.num_measurements)) < 0.05
    base = gen.base_dem(circuit)
    dems = gen(circuit, records)
    for dem in {id(d): d for d in dems}.values():
        _assert_identical(dem)
        _assert_identical(dem, base)
    _assert_identical(base)


def test_decoding_with_forced_python_decomposition_gives_same_predictions(monkeypatch):
    circuit = _make_surface_code_leakage_circuit(distance=3, rounds=2)
    gen = MarginalLeakageDemGenerator()
    rng = np.random.default_rng(1)
    records = rng.random((16, circuit.num_measurements)) < 0.05
    dets = np.packbits(
        rng.random((16, circuit.num_detectors)) < 0.05, axis=1, bitorder="little"
    )
    decoder = sinter.BUILT_IN_DECODERS["pymatching"]
    cpp = decode_with_generated_dems(decoder, gen(circuit, records), dets, decompose_errors=True)
    monkeypatch.setattr(dem_decoding, "_CPP_DECOMPOSITION_UNAVAILABLE", True)
    py = decode_with_generated_dems(decoder, gen(circuit, records), dets, decompose_errors=True)
    np.testing.assert_array_equal(cpp, py)


def test_falls_back_to_python_with_a_warning(monkeypatch):
    def broken(*args, **kwargs):
        raise OSError("no kernel library")

    monkeypatch.setattr(dem_decoding, "_decompose_dem_graphlike_cpp", broken)
    monkeypatch.setattr(dem_decoding, "_CPP_DECOMPOSITION_UNAVAILABLE", False)
    dem = stim.DetectorErrorModel("error(0.1) D0 D1\nerror(0.1) D0 D1 D2")
    with pytest.warns(RuntimeWarning, match="Python DEM decomposition"):
        out = _decompose_dem_graphlike(dem)
    assert out == _decompose_dem_graphlike_py(dem)
    assert dem_decoding._CPP_DECOMPOSITION_UNAVAILABLE


def test_dem_without_hyperedges_is_returned_unchanged():
    dem = stim.DetectorErrorModel("error(0.1) D0 D1\nerror(0.1) D2 L0")
    assert _decompose_dem_graphlike_cpp(dem) is dem
    assert _decompose_dem_graphlike_py(dem) is dem
