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
)
from stimside.op_handlers.leakage_handlers.leakage_tag_parsing_tableau import (
    parse_leakage_in_circuit,
)
from stimside.simulator_tableau import TablesideSimulator
from stimside.util.numpy_types import Bool2DArray, Int2DArray


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
        assert (
            self.batch_size == 1
        ), "The tableau leakage op handler can only take one shot a time"
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
        if (
            len(valid_qs) > 0
            and (1 in allowed_st_list[0] or 0 in allowed_st_list[0])
            and not (1 in allowed_st_list[0] and 0 in allowed_st_list[0])
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
            if getattr(sss, "_sync_flipside_rng", False):
                sss._do_bare_instruction(op)
                if (
                    op.name
                    in (
                        "R",
                        "RX",
                        "RY",
                        "RZ",
                        "M",
                        "MX",
                        "MY",
                        "MZ",
                        "MR",
                        "MRX",
                        "MRY",
                        "MRZ",
                    )
                    and np.any(self.state >= 2)
                ):
                    leaked_qs = [
                        int(t.qubit_value)
                        for t in op.targets_copy()
                        if t.qubit_value is not None
                        and self.state[int(t.qubit_value)] >= 2
                    ]
                    if leaked_qs:
                        sss._do_bare_only_on_tableau(
                            self._construct_stim_instruction(
                                "DEPOLARIZE1", leaked_qs, [0.75]
                            )
                        )
                return
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
                    filtered_targets = [
                        t for t, keep in zip(raw_targets, mask) if keep
                    ]
                elif gate_data.is_two_qubit_gate:
                    target_groups = op.target_groups()
                    filtered_targets = self._filter_targets_2q(
                        target_groups, (("U",), ("U",)), sss
                    )
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
        match params:
            case LeakageConditioningParams():
                self.leakage_conditioning(op=op, tss=sss, params=params)
            case LeakageTransition1Params():
                self.leakage_transition_1(op=op, tss=sss, params=params)
            case LeakageTransition2Params():
                self.leakage_transition_2(op=op, tss=sss, params=params)
            case LeakageMeasurementParams():
                self.leakage_measurement(op, tss=sss, params=params)
            case _:
                raise ValueError(f"Unrecognised LEAKAGE params: {params}")

    def leakage_conditioning(
        self,
        op: stim.CircuitInstruction,
        tss: TablesideSimulator,
        params: LeakageConditioningParams,
    ):
        """implement conditioning on qubit (leakage) state"""

        # controller state is an array where each qubit has the leakage state
        # of the qubit controlling it (or 0 if it's not being targeted)
        condition_groups = params.args
        if all(g == ("U",) for g in condition_groups) and not np.any(self.state >= 2):
            tss._do_bare_instruction(op)
            return

        gate_data = stim.gate_data(op.name)
        other_targets = params.targets
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

        target_mask = self.make_target_mask(op)
        sync_rng = getattr(tss, "_sync_tableside_rng", False)
        sync_flip = getattr(tss, "_sync_flipside_rng", False)

        if sync_flip and op.name in ("R", "RZ"):
            tss._do_bare_instruction(
                self._construct_stim_instruction("R", np.where(target_mask)[0])
            )

        if (
            not sync_rng
            and set(params.args_by_input_state.keys()) <= {1}
            and 1 in params.args_by_input_state
            and len(params.args_by_input_state[1]) == 1
            and str(params.args_by_input_state[1][0][0])
            not in ("V", "D", "X", "Y", "Z")
        ):
            # Candidate binomial thinning before calling _update_state_from_peek_Z
            transitions = params.args_by_input_state[1]
            total_p = sum(p for _, p in transitions)
            if total_p <= 0.0:
                self.state[self.state == 1] = 0
                return
            comp_qs = np.where(target_mask & (self.state < 2))[0]
            if len(comp_qs) == 0:
                return
            k_cand = (
                len(comp_qs)
                if total_p >= 1.0 - 1e-12
                else int(tss.np_rng.binomial(len(comp_qs), total_p))
            )
            if k_cand == 0:
                self.state[self.state == 1] = 0
                return
            cands = (
                comp_qs
                if k_cand == len(comp_qs)
                else comp_qs[tss.np_rng.choice(len(comp_qs), size=k_cand, replace=False)]
            )
            self._update_state_from_peek_Z(cands, tss)
            ones_qs = cands[self.state[cands] == 1]
            self.state[self.state == 1] = 0
            if len(ones_qs) == 0:
                return
            out_st = transitions[0][0]
            if str(out_st) != "1":
                if out_st == "U":
                    tss._do_bare_instruction(
                        self._construct_stim_instruction("DEPOLARIZE1", ones_qs, [0.75])
                    )
                elif out_st == 0 or out_st == "0":
                    tss._do_bare_instruction(
                        self._construct_stim_instruction("R", ones_qs)
                    )
                else:
                    self.state[ones_qs] = int(out_st)
            return

        if 0 in params.args_by_input_state or 1 in params.args_by_input_state:
            if any(
                sum(p for _, p in params.args_by_input_state.get(k, ())) > 0.0
                for k in (0, 1)
            ) or sync_rng:
                self._update_state_from_peek_Z(np.where(target_mask)[0], tss)

        initial_state = self.state.copy()

        to_depolarize = np.zeros_like(self.state, dtype=bool)
        to_pauli_X = np.zeros_like(self.state, dtype=bool)
        to_pauli_Y = np.zeros_like(self.state, dtype=bool)
        to_pauli_Z = np.zeros_like(self.state, dtype=bool)
        to_set_to_one = np.zeros_like(self.state, dtype=bool)
        to_set_to_zero = np.zeros_like(self.state, dtype=bool)
        to_randomize_X = np.zeros_like(self.state, dtype=bool)
        randomize_phase = np.zeros_like(self.state, dtype=bool)

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
                if out_st == "0" and (not sync_flip or not in_is_leaked):
                    to_set_to_zero[q] |= True
                    self.state[q] = 0
                elif out_st == "1" and (not sync_flip or not in_is_leaked):
                    to_set_to_one[q] |= True
                    self.state[q] = 0
                elif out_st == "V":
                    if not sync_flip:
                        to_set_to_zero[q] |= True
                        to_randomize_X[q] |= True
                        randomize_phase[q] |= True
                    else:
                        to_depolarize[q] |= True
                    self.state[q] = 0
                elif out_st == "D":
                    if in_is_leaked and not sync_flip:
                        to_set_to_zero[q] |= True
                    to_depolarize[q] |= True
                    self.state[q] = 0
                elif out_st == "U":
                    if in_is_leaked and not sync_flip:
                        to_set_to_zero[q] |= True
                    to_depolarize[q] |= True
                    self.state[q] = 0
                elif out_st == "X":
                    if in_is_leaked and not sync_flip:
                        to_set_to_zero[q] |= True
                    to_pauli_X[q] |= True
                    self.state[q] = 0
                elif out_st == "Y":
                    if in_is_leaked and not sync_flip:
                        to_set_to_zero[q] |= True
                    to_pauli_Y[q] |= True
                    self.state[q] = 0
                elif out_st == "Z":
                    if in_is_leaked and not sync_flip:
                        to_set_to_zero[q] |= True
                    to_pauli_Z[q] |= True
                    self.state[q] = 0
                else:
                    out_int = int(out_st)
                    self.state[q] = 0 if out_int < 2 else out_int
                    if sync_flip and (
                        (out_int >= 2 and not in_is_leaked)
                        or (out_int < 2 and in_is_leaked)
                    ):
                        to_depolarize[q] |= True

        self.state[self.state == 1] = 0

        if sync_flip and op.name in ("R", "RZ"):
            to_depolarize |= np.logical_and(target_mask, self.state >= 2)

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
        if sync_flip and op.name not in ("I", "R", "RZ"):
            tss._do_bare_instruction(
                self._construct_stim_instruction(
                    op.name, op.targets_copy(), op.gate_args_copy()
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
        targets_even = np.asarray(targets_list[::2], dtype=np.intp)
        targets_odd = np.asarray(targets_list[1::2], dtype=np.intp)
        sync_rng = getattr(tss, "_sync_tableside_rng", False)
        sync_flip = getattr(tss, "_sync_flipside_rng", False)

        if any(0 in k or 1 in k for k in params.args_by_input_state.keys()):
            if any(
                sum(p for _, p in br_list) > 0.0
                for br_list in params.args_by_input_state.values()
            ) or sync_rng:
                self._update_state_from_peek_Z(targets_list, tss)

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

                    if out_st == "0" and (not sync_flip or not in_is_leaked):
                        to_set_to_zero[q] |= True
                        self.state[q] = 0
                    elif out_st == "1" and (not sync_flip or not in_is_leaked):
                        to_set_to_one[q] |= True
                        self.state[q] = 0
                    elif out_st == "V":
                        if not sync_flip:
                            to_set_to_zero[q] |= True
                            to_randomize_X[q] |= True
                            randomize_phase[q] |= True
                        else:
                            to_depolarize[q] |= True
                        self.state[q] = 0
                    elif out_st == "D":
                        if in_is_leaked and not sync_flip:
                            to_set_to_zero[q] |= True
                        to_depolarize[q] |= True
                        self.state[q] = 0
                    elif out_st == "U":
                        if in_is_leaked and not sync_flip:
                            to_set_to_zero[q] |= True
                        to_depolarize[q] |= True
                        self.state[q] = 0
                    elif out_st == "X":
                        if in_is_leaked and not sync_flip:
                            to_set_to_zero[q] |= True
                        if sync_flip:
                            to_pauli_X[q] ^= True
                        else:
                            to_pauli_X[q] |= True
                        self.state[q] = 0
                    elif out_st == "Y":
                        if in_is_leaked and not sync_flip:
                            to_set_to_zero[q] |= True
                        if sync_flip:
                            to_pauli_Y[q] ^= True
                        else:
                            to_pauli_Y[q] |= True
                        self.state[q] = 0
                    elif out_st == "Z":
                        if in_is_leaked and not sync_flip:
                            to_set_to_zero[q] |= True
                        if sync_flip:
                            to_pauli_Z[q] ^= True
                        else:
                            to_pauli_Z[q] |= True
                        self.state[q] = 0
                    else:
                        out_int = int(out_st)
                        self.state[q] = 0 if out_int < 2 else out_int
                        if sync_flip and (
                            (out_int >= 2 and not in_is_leaked)
                            or (out_int < 2 and in_is_leaked)
                        ):
                            to_depolarize[q] |= True

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
        if sync_flip and op.name != "I":
            tss._do_bare_instruction(
                self._construct_stim_instruction(
                    op.name, op.targets_copy(), op.gate_args_copy()
                )
            )

    def leakage_measurement(
        self,
        op: stim.CircuitInstruction,
        tss: TablesideSimulator,
        params: LeakageMeasurementParams,
    ):
        """implement qubit state projection measurement that can be affected by leakage."""

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
            if op.name not in ("M", "MZ", "MR", "MRZ"):
                raise ValueError(
                    f"LEAKAGE_PROJECTION_Z is only implemented for 'M' operations, got {op}"
                )

        if getattr(tss, "_sync_flipside_rng", False):
            m_idx = len(tss._tableau_simulator.current_measurement_record())
            if is_mpad:
                is_inverted = [
                    bool(t.value) ^ bool(t.is_inverted_result_target)
                    for t in raw_op_targets
                ]
                tss._do_bare_instruction(
                    self._construct_stim_instruction("MPAD", raw_op_targets, [])
                )
                rec = None
                p0 = params.prob_for_input_state.get(0, 0.0)
                p1_eff = params.prob_for_input_state.get(1, 0.0)
            else:
                is_inverted = [
                    bool(t.is_inverted_result_target) for t in raw_op_targets
                ]
                tss._do_bare_instruction(
                    self._construct_stim_instruction(
                        op.name, raw_op_targets, []
                    )
                )
                rec = tss._tableau_simulator.current_measurement_record()[
                    m_idx : m_idx + len(targets)
                ]
                p0 = params.prob_for_input_state.get(0, 0.0)
                p1_eff = params.prob_for_input_state.get(1, 1.0)

            for i, q_np in enumerate(targets):
                q = int(q_np)
                st = int(self.state[q])
                if is_mpad:
                    comp_state = bool(tss.peek_z(q) == -1) if st < 2 else False
                else:
                    assert rec is not None
                    comp_state = bool(rec[i]) ^ is_inverted[i]
                if st < 2:
                    if not comp_state:
                        outcome = (
                            False
                            if p0 <= 0.0
                            else (
                                True
                                if p0 >= 1.0
                                else bool(tss.np_rng.random() < p0)
                            )
                        )
                    else:
                        outcome = (
                            True
                            if p1_eff >= 1.0
                            else (
                                False
                                if p1_eff <= 0.0
                                else bool(tss.np_rng.random() < p1_eff)
                            )
                        )
                else:
                    p_n = params.prob_for_input_state.get(st, 0.0)
                    outcome = (
                        False
                        if p_n <= 0.0
                        else (
                            True
                            if p_n >= 1.0
                            else bool(tss.np_rng.random() < p_n)
                        )
                    )
                final_bit = outcome ^ is_inverted[i]
                tss._sync_meas_overrides[m_idx + i] = final_bit

            if not is_mpad:
                leaked_qs = [
                    int(q) for q in targets if self.state[int(q)] >= 2
                ]
                if leaked_qs:
                    tss._do_bare_only_on_tableau(
                        self._construct_stim_instruction(
                            "DEPOLARIZE1", leaked_qs, [0.75]
                        )
                    )
            return

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
                if is_mpad and np.isclose(p0, p1):
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
            if not is_mpad and raw_op_targets[i].is_inverted_result_target:
                m_targets.append(stim.target_inv(int(q_new)))
            else:
                m_targets.append(int(q_new))

        tss._do_bare_only_on_tableau(
            self._construct_stim_instruction("M", m_targets)
        )
        if is_mpad:
            tss._append_to_new_reference_circuit(op)
        else:
            ref_m_targets = [
                stim.target_inv(int(q)) if raw_op_targets[i].is_inverted_result_target else int(q)
                for i, q in enumerate(targets)
            ]
            tss._append_to_new_reference_circuit(
                self._construct_stim_instruction("M", ref_m_targets)
            )
            if len(leaked_qubits) > 0:
                tss._do_bare_only_on_tableau(
                    self._construct_stim_instruction(
                        "DEPOLARIZE1", leaked_qubits, [0.75]
                    )
                )