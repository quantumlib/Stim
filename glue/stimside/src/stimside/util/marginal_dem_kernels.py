from __future__ import annotations

import ctypes
from pathlib import Path

import numpy as np
from numpy.typing import NDArray

from stimside.util._kernel_loader import (
    find_prebuilt_so as _find_prebuilt_so_candidate,
    load_cdll,
    resolve_so,
)

u8_p = ctypes.POINTER(ctypes.c_uint8)
i32_p = ctypes.POINTER(ctypes.c_int32)
f64_p = ctypes.POINTER(ctypes.c_double)
void_p = ctypes.c_void_p

MOP_NONE = 0
MOP_MEAS_FLAG = 1
MOP_MEAS_PROJ_Z = 2
MOP_TRANS_1 = 3
MOP_TRANS_2 = 4
MOP_UNTAGGED_1Q_UNITARY = 5
MOP_UNTAGGED_2Q_UNITARY = 6
MOP_BARE_NOOP = 7
MOP_BARE_1Q_UNITARY = 8
MOP_BARE_RESET = 9
MOP_BARE_OTHER = 10
MOP_CONDITIONED = 11


class OpDescC(ctypes.Structure):
    _fields_ = [
        ("kind", ctypes.c_int32),
        ("target_offset", ctypes.c_int32),
        ("target_count", ctypes.c_int32),
        ("aux_offset", ctypes.c_int32),
        ("aux_count", ctypes.c_int32),
        ("gate_id", ctypes.c_int32),
        ("reset_basis", ctypes.c_int32),
        ("ctrl_leg0_basis", ctypes.c_int32),
        ("ctrl_leg1_basis", ctypes.c_int32),
        ("cond_base_fires", ctypes.c_int32),
        ("cond_subkind", ctypes.c_int32),
        ("cond_channel_id", ctypes.c_int32),
    ]


class SourceDescC(ctypes.Structure):
    _fields_ = [
        ("op_index", ctypes.c_int32),
        ("group", ctypes.c_int32),
        ("qubit", ctypes.c_int32),
        ("state", ctypes.c_int32),
        ("weight", ctypes.c_double),
        ("partner", ctypes.c_int32),
        ("partner_channel_id", ctypes.c_int32),
    ]


class Trans1RuleC(ctypes.Structure):
    _fields_ = [
        ("state", ctypes.c_int32),
        ("p_unleak", ctypes.c_double),
        ("next_state", ctypes.c_int32),
    ]


class Trans2RuleC(ctypes.Structure):
    _fields_ = [
        ("state", ctypes.c_int32),
        ("leg", ctypes.c_int32),
        ("p_clear", ctypes.c_double),
        ("p_move", ctypes.c_double),
        ("move_offset", ctypes.c_int32),
        ("move_count", ctypes.c_int32),
        ("max_move_state", ctypes.c_int32),
        ("stay_next_state", ctypes.c_int32),
        ("channel_id", ctypes.c_int32),
    ]


class MoveBranchC(ctypes.Structure):
    _fields_ = [
        ("state", ctypes.c_int32),
        ("prob", ctypes.c_double),
    ]


class MeasStateProbC(ctypes.Structure):
    _fields_ = [
        ("state", ctypes.c_int32),
        ("prob", ctypes.c_double),
    ]


class CondFireRuleC(ctypes.Structure):
    _fields_ = [
        ("state", ctypes.c_int32),
        ("mask", ctypes.c_int32),
        ("fires", ctypes.c_int32),
    ]


_LIB_INSTANCE: ctypes.CDLL | None = None


def _resolve_marginal_dem_kernels_so(force_recompile: bool = False) -> Path:
    return resolve_so(__file__, "marginal_dem_kernels", force_recompile, _find_prebuilt_so_candidate)


