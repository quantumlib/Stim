"""Decoding helpers for per-shot DEMs produced by the DEM generators.

``decode_with_generated_dems`` decodes a batch with the DEM(s) or PyMatching
``edge_reweights`` returned by a ``dem_gen`` callable, and
``_decompose_dem_graphlike`` decomposes hyperedges into graph-like components
(C++ kernel when it can be loaded, else the Python reference implementation).
"""

from __future__ import annotations

import math
from typing import Any, Sequence
import warnings

import numpy as np
from numpy.typing import NDArray
import sinter  # type: ignore[import-untyped]
import stim  # type: ignore[import-untyped]

from stimside.dem_generators.dem_generator_marginal import (
    _GeneratedDemList,
    _canonical_symptom_from_components,
    _collect_known_graphlike,
    _get_item_analysis_meta,
    _get_item_leakage_events,
    _graphlike_components,
    _raw_inst_components,
    _sym_to_targets,
    _xor,
)

_MAX_PYMATCHING_WEIGHT = float((1 << 24) - 1)


def _is_pymatching_decoder(decoder: object) -> bool:
    if isinstance(decoder, str):
        return "pymatching" in decoder.lower()
    cls_name = type(decoder).__name__.lower()
    mod_name = getattr(type(decoder), "__module__", "").lower()
    return "pymatching" in cls_name or "pymatching" in mod_name


def _dem_has_hyperedge(dem: stim.DetectorErrorModel) -> bool:
    """Return True if any error component in ``dem`` affects >2 detectors."""
    for inst in dem.flattened():
        if inst.type != "error":
            continue
        args = inst.args_copy()
        if not args or args[0] <= 0:
            continue
        dets: set[int] = set()
        for t in inst.targets_copy():
            if t.is_separator():
                if len(dets) > 2:
                    return True
                dets.clear()
            elif t.is_relative_detector_id():
                dets ^= {t.val}
        if len(dets) > 2:
            return True
    return False


def _warn_if_pymatching_hyperedge(
    decoder: object, dem: stim.DetectorErrorModel
) -> None:
    if _is_pymatching_decoder(decoder) and _dem_has_hyperedge(dem):
        warnings.warn(
            "DetectorErrorModel contains undecomposed hyperedge error mechanisms "
            "with >2 detectors while decoding with pymatching; pass "
            "decompose_errors=True to decompose errors into matchable graphs.",
            UserWarning,
            stacklevel=3,
        )


def _decompose_dem_graphlike_py(
    dem: stim.DetectorErrorModel,
    base_dem: stim.DetectorErrorModel | None = None,
) -> stim.DetectorErrorModel:
    """Python reference implementation of ``_decompose_dem_graphlike``."""
    if not _dem_has_hyperedge(dem):
        return dem
    known_graphlike = _collect_known_graphlike(base_dem, dem)
    out = stim.DetectorErrorModel()
    for inst in dem.flattened():
        if inst.type != "error":
            out.append(inst)
            continue
        p = float(inst.args_copy()[0])
        comps = _graphlike_components(inst, known_graphlike)
        sym = _canonical_symptom_from_components(comps)
        if sym == ((), ()):
            continue
        out.append("error", p, _sym_to_targets(sym))
    return out


_CPP_DECOMPOSITION_UNAVAILABLE = False


def _decompose_dem_graphlike_cpp(
    dem: stim.DetectorErrorModel,
    base_dem: stim.DetectorErrorModel | None = None,
) -> stim.DetectorErrorModel:
    """C++ port of ``_decompose_dem_graphlike_py`` (identical output DEM)."""
    from stimside.util.marginal_dem_kernels import (
        decompose_dem_text_cpp,
        dem_text_has_hyperedge_cpp,
    )

    dem_text = str(dem.flattened()).encode("utf-8")
    if not dem_text_has_hyperedge_cpp(dem_text):
        return dem
    base_text = (
        None if base_dem is None else str(base_dem.flattened()).encode("utf-8")
    )
    return stim.DetectorErrorModel(decompose_dem_text_cpp(dem_text, base_text))


