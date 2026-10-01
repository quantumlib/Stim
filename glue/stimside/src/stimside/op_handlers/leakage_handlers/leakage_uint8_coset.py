from __future__ import annotations

import dataclasses
from typing import Literal, Sequence, cast

import numpy as np
from numpy.typing import NDArray
import stim  # type: ignore[import-untyped]

from stimside.op_handlers.abstract_op_handler import CompiledOpHandler, OpHandler
from stimside.op_handlers.leakage_handlers.leakage_parameters import (
    LeakageConditioningParams,
    LeakageMeasurementParams,
    LeakageParams,
    LeakageTransition1Params,
    LeakageTransition2Params,
)
from stimside.op_handlers.leakage_handlers.leakage_tag_parsing_tableau import (
    parse_leakage_in_circuit,
)
from stimside.simulator_coset import CosetsideSimulator


class LeakageStateArray(np.ndarray):
    """2D uint8 array view (num_qubits, batch_size) that also supports 1D broadcasting and scalar truth checks."""

    def __eq__(self, other: object) -> NDArray[np.bool_]:  # type: ignore[override]
        if (
            isinstance(other, np.ndarray)
            and self.ndim == 2
            and other.ndim == 1
            and other.shape[0] == self.shape[0]
        ):
            return super().__eq__(other[:, None]).view(LeakageStateArray)  # type: ignore[return-value]
        res = super().__eq__(other)
        if isinstance(res, np.ndarray):
            return res.view(LeakageStateArray)  # type: ignore[return-value]
        return res  # type: ignore[return-value]

    def __ne__(self, other: object) -> NDArray[np.bool_]:  # type: ignore[override]
        if (
            isinstance(other, np.ndarray)
            and self.ndim == 2
            and other.ndim == 1
            and other.shape[0] == self.shape[0]
        ):
            return super().__ne__(other[:, None]).view(LeakageStateArray)  # type: ignore[return-value]
        res = super().__ne__(other)
        if isinstance(res, np.ndarray):
            return res.view(LeakageStateArray)  # type: ignore[return-value]
        return res  # type: ignore[return-value]

    def __bool__(self) -> bool:
        return bool(np.all(np.asarray(self)))


@dataclasses.dataclass
class LeakageUint8Coset(OpHandler[CosetsideSimulator]):
    """Batched uint8 leakage OpHandler for CosetsideSimulator reusing tableside tag parsers."""

    unconditional_condition_on_U: bool = True

    def compile_op_handler(
        self, *, circuit: stim.Circuit, batch_size: int
    ) -> "CompiledLeakageUint8Coset":
        parsed_ops = parse_leakage_in_circuit(circuit)
        return CompiledLeakageUint8Coset(
            num_qubits=circuit.num_qubits,
            batch_size=batch_size,
            ops_to_params=parsed_ops,
            unconditional_condition_on_U=self.unconditional_condition_on_U,
        )


