from typing import cast
import dataclasses

import numpy as np
from numpy.typing import NDArray
import stim  # type: ignore[import-untyped]

from stimside.op_handlers.abstract_op_handler import CompiledOpHandler, OpHandler
from stimside.op_handlers.leakage_handlers.leakage_parameters import (
    LeakageParams,
    LeakageTransition1Params,
    LeakageTransition2Params,
    LeakageConditioningParams,
    LeakageMeasurementParams,
    LeakageSwapParams,
)
from stimside.simulator_tableau import TablesideSimulator
from stimside.util.numpy_types import Bool2DArray, Int2DArray
from stimside.util.stim_workarounds import split_fused_instruction


@dataclasses.dataclass
class LeakageUint8(OpHandler[TablesideSimulator]):
    """Implementing leakage using an array of 8bit Uints.

    Basically, we're straight up storing the leakage state for each qubit.
    Pro: hard to mess up
        Supports leakage states up to 255
        Easy to store and work with using numpy
    Con: probably not as fast as using a bool array for each leakage level we care about
        We spend a reasonable amount of time computing masks like state==2

    We leave state == 0 and 1 alone so as to not be confusing. 2 means 2.

    Args:
        unconditional_condition_on_U: if True (default), untagged unitary gates and
            noise channels are treated as conditioned on U, i.e. they skip leaked
            qubits, whose computational state stays frozen from when they leaked.
            If False, they act on that frozen state as if the qubits were unleaked.
            Instructions that produce measurement results (including heralded noise)
            and resets are never filtered: they act on, or read, the frozen state.
            Details are in "Untagged Instructions on Leaked Qubits" in the top-level
            README.
    """

    def __init__(self, unconditional_condition_on_U: bool = True) -> None:
        self.unconditional_condition_on_U = unconditional_condition_on_U

    def compile_op_handler(
            self, *, circuit: stim.Circuit, batch_size: int
            ) -> "CompiledLeakageUint8":
        from stimside.util.tableside_kernels import get_precompiled_circuit

        pre = get_precompiled_circuit(circuit, self.unconditional_condition_on_U)

        handler = CompiledLeakageUint8(
            num_qubits=circuit.num_qubits,
            batch_size=batch_size,
            ops_to_params=pre.parsed_ops,
            unconditional_condition_on_U=self.unconditional_condition_on_U,
        )
        handler._precompiled_circuit = pre
        return handler