def get_marginal_dem_kernels_lib() -> ctypes.CDLL:
    """Locate (or compile if needed) and return the cached ctypes CDLL for marginal_dem_kernels.cpp."""
    global _LIB_INSTANCE
    if _LIB_INSTANCE is not None:
        return _LIB_INSTANCE

    lib = load_cdll(_resolve_marginal_dem_kernels_so, "marginal_trace_source_flows")

    lib.marginal_trace_flows.argtypes = [
        ctypes.c_int32,
        ctypes.c_int32,
        ctypes.c_int32,
        ctypes.POINTER(OpDescC),
        i32_p,
        i32_p,
        i32_p,
        i32_p,
        ctypes.POINTER(MeasStateProbC),
        ctypes.POINTER(Trans1RuleC),
        ctypes.POINTER(Trans2RuleC),
        ctypes.POINTER(MoveBranchC),
        ctypes.POINTER(CondFireRuleC),
        ctypes.c_int32,
        ctypes.POINTER(SourceDescC),
    ]
    lib.marginal_trace_flows.restype = void_p
    lib.marginal_trace_source_flows.argtypes = lib.marginal_trace_flows.argtypes
    lib.marginal_trace_source_flows.restype = void_p

    lib.marginal_trace_result_get_counts.argtypes = [
        void_p,
        i32_p,
        i32_p,
        i32_p,
        i32_p,
        i32_p,
        i32_p,
    ]
    lib.marginal_trace_result_get_counts.restype = None

    lib.marginal_trace_result_copy_data.argtypes = [
        void_p,
        i32_p,
        i32_p,
        i32_p,
        i32_p,
        i32_p,
        f64_p,
        i32_p,
        i32_p,
    ]
    lib.marginal_trace_result_copy_data.restype = None

    lib.marginal_free_trace_result.argtypes = [void_p]
    lib.marginal_free_trace_result.restype = None

    lib.marginal_build_envelopes.argtypes = [
        ctypes.c_int32,
        ctypes.c_int32,
        ctypes.c_int32,
        i32_p,
        i32_p,
        f64_p,
        ctypes.c_int32,
        i32_p,
        i32_p,
        ctypes.c_int32,
        i32_p,
        i32_p,
        f64_p,
    ]
    lib.marginal_build_envelopes.restype = void_p

    lib.marginal_envelope_result_get_counts = None
    lib.marginal_envelope_result_get_total_entries.argtypes = [void_p]
    lib.marginal_envelope_result_get_total_entries.restype = ctypes.c_int32

    lib.marginal_envelope_result_copy_data.argtypes = [
        void_p,
        i32_p,
        i32_p,
        f64_p,
    ]
    lib.marginal_envelope_result_copy_data.restype = None

    lib.marginal_free_envelope_result.argtypes = [void_p]
    lib.marginal_free_envelope_result.restype = None

    lib.marginal_process_shots.argtypes = [
        ctypes.c_int32,
        ctypes.c_int32,
        u8_p,
        i32_p,
        i32_p,
        i32_p,
        i32_p,
        f64_p,
        ctypes.c_int32,
        ctypes.c_int32,
        f64_p,
        ctypes.c_int32,
        i32_p,
        i32_p,
        i32_p,
        f64_p,
    ]
    lib.marginal_process_shots.restype = void_p

    lib.marginal_batch_result_get_counts.argtypes = [void_p, i32_p, i32_p]
    lib.marginal_batch_result_get_counts.restype = None

    lib.marginal_batch_result_copy_data.argtypes = [
        void_p,
        i32_p,
        i32_p,
        i32_p,
        f64_p,
        f64_p,
    ]
    lib.marginal_batch_result_copy_data.restype = None

    lib.marginal_free_batch_result.argtypes = [void_p]
    lib.marginal_free_batch_result.restype = None

    _LIB_INSTANCE = lib
    return lib


def _ptr_i32(arr: NDArray[np.int32]) -> i32_p:
    return arr.ctypes.data_as(i32_p) if arr.size > 0 else ctypes.cast(None, i32_p)


def _ptr_f64(arr: NDArray[np.float64]) -> f64_p:
    return arr.ctypes.data_as(f64_p) if arr.size > 0 else ctypes.cast(None, f64_p)


def _ptr_u8(arr: NDArray[np.uint8]) -> u8_p:
    return arr.ctypes.data_as(u8_p) if arr.size > 0 else ctypes.cast(None, u8_p)


_DECOMPOSE_LIB: ctypes.CDLL | None = None