def _decompose_dem_graphlike(
    dem: stim.DetectorErrorModel,
    base_dem: stim.DetectorErrorModel | None = None,
) -> stim.DetectorErrorModel:
    """Ensure every error instruction in ``dem`` is decomposed into <=2-detector components.

    Uses the C++ kernel when it can be loaded and the Python implementation
    (``_decompose_dem_graphlike_py``) otherwise; both give the same DEM.
    """
    global _CPP_DECOMPOSITION_UNAVAILABLE
    if not _CPP_DECOMPOSITION_UNAVAILABLE:
        try:
            return _decompose_dem_graphlike_cpp(dem, base_dem)
        except (OSError, RuntimeError, AttributeError) as exc:
            _CPP_DECOMPOSITION_UNAVAILABLE = True
            warnings.warn(
                f"Falling back to the Python DEM decomposition ({exc!r}).",
                RuntimeWarning,
                stacklevel=2,
            )
    return _decompose_dem_graphlike_py(dem, base_dem)


def _inst_canonical_symptom(inst: stim.DemInstruction) -> Any:
    return _canonical_symptom_from_components(_raw_inst_components(inst))


def _apply_reweight_update_dem(
    base_dem: stim.DetectorErrorModel, update_dem: stim.DetectorErrorModel
) -> stim.DetectorErrorModel:
    """Apply a reweight-update DEM (containing updated total probabilities) onto ``base_dem``."""
    updates: dict[Any, float] = {}
    for inst in update_dem.flattened():
        if inst.type != "error":
            continue
        sym = _inst_canonical_symptom(inst)
        if sym == ((), ()):
            continue
        p = float(inst.args_copy()[0])
        updates[sym] = _xor(updates.get(sym, 0.0), p)

    if not updates:
        return base_dem

    base_syms: dict[Any, float] = {}
    out = stim.DetectorErrorModel()
    for inst in base_dem.flattened():
        if inst.type != "error":
            out.append(inst)
            continue
        sym = _inst_canonical_symptom(inst)
        if sym == ((), ()):
            continue
        p = float(inst.args_copy()[0])
        base_syms[sym] = _xor(base_syms.get(sym, 0.0), p)

    for sym, p_base in base_syms.items():
        p_final = updates.pop(sym, p_base)
        if p_final > 0:
            out.append("error", p_final, _sym_to_targets(sym))
    for sym, p_new in updates.items():
        if p_new > 0:
            out.append("error", p_new, _sym_to_targets(sym))
    return out


def _prob_to_weight(p: float) -> float:
    if p <= 0.0:
        return _MAX_PYMATCHING_WEIGHT
    if p >= 1.0:
        return 0.0
    w = math.log((1.0 - p) / p)
    if w < 0.0:
        return 0.0
    if w > _MAX_PYMATCHING_WEIGHT:
        return _MAX_PYMATCHING_WEIGHT
    return w


def _extract_graphlike_edges(
    dem: stim.DetectorErrorModel,
) -> tuple[dict[tuple[int, int], float], dict[tuple[int, int], tuple[int, ...]]]:
    """Return (edge -> merged_prob, edge -> first_obs) for a graphlike DEM."""
    probs: dict[tuple[int, int], float] = {}
    first_obs: dict[tuple[int, int], tuple[int, ...]] = {}
    for inst in dem.flattened():
        if inst.type != "error":
            continue
        p = float(inst.args_copy()[0])
        if p <= 0.0:
            continue
        for dets, obs in _graphlike_components(inst):
            if not (1 <= len(dets) <= 2):
                continue
            edge = (dets[0], -1) if len(dets) == 1 else (dets[0], dets[1])
            if edge not in first_obs:
                first_obs[edge] = obs
            probs[edge] = _xor(probs.get(edge, 0.0), p)
    return probs, first_obs


def _weight_to_prob(w: float) -> float:
    if w >= _MAX_PYMATCHING_WEIGHT:
        return 0.0
    if w <= -_MAX_PYMATCHING_WEIGHT:
        return 1.0
    if w >= 0.0:
        ew = math.exp(-w)
        return ew / (1.0 + ew)
    ew = math.exp(w)
    return 1.0 / (1.0 + ew)


