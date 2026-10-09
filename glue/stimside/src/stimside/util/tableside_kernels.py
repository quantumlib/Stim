from __future__ import annotations

import ctypes
import itertools as it
from pathlib import Path
from typing import TYPE_CHECKING, Any

import numpy as np
import stim  # type: ignore[import-untyped]

from stimside.op_handlers.leakage_handlers.leakage_parameters import (
    LeakageConditioningParams,
    LeakageMeasurementParams,
    LeakageParams,
    LeakageSwapParams,
    LeakageTransition1Params,
    LeakageTransition2Params,
)
from stimside.op_handlers.leakage_handlers.tag_registry import (
    parse_leakage_tag,
)
from stimside.util._kernel_loader import (
    find_prebuilt_so as _find_prebuilt_so_candidate,
    load_cdll,
    resolve_so,
)

if TYPE_CHECKING:
    from stimside.simulator_tableau import TablesideSimulator


class StepDescC(ctypes.Structure):
    _fields_ = [
        ("kind", ctypes.c_int32),
        ("target_offset", ctypes.c_int32),
        ("target_count", ctypes.c_int32),
        ("mask_offset", ctypes.c_int32),
        ("p_total_u", ctypes.c_double),
        ("trans_offset", ctypes.c_int32),
        ("trans_count", ctypes.c_int32),
        ("meas_leak_prob", ctypes.c_double),
        ("unrolled_idx", ctypes.c_int32),
    ]


class TransBranchC(ctypes.Structure):
    _fields_ = [
        ("cum_prob", ctypes.c_double),
        ("in0", ctypes.c_int8),
        ("in1", ctypes.c_int8),
        ("out0", ctypes.c_int8),
        ("out1", ctypes.c_int8),
        ("raw_prob", ctypes.c_double),
    ]


class EmittedActionC(ctypes.Structure):
    _fields_ = [
        ("kind", ctypes.c_int32),
        ("step_idx", ctypes.c_int32),
        ("slice_end", ctypes.c_int32),
        ("target_offset", ctypes.c_int32),
        ("target_count", ctypes.c_int32),
        ("prob", ctypes.c_double),
    ]


_LIB_INSTANCE: ctypes.CDLL | None = None


def _resolve_tableside_kernels_so(force_recompile: bool = False) -> Path:
    return resolve_so(__file__, "tableside_kernels", force_recompile, _find_prebuilt_so_candidate)


def get_tableside_kernels_lib() -> ctypes.CDLL:
    """Locate (or compile if needed) and return the cached ctypes CDLL for tableside_kernels.cpp."""
    global _LIB_INSTANCE
    if _LIB_INSTANCE is not None:
        return _LIB_INSTANCE

    lib = load_cdll(_resolve_tableside_kernels_so, "fast_engine_get_segment_events")
    lib.fast_engine_create.argtypes = [
        ctypes.c_int,
        ctypes.c_int,
        ctypes.c_uint64,
        ctypes.POINTER(StepDescC),
        ctypes.c_int,
        ctypes.POINTER(ctypes.c_uint32),
        ctypes.POINTER(ctypes.c_int32),
        ctypes.c_int,
        ctypes.POINTER(ctypes.c_uint64),
        ctypes.c_int,
        ctypes.POINTER(TransBranchC),
    ]
    lib.fast_engine_create.restype = ctypes.c_void_p
    lib.fast_engine_destroy.argtypes = [ctypes.c_void_p]
    lib.fast_engine_destroy.restype = None
    lib.fast_engine_clear.argtypes = [ctypes.c_void_p, ctypes.c_uint64]
    lib.fast_engine_clear.restype = None
    lib.fast_engine_set_record_leakage.argtypes = [ctypes.c_void_p, ctypes.c_int]
    lib.fast_engine_set_record_leakage.restype = None
    lib.fast_engine_get_segment_leaked_ops.argtypes = [
        ctypes.c_void_p,
        ctypes.POINTER(ctypes.POINTER(ctypes.c_int32)),
    ]
    lib.fast_engine_get_segment_leaked_ops.restype = ctypes.c_int
    lib.fast_engine_set_record_events.argtypes = [ctypes.c_void_p, ctypes.c_int]
    lib.fast_engine_set_record_events.restype = None
    lib.fast_engine_get_segment_events.argtypes = [
        ctypes.c_void_p,
        ctypes.POINTER(ctypes.POINTER(ctypes.c_int32)),
    ]
    lib.fast_engine_get_segment_events.restype = ctypes.c_int
    lib.fast_engine_get_state_ptr.argtypes = [ctypes.c_void_p]
    lib.fast_engine_get_state_ptr.restype = ctypes.POINTER(ctypes.c_uint8)
    lib.fast_engine_sync_leaked_mask.argtypes = [ctypes.c_void_p]
    lib.fast_engine_sync_leaked_mask.restype = None
    lib.fast_engine_run_segment.argtypes = [
        ctypes.c_void_p,
        ctypes.c_int,
        ctypes.POINTER(ctypes.POINTER(EmittedActionC)),
        ctypes.POINTER(ctypes.POINTER(ctypes.c_uint32)),
    ]
    lib.fast_engine_run_segment.restype = ctypes.c_int
    lib.fast_format_stim_op.argtypes = [
        ctypes.c_char_p,
        ctypes.c_int,
        ctypes.c_double,
        ctypes.c_void_p,
        ctypes.c_int,
        ctypes.c_char_p,
    ]
    lib.fast_format_stim_op.restype = ctypes.c_int
    _LIB_INSTANCE = lib
    return lib


_STATE_CODE_MAP = {
    "U": -1,
    "V": -2,
    "D": -3,
    "X": -4,
    "Y": -5,
    "Z": -6,
}


def _encode_state(st: int | str) -> int:
    if isinstance(st, int):
        return st
    return _STATE_CODE_MAP[st]


def format_stim_op_text(
    op_name: str,
    targets: np.ndarray,
    gate_args: list[float] | tuple[float, ...],
) -> str:
    if gate_args:
        args_str = "(" + ",".join(str(a) for a in gate_args) + ")"
    else:
        args_str = ""
    if targets.dtype == np.uint32 and np.any(targets & (1 << 31)):
        t_strs = [
            f"!{int(t & 0x7FFFFFFF)}" if (t & (1 << 31)) else str(int(t))
            for t in targets
        ]
    else:
        t_strs = [str(int(t)) for t in targets]
    return f"{op_name}{args_str} {' '.join(t_strs)}\n"


_EMPTY_U32 = np.empty(0, dtype=np.uint32)
_EMPTY_I32 = np.empty(0, dtype=np.int32)


def _circuit_of(insts: list[stim.CircuitInstruction]) -> stim.Circuit:
    """Circuit built by appending the instructions one at a time (as stim.Circuit.append would fuse them)."""
    c = stim.Circuit()
    for inst in insts:
        c.append(inst)
    return c


def _slice_circuit(
    insts: list[stim.CircuitInstruction],
    cache: dict[tuple[int, int], stim.Circuit],
    blocks: list[stim.Circuit],
    a: int,
    b: int,
) -> stim.Circuit:
    """Cached circuit of insts[a:b]; spans of >= 64 reuse lazily built 32-instruction blocks."""
    if a >= b:
        return stim.Circuit()
    sub = cache.get((a, b))
    if sub is None:
        if b - a >= 64:
            if not blocks:
                for i in range(0, len(insts), 32):
                    blocks.append(_circuit_of(insts[i : i + 32]))
            b0, b1 = (a + 31) >> 5, b >> 5
            sub = _circuit_of(insts[a : b0 << 5])
            for k in range(b0, b1):
                sub += blocks[k]
            for inst in insts[b1 << 5 : b]:
                sub.append(inst)
        else:
            sub = _circuit_of(insts[a:b])
        cache[(a, b)] = sub
    return sub


