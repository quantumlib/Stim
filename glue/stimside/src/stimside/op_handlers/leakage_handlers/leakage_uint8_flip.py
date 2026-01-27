import dataclasses
import itertools as it

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


@dataclasses.dataclass
class CompiledLeakageUint8(CompiledOpHandler[FlipsideSimulator]):

    num_qubits: int
    batch_size: int
    state: NDArray[np.uint8] = dataclasses.field(init=False)

    ops_to_params: dict[stim.CircuitInstruction, LeakageParams]

    # This is a safety feature
    # you should generally not turn it off unless you are performing testing
    # or are handling the depolarization of leaked qubits yourself somehow.
    # That said, if you are implementing physically realistic errors using appropriate tags,
    # and not accidentally letting the computational states of leaked qubits sneak out at measurements
    # this behaviour should not strictly be necessary
    _depolarize_on_leak: bool = True

    def __post_init__(self):
        self.clear()

    def clear(self):
        """just allocate a new state array
        in numpy this is faster than zeroing things out...

        """
        self.state = np.zeros(shape=(self.num_qubits, self.batch_size), dtype=np.uint8)

    def make_target_mask(self, op: stim.CircuitInstruction) -> Bool2DArray:
        """return a bool mask that is true if this qubit is in the op targets."""
        target_indices = [t.value for t in op.targets_copy()]
        target_mask = np.zeros_like(self.state, dtype=bool)
        target_mask[target_indices, :] = 1
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

        if op not in self.ops_to_params:
            sss.do_on_flip_simulator(op)
            if self._depolarize_on_leak and op.name in [
                "M",
                "MX",
                "MY",
                "R",
                "RX",
                "RY",
                "MR",
                "MRX",
                "MRY",
            ]:
                # if we're depolarizing on leak, these instructions undo that error, so we re-depolarize
                self._depolarize_leaked_qubits(fss=sss, op=op)
            return

        params = self.ops_to_params[op]
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

    def _depolarize_leaked_qubits(
        self, op: stim.CircuitInstruction, fss: FlipsideSimulator
    ):
        """Fully depolarize qubits that are leaked."""
        target_mask = self.make_target_mask(op)
        mask = np.logical_and(target_mask, self.state >= 2)  # in leakage state
        fss.broadcast_pauli_errors(error_mask=mask, p=0.5, pauli="X")
        fss.broadcast_pauli_errors(error_mask=mask, p=0.5, pauli="Z")
        fss.do_on_flip_simulator(op)

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

        # controller state is an array where each qubit has the leakage state
        # of the qubit controlling it (or 0 if it's not being targeted)
        controller_state = np.zeros_like(self.state, dtype=np.uint8)
        targets = [t.value for t in op.targets_copy()]
        for t0, t1 in it.batched(targets, 2):
            controller_state[t1, :] = self.state[t0, :]

        for p, s0, pauli in params.args:
            assert s0 != 0  # 0 is reserved for qubits not in the targets list
            # but the parsing should already have checked that s0 isn't 0 or 1
            mask = controller_state == s0  # correct state
            fss.broadcast_pauli_errors(error_mask=mask, p=p)  # error with probability
            fss.do_on_flip_simulator(op)

    def leakage_transition_1(
        self,
        op: stim.CircuitInstruction,
        fss: FlipsideSimulator,
        params: LeakageTransition1Params,
    ):
        """implement leakage state transitions on single qubits.

        when a qubit transitions to the unknown unleaked state U, we fully depolarize it.
        """

        target_mask = self.make_target_mask(op)

        to_depolarize = np.zeros_like(self.state, dtype=bool)

        for input_state in params.args_by_input_state.keys():

            if input_state == "U":
                input_state_mask = self.state < 2
            else:
                input_state_mask = self.state == input_state

            overwrite_mask = np.logical_and(target_mask, input_state_mask)

            samples_to_take = np.count_nonzero(overwrite_mask)
            if samples_to_take == 0:
                continue

            output_states = params.sample_transitions_from_state(
                input_state=input_state, num_samples=samples_to_take, np_rng=fss.np_rng
            )

            was_unleaked = output_states == "U"
            output_states[was_unleaked] = 0
            output_states = output_states.astype(np.uint8)
            # output_states now all ints

            self.state[overwrite_mask] = output_states

            if self._depolarize_on_leak:
                was_leaked = output_states >= 2
                to_depolarize[overwrite_mask] = np.logical_or(was_leaked, was_unleaked)
            else:
                to_depolarize[overwrite_mask] = was_unleaked

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

        targets = [t.value for t in op.targets_copy()]

        if not fss._all_targets_in_known_state(targets=targets, pauli="Z"):
            raise ValueError(
                f"{op} has the tag LEAKAGE_TRANSITIONS_Z that demands known Z states, but targets aren't in known Z states."
            )

        state_before_op = self.state.copy()
        # so we don't accidentally chain together transitions as we're altering self.state

        target_mask = self.make_target_mask(op)
        in_0_mask, in_1_mask = fss._get_current_known_state_masks(pauli="Z")

        # for accumulating which qubits to invert the computational Z state
        flip_Z_state = np.zeros_like(self.state, dtype=bool)
        # for accumulating which qubits need to have their phase randomized
        randomize_phase = np.zeros_like(self.state, dtype=bool)

        if self._depolarize_on_leak:
            to_depolarize = np.zeros_like(self.state, dtype=bool)

        for input_state in params.args_by_input_state.keys():
            if input_state == 0:
                in_state_mask = in_0_mask
            elif input_state == 1:
                in_state_mask = in_1_mask
            else:
                in_state_mask = state_before_op == input_state

            overwrite_mask = np.logical_and(target_mask, in_state_mask)
            # this is a mask that is true for target qubits in the correct state

            samples_to_take = np.count_nonzero(overwrite_mask)
            if samples_to_take == 0:
                continue

            output_states = params.sample_transitions_from_state(
                input_state=input_state, num_samples=samples_to_take, np_rng=fss.np_rng
            )

            # find qubits inside the overwrite mask that aren't in the correct Z state
            # ie where in_0/in_1 disagrees with the newly sampled output_state
            output_1_and_in_0 = np.logical_and(
                in_0_mask[overwrite_mask], (output_states == 1)
            )
            output_0_and_in_1 = np.logical_and(
                in_1_mask[overwrite_mask], (output_states == 0)
            )
            in_wrong_z_state = np.logical_or(output_1_and_in_0, output_0_and_in_1)
            flip_Z_state[overwrite_mask] = in_wrong_z_state

            # randomize the phase of any qubit that changed state
            randomize_phase[overwrite_mask] = output_states != input_state

            if input_state <= 1 and self._depolarize_on_leak:
                to_depolarize[overwrite_mask] = output_states >= 2

            output_states[output_states == 1] = (
                0  # don't pollute the state array with 1s
            )
            self.state[overwrite_mask] = (
                output_states  # put the new leakage states in self.state
            )

        # use broadcast error to invert the qubits that need to change computational state
        fss.broadcast_pauli_errors(error_mask=flip_Z_state, pauli="X", p=1)
        # also broadcast errors to randomize the phase of qubits in fixed Z states
        fss.broadcast_pauli_errors(error_mask=randomize_phase, pauli="Z", p=0.5)

        if self._depolarize_on_leak:
            if np.count_nonzero(to_depolarize):
                fss.broadcast_pauli_errors(error_mask=to_depolarize, pauli="Z", p=0.5)
                fss.broadcast_pauli_errors(error_mask=to_depolarize, pauli="X", p=0.5)

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

        # odd_target_mask, even_target_mask = self.make_pair_target_masks(op)
        targets_list = [t.value for t in op.targets_copy()]
        even_targets_list = targets_list[::2]
        odd_targets_list = targets_list[1::2]

        # extract out the states on just the targets in order
        # a 2D array sliced by a list compiles an array for just those rows
        even_target_states = self.state[even_targets_list]
        odd_target_states = self.state[odd_targets_list]
        # these are shape (#target_pairs, batch_size)

        to_depolarize = np.zeros_like(self.state, dtype=bool)

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

            samples_to_take = np.count_nonzero(target_state_mask)
            # we take a sample for each pair of targets in the right state

            # actually do the alias sampling
            output_states = params.sample_transitions_from_state(
                input_state=input_state, num_samples=samples_to_take, np_rng=fss.np_rng
            )
            # output_states has shape=(samples_to_take, 2={even, odd})

            # now we handle the various unleaked states, U and V
            # if a qubit went U-->V, or L-->U we need to depolarize it
            # (We're already banned L-->V at the parsing stage)
            # if it went L-->L or U-->U we do not depolarize it
            if input_state[0] == "U":
                depolarize_even_qubit = output_states[:, 0] == "V"
            else:  # Leaked
                depolarize_even_qubit = output_states[:, 0] == "U"

            if input_state[1] == "U":
                depolarize_odd_qubit = output_states[:, 1] == "V"
            else:  # Leaked
                depolarize_odd_qubit = output_states[:, 1] == "U"

            # actually overwrite the leakage states
            output_states[output_states == "U"] = 0
            output_states[output_states == "V"] = 0
            output_states = output_states.astype(np.uint8)
            self.state[even_overwrite_mask] = output_states[:, 0]
            self.state[odd_overwrite_mask] = output_states[:, 1]

            if self._depolarize_on_leak:
                # output_states have already been cleared out to 0s
                if input_state[0] == "U":
                    depolarize_odd_qubit[output_states[:, 0] >= 2] = True
                if input_state[1] == "U":
                    depolarize_even_qubit[output_states[:, 1] >= 2] = True

            to_depolarize[odd_overwrite_mask] = depolarize_odd_qubit
            to_depolarize[even_overwrite_mask] = depolarize_even_qubit

        fss.broadcast_pauli_errors(error_mask=to_depolarize, pauli="Z", p=0.5)
        fss.broadcast_pauli_errors(error_mask=to_depolarize, pauli="X", p=0.5)
        fss.do_on_flip_simulator(op)

    def leakage_projection_Z(
        self,
        op: stim.CircuitInstruction,
        fss: FlipsideSimulator,
        params: LeakageMeasurementParams,
    ):
        """Implement measurement projections for qubits in known Z eigenstates.

        This behaviour fully replaces the operation of an M gate, including
        appending to the measurement record and randomizing the phase of all target qubits.

        This behaviour does not change the leakage states of the qubits.
        """

        targets = np.array([t.value for t in op.targets_copy()])
        target_mask = np.zeros_like(self.state, dtype=bool)
        target_mask[targets, :] = True

        if op.name != "M":
            raise ValueError(
                f"LEAKAGE_PROJECTION_Z is only implemented for 'M' operations, got {op}"
            )

        if not fss._all_targets_in_known_state(targets=targets, pauli="Z"):
            raise ValueError(
                f"{op} has a LEAKAGE_PROJECTION_Z tag that demands known Z states, but targets aren't in known Z states."
            )

        # get actual qubit states
        _, known_state_mask = fss._get_current_known_state_masks(pauli="Z")
        # known_state_mask is true when the qubit is in -Z ie the 1 state
        # targets that are not in -Z are in +Z because we checked they're all in known Z states
        # TODO don't do the all_targets_in_known_state check and then remake the known_state_mask
        # pull the get_current_known_state_masks and check the targets are known yourself

        target_computational_states = known_state_mask[targets, :]
        target_leakage_state = self.state[targets, :]

        target_is_not_leaked = target_leakage_state < 2

        target_outcomes = np.zeros_like(target_computational_states, dtype=bool)

        target_states_0_mask = np.logical_and(
            target_computational_states == 0, target_is_not_leaked
        )
        target_outcomes[target_states_0_mask] = params.sample_projections(
            state=0,
            num_samples=np.count_nonzero(target_states_0_mask),
            np_rng=fss.np_rng,
        )

        target_states_1_mask = np.logical_and(
            target_computational_states == 1, target_is_not_leaked
        )
        target_outcomes[target_states_1_mask] = params.sample_projections(
            state=1,
            num_samples=np.count_nonzero(target_states_1_mask),
            np_rng=fss.np_rng,
        )

        for n in range(2, np.max(target_leakage_state) + 1):
            target_state_mask = target_leakage_state == n
            target_outcomes[target_state_mask] = params.sample_projections(
                state=n,
                num_samples=np.count_nonzero(target_state_mask),
                np_rng=fss.np_rng,
            )

        # implement the measurement gate:
        fss.append_measurement_flips(measurement_flip_data=target_outcomes)
        # randomize the non-commuting basis
        fss.broadcast_pauli_errors(error_mask=target_mask, pauli="X", p=0.5)