def _ensure_base_dem_has_reweight_edges(
    base_dem: stim.DetectorErrorModel,
    rw_list: Sequence[NDArray[np.float64] | None],
) -> stim.DetectorErrorModel:
    eff_base = _decompose_dem_graphlike(base_dem)
    existing_probs, existing_obs = _extract_graphlike_edges(eff_base)
    missing_edges: set[tuple[int, int]] = set()
    for rw in rw_list:
        if rw is None:
            continue
        for row in rw:
            u, v = int(row[0]), int(row[1])
            edge = (max(u, v), -1) if (u < 0 or v < 0) else (min(u, v), max(u, v))
            if edge not in existing_probs and edge not in existing_obs:
                missing_edges.add(edge)
    if not missing_edges:
        return eff_base
    out = eff_base.copy()
    for u, v in sorted(missing_edges):
        targets = (
            [stim.target_relative_detector_id(u)]
            if v < 0
            else [
                stim.target_relative_detector_id(u),
                stim.target_relative_detector_id(v),
            ]
        )
        out.append("error", 1e-12, targets)
    return out


def _apply_edge_reweights_to_dem(
    base_dem: stim.DetectorErrorModel,
    rw: NDArray[np.float64] | None,
) -> stim.DetectorErrorModel:
    eff_base = _decompose_dem_graphlike(base_dem)
    if rw is None or rw.shape[0] == 0:
        return eff_base
    base_probs, base_obs = _extract_graphlike_edges(eff_base)
    updated_probs = dict(base_probs)
    for row in rw:
        u, v, w = int(row[0]), int(row[1]), float(row[2])
        edge = (max(u, v), -1) if (u < 0 or v < 0) else (min(u, v), max(u, v))
        updated_probs[edge] = _weight_to_prob(w)
        if edge not in base_obs:
            base_obs[edge] = ()
    out = stim.DetectorErrorModel()
    for inst in eff_base.flattened():
        if inst.type != "error":
            out.append(inst)
    for edge, p in updated_probs.items():
        if p > 0.0:
            dets = (edge[0],) if edge[1] == -1 else (edge[0], edge[1])
            obs = base_obs.get(edge, ())
            out.append(
                "error",
                p,
                [stim.target_relative_detector_id(d) for d in dets]
                + [stim.target_logical_observable_id(o) for o in obs],
            )
    return out


def _decode_with_edge_reweights(
    decoder: Any,
    reweights: NDArray[np.float64] | Sequence[NDArray[np.float64] | None],
    bit_packed_dets: NDArray[np.uint8],
    *,
    base_dem: stim.DetectorErrorModel | None = None,
    analysis: Any = None,
) -> NDArray[np.uint8]:
    dets_2d = (
        bit_packed_dets[np.newaxis, :]
        if bit_packed_dets.ndim == 1
        else bit_packed_dets
    )
    num_shots = dets_2d.shape[0]
    if isinstance(reweights, np.ndarray):
        if reweights.ndim != 2 or reweights.shape[1] != 3:
            raise ValueError(
                f"Expected edge_reweights array of shape (num_reweights, 3), got {reweights.shape}."
            )
        rw_arr = (
            None
            if reweights.shape[0] == 0
            else np.ascontiguousarray(reweights, dtype=np.float64)
        )
        rw_list: list[NDArray[np.float64] | None] = [rw_arr] * num_shots
    else:
        if len(reweights) != num_shots:
            raise ValueError(
                f"dem_gen returned {len(reweights)} DEMs for a batch of {num_shots} shots."
            )
        rw_list = []
        seen_arrays: dict[int, NDArray[np.float64] | None] = {}
        for rw in reweights:
            if rw is None:
                rw_list.append(None)
                continue
            rid = id(rw)
            if rid not in seen_arrays:
                arr = np.asarray(rw, dtype=np.float64)
                if arr.ndim != 2 or arr.shape[1] != 3:
                    raise ValueError(
                        f"Expected edge_reweights array of shape (num_reweights, 3), got {arr.shape}."
                    )
                seen_arrays[rid] = (
                    None
                    if arr.shape[0] == 0
                    else np.ascontiguousarray(arr, dtype=np.float64)
                )
            rw_list.append(seen_arrays[rid])

    if not hasattr(decoder, "decode_batch") and not _is_pymatching_decoder(decoder):
        eff_base_dem = base_dem or (
            analysis.matching_base_dem if analysis is not None else None
        )
        if eff_base_dem is None:
            raise ValueError(
                "base_dem (or dems generated by MarginalLeakageDemGenerator) is required "
                "when decoding edge_reweights."
            )
        if num_shots == 0:
            return np.zeros((0, 0), dtype=np.uint8)
        shots_by_rw: dict[int, list[int]] = {}
        rw_by_key: dict[int, NDArray[np.float64] | None] = {}
        for shot_idx, rw_item in enumerate(rw_list):
            key = 0 if rw_item is None else id(rw_item)
            shots_by_rw.setdefault(key, []).append(shot_idx)
            rw_by_key[key] = rw_item
        out: NDArray[np.uint8] | None = None
        for key, shots in shots_by_rw.items():
            dem_for_group = _apply_edge_reweights_to_dem(
                eff_base_dem, rw_by_key[key]
            )
            preds = decoder.compile_decoder_for_dem(
                dem=dem_for_group
            ).decode_shots_bit_packed(
                bit_packed_detection_event_data=dets_2d[shots]
            )
            if out is None:
                out = np.zeros((num_shots, preds.shape[1]), dtype=np.uint8)
            out[shots] = preds
        assert out is not None
        return out

    if hasattr(decoder, "decode_batch"):
        matcher = decoder
    else:
        if analysis is not None and base_dem is None:
            if analysis._cached_matcher is None:
                import pymatching  # type: ignore[import-untyped]

                analysis._cached_matcher = (
                    pymatching.Matching.from_detector_error_model(
                        analysis.matching_base_dem
                    )
                )
            matcher = analysis._cached_matcher
        elif base_dem is not None:
            import pymatching  # type: ignore[import-untyped]

            eff_base = _ensure_base_dem_has_reweight_edges(base_dem, rw_list)
            matcher = pymatching.Matching.from_detector_error_model(eff_base)
        else:
            raise ValueError(
                "base_dem (or dems generated by MarginalLeakageDemGenerator) is required "
                "when decoding edge_reweights."
            )

    if num_shots == 0:
        num_obs_bytes = (matcher.num_fault_ids + 7) // 8
        return np.zeros((0, num_obs_bytes), dtype=np.uint8)

    return matcher.decode_batch(
        dets_2d,
        bit_packed_shots=True,
        bit_packed_predictions=True,
        edge_reweights=rw_list,
    )