class PrecompiledTablesideCircuit:
    """Pre-compiled circuit structure for TablesideSimulator Tier 1 (Python) and Tier 2 (C++)."""

    def __init__(
        self,
        circuit: stim.Circuit,
        unconditional_condition_on_U: bool = True,
    ) -> None:
        self.circuit = circuit
        self.num_qubits = circuit.num_qubits
        self.num_words = max(1, (self.num_qubits + 63) // 64)
        self.unconditional_condition_on_U = unconditional_condition_on_U
        self._m2d_converter: Any = None
        self._reference_circuit: stim.Circuit | None = None
        self._full_bare_circuit: stim.Circuit | None = None
        self._quantum_bare_circuit: stim.Circuit | None = None

        from stimside.util.known_states import _unroll_circuit

        has_repeat = any(isinstance(inst, stim.CircuitRepeatBlock) for inst in circuit)
        # Not circuit.flattened(): it fuses identical consecutive instructions, desyncing steps from step_to_unrolled_idx.
        unrolled_ops = _unroll_circuit(circuit) if has_repeat else list(circuit)
        self.parsed_ops: dict[stim.CircuitInstruction, LeakageParams] = {}

        self.final_qubit_coords: dict[int, list[float]] = {}
        self.final_qubit_tags: dict[int, str] = {}
        self.final_coords_shifts: list[float] = []
        self.ref_insts: list[stim.CircuitInstruction] = []

        self.steps_py: list[tuple[Any, ...]] = []
        self.orig_ops: list[stim.CircuitInstruction] = []
        self.full_bare_insts: list[stim.CircuitInstruction] = []
        self.quantum_bare_insts: list[stim.CircuitInstruction] = []
        self.step_to_full_idx: list[int] = []
        self.step_to_quant_idx: list[int] = []
        self.step_has_bare_op: list[bool] = []
        self.requires_tableau = False

        c_steps: list[StepDescC] = []
        raw_t_chunks: list[np.ndarray] = []
        qubit_t_chunks: list[np.ndarray] = []
        t_running_len = 0
        all_step_masks: list[int] = [0] * self.num_words
        all_branches: list[TransBranchC] = []
        op_template_cache: dict[
            stim.CircuitInstruction,
            tuple[
                stim.CircuitInstruction | None,
                stim.CircuitInstruction | None,
                StepDescC,
                tuple[Any, ...],
            ],
        ] = {}

        def push(bare, ref, c_step: StepDescC, s_py: tuple[Any, ...]) -> None:
            """Append one step (bare/ref instruction may be None) to the per-step lists."""
            if bare is not None:
                self.full_bare_insts.append(bare)
                self.quantum_bare_insts.append(bare)
            if ref is not None:
                self.ref_insts.append(ref)
            self.step_has_bare_op.append(bare is not None)
            c_steps.append(c_step)
            self.steps_py.append(s_py)

        def emit(bare, ref, kind, text="", p_tot=0.0, br=(0, 0), p_leak=0.0) -> None:
            """Build, cache and push the current op's step (reads this iteration's locals)."""
            c_step = StepDescC(kind, t_off, t_cnt, m_off, p_tot, *br, p_leak)
            s_py = (kind, op_name, gate_args, text, raw_u32, qubit_i32, params, mask_words)
            op_template_cache[op] = (bare, ref, c_step, s_py)
            push(bare, ref, c_step, s_py)

        for op in unrolled_ops:
            op_name = op.name
            if op_name in ("DETECTOR", "TICK", "OBSERVABLE_INCLUDE"):
                self.full_bare_insts.append(op)
                self.ref_insts.append(op)
                continue
            if op_name == "SHIFT_COORDS":
                shifts = op.gate_args_copy()
                for i, s_val in enumerate(shifts):
                    if i >= len(self.final_coords_shifts):
                        self.final_coords_shifts.append(s_val)
                    else:
                        self.final_coords_shifts[i] += s_val
                self.full_bare_insts.append(op)
                self.ref_insts.append(op)
                continue
            if op_name == "QUBIT_COORDS":
                [gt] = op.targets_copy()
                q_idx = gt.qubit_value
                self.final_qubit_coords[q_idx] = [
                    c + s
                    for c, s in zip(
                        op.gate_args_copy(),
                        it.chain(self.final_coords_shifts, it.repeat(0)),
                    )
                ]
                self.final_qubit_tags[q_idx] = op.tag
                self.full_bare_insts.append(op)
                self.ref_insts.append(op)
                continue

            self.step_to_full_idx.append(len(self.full_bare_insts))
            self.step_to_quant_idx.append(len(self.quantum_bare_insts))
            self.orig_ops.append(op)

            tpl = op_template_cache.get(op)
            if tpl is not None:
                push(*tpl)
                continue

            params: LeakageParams | None = None
            if op.tag != "":
                params = parse_leakage_tag(op, simulator="tableau")
                if params is not None:
                    self.parsed_ops[op] = params
            is_claimed = params is not None

            if not is_claimed:
                gate_data = stim.gate_data(op_name)
                if (
                    not self.unconditional_condition_on_U
                    or gate_data.produces_measurements
                    or not (gate_data.is_noisy_gate or gate_data.is_unitary)
                ):
                    c_step_inst = StepDescC(0, 0, 0, 0, 0.0, 0, 0, 0.0)
                    s_py: tuple[Any, ...] = (0, op_name, [], "", _EMPTY_U32, _EMPTY_I32, None, ())
                    op_template_cache[op] = (op, op, c_step_inst, s_py)
                    push(op, op, c_step_inst, s_py)
                    continue

            raw_t = op.targets_copy()
            gate_args = op.gate_args_copy()
            has_special_targets = len(gate_args) > 1 or any(
                not (
                    t.is_qubit_target
                    or (t.is_inverted_result_target and t.qubit_value is not None)
                )
                for t in raw_t
            )
            if not has_special_targets:
                raw_u32 = np.array(
                    [
                        t.qubit_value | ((1 << 31) if t.is_inverted_result_target else 0)
                        for t in raw_t
                    ],
                    dtype=np.uint32,
                )
                qubit_i32 = (raw_u32 & 0x7FFFFFFF).astype(np.int32)
            else:
                raw_u32 = np.array(
                    [
                        (
                            (t.qubit_value if t.qubit_value is not None else max(0, t.value))
                            | ((1 << 31) if t.is_inverted_result_target else 0)
                        )
                        for t in raw_t
                    ],
                    dtype=np.uint32,
                )
                qubit_vals = [t.qubit_value for t in raw_t]
                qubit_i32 = np.array(
                    [
                        q if q is not None else t.value
                        for q, t in zip(qubit_vals, raw_t)
                    ],
                    dtype=np.int32,
                )
            bare_inst = (
                op
                if not is_claimed
                else stim.CircuitInstruction(op_name, raw_t, gate_args)
            )

            has_dup_targets = len(set(qubit_i32.tolist())) < len(qubit_i32)
            if isinstance(params, LeakageTransition1Params) and not has_dup_targets:
                unique_qs = np.unique(qubit_i32[qubit_i32 >= 0])
                qubit_i32 = unique_qs.astype(np.int32)
                raw_u32 = unique_qs.astype(np.uint32)

            mask_words = [0] * self.num_words
            # With special targets, qubit_i32 holds .value for a combiner (0) or sweep[k] (k), not a qubit.
            mask_source = (
                params.targets
                if (isinstance(params, LeakageConditioningParams) and params.targets)
                else qubit_i32
                if not has_special_targets
                else [q for q in qubit_vals if q is not None]
            )
            for q in mask_source:
                q_int = int(q)
                if 0 <= q_int < self.num_qubits:
                    mask_words[q_int >> 6] |= 1 << (q_int & 63)

            t_off = t_running_len
            t_cnt = len(raw_u32)
            t_running_len += t_cnt
            m_off = len(all_step_masks)
            if t_cnt > 0:
                raw_t_chunks.append(raw_u32)
                qubit_t_chunks.append(qubit_i32)
            all_step_masks.extend(mask_words)

            if not is_claimed:
                if has_special_targets or gate_data.produces_measurements:
                    kind = 8
                elif gate_data.is_single_qubit_gate:
                    kind = 1
                elif gate_data.is_two_qubit_gate:
                    kind = 2
                else:
                    kind = 8
                emit(bare_inst, bare_inst, kind)
            else:
                if isinstance(params, LeakageConditioningParams):
                    cond_groups = params.args
                    tag_parts = getattr(params, "from_tag", "").split(":")
                    tag_has_01 = len(tag_parts) > 1 and any(
                        tok in ("0", "1") for tok in tag_parts[1].split()
                    )
                    if any(0 in g or 1 in g for g in cond_groups) or tag_has_01:
                        self.requires_tableau = True
                        kind = 6
                    elif (
                        not params.targets
                        and not has_special_targets
                        and len(cond_groups) == 1
                        and cond_groups[0] == ("U",)
                    ):
                        kind = 2 if stim.gate_data(op_name).is_two_qubit_gate else 1
                    elif (
                        not params.targets
                        and not has_special_targets
                        and len(cond_groups) == 2
                        and cond_groups == (("U",), ("U",))
                    ):
                        kind = 2
                    elif all(g == ("U",) for g in cond_groups):
                        kind = 8
                    else:
                        kind = 6
                    emit(bare_inst, bare_inst, kind)

                elif isinstance(params, LeakageTransition1Params):
                    has_01 = (0 in params.args_by_input_state) or (
                        1 in params.args_by_input_state
                    )
                    if has_01 or has_dup_targets:
                        self.requires_tableau = True
                        p1_tot = sum(
                            p for _, p in params.args_by_input_state.get(1, ())
                        )
                        emit(None, None, 6, p_tot=p1_tot)
                    else:
                        br_off = len(all_branches)
                        u_args = params.args_by_input_state.get("U", ())
                        p_tot_u = sum(p for _, p in u_args)
                        cum = 0.0
                        for out_st, p in u_args:
                            cum += p / p_tot_u if p_tot_u > 0 else 0.0
                            all_branches.append(
                                TransBranchC(
                                    cum, -1, -1, _encode_state(out_st), 0, p
                                )
                            )
                        for in_st, br_list in params.args_by_input_state.items():
                            if in_st != "U":
                                for out_st, p in br_list:
                                    all_branches.append(
                                        TransBranchC(
                                            0.0,
                                            int(in_st),
                                            -1,
                                            _encode_state(out_st),
                                            0,
                                            p,
                                        )
                                    )
                        br = (br_off, len(all_branches) - br_off)
                        emit(None, None, 3, p_tot=p_tot_u, br=br)

                elif isinstance(params, LeakageTransition2Params):
                    has_01 = any(
                        0 in k or 1 in k for k in params.args_by_input_state.keys()
                    )
                    has_overlapping_pairs = len(set(qubit_i32.tolist())) < len(
                        qubit_i32
                    )
                    if has_01:
                        self.requires_tableau = True
                        emit(None, None, 6)
                    elif has_overlapping_pairs:
                        emit(None, None, 6)
                    else:
                        br_off = len(all_branches)
                        uu_args = params.args_by_input_state.get(("U", "U"), ())
                        p_tot_u = sum(p for _, p in uu_args)
                        cum = 0.0
                        for (out0, out1), p in uu_args:
                            cum += p / p_tot_u if p_tot_u > 0 else 0.0
                            all_branches.append(
                                TransBranchC(
                                    cum,
                                    -1,
                                    -1,
                                    _encode_state(out0),
                                    _encode_state(out1),
                                    p,
                                )
                            )
                        for in_pair, br_list in params.args_by_input_state.items():
                            if in_pair != ("U", "U"):
                                k0 = _encode_state(in_pair[0])
                                k1 = _encode_state(in_pair[1])
                                for (out0, out1), p in br_list:
                                    all_branches.append(
                                        TransBranchC(
                                            0.0,
                                            k0,
                                            k1,
                                            _encode_state(out0),
                                            _encode_state(out1),
                                            p,
                                        )
                                    )
                        br = (br_off, len(all_branches) - br_off)
                        emit(None, None, 4, p_tot=p_tot_u, br=br)

                elif isinstance(params, LeakageMeasurementParams):
                    is_mpad = params.targets is not None
                    p0 = params.prob_for_input_state.get(0, 0.0)
                    p1 = params.prob_for_input_state.get(
                        1, 0.0 if is_mpad else 1.0
                    )
                    p2 = params.prob_for_input_state.get(2, 0.0)
                    meas_inst = stim.CircuitInstruction(
                        "M" if is_mpad else op_name, raw_t, []
                    )
                    ref_inst = op if is_mpad else meas_inst
                    extra_states = set(params.prob_for_input_state.keys()) - {
                        0,
                        1,
                        2,
                    }
                    has_01_cond = (0 in params.prob_for_input_state) or (
                        1 in params.prob_for_input_state
                    )
                    if (is_mpad and (has_01_cond or not np.isclose(p0, p1))) or (
                        not is_mpad and not np.isclose(p0, 1.0 - p1)
                    ):
                        self.requires_tableau = True
                    if (
                        not is_mpad
                        and not has_dup_targets
                        and op_name in ("M", "MZ")
                        and p0 == 0.0
                        and p1 == 1.0
                        and not extra_states
                    ):
                        kind = 5
                        bare_op_text = format_stim_op_text("M", raw_u32, [])
                    else:
                        kind = 6
                        bare_op_text = ""
                    emit(meas_inst, ref_inst, kind, text=bare_op_text, p_leak=p2)

                elif isinstance(params, LeakageSwapParams):
                    # The runners yield to the handler (bare SWAP + leakage state swap) when a target
                    # may be leaked; otherwise the bare SWAP runs in the slice (state swap is a no-op).
                    emit(bare_inst, bare_inst, 8)

        self.num_claimed_ops = len(self.parsed_ops)
        self.step_to_full_idx.append(len(self.full_bare_insts))
        self.step_to_quant_idx.append(len(self.quantum_bare_insts))
        self.num_steps = len(self.steps_py)

        # Fuse adjacent step s (kind==6, LEAKAGE_TRANSITION_1 with 1-->2) + step s+1 (kind==5, M[LEAKAGE_PROJECTION_Z: (1.0, 2)])
        for s in range(self.num_steps - 1):
            if c_steps[s].kind == 6 and c_steps[s + 1].kind == 5:
                p_s = self.steps_py[s][6]
                p_m = self.steps_py[s + 1][6]
                if (
                    isinstance(p_s, LeakageTransition1Params)
                    and isinstance(p_m, LeakageMeasurementParams)
                    and set(p_s.args_by_input_state.keys()) == {1}
                    and len(p_s.args_by_input_state[1]) == 1
                    and isinstance(p_s.args_by_input_state[1][0][0], int)
                    and p_s.args_by_input_state[1][0][0] == 2
                    and p_m.prob_for_input_state.get(2, 0.0) == 1.0
                    and len(set(self.steps_py[s][5].tolist()))
                    == len(self.steps_py[s][5])
                    and np.array_equal(
                        np.sort(self.steps_py[s][5]),
                        np.sort(self.steps_py[s + 1][5]),
                    )
                ):
                    # Copy first: step descriptors are shared between identical ops via the template cache.
                    c_steps[s] = StepDescC.from_buffer_copy(c_steps[s])
                    c_steps[s].kind = 7
                    self.steps_py[s] = (7, *self.steps_py[s][1:])

        self._full_slice_cache: dict[tuple[int, int], stim.Circuit] = {}
        self._quant_slice_cache: dict[tuple[int, int], stim.Circuit] = {}
        self._full_blocks: list[stim.Circuit] = []
        self._quant_blocks: list[stim.Circuit] = []

        self.unrolled_num_ops = len(unrolled_ops)
        self.step_to_unrolled_idx = [
            idx
            for idx, u_op in enumerate(unrolled_ops)
            if u_op.name
            not in (
                "DETECTOR",
                "TICK",
                "OBSERVABLE_INCLUDE",
                "SHIFT_COORDS",
                "QUBIT_COORDS",
            )
        ]

        self._c_steps = (StepDescC * len(c_steps))(*c_steps) if c_steps else None
        if self._c_steps is not None:
            for s_i, u_idx in enumerate(self.step_to_unrolled_idx):
                self._c_steps[s_i].unrolled_idx = int(u_idx)
        self._raw_t_np = (
            np.concatenate(raw_t_chunks)
            if raw_t_chunks
            else np.empty(0, dtype=np.uint32)
        )
        self._qubit_t_np = (
            np.concatenate(qubit_t_chunks)
            if qubit_t_chunks
            else np.empty(0, dtype=np.int32)
        )
        self._masks_np = np.asarray(all_step_masks, dtype=np.uint64)
        self._c_raw_t = (
            self._raw_t_np.ctypes.data_as(ctypes.POINTER(ctypes.c_uint32))
            if len(self._raw_t_np) > 0
            else None
        )
        self._c_qubit_t = (
            self._qubit_t_np.ctypes.data_as(ctypes.POINTER(ctypes.c_int32))
            if len(self._qubit_t_np) > 0
            else None
        )
        self._c_masks = (
            self._masks_np.ctypes.data_as(ctypes.POINTER(ctypes.c_uint64))
            if len(self._masks_np) > 0
            else None
        )
        self._c_branches = (
            (TransBranchC * len(all_branches))(*all_branches)
            if all_branches
            else None
        )
        self._runner_pool: list[CppTablesideRunner] = []

    @property
    def m2d_converter(self) -> Any:
        if self._m2d_converter is None:
            self._m2d_converter = self.circuit.compile_m2d_converter()
        return self._m2d_converter

    @property
    def reference_circuit(self) -> stim.Circuit:
        if self._reference_circuit is None:
            self._reference_circuit = _circuit_of(self.ref_insts)
        return self._reference_circuit

    @property
    def full_bare_circuit(self) -> stim.Circuit:
        if self._full_bare_circuit is None:
            self._full_bare_circuit = _circuit_of(self.full_bare_insts)
        return self._full_bare_circuit

    @property
    def quantum_bare_circuit(self) -> stim.Circuit:
        if self._quantum_bare_circuit is None:
            self._quantum_bare_circuit = _circuit_of(self.quantum_bare_insts)
        return self._quantum_bare_circuit

    def acquire_cpp_runner(self, seed: int) -> "CppTablesideRunner":
        if self._runner_pool:
            runner = self._runner_pool.pop()
            runner.clear(seed)
            return runner
        return CppTablesideRunner(self, seed)

    def release_cpp_runner(self, runner: "CppTablesideRunner") -> None:
        if len(self._runner_pool) < 8:
            self._runner_pool.append(runner)

    def get_full_slice(self, f0: int, f1: int) -> stim.Circuit:
        if f0 == 0 < f1 == len(self.full_bare_insts):
            return self.full_bare_circuit
        return _slice_circuit(self.full_bare_insts, self._full_slice_cache, self._full_blocks, f0, f1)

    def get_quant_slice(self, q0: int, q1: int) -> stim.Circuit:
        if q0 == 0 < q1 == len(self.quantum_bare_insts):
            return self.quantum_bare_circuit
        return _slice_circuit(self.quantum_bare_insts, self._quant_slice_cache, self._quant_blocks, q0, q1)


_PRECOMPILED_CACHE: dict[
    tuple[int, int, bool], tuple[stim.Circuit, PrecompiledTablesideCircuit]
] = {}


def get_precompiled_circuit(
    circuit: stim.Circuit, unconditional_condition_on_U: bool
) -> PrecompiledTablesideCircuit:
    key = (id(circuit), len(circuit), bool(unconditional_condition_on_U))
    entry = _PRECOMPILED_CACHE.get(key)
    if entry is not None and entry[0] is circuit and len(circuit) == key[1]:
        return entry[1]
    pre = PrecompiledTablesideCircuit(circuit, unconditional_condition_on_U)
    if len(_PRECOMPILED_CACHE) >= 16:
        _PRECOMPILED_CACHE.pop(next(iter(_PRECOMPILED_CACHE)))
    _PRECOMPILED_CACHE[key] = (circuit, pre)
    return pre


def _apply_final_metadata(
    tss: "TablesideSimulator", pre: PrecompiledTablesideCircuit
) -> None:
    tss.qubit_coords = pre.final_qubit_coords
    tss.qubit_tags = pre.final_qubit_tags
    tss.coords_shifts = pre.final_coords_shifts
    if tss._construct_reference_circuit:
        tss._new_reference_circuit = pre.reference_circuit.copy()
    tss._circuit_time = pre.unrolled_num_ops


def run_tableside_sync_rng(
    tss: "TablesideSimulator",
    pre: PrecompiledTablesideCircuit,
) -> None:
    """Tier 1 Python execution preserving 100% exact RNG lockstep with CosetsideSimulator(sync_tableside_rng=True)."""
    coh = tss._compiled_op_handler
    state = coh.state
    steps = pre.steps_py
    step_to_full = pre.step_to_full_idx
    step_to_quant = pre.step_to_quant_idx
    quant_insts = pre.quantum_bare_insts
    num_steps = pre.num_steps
    use_tableau = tss._running_tableau
    sim_do = tss._tableau_simulator.do

    f_cursor = 0
    q_cursor = 0
    for s in range(num_steps):
        kind, _, _, _, _, qubit_i32, params, _ = steps[s]
        if kind == 0:
            continue
        elif kind in (1, 2, 5, 8):
            is_other = isinstance(params, LeakageConditioningParams) and params.targets
            check_qs = (
                np.asarray(params.targets, dtype=np.int32)
                if is_other
                else qubit_i32[qubit_i32 >= 0]
            )
            # A stim-fused CONDITIONED_ON_OTHER op (more targets than the tag lists) goes to the
            # handler, which applies its copies one by one, drawing RNG like CosetsideSimulator.
            if not np.any(state[check_qs] >= 2) and not (
                is_other and len(qubit_i32) > len(params.targets)
            ):
                continue

        f_s = step_to_full[s]
        if f_s > f_cursor:
            tss._new_circuit += pre.get_full_slice(f_cursor, f_s)
        q_s = step_to_quant[s]
        if use_tableau and q_s > q_cursor:
            for inst in quant_insts[q_cursor:q_s]:
                sim_do(inst)
        tss._circuit_time = pre.step_to_unrolled_idx[s]
        coh.handle_op(op=pre.orig_ops[s], sss=tss)
        if not use_tableau and tss._running_tableau:
            use_tableau = True
            sim_do = tss._tableau_simulator.do
        consumes = 1 if pre.step_has_bare_op[s] else 0
        f_cursor = f_s + consumes
        q_cursor = q_s + consumes

    if step_to_full[-1] > f_cursor:
        tss._new_circuit += pre.get_full_slice(f_cursor, step_to_full[-1])
    if use_tableau and step_to_quant[-1] > q_cursor:
        for inst in quant_insts[q_cursor : step_to_quant[-1]]:
            sim_do(inst)
    _apply_final_metadata(tss, pre)


def run_tableside_python_v2(
    tss: "TablesideSimulator",
    pre: PrecompiledTablesideCircuit,
) -> None:
    """Tier 1 Pure-Python V2 execution with binary slice splicing, sparse binomial sampling, and fused Mode B thinning."""
    coh = tss._compiled_op_handler
    state = coh.state
    rng = tss.np_rng
    steps = pre.steps_py
    num_steps = pre.num_steps
    step_to_full = pre.step_to_full_idx
    step_to_quant = pre.step_to_quant_idx
    use_tableau = tss._running_tableau
    rec_ev = bool(getattr(tss, "record_leakage_events", False))

    num_leaked = int(np.count_nonzero(state >= 2))
    f_cursor = 0
    q_cursor = 0
    s = 0
    while s < num_steps:
        kind, op_name, gate_args, bare_op_text, raw_u32, qubit_i32, params, _ = (
            steps[s]
        )
        if kind == 0:
            s += 1
            continue
        elif kind == 1:
            if num_leaked > 0 and np.any(state[qubit_i32] >= 2):
                f_s = step_to_full[s]
                if f_s > f_cursor:
                    tss._new_circuit += pre.get_full_slice(f_cursor, f_s)
                q_s = step_to_quant[s]
                if use_tableau and q_s > q_cursor:
                    tss._tableau_simulator.do_circuit(
                        pre.get_quant_slice(q_cursor, q_s)
                    )
                keep = state[qubit_i32] < 2
                txt = (
                    format_stim_op_text(op_name, raw_u32[keep], gate_args)
                    if np.any(keep)
                    else ""
                )
                if txt:
                    tss._new_circuit.append_from_stim_program_text(txt)
                    if use_tableau:
                        tss._tableau_simulator.do_circuit(stim.Circuit(txt))
                f_cursor = f_s + 1
                q_cursor = q_s + 1
            s += 1
        elif kind == 2:
            if num_leaked > 0 and np.any(state[qubit_i32] >= 2):
                f_s = step_to_full[s]
                if f_s > f_cursor:
                    tss._new_circuit += pre.get_full_slice(f_cursor, f_s)
                q_s = step_to_quant[s]
                if use_tableau and q_s > q_cursor:
                    tss._tableau_simulator.do_circuit(
                        pre.get_quant_slice(q_cursor, q_s)
                    )
                q_pairs = qubit_i32.reshape(-1, 2)
                keep_pair = (state[q_pairs[:, 0]] < 2) & (
                    state[q_pairs[:, 1]] < 2
                )
                txt = (
                    format_stim_op_text(
                        op_name,
                        raw_u32.reshape(-1, 2)[keep_pair].ravel(),
                        gate_args,
                    )
                    if np.any(keep_pair)
                    else ""
                )
                if txt:
                    tss._new_circuit.append_from_stim_program_text(txt)
                    if use_tableau:
                        tss._tableau_simulator.do_circuit(stim.Circuit(txt))
                f_cursor = f_s + 1
                q_cursor = q_s + 1
            s += 1
        elif kind == 3:
            u_args = params.args_by_input_state.get("U", ())
            p_tot_u = sum(p for _, p in u_args)
            has_leaked_here = num_leaked > 0 and np.any(state[qubit_i32] >= 2)
            if not has_leaked_here and len(u_args) >= 1:
                n_qs = len(qubit_i32)
                if n_qs > 0 and p_tot_u > 0.0:
                    n_hits = (
                        n_qs
                        if p_tot_u >= 1.0 - 1e-12
                        else int(rng.binomial(n_qs, p_tot_u))
                    )
                    if n_hits > 0:
                        f_s = step_to_full[s]
                        if f_s > f_cursor:
                            tss._new_circuit += pre.get_full_slice(f_cursor, f_s)
                        q_s = step_to_quant[s]
                        if use_tableau and q_s > q_cursor:
                            tss._tableau_simulator.do_circuit(
                                pre.get_quant_slice(q_cursor, q_s)
                            )
                        f_cursor = f_s
                        q_cursor = q_s
                        hit_qs = (
                            qubit_i32
                            if n_hits == n_qs
                            else qubit_i32[rng.choice(n_qs, size=n_hits, replace=False)]
                        )
                        if len(u_args) == 1:
                            out_states = [u_args[0][0]] * n_hits
                        else:
                            probs = [p / p_tot_u for _, p in u_args]
                            br_indices = rng.choice(len(u_args), size=n_hits, p=probs)
                            out_states = [u_args[int(b)][0] for b in br_indices]
                        r_qs: list[int] = []
                        one_qs: list[int] = []
                        for q_val, out_st in zip(hit_qs, out_states):
                            q_int = int(q_val)
                            old_st = int(state[q_int]) if rec_ev else 0
                            if out_st == "U":
                                state[q_int] = 0
                            elif out_st == 0 or out_st == "0":
                                state[q_int] = 0
                                r_qs.append(q_int)
                            elif out_st == 1 or out_st == "1":
                                state[q_int] = 0
                                one_qs.append(q_int)
                            else:
                                state[q_int] = int(out_st)
                            if rec_ev:
                                tss._record_leakage_event(
                                    pre.step_to_unrolled_idx[s], q_int, old_st, state[q_int]
                                )
                        if tss.record_unleaked_to_leaked:
                            k_leaked = int(np.count_nonzero(state[hit_qs] >= 2))
                            if k_leaked > 0:
                                tss._record_unleaked_to_leaked_count(
                                    k_leaked, pre.step_to_unrolled_idx[s]
                                )
                        num_leaked = int(np.count_nonzero(state >= 2))
                        cmds_1q: list[str] = []
                        if r_qs:
                            cmds_1q.append(f"R {' '.join(str(q) for q in r_qs)}\n")
                        if one_qs:
                            t_one = " ".join(str(q) for q in one_qs)
                            cmds_1q.append(f"R {t_one}\nX {t_one}\n")
                        if cmds_1q:
                            cmd_1q = "".join(cmds_1q)
                            tss._new_circuit.append_from_stim_program_text(cmd_1q)
                            if use_tableau:
                                tss._tableau_simulator.do_circuit(stim.Circuit(cmd_1q))
            elif p_tot_u > 0.0 or has_leaked_here:
                f_s = step_to_full[s]
                if f_s > f_cursor:
                    tss._new_circuit += pre.get_full_slice(f_cursor, f_s)
                q_s = step_to_quant[s]
                if use_tableau and q_s > q_cursor:
                    tss._tableau_simulator.do_circuit(
                        pre.get_quant_slice(q_cursor, q_s)
                    )
                tss._circuit_time = pre.step_to_unrolled_idx[s]
                coh.handle_op(op=pre.orig_ops[s], sss=tss)
                if not use_tableau and tss._running_tableau:
                    use_tableau = True
                f_cursor = f_s
                q_cursor = q_s
                num_leaked = int(np.count_nonzero(state >= 2))
            s += 1
        elif kind == 4:
            uu_args = params.args_by_input_state.get(("U", "U"), ())
            p_uu = sum(p for _, p in uu_args)
            has_leaked_here = num_leaked > 0 and np.any(state[qubit_i32] >= 2)
            if (
                not has_leaked_here
                and len(uu_args) >= 1
                and all("V" not in pair_out for pair_out, _ in uu_args)
            ):
                q_pairs = qubit_i32.reshape(-1, 2)
                n_pairs = len(q_pairs)
                if n_pairs > 0 and p_uu > 0.0:
                    n_hits = (
                        n_pairs
                        if p_uu >= 1.0 - 1e-12
                        else int(rng.binomial(n_pairs, p_uu))
                    )
                    if n_hits > 0:
                        f_s = step_to_full[s]
                        if f_s > f_cursor:
                            tss._new_circuit += pre.get_full_slice(f_cursor, f_s)
                        q_s = step_to_quant[s]
                        if use_tableau and q_s > q_cursor:
                            tss._tableau_simulator.do_circuit(
                                pre.get_quant_slice(q_cursor, q_s)
                            )
                        f_cursor = f_s
                        q_cursor = q_s
                        hit_pairs = (
                            q_pairs
                            if n_hits == n_pairs
                            else q_pairs[
                                rng.choice(n_pairs, size=n_hits, replace=False)
                            ]
                        )
                        if len(uu_args) == 1:
                            chosen_outs = [uu_args[0][0]] * n_hits
                        else:
                            probs = [p / p_uu for _, p in uu_args]
                            br_indices = rng.choice(len(uu_args), size=n_hits, p=probs)
                            chosen_outs = [uu_args[int(b)][0] for b in br_indices]
                        r_2q: list[int] = []
                        one_2q: list[int] = []
                        depol_2q: list[int] = []
                        x_2q: list[int] = []
                        y_2q: list[int] = []
                        z_2q: list[int] = []
                        for k_hit in range(n_hits):
                            out_pair = chosen_outs[k_hit]
                            for leg in (0, 1):
                                out_leg = out_pair[leg]
                                q_leg = int(hit_pairs[k_hit, leg])
                                old_leg = int(state[q_leg]) if rec_ev else 0
                                if isinstance(out_leg, int) and out_leg >= 2:
                                    state[q_leg] = out_leg
                                elif out_leg == "D":
                                    state[q_leg] = 0
                                    depol_2q.append(q_leg)
                                elif out_leg == 0 or out_leg == "0":
                                    state[q_leg] = 0
                                    r_2q.append(q_leg)
                                elif out_leg == 1 or out_leg == "1":
                                    state[q_leg] = 0
                                    one_2q.append(q_leg)
                                elif out_leg == "X":
                                    state[q_leg] = 0
                                    x_2q.append(q_leg)
                                elif out_leg == "Y":
                                    state[q_leg] = 0
                                    y_2q.append(q_leg)
                                elif out_leg == "Z":
                                    state[q_leg] = 0
                                    z_2q.append(q_leg)
                                else:
                                    state[q_leg] = 0
                                if rec_ev:
                                    tss._record_leakage_event(
                                        pre.step_to_unrolled_idx[s], q_leg, old_leg, state[q_leg]
                                    )
                        if tss.record_unleaked_to_leaked:
                            k_leaked = int(np.count_nonzero(state[qubit_i32] >= 2))
                            if k_leaked > 0:
                                tss._record_unleaked_to_leaked_count(
                                    k_leaked, pre.step_to_unrolled_idx[s]
                                )
                        num_leaked = int(np.count_nonzero(state >= 2))
                        cmds: list[str] = []
                        if r_2q:
                            cmds.append(f"R {' '.join(str(q) for q in r_2q)}\n")
                        if one_2q:
                            t_one = " ".join(str(q) for q in one_2q)
                            cmds.append(f"R {t_one}\nX {t_one}\n")
                        if depol_2q:
                            cmds.append(f"DEPOLARIZE1(0.75) {' '.join(str(q) for q in depol_2q)}\n")
                        if x_2q:
                            cmds.append(f"X {' '.join(str(q) for q in x_2q)}\n")
                        if y_2q:
                            cmds.append(f"Y {' '.join(str(q) for q in y_2q)}\n")
                        if z_2q:
                            cmds.append(f"Z {' '.join(str(q) for q in z_2q)}\n")
                        if cmds:
                            cmd = "".join(cmds)
                            tss._new_circuit.append_from_stim_program_text(cmd)
                            if use_tableau:
                                tss._tableau_simulator.do_circuit(
                                    stim.Circuit(cmd)
                                )
            elif p_uu > 0.0 or has_leaked_here:
                f_s = step_to_full[s]
                if f_s > f_cursor:
                    tss._new_circuit += pre.get_full_slice(f_cursor, f_s)
                q_s = step_to_quant[s]
                if use_tableau and q_s > q_cursor:
                    tss._tableau_simulator.do_circuit(
                        pre.get_quant_slice(q_cursor, q_s)
                    )
                tss._circuit_time = pre.step_to_unrolled_idx[s]
                coh.handle_op(op=pre.orig_ops[s], sss=tss)
                if not use_tableau and tss._running_tableau:
                    use_tableau = True
                f_cursor = f_s
                q_cursor = q_s
                num_leaked = int(np.count_nonzero(state >= 2))
            s += 1
        elif kind == 5:
            if num_leaked > 0 and np.any(state[qubit_i32] >= 2):
                f_s = step_to_full[s]
                if f_s > f_cursor:
                    tss._new_circuit += pre.get_full_slice(f_cursor, f_s)
                q_s = step_to_quant[s]
                if use_tableau and q_s > q_cursor:
                    tss._tableau_simulator.do_circuit(
                        pre.get_quant_slice(q_cursor, q_s)
                    )
                leaked_qs = qubit_i32[state[qubit_i32] >= 2]
                if np.any(state[leaked_qs] > 2):
                    tss._circuit_time = pre.step_to_unrolled_idx[s]
                    coh.handle_op(op=pre.orig_ops[s], sss=tss)
                    if not use_tableau and tss._running_tableau:
                        use_tableau = True
                else:
                    t_str = " ".join(str(int(q)) for q in leaked_qs)
                    p2 = params.prob_for_input_state.get(2, 0.0)
                    cmd = (
                        f"R {t_str}\n"
                        + (f"X_ERROR({p2}) {t_str}\n" if p2 > 0 else "")
                        + bare_op_text
                        + f"DEPOLARIZE1(0.75) {t_str}\n"
                    )
                    tss._new_circuit.append_from_stim_program_text(cmd)
                    if use_tableau:
                        tss._tableau_simulator.do_circuit(stim.Circuit(cmd))
                f_cursor = f_s + 1
                q_cursor = q_s + 1
            s += 1
        elif kind == 7 and use_tableau:
            s_meas = s + 1
            _, _, _, meas_bare_text, _, meas_qs, meas_params, _ = steps[s_meas]
            if num_leaked > 0 and np.any(state[meas_qs] > 2):
                f_s = step_to_full[s]
                if f_s > f_cursor:
                    tss._new_circuit += pre.get_full_slice(f_cursor, f_s)
                q_s = step_to_quant[s]
                if q_s > q_cursor:
                    tss._tableau_simulator.do_circuit(
                        pre.get_quant_slice(q_cursor, q_s)
                    )
                tss._circuit_time = pre.step_to_unrolled_idx[s]
                coh.handle_op(op=pre.orig_ops[s], sss=tss)
                f_cursor = f_s
                q_cursor = q_s
                num_leaked = int(np.count_nonzero(state >= 2))
                s += 1
                continue

            one_args = params.args_by_input_state[1]
            p1_tot = sum(p for _, p in one_args)
            comp_qs = qubit_i32[state[qubit_i32] < 2]
            n_cand = (
                (
                    len(comp_qs)
                    if p1_tot >= 1.0 - 1e-12
                    else int(rng.binomial(len(comp_qs), p1_tot))
                )
                if (len(comp_qs) > 0 and p1_tot > 0.0)
                else 0
            )
            pre_leaked_qs = (
                meas_qs[state[meas_qs] >= 2]
                if num_leaked > 0
                else _EMPTY_I32
            )
            p2 = meas_params.prob_for_input_state.get(2, 0.0)

            if n_cand == 0:
                if len(pre_leaked_qs) > 0:
                    f_m = step_to_full[s_meas]
                    if f_m > f_cursor:
                        tss._new_circuit += pre.get_full_slice(f_cursor, f_m)
                    q_m = step_to_quant[s_meas]
                    if q_m > q_cursor:
                        tss._tableau_simulator.do_circuit(
                            pre.get_quant_slice(q_cursor, q_m)
                        )
                    t_str = " ".join(str(int(q)) for q in pre_leaked_qs)
                    cmd = (
                        f"R {t_str}\n"
                        + (f"X_ERROR({p2}) {t_str}\n" if p2 > 0 else "")
                        + meas_bare_text
                        + f"DEPOLARIZE1(0.75) {t_str}\n"
                    )
                    tss._new_circuit.append_from_stim_program_text(cmd)
                    tss._tableau_simulator.do_circuit(stim.Circuit(cmd))
                    f_cursor = f_m + 1
                    q_cursor = q_m + 1
            else:
                q_m = step_to_quant[s_meas]
                if len(pre_leaked_qs) == 0:
                    q_after_m = step_to_quant[s_meas + 1]
                    if q_after_m > q_cursor:
                        tss._tableau_simulator.do_circuit(
                            pre.get_quant_slice(q_cursor, q_after_m)
                        )
                    q_cursor = q_after_m
                else:
                    if q_m > q_cursor:
                        tss._tableau_simulator.do_circuit(
                            pre.get_quant_slice(q_cursor, q_m)
                        )
                    t_pre = " ".join(str(int(q)) for q in pre_leaked_qs)
                    pre_cmd = (
                        f"R {t_pre}\n"
                        + (f"X_ERROR({p2}) {t_pre}\n" if p2 > 0 else "")
                        + meas_bare_text
                        + f"DEPOLARIZE1(0.75) {t_pre}\n"
                    )
                    tss._tableau_simulator.do_circuit(stim.Circuit(pre_cmd))
                    q_cursor = q_m + 1

                cands = (
                    comp_qs
                    if n_cand == len(comp_qs)
                    else comp_qs[
                        rng.choice(len(comp_qs), size=n_cand, replace=False)
                    ]
                )
                newly_leaked = [
                    int(q)
                    for q in cands
                    if tss._tableau_simulator.peek_z(int(q)) == -1
                ]
                if newly_leaked:
                    if rec_ev:
                        for q_new in newly_leaked:
                            tss._record_leakage_event(
                                pre.step_to_unrolled_idx[s],
                                q_new,
                                state[q_new],
                                int(one_args[0][0]),
                            )
                    state[newly_leaked] = int(one_args[0][0])
                    if tss.record_unleaked_to_leaked:
                        tss._record_unleaked_to_leaked_count(
                            len(newly_leaked), pre.step_to_unrolled_idx[s]
                        )
                    num_leaked = int(np.count_nonzero(state >= 2))
                    t_new = " ".join(str(q) for q in newly_leaked)
                    tss._tableau_simulator.do_circuit(
                        stim.Circuit(f"R {t_new}\nDEPOLARIZE1(0.75) {t_new}\n")
                    )

                all_leaked_qs = (
                    meas_qs[state[meas_qs] >= 2]
                    if num_leaked > 0
                    else _EMPTY_I32
                )
                if len(all_leaked_qs) > 0:
                    f_m = step_to_full[s_meas]
                    if f_m > f_cursor:
                        tss._new_circuit += pre.get_full_slice(f_cursor, f_m)
                    t_all = " ".join(str(int(q)) for q in all_leaked_qs)
                    full_cmd = (
                        f"R {t_all}\n"
                        + (f"X_ERROR({p2}) {t_all}\n" if p2 > 0 else "")
                        + meas_bare_text
                        + f"DEPOLARIZE1(0.75) {t_all}\n"
                    )
                    tss._new_circuit.append_from_stim_program_text(full_cmd)
                    f_cursor = f_m + 1
            s = s_meas + 1
        elif kind == 8:
            if num_leaked > 0:
                f_s = step_to_full[s]
                if f_s > f_cursor:
                    tss._new_circuit += pre.get_full_slice(f_cursor, f_s)
                q_s = step_to_quant[s]
                if use_tableau and q_s > q_cursor:
                    tss._tableau_simulator.do_circuit(
                        pre.get_quant_slice(q_cursor, q_s)
                    )
                tss._circuit_time = pre.step_to_unrolled_idx[s]
                coh.handle_op(op=pre.orig_ops[s], sss=tss)
                if not use_tableau and tss._running_tableau:
                    use_tableau = True
                consumes = 1 if pre.step_has_bare_op[s] else 0
                f_cursor = f_s + consumes
                q_cursor = q_s + consumes
                num_leaked = int(np.count_nonzero(state >= 2))
            s += 1
        else:
            f_s = step_to_full[s]
            if f_s > f_cursor:
                tss._new_circuit += pre.get_full_slice(f_cursor, f_s)
            q_s = step_to_quant[s]
            if use_tableau and q_s > q_cursor:
                tss._tableau_simulator.do_circuit(
                    pre.get_quant_slice(q_cursor, q_s)
                )
            tss._circuit_time = pre.step_to_unrolled_idx[s]
            coh.handle_op(op=pre.orig_ops[s], sss=tss)
            if not use_tableau and tss._running_tableau:
                use_tableau = True
            consumes = 1 if pre.step_has_bare_op[s] else 0
            f_cursor = f_s + consumes
            q_cursor = q_s + consumes
            num_leaked = int(np.count_nonzero(state >= 2))
            s += 1

    if step_to_full[-1] > f_cursor:
        tss._new_circuit += pre.get_full_slice(f_cursor, step_to_full[-1])
    if use_tableau and step_to_quant[-1] > q_cursor:
        tss._tableau_simulator.do_circuit(
            pre.get_quant_slice(q_cursor, step_to_quant[-1])
        )
    _apply_final_metadata(tss, pre)


class CppTablesideRunner:
    """Tier 2 C++ FastEngine wrapper bound to a TablesideSimulator instance."""

    def __init__(
        self,
        pre: PrecompiledTablesideCircuit,
        seed: int,
    ) -> None:
        self.lib = get_tableside_kernels_lib()
        self.pre = pre
        self._shot_seed = int(seed) + 1000
        self._eng = self.lib.fast_engine_create(
            pre.num_qubits,
            pre.num_steps,
            ctypes.c_uint64(self._shot_seed),
            pre._c_steps,
            len(pre._raw_t_np),
            pre._c_raw_t,
            pre._c_qubit_t,
            len(pre._masks_np),
            pre._c_masks,
            len(pre._c_branches) if pre._c_branches is not None else 0,
            pre._c_branches,
        )
        state_ptr = self.lib.fast_engine_get_state_ptr(self._eng)
        self.state = np.ctypeslib.as_array(
            state_ptr, shape=(max(1, pre.num_qubits),)
        )[: pre.num_qubits]
        self._fmt_buf = ctypes.create_string_buffer(32768)

    def __del__(self) -> None:
        eng = getattr(self, "_eng", None)
        if eng is not None and getattr(self, "lib", None) is not None:
            self.lib.fast_engine_destroy(eng)
            self._eng = None

    def clear(self, new_seed: int) -> None:
        self._shot_seed = int(new_seed) + 1000
        self.lib.fast_engine_clear(self._eng, ctypes.c_uint64(self._shot_seed))

    def _cpp_format_op(
        self,
        op_name: str,
        gate_args: list[float],
        targets_ptr: Any,
        offset: int,
        count: int,
    ) -> str:
        if count <= 0:
            return ""
        needed = 64 + count * 12
        if len(self._fmt_buf) < needed:
            self._fmt_buf = ctypes.create_string_buffer(max(65536, needed))
        if len(gate_args) > 1:
            op_prefix = f"{op_name}(" + ",".join(str(a) for a in gate_args) + ")"
            has_arg = 0
            arg0 = 0.0
        else:
            op_prefix = op_name
            has_arg = 1 if len(gate_args) == 1 else 0
            arg0 = float(gate_args[0]) if has_arg else 0.0
        n_bytes = self.lib.fast_format_stim_op(
            op_prefix.encode("ascii"),
            has_arg,
            arg0,
            ctypes.byref(targets_ptr.contents, offset * 4),
            count,
            self._fmt_buf,
        )
        return self._fmt_buf.raw[:n_bytes].decode("ascii")

    def run(self, tss: "TablesideSimulator") -> None:
        pre = self.pre
        if pre.num_steps == 0:
            if len(pre.full_bare_circuit) > 0:
                tss._new_circuit += pre.full_bare_circuit
            _apply_final_metadata(tss, pre)
            return

        # Sync leaked mask in case user/test manually modified self._compiled_op_handler.state before run()
        if tss._compiled_op_handler.state is not self.state:
            np.copyto(self.state, tss._compiled_op_handler.state)
            tss._compiled_op_handler.state = self.state
        self.lib.fast_engine_sync_leaked_mask(self._eng)

        rec_leak = tss.record_unleaked_to_leaked
        self.lib.fast_engine_set_record_leakage(self._eng, 1 if rec_leak else 0)
        leaked_ops_ptr = ctypes.POINTER(ctypes.c_int32)() if rec_leak else None
        rec_ev = bool(getattr(tss, "record_leakage_events", False))
        self.lib.fast_engine_set_record_events(self._eng, 1 if rec_ev else 0)
        events_ptr = ctypes.POINTER(ctypes.c_int32)()

        actions_ptr = ctypes.POINTER(EmittedActionC)()
        targets_ptr = ctypes.POINTER(ctypes.c_uint32)()
        steps_py = pre.steps_py
        step_to_full = pre.step_to_full_idx
        step_to_quant = pre.step_to_quant_idx
        use_tableau = tss._running_tableau
        coh = tss._compiled_op_handler

        f_cursor = 0
        q_cursor = 0
        cur_step = 0
        num_steps = pre.num_steps

        while cur_step < num_steps:
            n_act = self.lib.fast_engine_run_segment(
                self._eng,
                cur_step,
                ctypes.byref(actions_ptr),
                ctypes.byref(targets_ptr),
            )
            if rec_leak and leaked_ops_ptr is not None:
                n_leaked_ops = self.lib.fast_engine_get_segment_leaked_ops(
                    self._eng, ctypes.byref(leaked_ops_ptr)
                )
                if n_leaked_ops > 0:
                    ev_list = tss._unleaked_to_leaked_events[0]
                    for k_ev in range(n_leaked_ops):
                        ev_list.append(int(leaked_ops_ptr[k_ev]))
            if rec_ev:
                n_events = self.lib.fast_engine_get_segment_events(
                    self._eng, ctypes.byref(events_ptr)
                )
                for k_ev in range(n_events):
                    tss._record_leakage_event(
                        events_ptr[4 * k_ev],
                        events_ptr[4 * k_ev + 1],
                        events_ptr[4 * k_ev + 2],
                        events_ptr[4 * k_ev + 3],
                    )
            next_step = num_steps
            for i in range(n_act):
                act = actions_ptr[i]
                kind = act.kind
                if kind == 0:  # ACT_RUN_SLICE
                    f_end = step_to_full[act.slice_end]
                    if f_end > f_cursor:
                        tss._new_circuit += pre.get_full_slice(f_cursor, f_end)
                        f_cursor = f_end
                    if use_tableau:
                        q_end = step_to_quant[act.slice_end]
                        if q_end > q_cursor:
                            tss._tableau_simulator.do_circuit(
                                pre.get_quant_slice(q_cursor, q_end)
                            )
                        q_cursor = q_end
                elif kind in (1, 2):  # ACT_FILTERED_1Q / ACT_FILTERED_2Q
                    s_idx = act.step_idx
                    f_s = step_to_full[s_idx]
                    if f_s > f_cursor:
                        tss._new_circuit += pre.get_full_slice(f_cursor, f_s)
                    _, op_name, gate_args, _, _, _, _, _ = steps_py[s_idx]
                    txt = self._cpp_format_op(
                        op_name,
                        gate_args,
                        targets_ptr,
                        act.target_offset,
                        act.target_count,
                    )
                    if txt:
                        tss._new_circuit.append_from_stim_program_text(txt)
                    f_cursor = f_s + 1
                    if use_tableau:
                        q_s = step_to_quant[s_idx]
                        if q_s > q_cursor:
                            tss._tableau_simulator.do_circuit(
                                pre.get_quant_slice(q_cursor, q_s)
                            )
                        if txt:
                            tss._tableau_simulator.do_circuit(stim.Circuit(txt))
                        q_cursor = q_s + 1
                elif kind in (3, 4, 5, 6, 7, 8, 9):
                    s_idx = act.step_idx
                    f_s = step_to_full[s_idx]
                    if f_s > f_cursor:
                        tss._new_circuit += pre.get_full_slice(f_cursor, f_s)
                        f_cursor = f_s
                    op_map = {
                        3: ("R", []),
                        4: ("X", []),
                        5: ("Y", []),
                        6: ("Z", []),
                        7: ("DEPOLARIZE1", [act.prob]),
                        8: ("X_ERROR", [act.prob]),
                        9: ("Z_ERROR", [act.prob]),
                    }
                    g_name, g_args = op_map[kind]
                    txt = self._cpp_format_op(
                        g_name,
                        g_args,
                        targets_ptr,
                        act.target_offset,
                        act.target_count,
                    )
                    if txt:
                        tss._new_circuit.append_from_stim_program_text(txt)
                    if use_tableau:
                        q_s = step_to_quant[s_idx]
                        if q_s > q_cursor:
                            tss._tableau_simulator.do_circuit(
                                pre.get_quant_slice(q_cursor, q_s)
                            )
                        if txt:
                            tss._tableau_simulator.do_circuit(stim.Circuit(txt))
                        q_cursor = q_s
                elif kind == 10:  # ACT_MODIFIED_MEAS
                    s_idx = act.step_idx
                    f_s = step_to_full[s_idx]
                    if f_s > f_cursor:
                        tss._new_circuit += pre.get_full_slice(f_cursor, f_s)
                    _, _, _, bare_op_text, _, _, _, _ = steps_py[s_idx]
                    r_str = self._cpp_format_op(
                        "R",
                        [],
                        targets_ptr,
                        act.target_offset,
                        act.target_count,
                    )
                    x_str = (
                        self._cpp_format_op(
                            "X_ERROR",
                            [act.prob],
                            targets_ptr,
                            act.target_offset,
                            act.target_count,
                        )
                        if act.prob > 0.0
                        else ""
                    )
                    d_str = self._cpp_format_op(
                        "DEPOLARIZE1",
                        [0.75],
                        targets_ptr,
                        act.target_offset,
                        act.target_count,
                    )
                    txt = r_str + x_str + bare_op_text + d_str
                    tss._new_circuit.append_from_stim_program_text(txt)
                    f_cursor = f_s + 1
                    if use_tableau:
                        q_s = step_to_quant[s_idx]
                        if q_s > q_cursor:
                            tss._tableau_simulator.do_circuit(
                                pre.get_quant_slice(q_cursor, q_s)
                            )
                        tss._tableau_simulator.do_circuit(stim.Circuit(txt))
                        q_cursor = q_s + 1
                elif kind == 12:  # ACT_YIELD_FUSED_MEAS
                    s_trans = act.step_idx
                    s_meas = act.slice_end
                    if not use_tableau:
                        f_s = step_to_full[s_trans]
                        if f_s > f_cursor:
                            tss._new_circuit += pre.get_full_slice(
                                f_cursor, f_s
                            )
                            f_cursor = f_s
                        tss._run_tableau()
                        use_tableau = True
                        q_cursor = step_to_quant[s_trans]

                    _, _, _, meas_bare_text, _, meas_qs, _, _ = steps_py[s_meas]
                    p2 = act.prob
                    pre_leaked_qs = meas_qs[self.state[meas_qs] >= 2]
                    q_m = step_to_quant[s_meas]

                    if len(pre_leaked_qs) == 0:
                        q_after_m = step_to_quant[s_meas + 1]
                        if q_after_m > q_cursor:
                            tss._tableau_simulator.do_circuit(
                                pre.get_quant_slice(q_cursor, q_after_m)
                            )
                        q_cursor = q_after_m
                    else:
                        if q_m > q_cursor:
                            tss._tableau_simulator.do_circuit(
                                pre.get_quant_slice(q_cursor, q_m)
                            )
                        t_pre = " ".join(str(int(q)) for q in pre_leaked_qs)
                        pre_cmd = (
                            f"R {t_pre}\n"
                            + (f"X_ERROR({p2}) {t_pre}\n" if p2 > 0.0 else "")
                            + meas_bare_text
                            + f"DEPOLARIZE1(0.75) {t_pre}\n"
                        )
                        tss._tableau_simulator.do_circuit(stim.Circuit(pre_cmd))
                        q_cursor = q_m + 1

                    newly_leaked = []
                    for k in range(act.target_count):
                        q_cand = int(targets_ptr[act.target_offset + k])
                        if tss._tableau_simulator.peek_z(q_cand) == -1:
                            newly_leaked.append(q_cand)
                    if newly_leaked:
                        out_state = int(
                            steps_py[s_trans][6].args_by_input_state[1][0][0]
                        )
                        if rec_ev:
                            for q_new in newly_leaked:
                                tss._record_leakage_event(
                                    pre.step_to_unrolled_idx[s_trans],
                                    q_new,
                                    self.state[q_new],
                                    out_state,
                                )
                        self.state[newly_leaked] = out_state
                        if rec_leak:
                            tss._record_unleaked_to_leaked_count(
                                len(newly_leaked),
                                pre.step_to_unrolled_idx[s_trans],
                            )
                        self.lib.fast_engine_sync_leaked_mask(self._eng)
                        t_new = " ".join(str(q) for q in newly_leaked)
                        tss._tableau_simulator.do_circuit(
                            stim.Circuit(f"R {t_new}\nDEPOLARIZE1(0.75) {t_new}\n")
                        )

                    all_leaked_qs = meas_qs[self.state[meas_qs] >= 2]
                    if len(all_leaked_qs) > 0:
                        f_m = step_to_full[s_meas]
                        if f_m > f_cursor:
                            tss._new_circuit += pre.get_full_slice(
                                f_cursor, f_m
                            )
                        t_all = " ".join(str(int(q)) for q in all_leaked_qs)
                        full_cmd = (
                            f"R {t_all}\n"
                            + (f"X_ERROR({p2}) {t_all}\n" if p2 > 0.0 else "")
                            + meas_bare_text
                            + f"DEPOLARIZE1(0.75) {t_all}\n"
                        )
                        tss._new_circuit.append_from_stim_program_text(full_cmd)
                        f_cursor = f_m + 1

                    next_step = s_meas + 1
                    break
                elif kind == 11:  # ACT_YIELD_PYTHON
                    s_idx = act.step_idx
                    f_s = step_to_full[s_idx]
                    if f_s > f_cursor:
                        tss._new_circuit += pre.get_full_slice(f_cursor, f_s)
                    q_s = step_to_quant[s_idx]
                    if use_tableau and q_s > q_cursor:
                        tss._tableau_simulator.do_circuit(
                            pre.get_quant_slice(q_cursor, q_s)
                        )
                    tss._circuit_time = pre.step_to_unrolled_idx[s_idx]
                    coh.handle_op(op=pre.orig_ops[s_idx], sss=tss)
                    if not use_tableau and tss._running_tableau:
                        use_tableau = True
                    consumes = 1 if pre.step_has_bare_op[s_idx] else 0
                    f_cursor = f_s + consumes
                    q_cursor = q_s + consumes
                    self.lib.fast_engine_sync_leaked_mask(self._eng)
                    next_step = s_idx + 1
                    break
            cur_step = next_step

        if step_to_full[-1] > f_cursor:
            tss._new_circuit += pre.get_full_slice(f_cursor, step_to_full[-1])
        if use_tableau and step_to_quant[-1] > q_cursor:
            tss._tableau_simulator.do_circuit(
                pre.get_quant_slice(q_cursor, step_to_quant[-1])
            )
        _apply_final_metadata(tss, pre)