def get_decompose_dem_lib() -> ctypes.CDLL:
    """The kernel library with the graph-like DEM decomposition entry points bound.

    Raises (OSError, RuntimeError, AttributeError, FileNotFoundError) if the
    library cannot be loaded or lacks the entry points; callers fall back to Python.
    """
    global _DECOMPOSE_LIB
    if _DECOMPOSE_LIB is not None:
        return _DECOMPOSE_LIB
    lib = get_marginal_dem_kernels_lib()
    lib.marginal_dem_text_has_hyperedge.argtypes = [ctypes.c_char_p, ctypes.c_int64]
    lib.marginal_dem_text_has_hyperedge.restype = ctypes.c_int32
    lib.marginal_decompose_dem_text.argtypes = [
        ctypes.c_char_p,
        ctypes.c_int64,
        ctypes.c_char_p,
        ctypes.c_int64,
    ]
    lib.marginal_decompose_dem_text.restype = void_p
    lib.marginal_text_result_size.argtypes = [void_p]
    lib.marginal_text_result_size.restype = ctypes.c_int64
    lib.marginal_text_result_data.argtypes = [void_p]
    lib.marginal_text_result_data.restype = void_p
    lib.marginal_free_text_result.argtypes = [void_p]
    lib.marginal_free_text_result.restype = None
    _DECOMPOSE_LIB = lib
    return lib


def dem_text_has_hyperedge_cpp(dem_text: bytes) -> bool:
    """Whether a flattened DEM's text has a p > 0 error component with > 2 detectors."""
    lib = get_decompose_dem_lib()
    return bool(lib.marginal_dem_text_has_hyperedge(dem_text, len(dem_text)))


def decompose_dem_text_cpp(dem_text: bytes, base_text: bytes | None) -> str:
    """Text of the graph-like decomposition of a flattened DEM's text (see `_decompose_dem_graphlike`)."""
    lib = get_decompose_dem_lib()
    res_ptr = lib.marginal_decompose_dem_text(
        dem_text,
        len(dem_text),
        base_text,
        0 if base_text is None else len(base_text),
    )
    try:
        size = int(lib.marginal_text_result_size(res_ptr))
        data = lib.marginal_text_result_data(res_ptr)
        return ctypes.string_at(data, size).decode("utf-8") if size > 0 else ""
    finally:
        lib.marginal_free_text_result(res_ptr)


