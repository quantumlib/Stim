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

        parsed_ops = parse_leakage_in_circuit(circuit)

        return CompiledLeakageUint8(
            num_qubits=circuit.num_qubits,
            batch_size=batch_size,
            ops_to_params=parsed_ops,
            unconditional_condition_on_U=self.unconditional_condition_on_U,
        )


@dataclasses.dataclass
class CompiledLeakageUint8(CompiledOpHandler[TablesideSimulator]):

    num_qubits: int
    batch_size: int
    unconditional_condition_on_U: bool

    state: NDArray[np.uint8] = dataclasses.field(init=False)

    ops_to_params: dict[stim.CircuitInstruction, LeakageParams]

    def __post_init__(self):
        assert (
            self.batch_size == 1
        ), "The tableau leakage op handler can only take one shot a time"
        self.claimed_ops_keys = set(self.ops_to_params.keys())
        self.clear()

    def clear(self):
        """just allocate a new state array
        in numpy this is faster than zeroing things out...

        """
        self.state = np.zeros(self.num_qubits, dtype=np.uint8)

    def make_target_mask(self, op: stim.CircuitInstruction) -> Bool2DArray:
        """return a bool mask that is true if this qubit is in the op targets."""
        target_indices = [t.value for t in op.targets_copy()]
        target_mask = np.zeros_like(self.state, dtype=bool)
        target_mask[target_indices] = 1
        return target_mask

    def _update_state_from_peek_Z(
        self, targets: list[int] | NDArray[np.int_], tss: TablesideSimulator
    ) -> tuple[NDArray[np.int_], NDArray[np.int_]]:
        """
        Update self.state for the targets based on the current Z pauli states.
        If a target is in a superposition, flip a coin to decide its state.
        """
        targets_in_qubit_space = np.array(targets)[self.state[targets] < 2]
        pauli_states = np.array(
            tss.get_current_noisy_tableau_pauli_state(targets_in_qubit_space, "Z")
        )

        new_state = np.zeros_like(targets_in_qubit_space)
        new_state[pauli_states == -1] = 1

        flip_coin_mask = pauli_states == 0
        new_state[flip_coin_mask] = tss.np_rng.binomial(
            1, 0.5, size=np.count_nonzero(flip_coin_mask)
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
        if (
            1 in allowed_st_list[0]
            or 0 in allowed_st_list[0]
            and not (1 in allowed_st_list[0] and 0 in allowed_st_list[0])
        ):
            self._update_state_from_peek_Z(targets, tss)

        target_states = self.state[targets]
        allowed_mask = np.zeros_like(targets, dtype=np.bool_)
        if "U" in allowed_st_list[0]:
            allowed_mask = target_states < 2
        for st in allowed_st_list[0]:
            if isinstance(st, int):
                allowed_mask = np.logical_or(target_states == st, allowed_mask)
            elif st != "U":
                raise ValueError(
                    f"Unrecognised allowed state {st} in allowed_st_list {allowed_st_list}"
                )
        return allowed_mask

    def _filter_targets_2q(
        self,
        targets: list[list[int]] | Int2DArray,
        allowed_st_list: tuple[tuple[int | str, ...], tuple[int | str, ...]],
        tss: TablesideSimulator,
    ) -> list[int] | NDArray[np.int_]:
        """
        Filter two-qubit op targets based on allowed states.
        If 0 or 1 is in allowed_st_list, update the state from the current Z pauli states.
        Return only the 2-qubit targets that are in the allowed states.
        """
        targets_np = np.array(targets)
        for i in [0, 1]:
            if 1 in allowed_st_list[i] or 0 in allowed_st_list[i]:
                self._update_state_from_peek_Z(targets_np[:, i], tss)

        target_states = self.state[targets_np]
        allowed_mask = np.zeros(len(targets_np), dtype=np.bool_)
        for n in range(len(allowed_st_list[0])):
            allowed_mask_pairs = np.zeros_like(targets_np, dtype=np.bool_)
            for i in [0, 1]:
                st = allowed_st_list[i][n]
                if st == "U":
                    allowed_mask_pairs[:, i] = target_states[:, i] < 2
                else:
                    if isinstance(st, int):
                        allowed_mask_pairs[:, i] = np.logical_or(
                            allowed_mask_pairs[:, i], target_states[:, i] == st
                        )
                    else:
                        raise ValueError(
                            f"Unrecognised allowed state {st} in allowed_st_list {allowed_st_list}"
                        )
            allowed_mask = np.logical_or(
                allowed_mask,
                np.logical_and(allowed_mask_pairs[:, 0], allowed_mask_pairs[:, 1]),
            )
        return targets_np[allowed_mask].flatten()

    def _construct_stim_instruction(
        self,
        op_name: str,
        targets: list[int] | np.ndarray,
        gate_args_copy: list[float] = [],
    ) -> stim.CircuitInstruction:
        """construct a stim CircuitInstruction from name, targets, and args."""
        circuit_instruction_str = op_name
        if len(gate_args_copy) > 0:
            circuit_instruction_str += "("
            for arg in gate_args_copy:
                circuit_instruction_str += str(arg) + ","
            circuit_instruction_str = circuit_instruction_str[:-1] + ")"
        for target in targets:
            circuit_instruction_str += " " + str(target)

        return stim.Circuit(circuit_instruction_str)[0]

    def handle_op(self, op: stim.CircuitInstruction, sss: TablesideSimulator):
        """handle a single stim CircuitInstruction."""
        if op not in self.claimed_ops_keys:
            if self.unconditional_condition_on_U:
                op_name = op.name
                gate_data = stim.GateData(op_name)
                if gate_data.produces_measurements or not (
                    gate_data.is_noisy_gate or gate_data.is_unitary
                ):
                    sss._do_bare_instruction(op)
                    return

                if stim.gate_data(op.name).is_single_qubit_gate:
                    targets = [target.value for target in op.targets_copy()]
                    filtered_targets = np.array(targets)[
                        self._filter_targets_1q_mask(targets, (("U",),), sss)
                    ]
                elif stim.gate_data(op.name).is_two_qubit_gate:
                    targets = [
                        [target[0].value, target[1].value]
                        for target in op.target_groups()
                    ]
                    filtered_targets = self._filter_targets_2q(
                        targets, (("U",), ("U",)), sss
                    )
                else:
                    print(
                        f"Not sure why this op {op.name} did not fall into any known categories"
                    )
                    sss._do_bare_instruction(op)
                    return
                if len(filtered_targets) > 0:
                    op_new = self._construct_stim_instruction(
                        op_name, filtered_targets, op.gate_args_copy()
                    )
                    sss._do_bare_instruction(op_new)
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
        other_targets = params.targets
        if other_targets:
            # CONDITIONED_ON_OTHER case
            targets = [target.value for target in op.targets_copy()]
            if len(other_targets) != len(targets):
                raise ValueError(
                    f"The number of targets of the {op} is not the same as the"
                    "number of targets specified in the CONDITIONED_ON_OTHER tag."
                )
            filtered_targets = np.array(targets)[
                self._filter_targets_1q_mask(
                    list(other_targets),
                    cast(tuple[tuple[int | str, ...]], condition_groups),
                    tss,
                )
            ]
        else:
            if len(condition_groups) == 1:
                # CONDITIONED_ON_SELF case
                targets = [target.value for target in op.targets_copy()]
                filtered_targets = np.array(targets)[
                    self._filter_targets_1q_mask(
                        targets, condition_groups, tss
                    )
                ]
            else:
                # CONDITIONED_ON_PAIR case
                targets = [
                    [target[0].value, target[1].value] for target in op.target_groups()
                ]
                filtered_targets = self._filter_targets_2q(
                    targets, condition_groups, tss
                )

        if len(filtered_targets) > 0:
            tss._do_bare_instruction(
                self._construct_stim_instruction(
                    op.name, filtered_targets, op.gate_args_copy()
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

        to_depolarize = np.zeros_like(self.state, dtype=bool)
        to_set_to_one = np.zeros_like(self.state, dtype=bool)
        to_set_to_zero = np.zeros_like(self.state, dtype=bool)

        for input_state in params.args_by_input_state.keys():

            if input_state in [0, 1]:
                self._update_state_from_peek_Z(np.where(target_mask)[0], tss)

            if input_state == "U":
                input_state_mask = self.state < 2
            else:
                input_state_mask = self.state == input_state

            overwrite_mask = np.logical_and(target_mask, input_state_mask)

            samples_to_take = np.count_nonzero(overwrite_mask)
            if samples_to_take == 0:
                continue

            output_states = params.sample_transitions_from_state(
                input_state=input_state, num_samples=samples_to_take, np_rng=tss.np_rng
            ).astype(str)

            was_unleaked = output_states == "U"
            set_to_one = output_states == "1"
            set_to_zero = output_states == "0"

            output_states[was_unleaked] = 0
            # actually overwrite the leakage states
            output_states = output_states.astype(np.uint8)

            to_depolarize[overwrite_mask] = was_unleaked
            to_set_to_one[overwrite_mask] = set_to_one
            to_set_to_zero[overwrite_mask] = set_to_zero

            output_states[was_unleaked] = 0
            # output_states now all ints
            self.state[overwrite_mask] = output_states

        idx_to_depolarize = np.where(to_depolarize)[0]
        idx_to_set_to_one = np.where(to_set_to_one)[0]
        idx_to_set_to_zero = np.where(to_set_to_zero)[0]
        if len(idx_to_depolarize) > 0:
            tss._do_bare_instruction(
                self._construct_stim_instruction(
                    "DEPOLARIZE1", idx_to_depolarize, [0.75]
                )
            )
        if len(idx_to_set_to_one) > 0:
            tss._do_bare_instruction(
                self._construct_stim_instruction("R", idx_to_set_to_one)
            )
            tss._do_bare_instruction(
                self._construct_stim_instruction("X", idx_to_set_to_one)
            )
        if len(idx_to_set_to_zero) > 0:
            tss._do_bare_instruction(
                self._construct_stim_instruction("R", idx_to_set_to_zero)
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

        targets_list = [target.value for target in op.targets_copy()]
        even_targets_list = targets_list[::2]
        odd_targets_list = targets_list[1::2]

        if (
            0 in params.args_by_input_state.keys()
            or 1 in params.args_by_input_state.keys()
        ):
            self._update_state_from_peek_Z(targets_list, tss)

        # extract out the states on just the targets in order
        # a 2D array sliced by a list compiles an array for just those rows
        even_target_states = self.state[even_targets_list]
        odd_target_states = self.state[odd_targets_list]
        # these are shape (#target_pairs, batch_size)

        to_depolarize = np.zeros_like(self.state, dtype=bool)
        to_pauli_X = np.zeros_like(self.state, dtype=bool)
        to_pauli_Y = np.zeros_like(self.state, dtype=bool)
        to_pauli_Z = np.zeros_like(self.state, dtype=bool)

        to_set_to_one = np.zeros_like(self.state, dtype=bool)
        to_set_to_zero = np.zeros_like(self.state, dtype=bool)

        # for accumulating which qubits need to have their phase randomized
        randomize_phase = np.zeros_like(self.state, dtype=bool)

        for input_state in params.args_by_input_state.keys():

            # we need to find pairs where the whole pair is in the right state
            if input_state[0] == "U":
                even_target_state_checks = even_target_states < 2
            else:
                even_target_state_checks = even_target_states == input_state[0]

            if input_state[1] == "U":
                odd_target_state_checks = odd_target_states < 2
            else:
                odd_target_state_checks = odd_target_states == input_state[1]

            target_state_mask = np.logical_and(
                even_target_state_checks, odd_target_state_checks
            )
            # this is shape (#pair_targets, batch_size),
            # and is true if this pair in this batch is in the correct state

            # make overwrite masks, showing where in self.states we need to put new samples
            odd_overwrite_mask = np.zeros_like(self.state, dtype=bool)
            odd_overwrite_mask[odd_targets_list] = target_state_mask

            even_overwrite_mask = np.zeros_like(self.state, dtype=bool)
            even_overwrite_mask[even_targets_list] = target_state_mask

            overwrite_mask = np.logical_or(even_overwrite_mask, odd_overwrite_mask)

            samples_to_take = np.count_nonzero(target_state_mask)
            # we take a sample for each pair of targets in the right state

            # actually do the alias sampling
            output_states = params.sample_transitions_from_state(
                input_state=input_state, num_samples=samples_to_take, np_rng=tss.np_rng
            ).astype(str)
            # output_states has shape=(samples_to_take, 2={even, odd})

            # now we handle the various unleaked states
            if input_state[0] in [0, 1, "U"]:
                depolarize_even_qubit = output_states[:, 0] == "D"
                to_pauli_X[even_overwrite_mask] = np.logical_or(
                    output_states[:, 0] == "X", output_states[:, 0] == "V"
                )
                to_pauli_Y[even_overwrite_mask] = output_states[:, 0] == "Y"
                to_pauli_Z[even_overwrite_mask] = output_states[:, 0] == "Z"
            else:  # unleaked
                depolarize_even_qubit = output_states[:, 0] == "U"

            if input_state[1] in [0, 1, "U"]:
                depolarize_odd_qubit = output_states[:, 1] == "D"
                to_pauli_X[odd_overwrite_mask] = np.logical_or(
                    output_states[:, 1] == "X", output_states[:, 1] == "V"
                )
                to_pauli_Y[odd_overwrite_mask] = output_states[:, 1] == "Y"
                to_pauli_Z[odd_overwrite_mask] = output_states[:, 1] == "Z"
            else:  # unleaked
                depolarize_odd_qubit = output_states[:, 1] == "U"

            set_to_one = output_states == "1"
            set_to_zero = output_states == "0"
            set_to_diff = output_states == "V"

            to_set_to_one[overwrite_mask] = set_to_one.flatten()
            to_set_to_zero[overwrite_mask] = set_to_zero.flatten()
            randomize_phase[overwrite_mask] = set_to_diff.flatten()

            # Setting strings to 0. Actual states needs to be determined next time
            for i in range(len(output_states)):
                for j in range(2):
                    if output_states[i, j] in ["D", "U", "X", "Y", "Z", "V"]:
                        output_states[i, j] = 0

            # actually overwrite the leakage states
            output_states = output_states.astype(np.uint8)

            self.state[even_overwrite_mask] = output_states[:, 0]
            self.state[odd_overwrite_mask] = output_states[:, 1]

            to_depolarize[even_overwrite_mask] = depolarize_even_qubit
            to_depolarize[odd_overwrite_mask] = depolarize_odd_qubit

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
        idx_to_set_to_one = np.where(to_set_to_one)[0]
        idx_to_set_to_zero = np.where(to_set_to_zero)[0]
        idx_to_randomize_phase = np.where(randomize_phase)[0]
        if len(idx_to_set_to_one) > 0:
            tss._do_bare_instruction(
                self._construct_stim_instruction("R", idx_to_set_to_one)
            )
            tss._do_bare_instruction(
                self._construct_stim_instruction("X", idx_to_set_to_one)
            )
        if len(idx_to_set_to_zero) > 0:
            tss._do_bare_instruction(
                self._construct_stim_instruction("R", idx_to_set_to_zero)
            )
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

        targets: list[int]
        ## The case of MPAD with LEAKAGE_MEASUREMENT tag
        if params.targets:
            targets = list(params.targets)
            if len(op.targets_copy()) != len(targets):
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
            targets = [t.value for t in op.targets_copy()]
            if op.name != "M":
                raise ValueError(
                    f"LEAKAGE_PROJECTION_Z is only implemented for 'M' operations, got {op}"
                )

        target_mask = np.zeros_like(self.state)
        target_mask[targets] = 1

        n_qbt = max(tss.num_qubits_in_new_circuit(), max(targets) + 1)

        # get actual qubit states
        target_leakage_state = self.state[targets]

        # Get pauli_z states and target qubits in the qubit space
        targets_in_qubit_space = np.array(targets)[target_leakage_state < 2]
        pauli_states = np.array(
            tss.get_current_noisy_tableau_pauli_state(targets_in_qubit_space, "Z")
        )

        idx_of_targets_in_qubit_space = np.where(target_leakage_state < 2)[0]

        targets_in_qubit_space_0_mask = pauli_states == 1
        targets_in_qubit_space_1_mask = pauli_states == -1
        targets_in_qubit_space_flip_mask = pauli_states == 0

        targets_new = np.array(targets)

        # Apply X flip errors based on the projection probabilities for |0> state
        target_states_0_idx = targets_in_qubit_space[targets_in_qubit_space_0_mask]
        if len(target_states_0_idx) > 0:
            if params.targets:
                new_targets_0_idx = list(range(n_qbt, n_qbt + len(target_states_0_idx)))
                n_qbt += len(target_states_0_idx)
                targets_new[
                    idx_of_targets_in_qubit_space[targets_in_qubit_space_0_mask]
                    ] = new_targets_0_idx
            else:
                new_targets_0_idx = target_states_0_idx
            if 0 in params.prob_for_input_state:
                tss._do_bare_only_on_tableau(
                    self._construct_stim_instruction(
                        "X_ERROR", new_targets_0_idx, [params.prob_for_input_state[0]]
                    )
                )
        # Apply X flip errors based on the projection probabilities for |1> state
        target_states_1_idx = targets_in_qubit_space[targets_in_qubit_space_1_mask]
        if len(target_states_1_idx) > 0:
            if params.targets:
                new_targets_1_idx = list(range(n_qbt, n_qbt + len(target_states_1_idx)))
                n_qbt += len(target_states_1_idx)
                targets_new[
                    idx_of_targets_in_qubit_space[targets_in_qubit_space_1_mask]
                    ] = new_targets_1_idx
                tss._do_bare_only_on_tableau(
                    self._construct_stim_instruction(
                        "X", new_targets_1_idx
                    )
                )
            else:
                new_targets_1_idx = target_states_1_idx
            if 1 in params.prob_for_input_state:
                tss._do_bare_only_on_tableau(
                    self._construct_stim_instruction(
                        "X_ERROR", new_targets_1_idx, [1.0 - params.prob_for_input_state[1]]
                    )
                )
        # Reset the states that are 50/50, and apply X flips according to the composite probability
        target_states_flip_idx = targets_in_qubit_space[targets_in_qubit_space_flip_mask]
        x_error_prob_0 = 0.5 * params.prob_for_input_state[0] if 0 in params.prob_for_input_state else 0.0
        x_error_prob_1 = 0.5 * params.prob_for_input_state[1] if 1 in params.prob_for_input_state else 0.5
        x_error_prob = x_error_prob_0 + x_error_prob_1
        if len(target_states_flip_idx) > 0:
            if params.targets:
                new_targets_flip_idx = list(
                    range(n_qbt, n_qbt + len(target_states_flip_idx))
                )
                n_qbt += len(target_states_flip_idx)
                targets_new[
                    idx_of_targets_in_qubit_space[targets_in_qubit_space_flip_mask]
                    ] = new_targets_flip_idx
            else:
                new_targets_flip_idx = target_states_flip_idx
                tss._do_bare_only_on_tableau(
                    self._construct_stim_instruction(
                        "R", new_targets_flip_idx
                    )
                )
            if x_error_prob > 0:
                tss._do_bare_only_on_tableau(
                    self._construct_stim_instruction(
                        "X_ERROR", new_targets_flip_idx, [x_error_prob]
                    )
                )
        # Reset the qubits in leaked states (>1), and apply X flips according to the specified probabilities
        target_leaked_idx = np.where(target_leakage_state > 1)[0]
        if len(target_leaked_idx) > 0:
            tss._do_bare_only_on_tableau(
                self._construct_stim_instruction("R", target_leaked_idx)
            )
            for n in range(2, np.max(target_leakage_state) + 1):
                if n in target_leakage_state and n in params.prob_for_input_state:
                    idx_n = target_leaked_idx[
                        target_leakage_state[target_leaked_idx] == n
                    ]
                    tss._do_bare_only_on_tableau(
                        self._construct_stim_instruction(
                            "X_ERROR", idx_n, [params.prob_for_input_state[n]]
                        )
                    )
        # Finally perform qubit space measurement
        if params.targets:
            tss._do_bare_only_on_tableau(
                self._construct_stim_instruction("M", targets_new)
            )
            tss._append_to_new_reference_circuit(op)
        else:
            tss._do_bare_instruction(
                self._construct_stim_instruction("M", targets)
            )