@dataclasses.dataclass
class CompiledLeakageUint8(CompiledOpHandler[TablesideSimulator]):

    num_qubits: int
    batch_size: int
    unconditional_condition_on_U: bool

    state: NDArray[np.uint8] = dataclasses.field(init=False)

    ops_to_params: dict[stim.CircuitInstruction, LeakageParams]
    _precompiled_circuit: object | None = dataclasses.field(
        default=None, init=False, repr=False
    )

    def __post_init__(self):
        self.claimed_ops_keys = set(self.ops_to_params.keys())
        self.state = np.zeros(self.num_qubits, dtype=np.uint8)

    def clear(self):
        """Clear the leakage state array in place (preserving any C++ shared buffer binding)."""
        if hasattr(self, "state") and self.state is not None and len(self.state) == self.num_qubits:
            self.state.fill(0)
        else:
            self.state = np.zeros(self.num_qubits, dtype=np.uint8)

    def make_target_mask(self, op: stim.CircuitInstruction) -> Bool2DArray:
        """return a bool mask that is true if this qubit is in the op targets."""
        target_indices = [
            t.qubit_value if t.qubit_value is not None else t.value
            for t in op.targets_copy()
        ]
        target_mask = np.zeros_like(self.state, dtype=bool)
        target_mask[target_indices] = 1
        return target_mask

    def _update_state_from_peek_Z(
        self, targets: list[int] | NDArray[np.int_], tss: TablesideSimulator
    ) -> tuple[NDArray[np.int_], NDArray[np.int_]]:
        """
        Update self.state for the targets based on the current Z pauli states.
        If a target is in a superposition, project its Z state in the tableau simulator
        to preserve multi-qubit stabilizer entanglement.
        """
        targets_arr = np.asarray(targets, dtype=np.intp)
        if len(targets_arr) == 0:
            return np.array([], dtype=np.intp), np.array([], dtype=np.intp)
        _, unique_idx = np.unique(targets_arr, return_index=True)
        targets_arr = targets_arr[np.sort(unique_idx)]
        targets_in_qubit_space = targets_arr[self.state[targets_arr] < 2]
        pauli_states = np.empty(len(targets_in_qubit_space), dtype=np.intp)
        new_state = np.zeros(len(targets_in_qubit_space), dtype=np.uint8)
        z_dephase_qubits: list[int] = []

        for idx, q in enumerate(targets_in_qubit_space):
            pz = tss.peek_z(int(q))
            pauli_states[idx] = pz
            if pz == -1:
                new_state[idx] = 1

        superpos_indices = np.where(pauli_states == 0)[0]
        for s_i, idx in enumerate(superpos_indices):
            q_int = int(targets_in_qubit_space[idx])
            pz = 0 if s_i == 0 else tss.peek_z(q_int)
            pauli_states[idx] = pz
            if pz == -1:
                new_state[idx] = 1
            elif pz == 0:
                bit = int(tss.np_rng.binomial(1, 0.5))
                new_state[idx] = bit
                tss._tableau_simulator.postselect_z(q_int, desired_value=bool(bit))
                z_dephase_qubits.append(q_int)

        if z_dephase_qubits and tss.batch_size > 1:
            tss._new_circuit.append(
                self._construct_stim_instruction("Z_ERROR", z_dephase_qubits, [0.5])
            )

        self.state[targets_in_qubit_space] = new_state
        return targets_in_qubit_space, pauli_states

    def _filter_targets_1q_mask(
        self,
        targets: list[int] | NDArray[np.int_],
        allowed_st_list: tuple[tuple[int | str, ...]],
        tss: TablesideSimulator,
    ) -> NDArray[np.bool_]:
        """
        Filter single qubit targets based on allowed states.
        If 0 or 1 is in allowed_st_list, update the state from the current Z pauli states.
        Return only the targets that are in the allowed states.
        """
        if allowed_st_list == (("U",),) and not np.any(self.state >= 2):
            return np.ones(len(targets), dtype=np.bool_)

        targets_arr = np.asarray(targets, dtype=np.intp)
        valid_qs = targets_arr[targets_arr >= 0]
        if len(valid_qs) > 0 and (
            1 in allowed_st_list[0] or 0 in allowed_st_list[0]
        ):
            self._update_state_from_peek_Z(valid_qs, tss)

        safe_qs = np.where(targets_arr >= 0, targets_arr, 0)
        target_states = np.where(targets_arr < 0, 0, self.state[safe_qs])
        self.state[self.state == 1] = 0
        is_classical = targets_arr < 0
        allowed_mask = np.zeros(len(targets_arr), dtype=np.bool_)
        if "U" in allowed_st_list[0]:
            allowed_mask |= is_classical | (target_states < 2)
        for st in allowed_st_list[0]:
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

    def _filter_targets_2q(
        self,
        targets: list[list[int]] | list[list[stim.GateTarget]] | Int2DArray,
        allowed_st_list: tuple[tuple[int | str, ...], tuple[int | str, ...]],
        tss: TablesideSimulator,
    ) -> list[int] | list[stim.GateTarget] | NDArray[np.int_]:
        """
        Filter two-qubit op targets based on allowed states.
        If 0 or 1 is in allowed_st_list, update the state from the current Z pauli states.
        Return only the 2-qubit targets that are in the allowed states.
        """
        if len(targets) == 0:
            return []
        has_gate_targets = isinstance(targets[0][0], stim.GateTarget)
        num_pairs = len(targets)
        q_pairs: list[tuple[int | None, int | None]] = []
        for pair in targets:
            q0 = (
                pair[0].qubit_value
                if isinstance(pair[0], stim.GateTarget)
                else int(pair[0])
            )
            q1 = (
                pair[1].qubit_value
                if isinstance(pair[1], stim.GateTarget)
                else int(pair[1])
            )
            q_pairs.append((q0, q1))

        qs_to_peek: list[int] = []
        for i in [0, 1]:
            if 1 in allowed_st_list[i] or 0 in allowed_st_list[i]:
                qs_to_peek.extend(
                    p[i] for p in q_pairs if p[i] is not None and p[i] >= 0
                )
        if len(qs_to_peek) > 0:
            self._update_state_from_peek_Z(qs_to_peek, tss)

        target_states = np.zeros((num_pairs, 2), dtype=np.intp)
        is_classical = np.zeros((num_pairs, 2), dtype=bool)
        for p_idx, (q0, q1) in enumerate(q_pairs):
            if q0 is None or q0 < 0:
                is_classical[p_idx, 0] = True
            else:
                target_states[p_idx, 0] = self.state[q0]
            if q1 is None or q1 < 0:
                is_classical[p_idx, 1] = True
            else:
                target_states[p_idx, 1] = self.state[q1]
        self.state[self.state == 1] = 0
        allowed_mask = np.zeros(num_pairs, dtype=bool)

        for n in range(len(allowed_st_list[0])):
            allowed_mask_pairs = np.zeros((num_pairs, 2), dtype=bool)
            for i in [0, 1]:
                st = allowed_st_list[i][n]
                if st == "U":
                    allowed_mask_pairs[:, i] = is_classical[:, i] | (
                        target_states[:, i] < 2
                    )
                elif isinstance(st, int):
                    allowed_mask_pairs[:, i] = (is_classical[:, i] & (st < 2)) | (
                        ~is_classical[:, i] & (target_states[:, i] == st)
                    )
                else:
                    raise ValueError(
                        f"Unrecognised allowed state {st} in allowed_st_list {allowed_st_list}"
                    )
            allowed_mask = np.logical_or(
                allowed_mask,
                np.logical_and(allowed_mask_pairs[:, 0], allowed_mask_pairs[:, 1]),
            )

        if has_gate_targets:
            return [
                t
                for pair, keep in zip(targets, allowed_mask)
                if keep
                for t in pair
            ]
        targets_np = np.asarray(targets)
        return targets_np[allowed_mask].flatten()

    def _construct_stim_instruction(
        self,
        op_name: str,
        targets: list[int] | list[stim.GateTarget] | np.ndarray,
        gate_args_copy: list[float] = [],
    ) -> stim.CircuitInstruction:
        """construct a stim CircuitInstruction from name, targets, and args."""
        return stim.CircuitInstruction(op_name, targets, gate_args_copy)

    def handle_op(self, op: stim.CircuitInstruction, sss: TablesideSimulator):
        """handle a single stim CircuitInstruction."""
        if op not in self.claimed_ops_keys:
            if self.unconditional_condition_on_U and np.any(self.state >= 2):
                op_name = op.name
                gate_data = stim.gate_data(op_name)
                if gate_data.produces_measurements or not (
                    gate_data.is_noisy_gate or gate_data.is_unitary
                ):
                    sss._do_bare_instruction(op)
                    return

                if gate_data.is_single_qubit_gate or op_name in (
                    "E",
                    "CORRELATED_ERROR",
                    "ELSE_CORRELATED_ERROR",
                ):
                    raw_targets = op.targets_copy()
                    qubit_indices = [
                        t.qubit_value if t.qubit_value is not None else -1
                        for t in raw_targets
                    ]
                    mask = self._filter_targets_1q_mask(qubit_indices, (("U",),), sss)
                    if not gate_data.is_single_qubit_gate and not np.all(mask):
                        # A correlated error is one Pauli product (like a pair for 2q ops): skip all of it.
                        mask = np.zeros_like(mask)
                    filtered_targets = [
                        t for t, keep in zip(raw_targets, mask) if keep
                    ]
                elif gate_data.is_two_qubit_gate:
                    target_groups = op.target_groups()
                    filtered_targets = self._filter_targets_2q(
                        target_groups, (("U",), ("U",)), sss
                    )
                elif op_name in ("SPP", "SPP_DAG"):
                    # Keep each Pauli product term only if none of its qubits is leaked.
                    filtered_targets = []
                    for term in op.target_groups():
                        if all(self.state[t.qubit_value] < 2 for t in term):
                            for k, t in enumerate(term):
                                if k > 0:
                                    filtered_targets.append(stim.target_combiner())
                                filtered_targets.append(t)
                else:
                    sss._do_bare_instruction(op)
                    return
                if len(filtered_targets) > 0:
                    op_new = self._construct_stim_instruction(
                        op_name, filtered_targets, op.gate_args_copy()
                    )
                    sss._do_bare_instruction(op_new)
                elif op_name in (
                    "E",
                    "CORRELATED_ERROR",
                    "ELSE_CORRELATED_ERROR",
                ):
                    sss._do_bare_instruction(
                        self._construct_stim_instruction(
                            op_name, [], op.gate_args_copy()
                        )
                    )
                return
            else:
                sss._do_bare_instruction(op)
                return

        params = self.ops_to_params[op]
        is_transition = isinstance(
            params, (LeakageTransition1Params, LeakageTransition2Params, LeakageSwapParams)
        )
        rec_ev = is_transition and getattr(sss, "record_leakage_events", False)
        if (sss.record_unleaked_to_leaked or rec_ev) and is_transition:
            target_indices = np.unique(
                [
                    t.qubit_value if t.qubit_value is not None else t.value
                    for t in op.targets_copy()
                    if (t.qubit_value if t.qubit_value is not None else t.value) >= 0
                ]
            )
            was_unleaked = (self.state[target_indices] < 2).copy()
            old_states = self.state[target_indices].copy() if rec_ev else None
        else:
            target_indices = None
            was_unleaked = None
            old_states = None

        match params:
            case LeakageConditioningParams():
                self.leakage_conditioning(op=op, tss=sss, params=params)
            case LeakageTransition1Params():
                self.leakage_transition_1(op=op, tss=sss, params=params)
            case LeakageTransition2Params():
                self.leakage_transition_2(op=op, tss=sss, params=params)
            case LeakageMeasurementParams():
                self.leakage_measurement(op, tss=sss, params=params)
            case LeakageSwapParams():
                self.leakage_swap(op=op, tss=sss)
            case _:
                raise ValueError(f"Unrecognised LEAKAGE params: {params}")

        if (
            sss.record_unleaked_to_leaked
            and was_unleaked is not None
            and target_indices is not None
        ):
            k = int(np.count_nonzero(was_unleaked & (self.state[target_indices] >= 2)))
            if k > 0:
                sss._record_unleaked_to_leaked_count(k, sss._circuit_time)
        if old_states is not None and target_indices is not None:
            for q, old_st, new_st in zip(
                target_indices, old_states, self.state[target_indices]
            ):
                if old_st != new_st:
                    sss._record_leakage_event(sss._circuit_time, q, old_st, new_st)

    def leakage_swap(self, op: stim.CircuitInstruction, tss: TablesideSimulator):
        """SWAP[LEAKAGE_SWAP]: SWAP the qubits, leaked ones included (their frozen computational
        states move with them), then swap their leakage states, pair by pair from left to right.
        """
        tss._do_bare_instruction(
            self._construct_stim_instruction(op.name, op.targets_copy(), op.gate_args_copy())
        )
        targets = [t.qubit_value for t in op.targets_copy()]
        for q0, q1 in zip(targets[::2], targets[1::2]):
            self.state[[q0, q1]] = self.state[[q1, q0]]

    def leakage_conditioning(
        self,
        op: stim.CircuitInstruction,
        tss: TablesideSimulator,
        params: LeakageConditioningParams,
    ):
        """implement conditioning on qubit (leakage) state"""

        if params.targets:
            copy_len = len(params.targets) * (2 if stim.gate_data(op.name).is_two_qubit_gate else 1)
            copies = split_fused_instruction(op, copy_len)
            if len(copies) > 1:
                for sub_op in copies:
                    self.leakage_conditioning(sub_op, tss, params)
                return

        # controller state is an array where each qubit has the leakage state
        # of the qubit controlling it (or 0 if it's not being targeted)
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
        if all(g == ("U",) for g in condition_groups) and not np.any(self.state >= 2):
            tss._do_bare_instruction(op)
            return

        gate_data = stim.gate_data(op.name)
        other_targets = params.targets
        if (
            any(0 in g or 1 in g for g in condition_groups)
            and op.name
            not in ("E", "CORRELATED_ERROR", "ELSE_CORRELATED_ERROR")
        ):
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
                    for g_idx, grp in enumerate(t_groups):
                        sub_other: tuple[int, ...] | None = None
                        if other_targets:
                            step_ot = len(other_targets) // len(t_groups)
                            sub_other = tuple(
                                other_targets[
                                    g_idx * step_ot : (g_idx + 1) * step_ot
                                ]
                            )
                        sub_params = LeakageConditioningParams(
                            args=condition_groups,
                            targets=sub_other,
                            from_tag=params.from_tag,
                        )
                        sub_op = self._construct_stim_instruction(
                            op.name, grp, op.gate_args_copy()
                        )
                        self.leakage_conditioning(sub_op, tss, sub_params)
                    return
        if other_targets:
            # CONDITIONED_ON_OTHER case
            raw_targets = op.targets_copy()
            if gate_data.is_two_qubit_gate:
                num_pairs = len(raw_targets) // 2
                if len(other_targets) == num_pairs:
                    pair_mask = self._filter_targets_1q_mask(
                        list(other_targets),
                        cast(tuple[tuple[int | str, ...]], condition_groups),
                        tss,
                    )
                elif len(other_targets) == len(raw_targets):
                    if len(condition_groups) == 1:
                        mask = self._filter_targets_1q_mask(
                            list(other_targets),
                            cast(tuple[tuple[int | str, ...]], condition_groups),
                            tss,
                        )
                        pair_mask = mask[0::2] & mask[1::2]
                    else:
                        other_pairs = [
                            [other_targets[i], other_targets[i + 1]]
                            for i in range(0, len(other_targets), 2)
                        ]
                        filtered_targets = []
                        target_groups = op.target_groups()
                        for p_idx in range(num_pairs):
                            op_p = self._filter_targets_2q(
                                [other_pairs[p_idx]],
                                cast(
                                    tuple[tuple[int | str, ...], tuple[int | str, ...]],
                                    condition_groups,
                                ),
                                tss,
                            )
                            if len(op_p) > 0:
                                filtered_targets.extend(target_groups[p_idx])
                        pair_mask = None
                else:
                    raise ValueError(
                        f"The number of targets of the {op} is not compatible with the "
                        "number of targets specified in the CONDITIONED_ON_OTHER tag."
                    )
                if pair_mask is not None:
                    target_groups = op.target_groups()
                    filtered_targets = [
                        t
                        for grp, keep in zip(target_groups, pair_mask)
                        if keep
                        for t in grp
                    ]
            else:
                if len(other_targets) != len(raw_targets):
                    raise ValueError(
                        f"The number of targets of the {op} is not the same as the"
                        "number of targets specified in the CONDITIONED_ON_OTHER tag."
                    )
                mask = self._filter_targets_1q_mask(
                    list(other_targets),
                    cast(tuple[tuple[int | str, ...]], condition_groups),
                    tss,
                )
                filtered_targets = [
                    t for t, keep in zip(raw_targets, mask) if keep
                ]
        else:
            if len(condition_groups) == 1:
                # CONDITIONED_ON_SELF / single-group CONDITIONED_ON case
                raw_targets = op.targets_copy()
                qubit_indices = [
                    t.qubit_value if t.qubit_value is not None else -1
                    for t in raw_targets
                ]
                mask = self._filter_targets_1q_mask(
                    qubit_indices, condition_groups, tss
                )
                if gate_data.is_two_qubit_gate:
                    pair_mask = mask[0::2] & mask[1::2]
                    target_groups = op.target_groups()
                    filtered_targets = [
                        t
                        for grp, keep in zip(target_groups, pair_mask)
                        if keep
                        for t in grp
                    ]
                else:
                    filtered_targets = [
                        t for t, keep in zip(raw_targets, mask) if keep
                    ]
            else:
                # CONDITIONED_ON_PAIR case
                target_groups = op.target_groups()
                filtered_targets = self._filter_targets_2q(
                    target_groups, condition_groups, tss
                )

        if len(filtered_targets) > 0:
            tss._do_bare_instruction(
                self._construct_stim_instruction(
                    op.name, filtered_targets, op.gate_args_copy()
                )
            )
        elif op.name in (
            "E",
            "CORRELATED_ERROR",
            "ELSE_CORRELATED_ERROR",
        ):
            tss._do_bare_instruction(
                self._construct_stim_instruction(
                    op.name, [], op.gate_args_copy()
                )
            )

    def leakage_transition_1(
        self,
        op: stim.CircuitInstruction,
        tss: TablesideSimulator,
        params: LeakageTransition1Params,
    ):
        """implement leakage state transitions on single qubits.

        Allows depolarization when something comes unleaked and is specified by 'U'
        """

        raw_t = op.targets_copy()
        if len(raw_t) > 1:
            qs_list = [t.qubit_value for t in raw_t if t.qubit_value is not None]
            if len(set(qs_list)) < len(qs_list):
                for t in raw_t:
                    sub_op = self._construct_stim_instruction(
                        op.name, [t], op.gate_args_copy()
                    )
                    self.leakage_transition_1(sub_op, tss, params)
                return

        target_mask = self.make_target_mask(op)
        sync_rng = getattr(tss, "_sync_tableside_rng", False)

        if 0 in params.args_by_input_state or 1 in params.args_by_input_state:
            self._update_state_from_peek_Z(np.where(target_mask)[0], tss)

        initial_state = self.state.copy()

        to_depolarize = np.zeros_like(self.state, dtype=bool)
        to_set_to_one = np.zeros_like(self.state, dtype=bool)
        to_set_to_zero = np.zeros_like(self.state, dtype=bool)

        for input_state in params.args_by_input_state.keys():
            if input_state == "U":
                input_state_mask = initial_state < 2
            else:
                input_state_mask = initial_state == input_state

            overwrite_mask = np.logical_and(target_mask, input_state_mask)

            samples_to_take = np.count_nonzero(overwrite_mask)
            if samples_to_take == 0:
                continue

            transitions = params.args_by_input_state[input_state]
            total_p = sum(p for _, p in transitions)
            if not sync_rng and total_p <= 0.0:
                continue

            if not sync_rng and total_p < 0.05 and len(transitions) >= 1:
                n_hits = int(tss.np_rng.binomial(samples_to_take, total_p))
                if n_hits == 0:
                    continue
                overwrite_indices = np.where(overwrite_mask)[0]
                chosen_indices = overwrite_indices[
                    tss.np_rng.choice(samples_to_take, size=n_hits, replace=False)
                ]
                overwrite_mask = np.zeros_like(self.state, dtype=bool)
                overwrite_mask[chosen_indices] = True
                if len(transitions) == 1:
                    output_states = np.full(n_hits, str(transitions[0][0]))
                else:
                    cond_probs = [p / total_p for _, p in transitions]
                    br_idx = tss.np_rng.choice(len(transitions), size=n_hits, p=cond_probs)
                    output_states = np.array([str(transitions[int(b)][0]) for b in br_idx])
            else:
                output_states = params.sample_transitions_from_state(
                    input_state=input_state, num_samples=samples_to_take, np_rng=tss.np_rng
                ).astype(str)

            target_qs = np.where(overwrite_mask)[0]
            for k, q_np in enumerate(target_qs):
                q = int(q_np)
                out_st = str(output_states[k])
                if out_st == str(input_state):
                    self.state[q] = 0 if input_state in (0, 1, "U") else int(input_state)
                    continue
                in_is_leaked = int(initial_state[q]) >= 2
                if out_st == "0":
                    to_set_to_zero[q] |= True
                    self.state[q] = 0
                elif out_st == "1":
                    to_set_to_one[q] |= True
                    self.state[q] = 0
                elif out_st == "U":
                    if in_is_leaked:
                        to_set_to_zero[q] |= True
                    to_depolarize[q] |= True
                    self.state[q] = 0
                else:
                    out_int = int(out_st)
                    self.state[q] = 0 if out_int < 2 else out_int

        self.state[self.state == 1] = 0

        idx_to_set_to_zero = np.where(to_set_to_zero)[0]
        if len(idx_to_set_to_zero) > 0:
            tss._do_bare_instruction(
                self._construct_stim_instruction("R", idx_to_set_to_zero)
            )
        idx_to_set_to_one = np.where(to_set_to_one)[0]
        if len(idx_to_set_to_one) > 0:
            tss._do_bare_instruction(
                self._construct_stim_instruction("R", idx_to_set_to_one)
            )
            tss._do_bare_instruction(
                self._construct_stim_instruction("X", idx_to_set_to_one)
            )
        idx_to_depolarize = np.where(to_depolarize)[0]
        if len(idx_to_depolarize) > 0:
            tss._do_bare_instruction(
                self._construct_stim_instruction(
                    "DEPOLARIZE1", idx_to_depolarize, [0.75]
                )
            )

    def leakage_transition_2(
        self,
        op: stim.CircuitInstruction,
        tss: TablesideSimulator,
        params: LeakageTransition2Params,
    ):
        """implement leakage state transitions on pairs of qubits.

        when something unleaks, fully depolarize it
        """

        targets_list = [
            target.qubit_value if target.qubit_value is not None else target.value
            for target in op.targets_copy()
        ]
        if len(targets_list) > 2 and len(set(targets_list)) < len(targets_list):
            for grp in op.target_groups():
                sub_op = self._construct_stim_instruction(
                    op.name, grp, op.gate_args_copy()
                )
                self.leakage_transition_2(sub_op, tss, params)
            return

        targets_even = np.asarray(targets_list[::2], dtype=np.intp)
        targets_odd = np.asarray(targets_list[1::2], dtype=np.intp)
        sync_rng = getattr(tss, "_sync_tableside_rng", False)

        if any(k[0] in (0, 1) for k in params.args_by_input_state.keys()):
            self._update_state_from_peek_Z(targets_list[::2], tss)
        if any(k[1] in (0, 1) for k in params.args_by_input_state.keys()):
            self._update_state_from_peek_Z(targets_list[1::2], tss)

        initial_state = self.state.copy()
        even_target_states = initial_state[targets_even]
        odd_target_states = initial_state[targets_odd]

        to_depolarize = np.zeros_like(self.state, dtype=bool)
        to_pauli_X = np.zeros_like(self.state, dtype=bool)
        to_pauli_Y = np.zeros_like(self.state, dtype=bool)
        to_pauli_Z = np.zeros_like(self.state, dtype=bool)

        to_set_to_one = np.zeros_like(self.state, dtype=bool)
        to_set_to_zero = np.zeros_like(self.state, dtype=bool)

        # for accumulating which qubits need to have their X / Z randomized (for 'V')
        to_randomize_X = np.zeros_like(self.state, dtype=bool)
        randomize_phase = np.zeros_like(self.state, dtype=bool)

        for input_state in params.args_by_input_state.keys():
            if input_state[0] == "U":
                even_target_state_checks = even_target_states < 2
            else:
                even_target_state_checks = even_target_states == input_state[0]

            if input_state[1] == "U":
                odd_target_state_checks = odd_target_states < 2
            else:
                odd_target_state_checks = odd_target_states == input_state[1]

            trigger_mask = np.logical_and(
                even_target_state_checks, odd_target_state_checks
            )
            samples_to_take = int(np.count_nonzero(trigger_mask))
            if samples_to_take == 0:
                continue

            transitions = params.args_by_input_state[input_state]
            total_p = sum(p for _, p in transitions)
            if not sync_rng and total_p <= 0.0:
                continue

            triggered_pairs = np.column_stack(
                (targets_even[trigger_mask], targets_odd[trigger_mask])
            )

            if not sync_rng and total_p < 0.05 and len(transitions) >= 1:
                n_hits = int(tss.np_rng.binomial(samples_to_take, total_p))
                if n_hits == 0:
                    continue
                chosen = tss.np_rng.choice(samples_to_take, size=n_hits, replace=False)
                triggered_pairs = triggered_pairs[chosen]
                samples_to_take = n_hits
                if len(transitions) == 1:
                    out_pair = transitions[0][0]
                    output_states = np.tile(
                        np.array([str(out_pair[0]), str(out_pair[1])]),
                        (n_hits, 1),
                    )
                else:
                    cond_probs = [p / total_p for _, p in transitions]
                    br_idx = tss.np_rng.choice(len(transitions), size=n_hits, p=cond_probs)
                    output_states = np.array(
                        [[str(transitions[int(b)][0][0]), str(transitions[int(b)][0][1])] for b in br_idx]
                    )
            else:
                output_states = params.sample_transitions_from_state(
                    input_state=input_state, num_samples=samples_to_take, np_rng=tss.np_rng
                ).astype(str)

            for k in range(samples_to_take):
                for leg in (0, 1):
                    q = int(triggered_pairs[k, leg])
                    in_st = input_state[leg]
                    out_st = output_states[k, leg]
                    if out_st == str(in_st):
                        self.state[q] = 0 if in_st in (0, 1, "U") else int(in_st)
                        continue
                    in_is_leaked = not (in_st in (0, 1, "U"))

                    if out_st == "0":
                        to_set_to_zero[q] |= True
                        self.state[q] = 0
                    elif out_st == "1":
                        to_set_to_one[q] |= True
                        self.state[q] = 0
                    elif out_st == "V":
                        to_set_to_zero[q] |= True
                        to_randomize_X[q] |= True
                        randomize_phase[q] |= True
                        self.state[q] = 0
                    elif out_st == "D":
                        if in_is_leaked:
                            to_set_to_zero[q] |= True
                        to_depolarize[q] |= True
                        self.state[q] = 0
                    elif out_st == "U":
                        if in_is_leaked:
                            to_set_to_zero[q] |= True
                        to_depolarize[q] |= True
                        self.state[q] = 0
                    elif out_st == "X":
                        if in_is_leaked:
                            to_set_to_zero[q] |= True
                        to_pauli_X[q] |= True
                        self.state[q] = 0
                    elif out_st == "Y":
                        if in_is_leaked:
                            to_set_to_zero[q] |= True
                        to_pauli_Y[q] |= True
                        self.state[q] = 0
                    elif out_st == "Z":
                        if in_is_leaked:
                            to_set_to_zero[q] |= True
                        to_pauli_Z[q] |= True
                        self.state[q] = 0
                    else:
                        out_int = int(out_st)
                        self.state[q] = 0 if out_int < 2 else out_int

        self.state[self.state == 1] = 0

        idx_to_set_to_zero = np.where(to_set_to_zero)[0]
        if len(idx_to_set_to_zero) > 0:
            tss._do_bare_instruction(
                self._construct_stim_instruction("R", idx_to_set_to_zero)
            )
        idx_to_set_to_one = np.where(to_set_to_one)[0]
        if len(idx_to_set_to_one) > 0:
            tss._do_bare_instruction(
                self._construct_stim_instruction("R", idx_to_set_to_one)
            )
            tss._do_bare_instruction(
                self._construct_stim_instruction("X", idx_to_set_to_one)
            )
        idx_to_depolarize = np.where(to_depolarize)[0]
        if len(idx_to_depolarize) > 0:
            tss._do_bare_instruction(
                self._construct_stim_instruction(
                    "DEPOLARIZE1", idx_to_depolarize, [0.75]
                )
            )
        idx_to_pauli_X = np.where(to_pauli_X)[0]
        if len(idx_to_pauli_X) > 0:
            tss._do_bare_instruction(
                self._construct_stim_instruction("X", idx_to_pauli_X)
            )
        idx_to_pauli_Y = np.where(to_pauli_Y)[0]
        if len(idx_to_pauli_Y) > 0:
            tss._do_bare_instruction(
                self._construct_stim_instruction("Y", idx_to_pauli_Y)
            )
        idx_to_pauli_Z = np.where(to_pauli_Z)[0]
        if len(idx_to_pauli_Z) > 0:
            tss._do_bare_instruction(
                self._construct_stim_instruction("Z", idx_to_pauli_Z)
            )
        idx_to_randomize_X = np.where(to_randomize_X)[0]
        if len(idx_to_randomize_X) > 0:
            tss._do_bare_instruction(
                self._construct_stim_instruction(
                    "X_ERROR", idx_to_randomize_X, [0.5]
                )
            )
        idx_to_randomize_phase = np.where(randomize_phase)[0]
        if len(idx_to_randomize_phase) > 0:
            tss._do_bare_instruction(
                self._construct_stim_instruction(
                    "Z_ERROR", idx_to_randomize_phase, [0.5]
                )
            )

    def leakage_measurement(
        self,
        op: stim.CircuitInstruction,
        tss: TablesideSimulator,
        params: LeakageMeasurementParams,
    ):
        """implement qubit state projection measurement that can be affected by leakage."""

        if params.targets:
            copies = split_fused_instruction(op, len(params.targets))
            if len(copies) > 1:
                for sub_op in copies:
                    self.leakage_measurement(sub_op, tss, params)
                return

        targets: list[int]
        is_mpad = params.targets is not None
        raw_op_targets = op.targets_copy()
        ## The case of MPAD with LEAKAGE_MEASUREMENT tag
        if is_mpad:
            assert params.targets is not None
            targets = list(params.targets)
            if len(raw_op_targets) != len(targets):
                raise ValueError(
                    "The number of targets in the MPAD operation with a LEAKAGE_MEASUREMENT tag"
                    "does not equal to the number of targets specified in the tag."
                )
            if op.name != "MPAD":
                raise ValueError(
                    f"LEAKAGE_MEASUREMENT is only implemented for 'MPAD' operations, got {op}"
                )
        ## The case of M with LEAKAGE_PROJECTION_Z tag
        else:
            targets = [
                t.qubit_value if t.qubit_value is not None else t.value
                for t in raw_op_targets
            ]
            if op.name not in ("M", "MZ", "MR", "MRZ", "MX", "MY", "MRX", "MRY"):
                raise ValueError(
                    f"LEAKAGE_PROJECTION_Z is only implemented for M/MZ/MR/MRZ/MX/MY/MRX/MRY operations, got {op}"
                )

        if not is_mpad and len(set(targets)) < len(targets):
            for t in raw_op_targets:
                sub_op = self._construct_stim_instruction(
                    op.name, [t], op.gate_args_copy()
                )
                self.leakage_measurement(sub_op, tss, params)
            return

        basis_rot_gate = (
            "H"
            if op.name in ("MX", "MRX")
            else ("H_YZ" if op.name in ("MY", "MRY") else None)
        )
        is_reset_meas = op.name in ("MR", "MRZ", "MRX", "MRY")
        if basis_rot_gate is not None:
            tss._do_bare_only_on_tableau(
                self._construct_stim_instruction(basis_rot_gate, targets)
            )

        n_qbt = max(
            tss.num_qubits, tss.num_qubits_in_new_circuit(), max(targets) + 1
        )

        # get actual qubit states
        target_leakage_state = self.state[targets]

        p0 = params.prob_for_input_state.get(0, 0.0)
        p1 = params.prob_for_input_state.get(1, 0.0 if is_mpad else 1.0)

        targets_arr = np.asarray(targets, dtype=np.intp)
        idx_of_targets_in_qubit_space = np.where(target_leakage_state < 2)[0]
        targets_in_qubit_space = targets_arr[idx_of_targets_in_qubit_space]
        targets_new = targets_arr.copy()

        if len(targets_in_qubit_space) > 0:
            if not is_mpad and p0 == 0.0 and p1 == 1.0:
                # Standard Z measurement on unleaked qubits: measure directly without R or X_ERROR
                # so post-measurement state and multi-qubit entanglement are preserved.
                pass
            elif not is_mpad and np.isclose(p0, 1.0 - p1) and not getattr(tss, "_sync_tableside_rng", False):
                # Symmetric readout flip probability p0 on unleaked qubits:
                # Copy Z state coherently to fresh ancilla via CX and apply readout error on ancilla.
                anc_comp = list(
                    range(n_qbt, n_qbt + len(targets_in_qubit_space))
                )
                n_qbt += len(targets_in_qubit_space)
                targets_new[idx_of_targets_in_qubit_space] = anc_comp
                cx_targets = []
                for q_d, q_a in zip(targets_in_qubit_space, anc_comp):
                    cx_targets.extend([int(q_d), int(q_a)])
                tss._do_bare_only_on_tableau(
                    self._construct_stim_instruction("CX", cx_targets)
                )
                if p0 > 0:
                    tss._do_bare_only_on_tableau(
                        self._construct_stim_instruction("X_ERROR", anc_comp, [p0])
                    )
            else:
                # Asymmetric readout probabilities on 0 vs 1 (or MPAD)
                has_01_cond = (0 in params.prob_for_input_state) or (
                    1 in params.prob_for_input_state
                )
                if is_mpad and not has_01_cond and np.isclose(p0, p1):
                    anc_comp = list(
                        range(n_qbt, n_qbt + len(targets_in_qubit_space))
                    )
                    n_qbt += len(targets_in_qubit_space)
                    targets_new[idx_of_targets_in_qubit_space] = anc_comp
                    if p0 > 0:
                        tss._do_bare_only_on_tableau(
                            self._construct_stim_instruction(
                                "X_ERROR", anc_comp, [p0]
                            )
                        )
                elif is_mpad and tss.batch_size > 1 and hasattr(tss, "_asymmetric_readout_postproc"):
                    # batch_size > 1 frame-samples _new_circuit, so the readout must follow each shot's own Z value:
                    # copy it onto a fresh ancilla (measuring the ancilla Z-collapses a superposed target, as at
                    # batch_size == 1) and apply p(0)/p(1) per shot when the records are sampled.
                    anc_comp = list(
                        range(n_qbt, n_qbt + len(targets_in_qubit_space))
                    )
                    n_qbt += len(targets_in_qubit_space)
                    targets_new[idx_of_targets_in_qubit_space] = anc_comp
                    cx_targets = []
                    for q_d, q_a in zip(targets_in_qubit_space, anc_comp):
                        cx_targets.extend([int(q_d), int(q_a)])
                    tss._do_bare_only_on_tableau(
                        self._construct_stim_instruction("CX", cx_targets)
                    )
                    base_meas_idx = tss._new_circuit.num_measurements
                    for i_pos in idx_of_targets_in_qubit_space:
                        t_raw = raw_op_targets[int(i_pos)]
                        tss._asymmetric_readout_postproc.append(
                            (
                                int(base_meas_idx + i_pos),
                                float(p0),
                                float(p1),
                                bool(t_raw.value) ^ bool(t_raw.is_inverted_result_target),
                            )
                        )
                else:
                    # Resolve superposition Z states in tableau_simulator when batch_size == 1
                    pauli_states = np.empty(
                        len(targets_in_qubit_space), dtype=np.intp
                    )
                    for i_q, q in enumerate(targets_in_qubit_space):
                        pauli_states[i_q] = tss.peek_z(int(q))
                    if tss.batch_size == 1:
                        superpos_indices = np.where(pauli_states == 0)[0]
                        for s_i, i_q in enumerate(superpos_indices):
                            q_int = int(targets_in_qubit_space[i_q])
                            pz = 0 if s_i == 0 else tss.peek_z(q_int)
                            if pz == 0:
                                bit = int(tss.np_rng.binomial(1, 0.5))
                                tss._tableau_simulator.postselect_z(
                                    q_int, desired_value=bool(bit)
                                )
                                pz = -1 if bit == 1 else 1
                            pauli_states[i_q] = pz

                    anc_comp = list(
                        range(n_qbt, n_qbt + len(targets_in_qubit_space))
                    )
                    n_qbt += len(targets_in_qubit_space)
                    targets_new[idx_of_targets_in_qubit_space] = anc_comp
                    anc_comp_arr = np.asarray(anc_comp, dtype=np.intp)

                    if not is_mpad:
                        cx_targets = []
                        for q_d, q_a in zip(targets_in_qubit_space, anc_comp):
                            cx_targets.extend([int(q_d), int(q_a)])
                        tss._do_bare_only_on_tableau(
                            self._construct_stim_instruction("CX", cx_targets)
                        )

                    m0_mask = pauli_states == 1
                    m1_mask = pauli_states == -1
                    mflip_mask = pauli_states == 0

                    if np.any(m0_mask) and p0 > 0:
                        tss._do_bare_only_on_tableau(
                            self._construct_stim_instruction(
                                "X_ERROR", anc_comp_arr[m0_mask], [p0]
                            )
                        )
                    if np.any(m1_mask):
                        if is_mpad:
                            tss._do_bare_only_on_tableau(
                                self._construct_stim_instruction(
                                    "X", anc_comp_arr[m1_mask]
                                )
                            )
                        if 1.0 - p1 > 0:
                            tss._do_bare_only_on_tableau(
                                self._construct_stim_instruction(
                                    "X_ERROR", anc_comp_arr[m1_mask], [1.0 - p1]
                                )
                            )
                    if np.any(mflip_mask):
                        if is_mpad:
                            x_error_prob = 0.5 * p0 + 0.5 * p1
                            if x_error_prob > 0:
                                tss._do_bare_only_on_tableau(
                                    self._construct_stim_instruction(
                                        "X_ERROR",
                                        anc_comp_arr[mflip_mask],
                                        [x_error_prob],
                                    )
                                )
                        elif (
                            tss.batch_size > 1
                            and abs(p0 - (1.0 - p1)) > 1e-12
                            and hasattr(tss, "_asymmetric_readout_postproc")
                        ):
                            base_meas_idx = tss._new_circuit.num_measurements
                            for i_pos in idx_of_targets_in_qubit_space[mflip_mask]:
                                tss._asymmetric_readout_postproc.append(
                                    (
                                        int(base_meas_idx + i_pos),
                                        float(p0),
                                        float(p1),
                                        bool(
                                            raw_op_targets[
                                                int(i_pos)
                                            ].is_inverted_result_target
                                        ),
                                    )
                                )
                        else:
                            if p1 >= p0:
                                x_error_prob = 0.5 * p0 + 0.5 * (1.0 - p1)
                            else:
                                tss._do_bare_only_on_tableau(
                                    self._construct_stim_instruction(
                                        "X", anc_comp_arr[mflip_mask]
                                    )
                                )
                                x_error_prob = 0.5 * (1.0 - p0) + 0.5 * p1
                            if x_error_prob > 0:
                                tss._do_bare_only_on_tableau(
                                    self._construct_stim_instruction(
                                        "X_ERROR",
                                        anc_comp_arr[mflip_mask],
                                        [x_error_prob],
                                    )
                                )

        # Handle qubits in leaked states (> 1)
        target_leaked_pos = np.where(target_leakage_state > 1)[0]
        leaked_qubits = targets_arr[target_leaked_pos]
        if len(target_leaked_pos) > 0:
            if is_mpad:
                leaked_meas_targets = np.asarray(
                    list(range(n_qbt, n_qbt + len(target_leaked_pos))),
                    dtype=np.intp,
                )
                n_qbt += len(target_leaked_pos)
                targets_new[target_leaked_pos] = leaked_meas_targets
            else:
                leaked_meas_targets = leaked_qubits
                tss._do_bare_only_on_tableau(
                    self._construct_stim_instruction("R", leaked_meas_targets)
                )
            for n in range(2, int(np.max(target_leakage_state)) + 1):
                if n in target_leakage_state and n in params.prob_for_input_state:
                    idx_n = leaked_meas_targets[
                        target_leakage_state[target_leaked_pos] == n
                    ]
                    if params.prob_for_input_state[n] > 0:
                        tss._do_bare_only_on_tableau(
                            self._construct_stim_instruction(
                                "X_ERROR", idx_n, [params.prob_for_input_state[n]]
                            )
                        )

        # Finally perform qubit space measurement, preserving any target inversion (!)
        m_targets: list[int | stim.GateTarget] = []
        for i, q_new in enumerate(targets_new):
            is_inv = (
                (bool(raw_op_targets[i].value) ^ bool(raw_op_targets[i].is_inverted_result_target))
                if is_mpad
                else bool(raw_op_targets[i].is_inverted_result_target)
            )
            if is_inv:
                m_targets.append(stim.target_inv(int(q_new)))
            else:
                m_targets.append(int(q_new))

        tss._do_bare_only_on_tableau(
            self._construct_stim_instruction("M", m_targets)
        )
        if is_reset_meas and len(targets_in_qubit_space) > 0:
            tss._do_bare_only_on_tableau(
                self._construct_stim_instruction("R", targets_in_qubit_space)
            )
        if basis_rot_gate is not None:
            tss._do_bare_only_on_tableau(
                self._construct_stim_instruction(basis_rot_gate, targets)
            )
        if is_mpad:
            tss._append_to_new_reference_circuit(op)
        else:
            ref_m_targets = [
                stim.target_inv(int(q)) if raw_op_targets[i].is_inverted_result_target else int(q)
                for i, q in enumerate(targets)
            ]
            tss._append_to_new_reference_circuit(
                self._construct_stim_instruction(op.name, ref_m_targets)
            )
            if len(leaked_qubits) > 0:
                tss._do_bare_only_on_tableau(
                    self._construct_stim_instruction(
                        "DEPOLARIZE1", leaked_qubits, [0.75]
                    )
                )