def trace_flows_cpp(
    num_ops: int,
    num_qubits: int,
    num_flags: int,
    ops_arr: ctypes.Array[OpDescC],
    target_qubits: NDArray[np.int32],
    touch_offsets: NDArray[np.int32],
    touch_indices: NDArray[np.int32],
    conj_table: NDArray[np.int32],
    meas_probs_arr: ctypes.Array[MeasStateProbC],
    trans1_rules_arr: ctypes.Array[Trans1RuleC],
    trans2_rules_arr: ctypes.Array[Trans2RuleC],
    move_branches_arr: ctypes.Array[MoveBranchC],
    cond_rules_arr: ctypes.Array[CondFireRuleC],
    sources_arr: ctypes.Array[SourceDescC],
    source_mode: bool = False,
) -> tuple[
    int,
    NDArray[np.int32],
    NDArray[np.int32],
    NDArray[np.int32],
    NDArray[np.int32],
    NDArray[np.int32],
    NDArray[np.float64],
    NDArray[np.int32],
    NDArray[np.int32],
]:
    """Trace the leakage flows (``marginal_trace_flows``).

    With ``source_mode=True`` calls ``marginal_trace_source_flows`` instead: the
    returned "flag" arrays are per source, and ``num_flags`` must be the number of sources.
    """
    lib = get_marginal_dem_kernels_lib()
    trace_fn = lib.marginal_trace_source_flows if source_mode else lib.marginal_trace_flows
    res_ptr = trace_fn(
        ctypes.c_int32(num_ops),
        ctypes.c_int32(num_qubits),
        ctypes.c_int32(num_flags),
        ops_arr if len(ops_arr) > 0 else ctypes.cast(None, ctypes.POINTER(OpDescC)),
        _ptr_i32(target_qubits),
        _ptr_i32(touch_offsets),
        _ptr_i32(touch_indices),
        _ptr_i32(conj_table),
        meas_probs_arr
        if len(meas_probs_arr) > 0
        else ctypes.cast(None, ctypes.POINTER(MeasStateProbC)),
        trans1_rules_arr
        if len(trans1_rules_arr) > 0
        else ctypes.cast(None, ctypes.POINTER(Trans1RuleC)),
        trans2_rules_arr
        if len(trans2_rules_arr) > 0
        else ctypes.cast(None, ctypes.POINTER(Trans2RuleC)),
        move_branches_arr
        if len(move_branches_arr) > 0
        else ctypes.cast(None, ctypes.POINTER(MoveBranchC)),
        cond_rules_arr
        if len(cond_rules_arr) > 0
        else ctypes.cast(None, ctypes.POINTER(CondFireRuleC)),
        ctypes.c_int32(len(sources_arr)),
        sources_arr
        if len(sources_arr) > 0
        else ctypes.cast(None, ctypes.POINTER(SourceDescC)),
    )
    try:
        c_err = ctypes.c_int32(-1)
        c_num_sites = ctypes.c_int32(0)
        c_num_used_flows = ctypes.c_int32(0)
        c_total_flow_sites = ctypes.c_int32(0)
        c_total_cands = ctypes.c_int32(0)
        c_total_preds = ctypes.c_int32(0)
        lib.marginal_trace_result_get_counts(
            res_ptr,
            ctypes.byref(c_err),
            ctypes.byref(c_num_sites),
            ctypes.byref(c_num_used_flows),
            ctypes.byref(c_total_flow_sites),
            ctypes.byref(c_total_cands),
            ctypes.byref(c_total_preds),
        )
        err_op = int(c_err.value)
        if err_op >= 0:
            empty_i32 = np.empty(0, dtype=np.int32)
            empty_f64 = np.empty(0, dtype=np.float64)
            return (
                err_op,
                empty_i32,
                empty_i32,
                empty_i32,
                empty_i32,
                empty_i32,
                empty_f64,
                empty_i32,
                empty_i32,
            )
        num_sites = int(c_num_sites.value)
        num_used_flows = int(c_num_used_flows.value)
        total_flow_sites = int(c_total_flow_sites.value)
        total_cands = int(c_total_cands.value)
        total_preds = int(c_total_preds.value)

        sites = np.empty((num_sites, 5), dtype=np.int32)
        flow_site_offsets = np.empty(num_used_flows + 1, dtype=np.int32)
        flow_site_ids = np.empty(total_flow_sites, dtype=np.int32)
        flag_cand_offsets = np.empty(num_flags + 1, dtype=np.int32)
        flag_cand_flows = np.empty(total_cands, dtype=np.int32)
        flag_cand_weights = np.empty(total_cands, dtype=np.float64)
        pred_offsets = np.empty(num_flags + 1, dtype=np.int32)
        pred_flags = np.empty(total_preds, dtype=np.int32)

        lib.marginal_trace_result_copy_data(
            res_ptr,
            _ptr_i32(sites),
            _ptr_i32(flow_site_offsets),
            _ptr_i32(flow_site_ids),
            _ptr_i32(flag_cand_offsets),
            _ptr_i32(flag_cand_flows),
            _ptr_f64(flag_cand_weights),
            _ptr_i32(pred_offsets),
            _ptr_i32(pred_flags),
        )
        return (
            -1,
            sites,
            flow_site_offsets,
            flow_site_ids,
            flag_cand_offsets,
            flag_cand_flows,
            flag_cand_weights,
            pred_offsets,
            pred_flags,
        )
    finally:
        lib.marginal_free_trace_result(res_ptr)


def build_envelopes_cpp(
    num_sites: int,
    num_symptoms: int,
    entry_site_ids: NDArray[np.int32],
    entry_sym_ids: NDArray[np.int32],
    entry_probs: NDArray[np.float64],
    num_used_flows: int,
    flow_site_offsets: NDArray[np.int32],
    flow_site_ids: NDArray[np.int32],
    num_flags: int,
    flag_cand_offsets: NDArray[np.int32],
    flag_cand_flows: NDArray[np.int32],
    flag_cand_weights: NDArray[np.float64],
) -> tuple[NDArray[np.int32], NDArray[np.int32], NDArray[np.float64]]:
    lib = get_marginal_dem_kernels_lib()
    res_ptr = lib.marginal_build_envelopes(
        ctypes.c_int32(num_sites),
        ctypes.c_int32(num_symptoms),
        ctypes.c_int32(entry_site_ids.size),
        _ptr_i32(entry_site_ids),
        _ptr_i32(entry_sym_ids),
        _ptr_f64(entry_probs),
        ctypes.c_int32(num_used_flows),
        _ptr_i32(flow_site_offsets),
        _ptr_i32(flow_site_ids),
        ctypes.c_int32(num_flags),
        _ptr_i32(flag_cand_offsets),
        _ptr_i32(flag_cand_flows),
        _ptr_f64(flag_cand_weights),
    )
    try:
        total_entries = int(lib.marginal_envelope_result_get_total_entries(res_ptr))
        flag_env_offsets = np.empty(num_flags + 1, dtype=np.int32)
        flag_env_sym_ids = np.empty(total_entries, dtype=np.int32)
        flag_env_probs = np.empty(total_entries, dtype=np.float64)
        lib.marginal_envelope_result_copy_data(
            res_ptr,
            _ptr_i32(flag_env_offsets),
            _ptr_i32(flag_env_sym_ids),
            _ptr_f64(flag_env_probs),
        )
        return flag_env_offsets, flag_env_sym_ids, flag_env_probs
    finally:
        lib.marginal_free_envelope_result(res_ptr)


