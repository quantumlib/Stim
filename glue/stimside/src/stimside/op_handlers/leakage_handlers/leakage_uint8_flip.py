import dataclasses
from typing import Literal

import numpy as np
from numpy.typing import NDArray
import stim  # type: ignore[import-untyped]

from stimside.op_handlers.abstract_op_handler import CompiledOpHandler, OpHandler
from stimside.op_handlers.leakage_handlers.leakage_parameters import (
    LeakageParams,
    LeakageControlledErrorParams,
    LeakageTransition1Params,
    LeakageTransition2Params,
    LeakageTransitionZParams,
    LeakageMeasurementParams
)
from stimside.op_handlers.leakage_handlers.leakage_tag_parsing_flip import (
    parse_leakage_in_circuit,
)
from stimside.simulator_flip import FlipsideSimulator
from stimside.util.numpy_types import Bool2DArray


@dataclasses.dataclass
class LeakageUint8(OpHandler[FlipsideSimulator]):
    """Implementing leakage using an array of 8bit Uints.

    Basically, we store the leakage state as an int for each qubit.
    Pro: hard to mess up
        Supports leakage states up to 255
        Easy to store and work with using numpy
    Con: probably not as fast as using a bool array for each leakage level we care about
        We spend a reasonable amount of time computing masks like state>=2

    We reserve state == 0 for computational states and avoid ever setting state == 1.
    This means state == 2 is the 2 state, state == 3 is the 3 state, etc.
    """

    def compile_op_handler(
            self, *, circuit: stim.Circuit, batch_size: int
            ) -> "CompiledLeakageUint8":

        parsed_ops = parse_leakage_in_circuit(circuit)

        return CompiledLeakageUint8(
            num_qubits=circuit.num_qubits,
            batch_size=batch_size,
            ops_to_params=parsed_ops,
        )


_DEPOLARIZE_AFTER_OPS: frozenset[str] = frozenset(
    {
        "M",
        "MZ",
        "MX",
        "MY",
        "R",
        "RZ",
        "RX",
        "RY",
        "MR",
        "MRZ",
        "MRX",
        "MRY",
    }
)