def decode_with_generated_dems(
    decoder: sinter.Decoder | Any,
    dems: (
        stim.DetectorErrorModel
        | Sequence[stim.DetectorErrorModel]
        | NDArray[np.float64]
        | Sequence[NDArray[np.float64] | None]
    ),
    bit_packed_dets: NDArray[np.uint8] | None = None,
    decompose_errors: bool = False,
    reweight_only: bool = False,
    *,
    base_dem: stim.DetectorErrorModel | None = None,
) -> Any:
    """Decode a batch with the DEM(s) or edge_reweights returned by a dem_gen callable.

    Args:
        decoder: A ``sinter.Decoder`` (such as ``sinter.BUILT_IN_DECODERS["pymatching"]``)
            or a ``pymatching.Matching`` instance.
        dems: One DEM for the whole batch, one DEM per shot, or PyMatching
            ``edge_reweights`` array(s) of shape ``(num_reweights, 3)``.
        bit_packed_dets: Bit-packed detection event array of shape
            ``(num_shots, ceil(num_detectors / 8))``. If ``None``, returns the
            transformed ``dems`` / ``edge_reweights`` instead of decoding.
        decompose_errors: If True, decompose hyperedge errors into matchable
            graphlike components using Stim's native APIs.
        reweight_only: If True, use DEM reweight updates instead of full DEMs.
            When both ``decompose_errors=True`` and ``reweight_only=True``, uses
            PyMatching's ``edge_reweights`` format ``[node1, node2, weight]``.
        base_dem: Optional baseline ``stim.DetectorErrorModel`` when ``dems`` is
            passed as raw reweight updates or ``edge_reweights`` arrays without
            generator metadata.
    """
    dec_flag, rew_flag = bool(decompose_errors), bool(reweight_only)
    if isinstance(decoder, str):
        decoder = sinter.BUILT_IN_DECODERS[decoder]

    # If dems came from MarginalLeakageDemGenerator and caller requested
    # decompose_errors or reweight_only on decode_with_generated_dems, regenerate
    # directly from the generator's fast C++ pipeline so we don't combine with
    # baseline and diff again.
    single_imeta = (
        _get_item_analysis_meta(dems)
        if isinstance(dems, (stim.DetectorErrorModel, np.ndarray))
        else None
    )
    regenerated_from_gen = False
    if isinstance(dems, _GeneratedDemList) and dems._generator is not None:
        eff_dec = dec_flag or dems.decompose_errors
        eff_rew = rew_flag or dems.reweight_only
        if (eff_dec, eff_rew) != (dems.decompose_errors, dems.reweight_only):
            assert dems._circuit is not None and dems._records is not None
            dems = dems._generator(
                dems._circuit,
                dems._records,
                decompose_errors=eff_dec,
                reweight_only=eff_rew,
                leakage_events=dems.leakage_events,
            )  # type: ignore[assignment]
        dec_flag, rew_flag = eff_dec, eff_rew
        regenerated_from_gen = True
    elif (
        single_imeta is not None
        and single_imeta[3] is not None
        and single_imeta[4] is not None
        and single_imeta[5] is not None
    ):
        _, orig_dec, orig_rew, gen_inst, circ_inst, rec_inst = single_imeta
        eff_dec = dec_flag or orig_dec
        eff_rew = rew_flag or orig_rew
        if (eff_dec, eff_rew) != (orig_dec, orig_rew):
            dems = gen_inst(
                circ_inst,
                rec_inst,
                decompose_errors=eff_dec,
                reweight_only=eff_rew,
                leakage_events=_get_item_leakage_events(dems),
            )
            single_imeta = _get_item_analysis_meta(dems)
        dec_flag, rew_flag = eff_dec, eff_rew
        regenerated_from_gen = True
    elif not isinstance(dems, (stim.DetectorErrorModel, np.ndarray)):
        dems_seq_check = dems if isinstance(dems, list) else list(dems)
        if dems_seq_check:
            metas = [_get_item_analysis_meta(x) for x in dems_seq_check]
            first_m = metas[0]
            if (
                first_m is not None
                and first_m[3] is not None
                and first_m[4] is not None
                and all(
                    m is not None
                    and m[3] is first_m[3]
                    and m[4] is first_m[4]
                    and m[5] is not None
                    for m in metas
                )
            ):
                _, orig_dec, orig_rew, gen_inst, circ_inst, _ = first_m
                assert gen_inst is not None and circ_inst is not None
                eff_dec = dec_flag or orig_dec
                eff_rew = rew_flag or orig_rew
                if (eff_dec, eff_rew) != (orig_dec, orig_rew):
                    rec_2d = np.stack([m[5] for m in metas if m is not None and m[5] is not None], axis=0)
                    events_2d = [
                        ev
                        for ev in (_get_item_leakage_events(x) for x in dems_seq_check)
                        if ev is not None
                    ]
                    dems = gen_inst(
                        circ_inst,
                        rec_2d,
                        decompose_errors=eff_dec,
                        reweight_only=eff_rew,
                        leakage_events=events_2d or None,
                    )
                dec_flag, rew_flag = eff_dec, eff_rew
                regenerated_from_gen = True

    if bit_packed_dets is None and regenerated_from_gen:
        return dems

    dets_2d = (
        None
        if bit_packed_dets is None
        else (
            bit_packed_dets[np.newaxis, :]
            if bit_packed_dets.ndim == 1
            else bit_packed_dets
        )
    )

    # Case 1: dems is already in PyMatching edge_reweights format (np.ndarray or Sequence[np.ndarray | None])
    if isinstance(dems, np.ndarray):
        if dets_2d is None:
            return dems
        single_analysis = single_imeta[0] if single_imeta is not None else None
        if (
            not _is_pymatching_decoder(decoder)
            and not hasattr(decoder, "decode_batch")
        ):
            if (
                single_imeta is not None
                and single_imeta[3] is not None
                and single_imeta[4] is not None
                and single_imeta[5] is not None
            ):
                dem_full = single_imeta[3](
                    single_imeta[4],
                    single_imeta[5],
                    decompose_errors=True,
                    reweight_only=False,
                    leakage_events=_get_item_leakage_events(dems),
                )
                return decode_with_generated_dems(
                    decoder, dem_full, dets_2d, decompose_errors=True
                )
        return _decode_with_edge_reweights(
            decoder,
            dems,
            dets_2d,
            base_dem=base_dem,
            analysis=single_analysis,
        )
    if not isinstance(dems, stim.DetectorErrorModel):
        dems_seq = dems if isinstance(dems, list) else list(dems)
        first_non_none = next((x for x in dems_seq if x is not None), None)
        if isinstance(first_non_none, np.ndarray) or (
            len(dems_seq) > 0 and first_non_none is None
        ):
            if dets_2d is None:
                return dems
            analysis = getattr(dems, "analysis", None)
            if analysis is None and first_non_none is not None:
                imeta = _get_item_analysis_meta(first_non_none)
                if imeta is not None:
                    analysis = imeta[0]
            if (
                not _is_pymatching_decoder(decoder)
                and not hasattr(decoder, "decode_batch")
                and isinstance(dems, _GeneratedDemList)
                and dems._generator is not None
            ):
                # Fallback to decomposed full DEMs for non-pymatching decoders
                assert dems._circuit is not None and dems._records is not None
                dems_full = dems._generator(
                    dems._circuit,
                    dems._records,
                    decompose_errors=True,
                    reweight_only=False,
                    leakage_events=dems.leakage_events,
                )
                return decode_with_generated_dems(
                    decoder, dems_full, dets_2d, decompose_errors=True
                )
            return _decode_with_edge_reweights(
                decoder,
                dems_seq,  # type: ignore[arg-type]
                dets_2d,
                base_dem=base_dem,
                analysis=analysis,
            )

    # Case 2: Single stim.DetectorErrorModel
    if isinstance(dems, stim.DetectorErrorModel):
        imeta = single_imeta
        single_analysis = imeta[0] if imeta is not None else None
        is_rew_single = rew_flag or (imeta is not None and imeta[2])
        eff_base_dem = base_dem or (
            single_analysis.baseline if single_analysis is not None else None
        )

        if (
            dec_flag
            and rew_flag
            and (dets_2d is None or _is_pymatching_decoder(decoder))
            and eff_base_dem is not None
        ):
            if dets_2d is None:
                res_list = decode_with_generated_dems(
                    decoder,
                    [dems],
                    None,
                    decompose_errors=True,
                    reweight_only=True,
                    base_dem=eff_base_dem,
                )
                return res_list[0]
            num_shots = dets_2d.shape[0]
            return decode_with_generated_dems(
                decoder,
                [dems] * num_shots,
                dets_2d,
                decompose_errors=True,
                reweight_only=True,
                base_dem=eff_base_dem,
            )

        dem_single = dems
        if is_rew_single and eff_base_dem is not None:
            dem_single = _apply_reweight_update_dem(eff_base_dem, dem_single)
        if dec_flag:
            dem_single = _decompose_dem_graphlike(dem_single, base_dem=eff_base_dem)
        if dets_2d is None:
            return dem_single
        if not dec_flag:
            _warn_if_pymatching_hyperedge(decoder, dem_single)
        return decoder.compile_decoder_for_dem(dem=dem_single).decode_shots_bit_packed(
            bit_packed_detection_event_data=dets_2d
        )

    # Case 3: Sequence of stim.DetectorErrorModel
    dems_list: list[stim.DetectorErrorModel] = list(dems)  # type: ignore[arg-type]
    if dets_2d is not None:
        num_shots = dets_2d.shape[0]
        if len(dems_list) != num_shots:
            raise ValueError(
                f"dem_gen returned {len(dems_list)} DEMs for a batch of {num_shots} shots."
            )
        if num_shots == 0:
            return np.zeros((0, 0), dtype=np.uint8)
    elif not dems_list:
        return []

    imeta0 = _get_item_analysis_meta(dems_list[0])
    analysis = getattr(dems, "analysis", None) or (
        imeta0[0] if imeta0 is not None else None
    )
    is_reweight_dems = bool(getattr(dems, "reweight_only", False)) or (
        imeta0[2] if imeta0 is not None else False
    )
    gen_base_dem = (
        base_dem
        or getattr(dems, "base_dem", None)
        or (analysis.baseline if analysis is not None else None)
    )

    shots_by_dem: dict[int, list[int]] = {}
    for shot, dem in enumerate(dems_list):
        shots_by_dem.setdefault(id(dem), []).append(shot)

    # If both decompose_errors and reweight_only are requested on plain DEMs with PyMatching (or bit_packed_dets is None):
    if dec_flag and rew_flag and (dets_2d is None or _is_pymatching_decoder(decoder)):
        unique_ids = list(shots_by_dem.keys())
        resolved_by_id: dict[int, stim.DetectorErrorModel] = {}
        for did in unique_ids:
            shot0 = shots_by_dem[did][0]
            raw_d = dems_list[shot0]
            if (rew_flag or is_reweight_dems) and gen_base_dem is not None:
                raw_d = _apply_reweight_update_dem(gen_base_dem, raw_d)
            resolved_by_id[did] = _decompose_dem_graphlike(
                raw_d, base_dem=gen_base_dem
            )

        ref_dem = (
            _decompose_dem_graphlike(gen_base_dem)
            if gen_base_dem is not None
            else resolved_by_id[unique_ids[0]]
        )
        base_probs, base_obs = _extract_graphlike_edges(ref_dem)
        obs_compatible = True
        edges_by_id: dict[int, dict[tuple[int, int], float]] = {}
        for did, r_dem in resolved_by_id.items():
            e_probs, e_obs = _extract_graphlike_edges(r_dem)
            edges_by_id[did] = e_probs
            for edge, obs in e_obs.items():
                if edge in base_obs and base_obs[edge] != obs:
                    obs_compatible = False
                    break
                if edge not in base_obs:
                    base_obs[edge] = obs

        if obs_compatible:
            full_base_dem = ref_dem.copy()
            for edge, obs in base_obs.items():
                if edge not in base_probs:
                    dets = (edge[0],) if edge[1] == -1 else (edge[0], edge[1])
                    full_base_dem.append(
                        "error",
                        1e-12,
                        [stim.target_relative_detector_id(d) for d in dets]
                        + [stim.target_logical_observable_id(o) for o in obs],
                    )
            rw_by_id: dict[int, NDArray[np.float64] | None] = {}
            empty_arr = np.empty((0, 3), dtype=np.float64)
            for did, e_probs in edges_by_id.items():
                rows: list[list[float]] = []
                all_edges = set(base_probs.keys()) | set(e_probs.keys())
                for edge in sorted(all_edges):
                    p_b = base_probs.get(edge, 0.0)
                    p_cur = e_probs.get(edge, 0.0)
                    if abs(p_cur - p_b) > 1e-15:
                        rows.append(
                            [float(edge[0]), float(edge[1]), _prob_to_weight(p_cur)]
                        )
                rw_by_id[did] = (
                    np.asarray(rows, dtype=np.float64)
                    if rows
                    else (empty_arr if dets_2d is None else None)
                )
            rw_per_shot = [rw_by_id[id(d)] for d in dems_list]
            if dets_2d is None:
                return rw_per_shot
            return _decode_with_edge_reweights(
                decoder,
                rw_per_shot,
                dets_2d,
                base_dem=full_base_dem,
            )

    if dets_2d is None:
        transformed_by_id: dict[int, stim.DetectorErrorModel] = {}
        for did, shots in shots_by_dem.items():
            dem_item = dems_list[shots[0]]
            if (rew_flag or is_reweight_dems) and gen_base_dem is not None:
                dem_item = _apply_reweight_update_dem(gen_base_dem, dem_item)
            if dec_flag:
                dem_item = _decompose_dem_graphlike(dem_item, base_dem=gen_base_dem)
            transformed_by_id[did] = dem_item
        return [transformed_by_id[id(d)] for d in dems_list]

    out = None
    for shots in shots_by_dem.values():
        dem_item = dems_list[shots[0]]
        if (rew_flag or is_reweight_dems) and gen_base_dem is not None:
            dem_item = _apply_reweight_update_dem(gen_base_dem, dem_item)
        if dec_flag:
            dem_item = _decompose_dem_graphlike(dem_item, base_dem=gen_base_dem)
        else:
            _warn_if_pymatching_hyperedge(decoder, dem_item)
        predictions = decoder.compile_decoder_for_dem(
            dem=dem_item
        ).decode_shots_bit_packed(
            bit_packed_detection_event_data=dets_2d[shots]
        )
        if out is None:
            out = np.zeros((num_shots, predictions.shape[1]), dtype=np.uint8)
        out[shots] = predictions
    assert out is not None
    return out