def process_shots_cpp(
    raised: NDArray[np.uint8],
    pred_offsets: NDArray[np.int32],
    pred_flags: NDArray[np.int32],
    flag_env_offsets: NDArray[np.int32],
    flag_env_sym_ids: NDArray[np.int32],
    flag_env_probs: NDArray[np.float64],
    num_symptoms: int,
    mode: int,
    base_sym_probs: NDArray[np.float64] | None = None,
    sym_edge_offsets: NDArray[np.int32] | None = None,
    sym_edge_ids: NDArray[np.int32] | None = None,
    edge_nodes: NDArray[np.int32] | None = None,
    base_edge_probs: NDArray[np.float64] | None = None,
) -> tuple[
    NDArray[np.int32],
    NDArray[np.int32],
    NDArray[np.int32] | None,
    NDArray[np.float64] | None,
    NDArray[np.float64] | None,
]:
    lib = get_marginal_dem_kernels_lib()
    raised_c = np.ascontiguousarray(raised, dtype=np.uint8)
    num_shots, num_flags = raised_c.shape
    if num_flags + 1 != pred_offsets.size or num_flags + 1 != flag_env_offsets.size:
        raise ValueError(
            f"Expected raised with {pred_offsets.size - 1} flag columns, got {num_flags}."
        )
    num_edges = 0 if base_edge_probs is None else int(base_edge_probs.size)

    res_ptr = lib.marginal_process_shots(
        ctypes.c_int32(num_shots),
        ctypes.c_int32(num_flags),
        _ptr_u8(raised_c),
        _ptr_i32(pred_offsets),
        _ptr_i32(pred_flags),
        _ptr_i32(flag_env_offsets),
        _ptr_i32(flag_env_sym_ids),
        _ptr_f64(flag_env_probs),
        ctypes.c_int32(num_symptoms),
        ctypes.c_int32(mode),
        _ptr_f64(base_sym_probs) if base_sym_probs is not None else ctypes.cast(None, f64_p),
        ctypes.c_int32(num_edges),
        _ptr_i32(sym_edge_offsets) if sym_edge_offsets is not None else ctypes.cast(None, i32_p),
        _ptr_i32(sym_edge_ids) if sym_edge_ids is not None else ctypes.cast(None, i32_p),
        _ptr_i32(edge_nodes) if edge_nodes is not None else ctypes.cast(None, i32_p),
        _ptr_f64(base_edge_probs) if base_edge_probs is not None else ctypes.cast(None, f64_p),
    )
    try:
        c_num_groups = ctypes.c_int32(0)
        c_total_items = ctypes.c_int32(0)
        lib.marginal_batch_result_get_counts(
            res_ptr,
            ctypes.byref(c_num_groups),
            ctypes.byref(c_total_items),
        )
        num_groups = int(c_num_groups.value)
        total_items = int(c_total_items.value)
        shot_group_ids = np.empty(num_shots, dtype=np.int32)
        group_item_offsets = np.empty(num_groups + 1, dtype=np.int32)
        if mode == 2:
            group_reweights = np.empty((total_items, 3), dtype=np.float64)
            lib.marginal_batch_result_copy_data(
                res_ptr,
                _ptr_i32(shot_group_ids),
                _ptr_i32(group_item_offsets),
                ctypes.cast(None, i32_p),
                ctypes.cast(None, f64_p),
                _ptr_f64(group_reweights),
            )
            return shot_group_ids, group_item_offsets, None, None, group_reweights
        else:
            group_sym_ids = np.empty(total_items, dtype=np.int32)
            group_probs = np.empty(total_items, dtype=np.float64)
            lib.marginal_batch_result_copy_data(
                res_ptr,
                _ptr_i32(shot_group_ids),
                _ptr_i32(group_item_offsets),
                _ptr_i32(group_sym_ids),
                _ptr_f64(group_probs),
                ctypes.cast(None, f64_p),
            )
            return shot_group_ids, group_item_offsets, group_sym_ids, group_probs, None
    finally:
        lib.marginal_free_batch_result(res_ptr)