@dataclasses.dataclass
class CompiledLeakageUint8(CompiledOpHandler[FlipsideSimulator]):

    num_qubits: int
    batch_size: int
    state: NDArray[np.uint8] = dataclasses.field(init=False)

    ops_to_params: dict[stim.CircuitInstruction, LeakageParams]

    _depolarize_on_leak: bool = True

    def __post_init__(self):
        self._op_targets_cache: dict[stim.CircuitInstruction, np.ndarray] = {}
        self._op_pair_targets_cache: dict[
            stim.CircuitInstruction, tuple[np.ndarray, np.ndarray]
        ] = {}
        self.clear()

    def clear(self):
        """just allocate a new state array
        in numpy this is faster than zeroing things out...

        """
        self.state = np.zeros(shape=(self.num_qubits, self.batch_size), dtype=np.uint8)

    def _get_target_indices(self, op: stim.CircuitInstruction) -> np.ndarray:
        cached = self._op_targets_cache.get(op)
        if cached is not None:
            return cached
        indices = np.fromiter(
            (
                t.qubit_value if t.qubit_value is not None else t.value
                for t in op.targets_copy()
            ),
            dtype=np.intp,
        )
        self._op_targets_cache[op] = indices
        return indices

    def _get_pair_target_indices(
        self, op: stim.CircuitInstruction
    ) -> tuple[np.ndarray, np.ndarray]:
        cached = self._op_pair_targets_cache.get(op)
        if cached is not None:
            return cached
        indices = self._get_target_indices(op)
        pairs = (indices[::2], indices[1::2])
        self._op_pair_targets_cache[op] = pairs
        return pairs

    def make_target_mask(self, op: stim.CircuitInstruction) -> Bool2DArray:
        """return a bool mask that is true if this qubit is in the op targets."""
        target_indices = self._get_target_indices(op)
        target_mask = np.zeros_like(self.state, dtype=bool)
        target_mask[target_indices, :] = True
        return target_mask

    def make_pair_target_masks(
        self, op: stim.CircuitInstruction
    ) -> tuple[Bool2DArray, Bool2DArray]:
        """return two bool masks for qubits in the odd and even targets respectively."""
        target_indices = [t.value for t in op.targets_copy()]
        odd_target_mask = np.zeros_like(self.state, dtype=bool)
        even_target_mask = np.zeros_like(self.state, dtype=bool)
        odd_target_mask[target_indices[::2], :] = 1
        even_target_mask[target_indices[1::2], :] = 1
        return odd_target_mask, even_target_mask

    def handle_op(self, op: stim.CircuitInstruction, sss: FlipsideSimulator):
        params = self.ops_to_params.get(op)
        if params is None:
            sss.do_on_flip_simulator(op)
            if self._depolarize_on_leak and op.name in _DEPOLARIZE_AFTER_OPS:
                self._depolarize_leaked_qubits(fss=sss, op=op)
            return

        if sss.record_unleaked_to_leaked and isinstance(
            params,
            (
                LeakageTransition1Params,
                LeakageTransitionZParams,
                LeakageTransition2Params,
            ),
        ):
            target_indices = np.unique(self._get_target_indices(op))
            was_unleaked = (self.state[target_indices, :] < 2).copy()
        else:
            target_indices = None
            was_unleaked = None

        match params:
            case LeakageControlledErrorParams():
                self.leakage_controlled_error(op=op, fss=sss, params=params)
            case LeakageTransition1Params():
                self.leakage_transition_1(op=op, fss=sss, params=params)
            case LeakageTransitionZParams():
                self.leakage_transition_Z(op=op, fss=sss, params=params)
            case LeakageTransition2Params():
                self.leakage_transition_2(op=op, fss=sss, params=params)
            case LeakageMeasurementParams():
                self.leakage_projection_Z(op=op, fss=sss, params=params)
            case _:
                raise ValueError(f"Unrecognised LEAKAGE params: {params}")

        if was_unleaked is not None and target_indices is not None:
            now_leaked = self.state[target_indices, :] >= 2
            counts = np.count_nonzero(was_unleaked & now_leaked, axis=0)
            sss._record_unleaked_to_leaked_counts(counts, sss._circuit_time)

    def _depolarize_leaked_qubits(
        self, op: stim.CircuitInstruction, fss: FlipsideSimulator
    ):
        """Fully depolarize qubits that are leaked (post-gate)."""
        target_indices = self._get_target_indices(op)
        if len(target_indices) == 0:
            return
        sub_leaked = self.state[target_indices, :] >= 2
        if not np.any(sub_leaked):
            return
        mask = np.zeros_like(self.state, dtype=bool)
        mask[target_indices, :] = sub_leaked
        fss.broadcast_pauli_errors(error_mask=mask, p=0.5, pauli="X")
        fss.broadcast_pauli_errors(error_mask=mask, p=0.5, pauli="Z")

    def leakage_controlled_error(
        self,
        op: stim.CircuitInstruction,
        fss: FlipsideSimulator,
        params: LeakageControlledErrorParams,
    ):
        """implement Pauli errors conditional on a control qubit being in a leakage state.

        gate targets implicitly come in pairs t0 t1,
        if t0 is in the given leakage state, t1 has a Pauli error applied to it
        """
        t0_arr, t1_arr = self._get_pair_target_indices(op)
        if len(t0_arr) == 0:
            fss.do_on_flip_simulator(op)
            return

        ctrl_states = self.state[t0_arr, :]
        if not np.any(ctrl_states >= 2):
            fss.do_on_flip_simulator(op)
            return

        fss.do_on_flip_simulator(op)

        has_dup_t1 = len(set(t1_arr.tolist())) != len(t1_arr)

        for s0, branches in params.args_by_input_state.items():
            assert s0 != 0
            active_pairs = ctrl_states == s0
            n_active = int(np.count_nonzero(active_pairs))
            if n_active == 0:
                continue

            if len(branches) == 1 and not has_dup_t1:
                pauli, p = branches[0]
                if p <= 0.0:
                    continue
                mask = np.zeros_like(self.state, dtype=bool)
                mask[t1_arr, :] = active_pairs
                fss.broadcast_pauli_errors(error_mask=mask, p=p, pauli=pauli)
            else:
                total_p = sum(p for _, p in branches)
                if total_p <= 0.0:
                    continue
                n_hits = (
                    n_active
                    if total_p >= 1.0 - 1e-12
                    else int(fss.np_rng.binomial(n_active, total_p))
                )
                if n_hits == 0:
                    continue
                elig_p, elig_b = np.where(active_pairs)
                if n_hits < n_active:
                    chosen = fss.np_rng.choice(n_active, size=n_hits, replace=False)
                    hit_p, hit_b = elig_p[chosen], elig_b[chosen]
                else:
                    hit_p, hit_b = elig_p, elig_b
                hit_t1 = t1_arr[hit_p]

                if len(branches) == 1:
                    pauli = branches[0][0]
                    mask = np.zeros_like(self.state, dtype=bool)
                    np.logical_xor.at(mask, (hit_t1, hit_b), True)
                    fss.broadcast_pauli_errors(error_mask=mask, p=1.0, pauli=pauli)
                else:
                    cond_probs = [p / total_p for _, p in branches]
                    br_idx = fss.np_rng.choice(
                        len(branches), size=n_hits, p=cond_probs
                    )
                    for b_i, (pauli, _) in enumerate(branches):
                        sel = br_idx == b_i
                        if np.any(sel):
                            mask = np.zeros_like(self.state, dtype=bool)
                            np.logical_xor.at(mask, (hit_t1[sel], hit_b[sel]), True)
                            fss.broadcast_pauli_errors(
                                error_mask=mask, p=1.0, pauli=pauli
                            )

    def leakage_transition_1(
        self,
        op: stim.CircuitInstruction,
        fss: FlipsideSimulator,
        params: LeakageTransition1Params,
    ):
        """implement leakage state transitions on single qubits.

        when a qubit transitions to the unknown unleaked state U, we fully depolarize it.
        """
        target_indices = self._get_target_indices(op)
        num_targets = len(target_indices)
        if num_targets == 0:
            fss.do_on_flip_simulator(op)
            return
        if num_targets > 1 and len(set(target_indices.tolist())) < num_targets:
            for t in op.targets_copy():
                sub_op = stim.CircuitInstruction(
                    op.name, [t], op.gate_args_copy()
                )
                self.leakage_transition_1(sub_op, fss, params)
            return

        initial_state = (
            self.state.copy()
            if len(params.args_by_input_state) > 1
            else self.state
        )
        sub_state = initial_state[target_indices, :]
        any_initially_leaked = bool(np.any(sub_state >= 2))

        to_depolarize: Bool2DArray | None = None

        for input_state, transitions in params.args_by_input_state.items():
            total_p = sum(p for _, p in transitions)
            if total_p <= 0.0:
                continue

            if input_state == "U" and not any_initially_leaked:
                M = num_targets * self.batch_size
                n_hits = (
                    M
                    if total_p >= 1.0 - 1e-12
                    else int(fss.np_rng.binomial(M, total_p))
                )
                if n_hits == 0:
                    continue
                if n_hits == M:
                    row_idx = np.repeat(
                        np.arange(num_targets, dtype=np.intp), self.batch_size
                    )
                    b_idx = np.tile(
                        np.arange(self.batch_size, dtype=np.intp), num_targets
                    )
                else:
                    flat_idx = fss.np_rng.choice(M, size=n_hits, replace=False)
                    row_idx = flat_idx // self.batch_size
                    b_idx = flat_idx % self.batch_size
            else:
                if input_state != "U" and not any_initially_leaked and isinstance(input_state, int) and input_state >= 2:
                    continue
                in_mask = (
                    (sub_state < 2)
                    if input_state == "U"
                    else (sub_state == input_state)
                )
                M = int(np.count_nonzero(in_mask))
                if M == 0:
                    continue
                n_hits = (
                    M
                    if total_p >= 1.0 - 1e-12
                    else int(fss.np_rng.binomial(M, total_p))
                )
                if n_hits == 0:
                    continue
                elig_rows, elig_bs = np.where(in_mask)
                if n_hits == M:
                    row_idx, b_idx = elig_rows, elig_bs
                else:
                    chosen = fss.np_rng.choice(M, size=n_hits, replace=False)
                    row_idx = elig_rows[chosen]
                    b_idx = elig_bs[chosen]

            q_all = target_indices[row_idx]

            if len(transitions) == 1:
                branch_groups = [(transitions[0][0], q_all, b_idx)]
            else:
                cond_probs = [p / total_p for _, p in transitions]
                br_idx = fss.np_rng.choice(
                    len(transitions), size=n_hits, p=cond_probs
                )
                branch_groups = []
                for b_i, (out_st, _) in enumerate(transitions):
                    sel = br_idx == b_i
                    if np.any(sel):
                        branch_groups.append((out_st, q_all[sel], b_idx[sel]))

            for out_st, q_hit, b_hit in branch_groups:
                if out_st == input_state:
                    continue
                if out_st == "U":
                    self.state[q_hit, b_hit] = 0
                    if to_depolarize is None:
                        to_depolarize = np.zeros_like(self.state, dtype=bool)
                    to_depolarize[q_hit, b_hit] = True
                else:
                    out_int = int(out_st)
                    self.state[q_hit, b_hit] = 0 if out_int < 2 else out_int
                    in_unleaked = input_state in ("U", 0, 1)
                    if (
                        (out_int >= 2 and self._depolarize_on_leak and in_unleaked)
                        or (out_int < 2 and not in_unleaked)
                    ):
                        if to_depolarize is None:
                            to_depolarize = np.zeros_like(self.state, dtype=bool)
                        to_depolarize[q_hit, b_hit] = True

        if to_depolarize is not None:
            fss.broadcast_pauli_errors(error_mask=to_depolarize, pauli="Z", p=0.5)
            fss.broadcast_pauli_errors(error_mask=to_depolarize, pauli="X", p=0.5)

        fss.do_on_flip_simulator(op)

    def leakage_transition_Z(
        self,
        op: stim.CircuitInstruction,
        fss: FlipsideSimulator,
        params: LeakageTransitionZParams,
    ):
        """Implement leakage state transitions on single qubits in known Z eigenstates.

        When a qubit transitions to a computational state (0, 1) we prepare it in that Z eigenstate.
        When any qubit changes state, we randomize its phase by applying a Z flip with 50% probability.
        """
        target_indices = self._get_target_indices(op)
        if len(target_indices) > 1 and len(set(target_indices.tolist())) < len(
            target_indices
        ):
            for t in op.targets_copy():
                sub_op = stim.CircuitInstruction(
                    op.name, [t], op.gate_args_copy()
                )
                self.leakage_transition_Z(sub_op, fss, params)
            return
        is_reset = op.name in ("R", "RZ")

        if is_reset:
            fss.do_on_flip_simulator(op)
            eval_time = fss.get_current_circuit_time() + 1
        else:
            eval_time = fss.get_current_circuit_time()

        if not fss._all_targets_in_known_state(
            targets=target_indices, pauli="Z", circuit_time=eval_time
        ):
            raise ValueError(
                f"{op} has the tag LEAKAGE_TRANSITIONS_Z that demands known Z states, but targets aren't in known Z states."
            )

        state_before_op = self.state.copy()
        target_mask = self.make_target_mask(op)
        raw_in_0_mask, raw_in_1_mask = fss._get_current_known_state_masks(
            pauli="Z", circuit_time=eval_time
        )
        unleaked_mask = state_before_op < 2
        in_0_mask = np.logical_and(raw_in_0_mask, unleaked_mask)
        in_1_mask = np.logical_and(raw_in_1_mask, unleaked_mask)

        flip_Z_state = np.zeros_like(self.state, dtype=bool)
        randomize_phase = np.zeros_like(self.state, dtype=bool)
        to_depolarize = np.zeros_like(self.state, dtype=bool)

        for input_state in params.args_by_input_state.keys():
            if input_state == 0:
                in_state_mask = in_0_mask
            elif input_state == 1:
                in_state_mask = in_1_mask
            else:
                in_state_mask = state_before_op == input_state

            overwrite_mask = np.logical_and(target_mask, in_state_mask)
            samples_to_take = int(np.count_nonzero(overwrite_mask))
            if samples_to_take == 0:
                continue

            output_states = params.sample_transitions_from_state(
                input_state=input_state,
                num_samples=samples_to_take,
                np_rng=fss.np_rng,
            )

            changed = output_states != input_state
            if not np.any(changed):
                continue

            output_1_and_in_0 = np.logical_and(
                raw_in_0_mask[overwrite_mask], (output_states == 1) & changed
            )
            output_0_and_in_1 = np.logical_and(
                raw_in_1_mask[overwrite_mask], (output_states == 0) & changed
            )
            in_wrong_z_state = np.logical_or(output_1_and_in_0, output_0_and_in_1)
            flip_Z_state[overwrite_mask] |= in_wrong_z_state

            randomize_phase[overwrite_mask] |= changed

            if input_state <= 1 and self._depolarize_on_leak:
                to_depolarize[overwrite_mask] |= (output_states >= 2) & changed

            new_st = output_states.copy()
            new_st[new_st == 1] = 0
            # Only overwrite entries that actually transitioned (or keep existing state)
            cur_vals = self.state[overwrite_mask]
            cur_vals[changed] = new_st[changed].astype(np.uint8)
            self.state[overwrite_mask] = cur_vals

        if is_reset and self._depolarize_on_leak:
            # Since R cleared Pauli flips on all targets, re-depolarize any target that remains leaked
            to_depolarize |= np.logical_and(target_mask, self.state >= 2)

        if np.any(flip_Z_state):
            fss.broadcast_pauli_errors(error_mask=flip_Z_state, pauli="X", p=1)
        if np.any(randomize_phase):
            fss.broadcast_pauli_errors(error_mask=randomize_phase, pauli="Z", p=0.5)
        if self._depolarize_on_leak and np.any(to_depolarize):
            fss.broadcast_pauli_errors(error_mask=to_depolarize, pauli="Z", p=0.5)
            fss.broadcast_pauli_errors(error_mask=to_depolarize, pauli="X", p=0.5)

        if not is_reset:
            fss.do_on_flip_simulator(op)

    def leakage_transition_2(
        self,
        op: stim.CircuitInstruction,
        fss: FlipsideSimulator,
        params: LeakageTransition2Params,
    ):
        """Implement leakage state transitions on pairs of qubits.

        When a qubit transitions from a leaked state to the unknown unleaked state U,
        or from an unknown unleaked state U to a different unknown unleaked state V,
        we fully depolarize it.
        """
        all_indices = self._get_target_indices(op)
        if len(all_indices) > 2 and len(set(all_indices.tolist())) < len(
            all_indices
        ):
            for grp in op.target_groups():
                sub_op = stim.CircuitInstruction(
                    op.name, grp, op.gate_args_copy()
                )
                self.leakage_transition_2(sub_op, fss, params)
            return
        even_targets, odd_targets = self._get_pair_target_indices(op)
        num_pairs = len(even_targets)
        if num_pairs == 0:
            fss.do_on_flip_simulator(op)
            return

        initial_state = (
            self.state.copy()
            if len(params.args_by_input_state) > 1
            else self.state
        )
        even_target_states = initial_state[even_targets, :]
        odd_target_states = initial_state[odd_targets, :]
        any_initially_leaked = bool(
            np.any(even_target_states >= 2) or np.any(odd_target_states >= 2)
        )

        to_depolarize: Bool2DArray | None = None
        to_pauli_X: Bool2DArray | None = None
        to_pauli_Y: Bool2DArray | None = None
        to_pauli_Z: Bool2DArray | None = None

        for input_state, transitions in params.args_by_input_state.items():
            total_p = sum(p for _, p in transitions)
            if total_p <= 0.0:
                continue

            if input_state == ("U", "U") and not any_initially_leaked:
                M = num_pairs * self.batch_size
                n_hits = (
                    M
                    if total_p >= 1.0 - 1e-12
                    else int(fss.np_rng.binomial(M, total_p))
                )
                if n_hits == 0:
                    continue
                if n_hits == M:
                    pair_idx = np.repeat(
                        np.arange(num_pairs, dtype=np.intp), self.batch_size
                    )
                    b_idx = np.tile(
                        np.arange(self.batch_size, dtype=np.intp), num_pairs
                    )
                else:
                    flat_idx = fss.np_rng.choice(M, size=n_hits, replace=False)
                    pair_idx = flat_idx // self.batch_size
                    b_idx = flat_idx % self.batch_size
            else:
                if not any_initially_leaked and (
                    (isinstance(input_state[0], int) and input_state[0] >= 2)
                    or (isinstance(input_state[1], int) and input_state[1] >= 2)
                ):
                    continue
                check_0 = (
                    (even_target_states < 2)
                    if input_state[0] == "U"
                    else (even_target_states == input_state[0])
                )
                check_1 = (
                    (odd_target_states < 2)
                    if input_state[1] == "U"
                    else (odd_target_states == input_state[1])
                )
                target_state_mask = np.logical_and(check_0, check_1)
                M = int(np.count_nonzero(target_state_mask))
                if M == 0:
                    continue
                n_hits = (
                    M
                    if total_p >= 1.0 - 1e-12
                    else int(fss.np_rng.binomial(M, total_p))
                )
                if n_hits == 0:
                    continue
                elig_pairs, elig_bs = np.where(target_state_mask)
                if n_hits == M:
                    pair_idx, b_idx = elig_pairs, elig_bs
                else:
                    chosen = fss.np_rng.choice(M, size=n_hits, replace=False)
                    pair_idx = elig_pairs[chosen]
                    b_idx = elig_bs[chosen]

            q0_all = even_targets[pair_idx]
            q1_all = odd_targets[pair_idx]

            if len(transitions) == 1:
                branch_groups = [(transitions[0][0], q0_all, q1_all, b_idx)]
            else:
                cond_probs = [p / total_p for _, p in transitions]
                br_idx = fss.np_rng.choice(
                    len(transitions), size=n_hits, p=cond_probs
                )
                branch_groups = []
                for b_i, (out_pair, _) in enumerate(transitions):
                    sel = br_idx == b_i
                    if np.any(sel):
                        branch_groups.append(
                            (out_pair, q0_all[sel], q1_all[sel], b_idx[sel])
                        )

            for out_pair, q0_hit, q1_hit, b_hit in branch_groups:
                for leg_in, leg_out, q_hit in (
                    (input_state[0], out_pair[0], q0_hit),
                    (input_state[1], out_pair[1], q1_hit),
                ):
                    if leg_out == leg_in:
                        continue
                    if leg_out in ("U", "V", "D"):
                        self.state[q_hit, b_hit] = 0
                        if to_depolarize is None:
                            to_depolarize = np.zeros_like(self.state, dtype=bool)
                        to_depolarize[q_hit, b_hit] = True
                    elif leg_out == "X":
                        self.state[q_hit, b_hit] = 0
                        if to_pauli_X is None:
                            to_pauli_X = np.zeros_like(self.state, dtype=bool)
                        np.logical_xor.at(to_pauli_X, (q_hit, b_hit), True)
                    elif leg_out == "Y":
                        self.state[q_hit, b_hit] = 0
                        if to_pauli_Y is None:
                            to_pauli_Y = np.zeros_like(self.state, dtype=bool)
                        np.logical_xor.at(to_pauli_Y, (q_hit, b_hit), True)
                    elif leg_out == "Z":
                        self.state[q_hit, b_hit] = 0
                        if to_pauli_Z is None:
                            to_pauli_Z = np.zeros_like(self.state, dtype=bool)
                        np.logical_xor.at(to_pauli_Z, (q_hit, b_hit), True)
                    else:
                        out_int = int(leg_out)
                        self.state[q_hit, b_hit] = 0 if out_int < 2 else out_int
                        in_unleaked = leg_in in ("U", 0, 1)
                        if (
                            (out_int >= 2 and self._depolarize_on_leak and in_unleaked)
                            or (out_int < 2 and not in_unleaked)
                        ):
                            if to_depolarize is None:
                                to_depolarize = np.zeros_like(self.state, dtype=bool)
                            to_depolarize[q_hit, b_hit] = True

        if to_depolarize is not None:
            fss.broadcast_pauli_errors(error_mask=to_depolarize, pauli="Z", p=0.5)
            fss.broadcast_pauli_errors(error_mask=to_depolarize, pauli="X", p=0.5)
        if to_pauli_X is not None:
            fss.broadcast_pauli_errors(error_mask=to_pauli_X, pauli="X", p=1.0)
        if to_pauli_Y is not None:
            fss.broadcast_pauli_errors(error_mask=to_pauli_Y, pauli="Y", p=1.0)
        if to_pauli_Z is not None:
            fss.broadcast_pauli_errors(error_mask=to_pauli_Z, pauli="Z", p=1.0)

        fss.do_on_flip_simulator(op)

    def leakage_projection_Z(
        self,
        op: stim.CircuitInstruction,
        fss: FlipsideSimulator,
        params: LeakageMeasurementParams,
    ):
        """Implement measurement projections for qubits in known eigenstates."""
        is_mpad = params.targets is not None
        raw_op_targets = op.targets_copy()

        pauli: Literal["X", "Y", "Z"] = (
            "X"
            if op.name in ("MX", "MRX")
            else ("Y" if op.name in ("MY", "MRY") else "Z")
        )
        is_reset = op.name in ("MR", "MRZ", "MRX", "MRY")

        p0 = params.prob_for_input_state.get(0, 0.0)
        p1 = params.prob_for_input_state.get(1, 1.0)

        if is_mpad:
            assert params.targets is not None
            targets = np.asarray(params.targets, dtype=np.intp)
            if len(raw_op_targets) != len(targets):
                raise ValueError(
                    "The number of targets in the MPAD operation with a LEAKAGE_MEASUREMENT tag "
                    "does not equal the number of targets specified in the tag."
                )
            if op.name != "MPAD":
                raise ValueError(
                    f"LEAKAGE_MEASUREMENT is only implemented for 'MPAD' operations, got {op}"
                )
        else:
            targets = self._get_target_indices(op)
            if op.name not in ("M", "MZ", "MR", "MRZ", "MX", "MY", "MRX", "MRY"):
                raise ValueError(
                    f"LEAKAGE_PROJECTION_Z is only implemented for M/MZ/MR/MRZ/MX/MY/MRX/MRY operations, got {op}"
                )
            if not np.isclose(p0, 1.0 - p1) and not fss._all_targets_in_known_state(
                targets=targets, pauli=pauli
            ):
                raise ValueError(
                    f"{op} has a LEAKAGE_PROJECTION_Z tag that demands known {pauli} states, but targets aren't in known {pauli} states."
                )

        m_idx = fss._flip_simulator.num_measurements
        ref_slice = fss.ref_measurements[m_idx : m_idx + len(targets)]

        if is_mpad:
            # For MPAD, if targets are in known Z states, use their Z state; otherwise use xs flip state
            if fss._all_targets_in_known_state(targets=targets, pauli="Z"):
                _, known_state_mask = fss._get_current_known_state_masks(pauli="Z")
                target_computational_states = known_state_mask[targets, :]
            else:
                xs, _, _, _, _ = fss._flip_simulator.to_numpy(
                    output_xs=True, output_zs=False
                )
                target_computational_states = xs[targets, :]
            inv_arr = np.array(
                [
                    bool(t.value) ^ bool(t.is_inverted_result_target)
                    for t in raw_op_targets
                ],
                dtype=bool,
            )[:, None]
        else:
            inv_arr = np.array(
                [bool(t.is_inverted_result_target) for t in raw_op_targets],
                dtype=bool,
            )[:, None]
            clean_plus, clean_minus = fss._get_clean_known_states(pauli=pauli)
            is_known = (clean_plus | clean_minus)[targets]
            _, known_state_mask = fss._get_current_known_state_masks(pauli=pauli)
            if np.all(is_known):
                target_computational_states = known_state_mask[targets, :].copy()
            else:
                xs, zs, _, _, _ = fss._flip_simulator.to_numpy(
                    output_xs=True, output_zs=True
                )
                if pauli == "X":
                    basis_flips = zs[targets, :]
                elif pauli == "Y":
                    basis_flips = xs[targets, :] ^ zs[targets, :]
                else:
                    basis_flips = xs[targets, :]
                ref_states = (ref_slice[:, None] ^ inv_arr) ^ basis_flips
                target_computational_states = np.where(
                    is_known[:, None], known_state_mask[targets, :], ref_states
                ).copy()
            if len(set(int(q) for q in targets)) < len(targets):
                first_occ_f: dict[int, int] = {}
                for k_i, q_np in enumerate(targets):
                    q_int = int(q_np)
                    if q_int in first_occ_f:
                        target_computational_states[k_i, :] = (
                            False
                            if is_reset
                            else target_computational_states[
                                first_occ_f[q_int], :
                            ]
                        )
                    else:
                        first_occ_f[q_int] = k_i

        target_leakage_state = self.state[targets, :]
        target_is_not_leaked = target_leakage_state < 2
        target_outcomes = np.zeros_like(target_computational_states, dtype=bool)

        if p0 > 0.0:
            target_states_0_mask = np.logical_and(
                target_computational_states == 0, target_is_not_leaked
            )
            n0 = int(np.count_nonzero(target_states_0_mask))
            if n0 > 0:
                target_outcomes[target_states_0_mask] = (
                    True if p0 >= 1.0 else (fss.np_rng.random(n0) < p0)
                )

        p1 = params.prob_for_input_state.get(1, 0.0 if is_mpad else 1.0)
        if p1 > 0.0:
            target_states_1_mask = np.logical_and(
                target_computational_states == 1, target_is_not_leaked
            )
            n1 = int(np.count_nonzero(target_states_1_mask))
            if n1 > 0:
                target_outcomes[target_states_1_mask] = (
                    True if p1 >= 1.0 else (fss.np_rng.random(n1) < p1)
                )

        max_leak = int(np.max(target_leakage_state)) if len(targets) > 0 else 0
        for n in range(2, max_leak + 1):
            p_n = params.prob_for_input_state.get(n, 0.0)
            if p_n > 0.0:
                target_state_mask = target_leakage_state == n
                nn = int(np.count_nonzero(target_state_mask))
                if nn > 0:
                    target_outcomes[target_state_mask] = (
                        True if p_n >= 1.0 else (fss.np_rng.random(nn) < p_n)
                    )

        final_outcomes = np.logical_xor(target_outcomes, inv_arr)
        measurement_flips = np.logical_xor(final_outcomes, ref_slice[:, None])
        fss.append_measurement_flips(measurement_flip_data=measurement_flips)

        if not is_mpad:
            target_mask = np.zeros_like(self.state, dtype=bool)
            target_mask[targets, :] = True
            unleaked_targets_mask = np.logical_and(target_mask, self.state < 2)
            leaked_targets_mask = np.logical_and(target_mask, self.state >= 2)

            if is_reset:
                xs, zs, _, _, _ = fss._flip_simulator.to_numpy(
                    output_xs=True, output_zs=True
                )
                reset_flip_x = np.logical_and(unleaked_targets_mask, xs)
                reset_flip_z = np.logical_and(unleaked_targets_mask, zs)
                if np.any(reset_flip_x):
                    fss.broadcast_pauli_errors(
                        error_mask=reset_flip_x, pauli="X", p=1.0
                    )
                if np.any(reset_flip_z):
                    fss.broadcast_pauli_errors(
                        error_mask=reset_flip_z, pauli="Z", p=1.0
                    )
                fss.broadcast_pauli_errors(
                    error_mask=unleaked_targets_mask, pauli=pauli, p=0.5
                )
            else:
                fss.broadcast_pauli_errors(
                    error_mask=unleaked_targets_mask, pauli=pauli, p=0.5
                )

            if self._depolarize_on_leak and np.any(leaked_targets_mask):
                fss.broadcast_pauli_errors(
                    error_mask=leaked_targets_mask, pauli="Z", p=0.5
                )
                fss.broadcast_pauli_errors(
                    error_mask=leaked_targets_mask, pauli="X", p=0.5
                )