@dataclasses.dataclass
class CompiledLeakageUint8Coset(CompiledOpHandler[CosetsideSimulator]):
    """Compiled batched uint8 leakage OpHandler for CosetsideSimulator."""

    num_qubits: int
    batch_size: int
    unconditional_condition_on_U: bool
    ops_to_params: dict[stim.CircuitInstruction, LeakageParams]

    _state: NDArray[np.uint8] = dataclasses.field(init=False)
    _scratch_depolarize: NDArray[np.bool_] = dataclasses.field(init=False)
    _scratch_pauli_X: NDArray[np.bool_] = dataclasses.field(init=False)
    _scratch_pauli_Y: NDArray[np.bool_] = dataclasses.field(init=False)
    _scratch_pauli_Z: NDArray[np.bool_] = dataclasses.field(init=False)
    _scratch_set_one: NDArray[np.bool_] = dataclasses.field(init=False)
    _scratch_set_zero: NDArray[np.bool_] = dataclasses.field(init=False)
    _scratch_rand_phase: NDArray[np.bool_] = dataclasses.field(init=False)
    _scratch_zero_mask: NDArray[np.bool_] = dataclasses.field(init=False)

    def __post_init__(self) -> None:
        self.claimed_ops_keys = set(self.ops_to_params.keys())
        self._alloc_scratch(self.batch_size)
        self.clear()

    def _alloc_scratch(self, b_size: int) -> None:
        shape = (self.num_qubits, b_size)
        self._scratch_depolarize = np.zeros(shape, dtype=np.bool_)
        self._scratch_pauli_X = np.zeros(shape, dtype=np.bool_)
        self._scratch_pauli_Y = np.zeros(shape, dtype=np.bool_)
        self._scratch_pauli_Z = np.zeros(shape, dtype=np.bool_)
        self._scratch_set_one = np.zeros(shape, dtype=np.bool_)
        self._scratch_set_zero = np.zeros(shape, dtype=np.bool_)
        self._scratch_rand_phase = np.zeros(shape, dtype=np.bool_)
        self._scratch_zero_mask = np.zeros(shape, dtype=np.bool_)

    @property
    def state(self) -> NDArray[np.uint8]:
        if self.batch_size == 1:
            return self._state[:, 0]
        return self._state.view(LeakageStateArray)

    @state.setter
    def state(self, val: NDArray[np.uint8]) -> None:
        arr = np.asarray(val, dtype=np.uint8)
        if arr.ndim == 1:
            self._state = np.broadcast_to(
                arr[:, None], (self.num_qubits, self.batch_size)
            ).copy()
        else:
            self._state = arr.copy()

    def _ensure_batch_size(self, sim_batch_size: int) -> None:
        if self._state.shape[1] != sim_batch_size:
            old_col = self._state[:, :1].copy()
            self.batch_size = sim_batch_size
            self._state = np.broadcast_to(
                old_col, (self.num_qubits, sim_batch_size)
            ).copy()
            self._alloc_scratch(sim_batch_size)

    def clear(self) -> None:
        self._state = np.zeros(
            (self.num_qubits, self.batch_size), dtype=np.uint8
        )

    def make_target_mask(self, op: stim.CircuitInstruction) -> NDArray[np.bool_]:
        target_indices = [
            t.qubit_value for t in op.targets_copy() if t.qubit_value is not None
        ]
        if self.batch_size == 1:
            mask_1d = np.zeros(self.num_qubits, dtype=np.bool_)
            mask_1d[target_indices] = True
            return mask_1d
        mask_2d = np.zeros((self.num_qubits, self.batch_size), dtype=np.bool_)
        mask_2d[target_indices, :] = True
        return mask_2d

    def _update_state_from_peek_Z(
        self, targets: Sequence[int] | NDArray[np.int_], css: CosetsideSimulator
    ) -> None:
        self._ensure_batch_size(css.batch_size)
        targets_arr = np.asarray(targets, dtype=np.intp)
        if len(targets_arr) == 0:
            return

        unique_targets = list(dict.fromkeys(int(t) for t in targets_arr if t >= 0))
        if len(unique_targets) == 0:
            return

        z_exps = css.peek_pauli_batch(unique_targets, "Z")
        any_projected = False
        for idx, q in enumerate(unique_targets):
            comp_mask = self._state[q, :] < 2
            if not np.any(comp_mask):
                continue
            z_row = (
                css.peek_pauli_batch([q], "Z")[0]
                if any_projected
                else z_exps[idx]
            )
            new_row = np.zeros(css.batch_size, dtype=np.uint8)
            new_row[z_row == -1] = 1
            flip_mask = comp_mask & (z_row == 0)
            num_flips = int(np.count_nonzero(flip_mask))
            if num_flips > 0:
                if css._shot_np_rngs is not None:
                    for b_np in np.flatnonzero(flip_mask):
                        b_idx = int(b_np)
                        new_row[b_idx] = int(
                            css._shot_np_rngs[b_idx].binomial(1, 0.5)
                        )
                else:
                    new_row[flip_mask] = css.np_rng.binomial(
                        1, 0.5, size=num_flips
                    ).astype(np.uint8)
                css.postselect_z_batch(q, flip_mask, new_row)
                any_projected = True
            self._state[q, comp_mask] = new_row[comp_mask]

    def _filter_targets_1q_mask(
        self,
        targets: Sequence[int] | NDArray[np.int_],
        allowed_st_list: tuple[tuple[int | str, ...], ...],
        css: CosetsideSimulator,
    ) -> NDArray[np.bool_]:
        self._ensure_batch_size(css.batch_size)
        targets_arr = np.asarray(targets, dtype=np.intp)
        allowed_group = allowed_st_list[0]
        valid_qs = targets_arr[targets_arr >= 0]
        if len(valid_qs) > 0 and (
            1 in allowed_group or 0 in allowed_group
        ):
            self._update_state_from_peek_Z(valid_qs, css)

        safe_qs = np.where(targets_arr >= 0, targets_arr, 0)
        target_states = self._state[safe_qs, :].copy()  # (len(targets), batch_size)
        is_classical = (targets_arr < 0)[:, None]
        target_states = np.where(is_classical, 0, target_states)
        self._state[self._state == 1] = 0

        allowed_mask = np.zeros(
            (len(targets_arr), css.batch_size), dtype=np.bool_
        )
        if "U" in allowed_group:
            allowed_mask |= is_classical | (target_states < 2)
        for st in allowed_group:
            if isinstance(st, int):
                if st < 2:
                    allowed_mask |= is_classical | (target_states == st)
                else:
                    allowed_mask |= (~is_classical) & (target_states == st)
            elif st != "U":
                raise ValueError(
                    f"Unrecognised allowed state {st} in allowed_st_list {allowed_st_list}"
                )
        return allowed_mask

    def _filter_targets_2q_mask(
        self,
        targets: Sequence[Sequence[int]] | NDArray[np.int_],
        allowed_st_list: tuple[tuple[int | str, ...], tuple[int | str, ...]],
        css: CosetsideSimulator,
    ) -> NDArray[np.bool_]:
        self._ensure_batch_size(css.batch_size)
        targets_np = np.asarray(targets, dtype=np.intp)  # (num_pairs, 2)
        if len(targets_np) == 0:
            return np.zeros((0, css.batch_size), dtype=np.bool_)
        for i in (0, 1):
            if 1 in allowed_st_list[i] or 0 in allowed_st_list[i]:
                valid_qs = targets_np[targets_np[:, i] >= 0, i]
                if len(valid_qs) > 0:
                    self._update_state_from_peek_Z(valid_qs, css)

        is_c0 = (targets_np[:, 0] < 0)[:, None]
        is_c1 = (targets_np[:, 1] < 0)[:, None]
        q0_safe = np.where(targets_np[:, 0] >= 0, targets_np[:, 0], 0)
        q1_safe = np.where(targets_np[:, 1] >= 0, targets_np[:, 1], 0)
        s0 = np.where(is_c0, 0, self._state[q0_safe, :])
        s1 = np.where(is_c1, 0, self._state[q1_safe, :])
        self._state[self._state == 1] = 0
        allowed_mask = np.zeros((len(targets_np), css.batch_size), dtype=np.bool_)

        for n in range(len(allowed_st_list[0])):
            st0 = allowed_st_list[0][n]
            st1 = allowed_st_list[1][n]
            if st0 == "U":
                m0 = is_c0 | (s0 < 2)
            elif isinstance(st0, int):
                m0 = (is_c0 & (st0 < 2)) | ((~is_c0) & (s0 == st0))
            else:
                raise ValueError(
                    f"Unrecognised allowed state {st0} in allowed_st_list {allowed_st_list}"
                )

            if st1 == "U":
                m1 = is_c1 | (s1 < 2)
            elif isinstance(st1, int):
                m1 = (is_c1 & (st1 < 2)) | ((~is_c1) & (s1 == st1))
            else:
                raise ValueError(
                    f"Unrecognised allowed state {st1} in allowed_st_list {allowed_st_list}"
                )

            allowed_mask |= m0 & m1
        return allowed_mask

    def handle_op(
        self, op: stim.CircuitInstruction, sss: CosetsideSimulator
    ) -> None:
        self._ensure_batch_size(sss.batch_size)
        if op not in self.claimed_ops_keys:
            if self.unconditional_condition_on_U:
                op_name = op.name
                gd = stim.gate_data(op_name)
                if gd.produces_measurements or not (
                    gd.is_noisy_gate or gd.is_unitary
                ):
                    sss._do_bare_instruction(op)
                    return

                if gd.is_single_qubit_gate or op_name in (
                    "E",
                    "CORRELATED_ERROR",
                    "ELSE_CORRELATED_ERROR",
                ):
                    targets = [
                        t.qubit_value if t.qubit_value is not None else -1
                        for t in op.targets_copy()
                    ]
                    cond_mask = self._filter_targets_1q_mask(
                        targets, (("U",),), sss
                    )
                elif gd.is_two_qubit_gate:
                    target_pairs = [
                        [
                            grp[0].qubit_value
                            if grp[0].qubit_value is not None
                            else -1,
                            grp[1].qubit_value
                            if grp[1].qubit_value is not None
                            else -1,
                        ]
                        for grp in op.target_groups()
                    ]
                    cond_mask = self._filter_targets_2q_mask(
                        target_pairs, (("U",), ("U",)), sss
                    )
                else:
                    sss._do_bare_instruction(op)
                    return

                if gd.is_unitary:
                    sss.do_conditional_clifford(op, cond_mask)
                else:
                    sss.do_conditional_pauli_noise(op, cond_mask)
                return

            sss._do_bare_instruction(op)
            return

        params = self.ops_to_params[op]
        if sss.record_unleaked_to_leaked and isinstance(
            params, (LeakageTransition1Params, LeakageTransition2Params)
        ):
            self._ensure_batch_size(sss.batch_size)
            target_indices = np.unique(
                [
                    t.qubit_value if t.qubit_value is not None else t.value
                    for t in op.targets_copy()
                    if (t.qubit_value if t.qubit_value is not None else t.value) >= 0
                ]
            )
            was_unleaked = (self._state[target_indices, :] < 2).copy()
        else:
            target_indices = None
            was_unleaked = None

        match params:
            case LeakageConditioningParams():
                self.leakage_conditioning(op=op, tss=sss, params=params)
            case LeakageTransition1Params():
                self.leakage_transition_1(op=op, tss=sss, params=params)
            case LeakageTransition2Params():
                self.leakage_transition_2(op=op, tss=sss, params=params)
            case LeakageMeasurementParams():
                self.leakage_measurement(op=op, tss=sss, params=params)
            case _:
                raise ValueError(f"Unrecognised LEAKAGE params: {params}")

        if was_unleaked is not None and target_indices is not None:
            now_leaked = self._state[target_indices, :] >= 2
            counts = np.count_nonzero(was_unleaked & now_leaked, axis=0)
            sss._record_unleaked_to_leaked_counts(counts, sss._circuit_time)

    def leakage_conditioning(
        self,
        op: stim.CircuitInstruction,
        tss: CosetsideSimulator,
        params: LeakageConditioningParams,
    ) -> None:
        condition_groups = params.args
        tag_parts = getattr(params, "from_tag", "").split(":")
        if (
            len(condition_groups) == 1
            and "U" in condition_groups[0]
            and len(tag_parts) > 1
            and any(tok in ("0", "1") for tok in tag_parts[1].split())
        ):
            condition_groups = (
                tuple(
                    st
                    for x in condition_groups[0]
                    for st in ((0, 1) if x == "U" else (x,))
                ),
            )
        other_targets = params.targets
        gd = stim.gate_data(op.name)
        raw_targets = op.targets_copy()
        if gd.is_unitary and any(0 in g or 1 in g for g in condition_groups):
            t_groups = op.target_groups()
            if len(t_groups) > 1:
                all_qs = [
                    t.qubit_value
                    for grp in t_groups
                    for t in grp
                    if t.qubit_value is not None
                ]
                if other_targets:
                    all_qs.extend(other_targets)
                if len(set(all_qs)) < len(all_qs):
                    def _eval_group_cond(g_idx: int) -> NDArray[np.bool_]:
                        grp = t_groups[g_idx]
                        if other_targets:
                            step_ot = len(other_targets) // len(t_groups)
                            sub_ot = list(
                                other_targets[
                                    g_idx * step_ot : (g_idx + 1) * step_ot
                                ]
                            )
                            if len(sub_ot) == 1:
                                return self._filter_targets_1q_mask(
                                    sub_ot,
                                    cast(
                                        tuple[tuple[int | str, ...], ...],
                                        condition_groups,
                                    ),
                                    tss,
                                )[0]
                            if len(condition_groups) == 1:
                                m_1q = self._filter_targets_1q_mask(
                                    sub_ot,
                                    cast(
                                        tuple[tuple[int | str, ...], ...],
                                        condition_groups,
                                    ),
                                    tss,
                                )
                                return m_1q[0] & m_1q[1]
                            return self._filter_targets_2q_mask(
                                [sub_ot],
                                cast(
                                    tuple[
                                        tuple[int | str, ...],
                                        tuple[int | str, ...],
                                    ],
                                    condition_groups,
                                ),
                                tss,
                            )[0]
                        if len(condition_groups) == 1:
                            sub_qs = [
                                t.qubit_value if t.qubit_value is not None else -1
                                for t in grp
                            ]
                            m_1q = self._filter_targets_1q_mask(
                                sub_qs, condition_groups, tss
                            )
                            return (
                                (m_1q[0] & m_1q[1])
                                if gd.is_two_qubit_gate
                                else m_1q[0]
                            )
                        pair = [
                            [
                                grp[0].qubit_value
                                if grp[0].qubit_value is not None
                                else -1,
                                grp[1].qubit_value
                                if grp[1].qubit_value is not None
                                else -1,
                            ]
                        ]
                        return self._filter_targets_2q_mask(
                            pair,
                            cast(
                                tuple[
                                    tuple[int | str, ...],
                                    tuple[int | str, ...],
                                ],
                                condition_groups,
                            ),
                            tss,
                        )[0]

                    tss.do_conditional_clifford(
                        op, condition_group_fn=_eval_group_cond
                    )
                    return
        if other_targets:
            if gd.is_two_qubit_gate:
                num_pairs = len(raw_targets) // 2
                if len(other_targets) == num_pairs:
                    cond_mask = self._filter_targets_1q_mask(
                        list(other_targets),
                        cast(tuple[tuple[int | str, ...], ...], condition_groups),
                        tss,
                    )
                elif len(other_targets) == len(raw_targets):
                    if len(condition_groups) == 1:
                        m_1q = self._filter_targets_1q_mask(
                            list(other_targets),
                            cast(
                                tuple[tuple[int | str, ...], ...],
                                condition_groups,
                            ),
                            tss,
                        )
                        cond_mask = m_1q[0::2, :] & m_1q[1::2, :]
                    else:
                        other_pairs = [
                            [other_targets[i], other_targets[i + 1]]
                            for i in range(0, len(other_targets), 2)
                        ]
                        cond_mask = self._filter_targets_2q_mask(
                            other_pairs,
                            cast(
                                tuple[
                                    tuple[int | str, ...],
                                    tuple[int | str, ...],
                                ],
                                condition_groups,
                            ),
                            tss,
                        )
                else:
                    raise ValueError(
                        f"The number of targets of the {op} is not compatible with the "
                        "number of targets specified in the CONDITIONED_ON_OTHER tag."
                    )
            else:
                if len(other_targets) != len(raw_targets):
                    raise ValueError(
                        f"The number of targets of the {op} is not the same as the "
                        "number of targets specified in the CONDITIONED_ON_OTHER tag."
                    )
                cond_mask = self._filter_targets_1q_mask(
                    list(other_targets),
                    cast(tuple[tuple[int | str, ...], ...], condition_groups),
                    tss,
                )
        else:
            if len(condition_groups) == 1:
                targets = [
                    t.qubit_value if t.qubit_value is not None else -1
                    for t in raw_targets
                ]
                m_1q = self._filter_targets_1q_mask(
                    targets, condition_groups, tss
                )
                if gd.is_two_qubit_gate:
                    cond_mask = m_1q[0::2, :] & m_1q[1::2, :]
                else:
                    cond_mask = m_1q
            else:
                target_pairs = [
                    [
                        grp[0].qubit_value
                        if grp[0].qubit_value is not None
                        else -1,
                        grp[1].qubit_value
                        if grp[1].qubit_value is not None
                        else -1,
                    ]
                    for grp in op.target_groups()
                ]
                cond_mask = self._filter_targets_2q_mask(
                    target_pairs,
                    cast(
                        tuple[tuple[int | str, ...], tuple[int | str, ...]],
                        condition_groups,
                    ),
                    tss,
                )

        if gd.is_unitary:
            tss.do_conditional_clifford(op, cond_mask)
        else:
            tss.do_conditional_pauli_noise(op, cond_mask)

    def _apply_transition_effects(
        self,
        tss: CosetsideSimulator,
        to_set_to_zero: NDArray[np.bool_],
        to_set_to_one: NDArray[np.bool_],
        to_depolarize: NDArray[np.bool_],
        to_pauli_X: NDArray[np.bool_],
        to_pauli_Y: NDArray[np.bool_],
        to_pauli_Z: NDArray[np.bool_],
        to_randomize_X: NDArray[np.bool_],
        randomize_phase: NDArray[np.bool_],
    ) -> None:
        if np.any(to_set_to_zero):
            tss.do_unmatched_noisy_reset_z(
                to_set_to_zero, self._scratch_zero_mask
            )
        if np.any(to_set_to_one):
            tss.do_unmatched_noisy_reset_z(
                self._scratch_zero_mask, to_set_to_one
            )
        if np.any(to_depolarize):
            if tss._tab_rngs is not None:
                targets_per_shot = [
                    list(np.where(to_depolarize[:, b])[0])
                    for b in range(tss.batch_size)
                ]
                tss._sample_pauli_noise_via_tab_rng(
                    "DEPOLARIZE1", targets_per_shot, [0.75]
                )
            else:
                tss._flip_simulator.broadcast_pauli_errors(
                    pauli="X", mask=to_depolarize, p=0.5
                )
                tss._flip_simulator.broadcast_pauli_errors(
                    pauli="Z", mask=to_depolarize, p=0.5
                )
        if np.any(to_pauli_X):
            tss._flip_simulator.broadcast_pauli_errors(
                pauli="X", mask=to_pauli_X, p=1.0
            )
        if np.any(to_pauli_Y):
            tss._flip_simulator.broadcast_pauli_errors(
                pauli="Y", mask=to_pauli_Y, p=1.0
            )
        if np.any(to_pauli_Z):
            tss._flip_simulator.broadcast_pauli_errors(
                pauli="Z", mask=to_pauli_Z, p=1.0
            )
        if np.any(to_randomize_X):
            if tss._tab_rngs is not None:
                targets_per_shot = [
                    list(np.where(to_randomize_X[:, b])[0])
                    for b in range(tss.batch_size)
                ]
                tss._sample_pauli_noise_via_tab_rng(
                    "X_ERROR", targets_per_shot, [0.5]
                )
            else:
                tss._flip_simulator.broadcast_pauli_errors(
                    pauli="X", mask=to_randomize_X, p=0.5
                )
        if np.any(randomize_phase):
            if tss._tab_rngs is not None:
                targets_per_shot = [
                    list(np.where(randomize_phase[:, b])[0])
                    for b in range(tss.batch_size)
                ]
                tss._sample_pauli_noise_via_tab_rng(
                    "Z_ERROR", targets_per_shot, [0.5]
                )
            else:
                tss._flip_simulator.broadcast_pauli_errors(
                    pauli="Z", mask=randomize_phase, p=0.5
                )

    def leakage_transition_1(
        self,
        op: stim.CircuitInstruction,
        tss: CosetsideSimulator,
        params: LeakageTransition1Params,
        *,
        _advance_step: bool = True,
    ) -> None:
        raw_t = op.targets_copy()
        target_indices = [
            t.qubit_value if t.qubit_value is not None else t.value
            for t in raw_t
        ]
        if _advance_step:
            tss._next_step_meta(op)
        if len(target_indices) > 1 and len(set(target_indices)) < len(
            target_indices
        ):
            for t in raw_t:
                sub_op = stim.CircuitInstruction(
                    op.name, [t], op.gate_args_copy()
                )
                self.leakage_transition_1(
                    sub_op, tss, params, _advance_step=False
                )
            return
        target_mask_1d = np.zeros(self.num_qubits, dtype=np.bool_)
        target_mask_1d[target_indices] = True
        sorted_target_indices = np.where(target_mask_1d)[0]

        to_depolarize = self._scratch_depolarize
        to_pauli_X = self._scratch_pauli_X
        to_pauli_Y = self._scratch_pauli_Y
        to_pauli_Z = self._scratch_pauli_Z
        to_set_to_one = self._scratch_set_one
        to_set_to_zero = self._scratch_set_zero
        randomize_phase = self._scratch_rand_phase
        to_randomize_X = np.zeros_like(randomize_phase)
        to_depolarize.fill(False)
        to_pauli_X.fill(False)
        to_pauli_Y.fill(False)
        to_pauli_Z.fill(False)
        to_set_to_one.fill(False)
        to_set_to_zero.fill(False)
        randomize_phase.fill(False)

        if any(k in (0, 1) for k in params.args_by_input_state.keys()):
            self._update_state_from_peek_Z(sorted_target_indices, tss)
        initial_state = self._state.copy()

        if tss._shot_np_rngs is not None or (
            tss.batch_size == 1 and tss.seed is not None
        ):
            for input_state in params.args_by_input_state.keys():
                for b in range(tss.batch_size):
                    if input_state == "U":
                        in_mask_b = initial_state[:, b] < 2
                    else:
                        in_mask_b = initial_state[:, b] == input_state
                    ow_b = target_mask_1d & in_mask_b
                    samples_b = int(np.count_nonzero(ow_b))
                    if samples_b == 0:
                        continue
                    rng_b = (
                        tss._shot_np_rngs[b]
                        if tss._shot_np_rngs is not None
                        else tss.np_rng
                    )
                    out_b = params.sample_transitions_from_state(
                        input_state=input_state,
                        num_samples=samples_b,
                        np_rng=rng_b,
                    ).astype(str)
                    ow_qs = np.where(ow_b)[0]
                    for k_i, q_np in enumerate(ow_qs):
                        q = int(q_np)
                        out_st = str(out_b[k_i])
                        if out_st == str(input_state):
                            self._state[q, b] = (
                                0 if input_state in (0, 1, "U") else int(input_state)
                            )
                            continue
                        in_is_leaked = int(initial_state[q, b]) >= 2
                        if out_st == "0":
                            to_set_to_zero[q, b] |= True
                            self._state[q, b] = 0
                        elif out_st == "1":
                            to_set_to_one[q, b] |= True
                            self._state[q, b] = 0
                        elif out_st == "U":
                            if in_is_leaked:
                                to_set_to_zero[q, b] |= True
                            to_depolarize[q, b] |= True
                            self._state[q, b] = 0
                        else:
                            out_int = int(out_st)
                            self._state[q, b] = 0 if out_int < 2 else out_int
        else:
            target_mask = target_mask_1d[:, None]
            for input_state, transitions in params.args_by_input_state.items():
                total_p = float(sum(p for _, p in transitions))
                if total_p <= 0.0:
                    continue
                if input_state == "U":
                    input_state_mask = initial_state < 2
                else:
                    input_state_mask = initial_state == input_state

                overwrite_mask = target_mask & input_state_mask
                samples_to_take = int(np.count_nonzero(overwrite_mask))
                if samples_to_take == 0:
                    continue

                if total_p < 1.0 - 1e-12:
                    k_trans = int(
                        tss.np_rng.binomial(samples_to_take, total_p)
                    )
                else:
                    k_trans = samples_to_take
                if k_trans == 0:
                    continue

                q_all, b_all = np.where(overwrite_mask)
                if k_trans < samples_to_take:
                    chosen = tss.np_rng.choice(
                        samples_to_take, size=k_trans, replace=False
                    )
                    q_trans, b_trans = q_all[chosen], b_all[chosen]
                else:
                    q_trans, b_trans = q_all, b_all

                if len(transitions) == 1:
                    out_groups = [(transitions[0][0], q_trans, b_trans)]
                else:
                    cond_probs = np.array(
                        [p / total_p for _, p in transitions], dtype=np.float64
                    )
                    cond_probs /= cond_probs.sum()
                    t_idx_sampled = tss.np_rng.choice(
                        len(transitions), size=k_trans, p=cond_probs
                    )
                    out_groups = [
                        (
                            transitions[t_i][0],
                            q_trans[t_idx_sampled == t_i],
                            b_trans[t_idx_sampled == t_i],
                        )
                        for t_i in range(len(transitions))
                    ]

                for out_s, q_sub, b_sub in out_groups:
                    if len(q_sub) == 0 or str(out_s) == str(input_state):
                        continue
                    in_is_leaked = not (input_state in (0, 1, "U"))
                    if out_s == "U":
                        to_depolarize[q_sub, b_sub] = True
                        if in_is_leaked:
                            to_set_to_zero[q_sub, b_sub] = True
                        self._state[q_sub, b_sub] = 0
                    elif out_s == 1 or out_s == "1":
                        to_set_to_one[q_sub, b_sub] = True
                        self._state[q_sub, b_sub] = 0
                    elif out_s == 0 or out_s == "0":
                        to_set_to_zero[q_sub, b_sub] = True
                        self._state[q_sub, b_sub] = 0
                    else:
                        self._state[q_sub, b_sub] = int(out_s)

        self._state[self._state == 1] = 0

        self._apply_transition_effects(
            tss,
            to_set_to_zero,
            to_set_to_one,
            to_depolarize,
            to_pauli_X,
            to_pauli_Y,
            to_pauli_Z,
            to_randomize_X,
            randomize_phase,
        )

    def leakage_transition_2(
        self,
        op: stim.CircuitInstruction,
        tss: CosetsideSimulator,
        params: LeakageTransition2Params,
        *,
        _advance_step: bool = True,
    ) -> None:
        if _advance_step:
            tss._next_step_meta(op)
        targets_list = [t.qubit_value for t in op.targets_copy()]
        if len(targets_list) > 2 and len(set(targets_list)) < len(targets_list):
            for grp in op.target_groups():
                sub_op = stim.CircuitInstruction(
                    op.name, grp, op.gate_args_copy()
                )
                self.leakage_transition_2(
                    sub_op, tss, params, _advance_step=False
                )
            return
        even_targets_list = targets_list[::2]
        odd_targets_list = targets_list[1::2]
        even_targets_arr = np.asarray(even_targets_list, dtype=np.intp)
        odd_targets_arr = np.asarray(odd_targets_list, dtype=np.intp)

        if any(k[0] in (0, 1) for k in params.args_by_input_state.keys()):
            self._update_state_from_peek_Z(even_targets_list, tss)
        if any(k[1] in (0, 1) for k in params.args_by_input_state.keys()):
            self._update_state_from_peek_Z(odd_targets_list, tss)

        even_target_states = self._state[even_targets_arr, :].copy()
        odd_target_states = self._state[odd_targets_arr, :].copy()

        to_depolarize = self._scratch_depolarize
        to_pauli_X = self._scratch_pauli_X
        to_pauli_Y = self._scratch_pauli_Y
        to_pauli_Z = self._scratch_pauli_Z
        to_set_to_one = self._scratch_set_one
        to_set_to_zero = self._scratch_set_zero
        randomize_phase = self._scratch_rand_phase
        to_randomize_X = np.zeros_like(randomize_phase)
        to_depolarize.fill(False)
        to_pauli_X.fill(False)
        to_pauli_Y.fill(False)
        to_pauli_Z.fill(False)
        to_set_to_one.fill(False)
        to_set_to_zero.fill(False)
        randomize_phase.fill(False)

        if tss._shot_np_rngs is not None or (
            tss.batch_size == 1 and tss.seed is not None
        ):
            for input_state in params.args_by_input_state.keys():
                if input_state[0] == "U":
                    even_checks = even_target_states < 2
                else:
                    even_checks = even_target_states == input_state[0]

                if input_state[1] == "U":
                    odd_checks = odd_target_states < 2
                else:
                    odd_checks = odd_target_states == input_state[1]

                target_state_mask = even_checks & odd_checks
                for b in range(tss.batch_size):
                    t_mask_b = target_state_mask[:, b]
                    samples_b = int(np.count_nonzero(t_mask_b))
                    if samples_b == 0:
                        continue
                    trig_pairs_b = np.column_stack(
                        (even_targets_arr[t_mask_b], odd_targets_arr[t_mask_b])
                    )

                    rng_b = (
                        tss._shot_np_rngs[b]
                        if tss._shot_np_rngs is not None
                        else tss.np_rng
                    )
                    out_b = params.sample_transitions_from_state(
                        input_state=input_state,
                        num_samples=samples_b,
                        np_rng=rng_b,
                    ).astype(str)

                    for k_i in range(samples_b):
                        for leg in (0, 1):
                            q = int(trig_pairs_b[k_i, leg])
                            in_st = input_state[leg]
                            out_st = out_b[k_i, leg]
                            if out_st == str(in_st):
                                self._state[q, b] = (
                                    0 if in_st in (0, 1, "U") else int(in_st)
                                )
                                continue
                            in_is_leaked = not (in_st in (0, 1, "U"))
                            if out_st == "0":
                                to_set_to_zero[q, b] |= True
                                self._state[q, b] = 0
                            elif out_st == "1":
                                to_set_to_one[q, b] |= True
                                self._state[q, b] = 0
                            elif out_st == "V":
                                to_set_to_zero[q, b] |= True
                                to_randomize_X[q, b] |= True
                                randomize_phase[q, b] |= True
                                self._state[q, b] = 0
                            elif out_st == "D":
                                if in_is_leaked:
                                    to_set_to_zero[q, b] |= True
                                to_depolarize[q, b] |= True
                                self._state[q, b] = 0
                            elif out_st == "U":
                                if in_is_leaked:
                                    to_set_to_zero[q, b] |= True
                                to_depolarize[q, b] |= True
                                self._state[q, b] = 0
                            elif out_st == "X":
                                if in_is_leaked:
                                    to_set_to_zero[q, b] |= True
                                to_pauli_X[q, b] |= True
                                self._state[q, b] = 0
                            elif out_st == "Y":
                                if in_is_leaked:
                                    to_set_to_zero[q, b] |= True
                                to_pauli_Y[q, b] |= True
                                self._state[q, b] = 0
                            elif out_st == "Z":
                                if in_is_leaked:
                                    to_set_to_zero[q, b] |= True
                                to_pauli_Z[q, b] |= True
                                self._state[q, b] = 0
                            else:
                                out_int = int(out_st)
                                self._state[q, b] = 0 if out_int < 2 else out_int
        else:

            for input_state, transitions in params.args_by_input_state.items():
                total_p = float(sum(p for _, p in transitions))
                if total_p <= 0.0:
                    continue

                if input_state[0] == "U":
                    even_checks = even_target_states < 2
                else:
                    even_checks = even_target_states == input_state[0]

                if input_state[1] == "U":
                    odd_checks = odd_target_states < 2
                else:
                    odd_checks = odd_target_states == input_state[1]

                target_state_mask = even_checks & odd_checks
                samples_to_take = int(np.count_nonzero(target_state_mask))
                if samples_to_take == 0:
                    continue

                if total_p < 1.0 - 1e-12:
                    k_trans = int(
                        tss.np_rng.binomial(samples_to_take, total_p)
                    )
                else:
                    k_trans = samples_to_take

                if k_trans == 0:
                    continue

                p_all, b_all = np.where(target_state_mask)
                if k_trans < samples_to_take:
                    chosen = tss.np_rng.choice(
                        samples_to_take, size=k_trans, replace=False
                    )
                    p_trans = p_all[chosen]
                    b_trans = b_all[chosen]
                else:
                    p_trans = p_all
                    b_trans = b_all

                if len(transitions) == 1:
                    out_groups = [(transitions[0][0], p_trans, b_trans)]
                else:
                    cond_probs = np.array(
                        [p / total_p for _, p in transitions], dtype=np.float64
                    )
                    cond_probs /= cond_probs.sum()
                    t_idx_sampled = tss.np_rng.choice(
                        len(transitions), size=k_trans, p=cond_probs
                    )
                    out_groups = [
                        (
                            transitions[t_i][0],
                            p_trans[t_idx_sampled == t_i],
                            b_trans[t_idx_sampled == t_i],
                        )
                        for t_i in range(len(transitions))
                    ]

                for out_pair, p_sub, b_sub in out_groups:
                    if len(p_sub) == 0:
                        continue
                    qe_sub = even_targets_arr[p_sub]
                    qo_sub = odd_targets_arr[p_sub]
                    for q_sub, in_s, out_s in (
                        (qe_sub, input_state[0], out_pair[0]),
                        (qo_sub, input_state[1], out_pair[1]),
                    ):
                        if str(out_s) == str(in_s):
                            self._state[q_sub, b_sub] = (
                                0 if in_s in (0, 1, "U") else int(in_s)
                            )
                            continue
                        in_is_leaked = not (in_s in (0, 1, "U"))
                        if out_s == "D":
                            if in_is_leaked:
                                to_set_to_zero[q_sub, b_sub] = True
                            to_depolarize[q_sub, b_sub] = True
                            self._state[q_sub, b_sub] = 0
                        elif out_s == "X":
                            if in_is_leaked:
                                to_set_to_zero[q_sub, b_sub] = True
                            to_pauli_X[q_sub, b_sub] ^= True
                            self._state[q_sub, b_sub] = 0
                        elif out_s == "Y":
                            if in_is_leaked:
                                to_set_to_zero[q_sub, b_sub] = True
                            to_pauli_Y[q_sub, b_sub] ^= True
                            self._state[q_sub, b_sub] = 0
                        elif out_s == "Z":
                            if in_is_leaked:
                                to_set_to_zero[q_sub, b_sub] = True
                            to_pauli_Z[q_sub, b_sub] ^= True
                            self._state[q_sub, b_sub] = 0
                        elif out_s == "V":
                            to_set_to_zero[q_sub, b_sub] = True
                            to_randomize_X[q_sub, b_sub] = True
                            randomize_phase[q_sub, b_sub] = True
                            self._state[q_sub, b_sub] = 0
                        elif out_s == 1 or out_s == "1":
                            to_set_to_one[q_sub, b_sub] = True
                            self._state[q_sub, b_sub] = 0
                        elif out_s == 0 or out_s == "0":
                            to_set_to_zero[q_sub, b_sub] = True
                            self._state[q_sub, b_sub] = 0
                        elif out_s == "U":
                            if in_is_leaked:
                                to_set_to_zero[q_sub, b_sub] = True
                            to_depolarize[q_sub, b_sub] = True
                            self._state[q_sub, b_sub] = 0
                        else:
                            self._state[q_sub, b_sub] = int(out_s)

        self._state[self._state == 1] = 0

        self._apply_transition_effects(
            tss,
            to_set_to_zero,
            to_set_to_one,
            to_depolarize,
            to_pauli_X,
            to_pauli_Y,
            to_pauli_Z,
            to_randomize_X,
            randomize_phase,
        )

    def leakage_measurement(
        self,
        op: stim.CircuitInstruction,
        tss: CosetsideSimulator,
        params: LeakageMeasurementParams,
    ) -> None:
        raw_op_targets = op.targets_copy()
        if params.targets:
            targets = list(params.targets)
            inv_flags = [
                bool(t.value) ^ bool(t.is_inverted_result_target)
                for t in raw_op_targets
            ]
            if len(raw_op_targets) != len(targets):
                raise ValueError(
                    "The number of targets in the MPAD operation with a LEAKAGE_MEASUREMENT tag "
                    "does not equal to the number of targets specified in the tag."
                )
            if op.name != "MPAD":
                raise ValueError(
                    f"LEAKAGE_MEASUREMENT is only implemented for 'MPAD' operations, got {op}"
                )
        else:
            targets = [t.qubit_value for t in raw_op_targets]
            inv_flags = [bool(t.is_inverted_result_target) for t in raw_op_targets]
            if op.name not in ("M", "MZ", "MR", "MRZ", "MX", "MY", "MRX", "MRY"):
                raise ValueError(
                    f"LEAKAGE_PROJECTION_Z is only implemented for M/MZ/MR/MRZ/MX/MY/MRX/MRY operations, got {op}"
                )

        target_leakage_state = self._state[targets, :].copy()  # (len(targets), batch_size)
        targets_arr = np.asarray(targets, dtype=np.intp)
        is_mpad = params.targets is not None
        p0 = params.prob_for_input_state.get(0, 0.0)
        p1 = params.prob_for_input_state.get(1, 0.0 if is_mpad else 1.0)
        p_super = 0.5 * p0 + 0.5 * p1
        basis: Literal["X", "Y", "Z"] = (
            "X"
            if op.name in ("MX", "MRX")
            else ("Y" if op.name in ("MY", "MRY") else "Z")
        )

        if not is_mpad and tss._tab_rngs is not None and len(set(targets)) < len(targets):
            if p0 > 0.0 or p1 < 1.0 or np.any(target_leakage_state > 1):
                raise NotImplementedError(
                    f"Duplicate targets in {op.name} with LEAKAGE_PROJECTION_Z are not supported with "
                    "sync_tableside_rng=True when readout is noisy or any target is leaked."
                )

        # Fast-path for LEAKAGE_PROJECTION_Z measurements with noiseless computational readout (p0=0, p1=1) and sync_tableside_rng
        if (
            not is_mpad
            and tss._tab_rngs is not None
            and p0 == 0.0
            and p1 == 1.0
        ):
            has_leaked = np.any(target_leakage_state > 1)
            leaked_read_one = np.zeros(
                (len(targets), tss.batch_size), dtype=np.bool_
            )

            if has_leaked:
                leaked_zero_mask = self._scratch_set_zero
                leaked_zero_mask.fill(False)
                leaked_zero_mask[targets_arr, :] = target_leakage_state > 1
                tss.do_unmatched_noisy_reset_z(
                    leaked_zero_mask, self._scratch_zero_mask, pauli=basis, order=targets
                )
                for b in range(tss.batch_size):
                    st_b = target_leakage_state[:, b]
                    if not np.any(st_b > 1):
                        continue
                    tr = tss._tab_rngs[b]
                    tr_x = (
                        tss._tab_rngs_x[b]
                        if tss._tab_rngs_x is not None
                        else None
                    )
                    max_leak_b = int(np.max(st_b))
                    for n in range(2, max_leak_b + 1):
                        if n in st_b and n in params.prob_for_input_state:
                            idx_n = np.where(st_b == n)[0]
                            q_n_list = [int(targets_arr[k]) for k in idx_n]
                            prob_n = params.prob_for_input_state[n]
                            if prob_n > 0:
                                inst_n = stim.CircuitInstruction(
                                    "X_ERROR", q_n_list, [prob_n]
                                )
                                tr.do(inst_n)
                                if tr_x is not None:
                                    tr_x.do(inst_n)
                                for k_idx, q_n in zip(idx_n, q_n_list):
                                    if tr.peek_z(q_n) == -1:
                                        tr.x(q_n)
                                        leaked_read_one[int(k_idx), b] = True

            tss._do_bare_instruction(
                stim.CircuitInstruction(op.name, raw_op_targets)
            )

            num_m = len(targets)

            if has_leaked:
                net_x_flip = np.zeros(
                    (self.num_qubits, tss.batch_size), dtype=np.bool_
                )
                for k_i, q in enumerate(targets):
                    mask_l = target_leakage_state[k_i] > 1
                    if not np.any(mask_l):
                        continue
                    m_col = tss._measurement_columns[-num_m + k_i]
                    inv_k = inv_flags[k_i]
                    new_col = np.where(
                        mask_l, leaked_read_one[k_i] ^ inv_k, m_col
                    )
                    diff = m_col ^ new_col
                    if np.any(diff):
                        m_col[:] = new_col
                        net_x_flip[q, :] ^= diff
                if np.any(net_x_flip):
                    tss._flip_simulator.broadcast_pauli_errors(
                        pauli=("Z" if basis == "X" else "X"),
                        mask=net_x_flip,
                        p=1.0,
                    )
                leaked_per_shot = [
                    list(targets_arr[target_leakage_state[:, b] > 1])
                    for b in range(tss.batch_size)
                ]
                tss._sample_pauli_noise_via_tab_rng(
                    "DEPOLARIZE1", leaked_per_shot, [0.75]
                )
            return

        is_reset_meas = op.name in ("MR", "MRZ", "MRX", "MRY")
        first_occ: dict[int, int] = {}
        has_dup_targets = False
        if not is_mpad:
            for k_i, q_val in enumerate(targets):
                q_int = int(q_val)
                if q_int in first_occ:
                    has_dup_targets = True
                else:
                    first_occ[q_int] = k_i

        has_01_cond = (0 in params.prob_for_input_state) or (
            1 in params.prob_for_input_state
        )
        if is_mpad and has_01_cond and tss._tab_rngs is None:
            self._update_state_from_peek_Z(targets, tss)
            self._state[self._state == 1] = 0

        need_pauli_peek = (
            tss._tab_rngs is not None
            or is_mpad
            or has_01_cond
        )
        pauli_states = (
            tss.peek_pauli_batch(targets, basis)
            if need_pauli_peek
            else np.zeros((len(targets), tss.batch_size), dtype=np.int8)
        )

        if tss._tab_rngs is not None:
            final_outcomes = np.zeros(
                (len(targets), tss.batch_size), dtype=np.bool_
            )
            has_leaked = np.any(target_leakage_state > 1)

            # Resolve superpositions via _shot_np_rngs + postselect_z_batch when not (is_mpad and not has_01_cond and np.isclose(p0, p1))
            if not (is_mpad and not has_01_cond and np.isclose(p0, p1)):
                for b in range(tss.batch_size):
                    in_comp_b = target_leakage_state[:, b] < 2
                    idx_in_q = np.where(in_comp_b)[0]
                    superpos_local = np.where(pauli_states[idx_in_q, b] == 0)[0]
                    for s_i, loc_idx in enumerate(superpos_local):
                        k_i = int(idx_in_q[loc_idx])
                        q_int = int(targets_arr[k_i])
                        if has_dup_targets and first_occ[q_int] < k_i:
                            pauli_states[k_i, b] = (
                                1
                                if is_reset_meas
                                else pauli_states[first_occ[q_int], b]
                            )
                            continue
                        pz = (
                            0
                            if s_i == 0
                            else int(tss.peek_pauli_batch([q_int], basis)[0, b])
                        )
                        if pz == 0:
                            bit = int(
                                tss._shot_np_rngs[b].binomial(1, 0.5)
                            )
                            flip_mask_1d = np.zeros(
                                tss.batch_size, dtype=np.bool_
                            )
                            flip_mask_1d[b] = True
                            desired_1d = np.zeros(
                                tss.batch_size, dtype=np.uint8
                            )
                            desired_1d[b] = bit
                            tss.postselect_z_batch(
                                q_int, flip_mask_1d, desired_1d, pauli=basis
                            )
                            pz = -1 if bit == 1 else 1
                        pauli_states[k_i, b] = pz
            if has_dup_targets:
                for k_i, q_val in enumerate(targets):
                    f_k = first_occ[int(q_val)]
                    if f_k < k_i:
                        in_c = target_leakage_state[k_i, :] < 2
                        pauli_states[k_i, in_c] = (
                            1 if is_reset_meas else pauli_states[f_k, in_c]
                        )

            for b in range(tss.batch_size):
                tr = tss._tab_rngs[b]
                tr_x = (
                    tss._tab_rngs_x[b] if tss._tab_rngs_x is not None else None
                )
                st_b = target_leakage_state[:, b]
                in_comp_b = st_b < 2
                targets_in_q = targets_arr[in_comp_b]
                idx_in_q = np.where(in_comp_b)[0]
                ps_b = pauli_states[in_comp_b, b]

                if is_mpad and np.isclose(p0, p1):
                    t_all_q = [int(q) for q in targets_in_q]
                    if len(t_all_q) > 0 and p0 > 0:
                        inst_c = stim.CircuitInstruction(
                            "X_ERROR", t_all_q, [p0]
                        )
                        tr.do(inst_c)
                        if tr_x is not None:
                            tr_x.do(inst_c)
                        for k_i, q_tmp in enumerate(t_all_q):
                            if tr.peek_z(q_tmp) == -1:
                                tr.x(q_tmp)
                                final_outcomes[idx_in_q[k_i], b] = True
                else:
                    m0_mask = ps_b == 1
                    m1_mask = ps_b == -1

                    # 1. |0> states
                    t_0 = [int(q) for q in targets_in_q[m0_mask]]
                    idx_0 = idx_in_q[m0_mask]
                    if len(t_0) > 0 and p0 > 0:
                        inst_0 = stim.CircuitInstruction(
                            "X_ERROR", t_0, [p0]
                        )
                        tr.do(inst_0)
                        if tr_x is not None:
                            tr_x.do(inst_0)
                        for k_i, q_tmp in enumerate(t_0):
                            if tr.peek_z(q_tmp) == -1:
                                tr.x(q_tmp)
                                final_outcomes[idx_0[k_i], b] = True

                    # 2. |1> states
                    t_1 = [int(q) for q in targets_in_q[m1_mask]]
                    idx_1 = idx_in_q[m1_mask]
                    if len(t_1) > 0:
                        final_outcomes[idx_1, b] = True
                        prob_1_err = 1.0 - p1
                        if prob_1_err > 0:
                            inst_1 = stim.CircuitInstruction(
                                "X_ERROR", t_1, [prob_1_err]
                            )
                            tr.do(inst_1)
                            if tr_x is not None:
                                tr_x.do(inst_1)
                            for k_i, q_tmp in enumerate(t_1):
                                if tr.peek_z(q_tmp) == -1:
                                    tr.x(q_tmp)
                                    final_outcomes[idx_1[k_i], b] = (
                                        not final_outcomes[idx_1[k_i], b]
                                    )

            if not is_mpad and has_leaked:
                leaked_zero_mask = self._scratch_set_zero
                leaked_zero_mask.fill(False)
                leaked_zero_mask[targets_arr, :] = target_leakage_state > 1
                tss.do_unmatched_noisy_reset_z(
                    leaked_zero_mask, self._scratch_zero_mask, pauli=basis, order=targets
                )

            if has_leaked:
                for b in range(tss.batch_size):
                    tr = tss._tab_rngs[b]
                    tr_x = (
                        tss._tab_rngs_x[b]
                        if tss._tab_rngs_x is not None
                        else None
                    )
                    st_b = target_leakage_state[:, b]
                    t_leaked_idx = np.where(st_b > 1)[0]
                    if len(t_leaked_idx) > 0:
                        max_leak_b = int(np.max(st_b))
                        for n in range(2, max_leak_b + 1):
                            if n in st_b and n in params.prob_for_input_state:
                                idx_n = t_leaked_idx[st_b[t_leaked_idx] == n]
                                q_n_list = [int(targets_arr[k]) for k in idx_n]
                                prob_n = params.prob_for_input_state[n]
                                if prob_n > 0:
                                    inst_n = stim.CircuitInstruction(
                                        "X_ERROR", q_n_list, [prob_n]
                                    )
                                    tr.do(inst_n)
                                    if tr_x is not None:
                                        tr_x.do(inst_n)
                                    for k_idx, q_n in zip(idx_n, q_n_list):
                                        if tr.peek_z(q_n) == -1:
                                            tr.x(q_n)
                                            final_outcomes[int(k_idx), b] = True

            if is_mpad:
                tss._next_step_meta(op)
                inv_arr = np.asarray(inv_flags, dtype=np.bool_)[:, None]
                tss.record_direct_measurements(final_outcomes ^ inv_arr)
            else:
                tss._do_bare_instruction(
                    stim.CircuitInstruction(op.name, raw_op_targets)
                )
                num_m = len(targets)
                net_x_flip = np.zeros(
                    (self.num_qubits, tss.batch_size), dtype=np.bool_
                )
                for k_i, q in enumerate(targets):
                    m_col = tss._measurement_columns[-num_m + k_i]
                    inv_k = inv_flags[k_i]
                    target_out = final_outcomes[k_i] ^ inv_k
                    diff = m_col ^ target_out
                    if np.any(diff):
                        m_col[:] = target_out
                        leaked_diff = diff & (target_leakage_state[k_i] > 1)
                        if np.any(leaked_diff):
                            net_x_flip[q, :] ^= leaked_diff
                if np.any(net_x_flip):
                    tss._flip_simulator.broadcast_pauli_errors(
                        pauli=("Z" if basis == "X" else "X"),
                        mask=net_x_flip,
                        p=1.0,
                    )
                if has_leaked:
                    leaked_per_shot = [
                        list(targets_arr[target_leakage_state[:, b] > 1])
                        for b in range(tss.batch_size)
                    ]
                    tss._sample_pauli_noise_via_tab_rng(
                        "DEPOLARIZE1", leaked_per_shot, [0.75]
                    )
            return

        if has_dup_targets:
            for k_i, q_val in enumerate(targets):
                f_k = first_occ[int(q_val)]
                if f_k < k_i:
                    in_c = target_leakage_state[k_i, :] < 2
                    pauli_states[k_i, in_c] = (
                        1 if is_reset_meas else pauli_states[f_k, in_c]
                    )

        if not is_mpad:
            if np.any(target_leakage_state > 1):
                leaked_zero_mask = self._scratch_set_zero
                leaked_zero_mask.fill(False)
                leaked_zero_mask[targets_arr, :] = target_leakage_state > 1
                tss.do_unmatched_noisy_reset_z(
                    leaked_zero_mask, self._scratch_zero_mask, pauli=basis
                )
            # Execute the matched measurement first so the reference tableau and active cosets collapse in O(1) time
            tss._do_bare_instruction(
                stim.CircuitInstruction(op.name, raw_op_targets)
            )

        prob_Read1 = np.zeros(
            (len(targets), tss.batch_size), dtype=np.float64
        )
        in_comp = target_leakage_state < 2
        prob_Read1[in_comp & (pauli_states == 1)] = p0
        prob_Read1[in_comp & (pauli_states == -1)] = p1
        prob_Read1[in_comp & (pauli_states == 0)] = p_super
        max_leak = int(np.max(target_leakage_state))
        for n in range(2, max_leak + 1):
            if n in params.prob_for_input_state:
                prob_Read1[target_leakage_state == n] = (
                    params.prob_for_input_state[n]
                )

        if is_mpad:
            # MPAD[LEAKAGE_MEASUREMENT]: non-destructive readout on ancilla record
            tss._next_step_meta(op)
            outcomes = (
                tss.np_rng.random((len(targets), tss.batch_size)) < prob_Read1
            )
            inv_arr = np.asarray(inv_flags, dtype=np.bool_)[:, None]
            tss.record_direct_measurements(outcomes ^ inv_arr)
            return

        # LEAKAGE_PROJECTION_Z: fast vectorized update after matched measurement
        num_m = len(targets)
        for k_i in range(num_m):
            m_col = tss._measurement_columns[-num_m + k_i]
            m_phys = m_col ^ inv_flags[k_i]
            super_k = in_comp[k_i] & (pauli_states[k_i] == 0)
            if np.any(super_k):
                prob_Read1[k_i, super_k & (~m_phys)] = p0
                prob_Read1[k_i, super_k & m_phys] = p1
        need_resample = (
            (in_comp & (pauli_states == 1) & (p0 > 0.0))
            | (in_comp & (pauli_states == -1) & (p1 < 1.0))
            | (in_comp & (pauli_states == 0) & ((p0 > 0.0) | (p1 < 1.0)))
            | (~in_comp)
        )
        if np.any(need_resample):
            sampled_outcomes = (
                tss.np_rng.random((len(targets), tss.batch_size)) < prob_Read1
            )
            for k_i, q in enumerate(targets):
                mask_k = need_resample[k_i]
                if not np.any(mask_k):
                    continue
                m_col = tss._measurement_columns[-num_m + k_i]
                inv_k = inv_flags[k_i]
                new_col = np.where(mask_k, sampled_outcomes[k_i] ^ inv_k, m_col)
                m_col[:] = new_col
        if np.any(~in_comp):
            to_dep = self._scratch_depolarize
            to_dep.fill(False)
            to_dep[targets_arr, :] = ~in_comp
            tss._flip_simulator.broadcast_pauli_errors(
                pauli="X", mask=to_dep, p=0.5
            )
            tss._flip_simulator.broadcast_pauli_errors(
                pauli="Z", mask=to_dep, p=0.5
            )
