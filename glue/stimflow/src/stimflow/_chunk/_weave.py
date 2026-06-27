import itertools
from collections.abc import Callable, Generator, Iterable, Iterator
from typing import TypeVar

import stim

T = TypeVar("T")


def pairs(iterable: Iterable[T]) -> Iterator[tuple[T, T]]:
    prev = None
    has_prev = False
    for e in iterable:
        if has_prev:
            yield prev, e
        else:
            prev = e
        has_prev ^= True


class StimCircuitLoom:
    """Class for combining stim circuits running in parallel at separate locations.

    for standard usage, call StimCircuitLoom.weave(...), which returns the weaved circuit
    for usage details, see the docstring to that function

    for complex usage, you can instantiate a loom StimCircuitLoom(...)
    This is lets you access details of the weaving afterward, such as the measurement mapping
    """

    NON_MATCHING_INSTRUCTIONS = ["DETECTOR", "OBSERVABLE_INCLUDE", "QUBIT_COORDS", "SHIFT_COORDS"]

    @classmethod
    def weave(
        cls,
        c0: stim.Circuit,
        c1: stim.Circuit,
        sweep_bit_func: Callable[[int, int], int] | None = None,
    ) -> stim.Circuit:
        """Combines two stim circuits instruction by instruction.

        Example usage:
            StimCircuitLoom.weave(circuit_0, circuit_1) -> stim.Circuit

        Expects that the input circuit have 'matching instructions', in that they
        contain exactly the same sequence of instructions which can be matched up
        1-to-1. This may require one circuit to have instructions with no targets,
        purely to match instructions in the other circuit. Exceptions to this are
        the annotation instructions DETECTOR, OBSERVABLE_INCLUDE, QUBIT_COORDS,
        and SHIFT_COORDS, which do not need a matching statement in the other
        circuit. This may not be what you want, as it will produce duplicate
        DETECTOR or QUBIT_COORD instructions if they are included in both circuits.
        The annotation TICK is considered a matching instruction.

        Generally, instructions are combined by placing all targets from the
        first circuit instruction, followed by all targets from the second.

        In most gates, if a gate target is present in the first instruction
        target list, it is removed from the second instructions target list.
        As such, we do not permit instructions in the input circuits to have
        duplicate targets. This avoids the ambiguity of deciding whether one
        or both duplicates between circuits have to match up.

        Measure record targets are adjusted to point to the correct record in the
        combined circuit e.g. DETECTOR rec[-1] or CX rec[-1] 1

        Sweep bits are not handled by default, and will produce a ValueError.
        If sweep_bit_func is provided, it will be used to produce new sweep bit
        targets as follows:
            new_sweep_bit_index = sweep_bit_func(circuit_index, sweep_bit_index)
            where:
                circuit_index = 0 for circuit_0 and 1 for circuit_1
                sweep_bit_index is the sweep bit index used in the input circuit
        """
        return cls(c0, c1, sweep_bit_func).circuit

    def __init__(
        self,
        c0: stim.Circuit,
        c1: stim.Circuit,
        sweep_bit_func: Callable[[int, int], int] | None = None,
    ):
        self.circuit = stim.Circuit()
        self.sweep_bit_func = sweep_bit_func

        self._c0_global_meas_idxs: list[int] = []
        self._c1_global_meas_idxs: list[int] = []

        self._num_global_meas: int = 0
        # this isn't necessarily just the sum of meas in each sub-circuit
        # (because we could have combined measurements)
        # or the number of measurements actually added to self.circuit
        # (because we could be halfway through processing an M instruction)

        self._c0_iter = enumerate(iter(c0))
        self._c1_iter = enumerate(iter(c1))

        self._weave()

    # PUBLIC INTERFACES

    def weaved_target_rec_from_c0(self, target_rec: int) -> int:
        """given a target rec in circuit_0, return the equiv rec in the weaved circuit.

        args:
            target_rec: a valid measurement record target in the input circuit
                follows python indexing semantics:
                can be either positive (counting from the start of the circuit, 0 indexed)
                or negative (counting from the end backwards, last measurement is  [-1])
                The second is compatible with stim instruction target rec values

        returns:
            The same measurements target rec in the weaved circuit.
                Always returns a negative 'lookback' compatible with a stim circuit
                Add StimCircuitWeave.circuit.num_measurements for an absolute measurement index
        """
        return self._global_lookback(
            local_lookback=target_rec, global_meas_idxs=self._c0_global_meas_idxs
        )

    def weaved_target_rec_from_c1(self, target_rec: int) -> int:
        """given a target rec in circuit_1, return the equiv rec in the weaved circuit."""
        return self._global_lookback(
            local_lookback=target_rec, global_meas_idxs=self._c1_global_meas_idxs
        )

    # PRIVATE METHODS

    def _add_c0_measurement(self):
        self._c0_global_meas_idxs.append(self._num_global_meas)
        self._num_global_meas += 1

    def _add_c1_measurement(self):
        self._c1_global_meas_idxs.append(self._num_global_meas)
        self._num_global_meas += 1

    def _dedup_c0_measurement(self, lookback_from_current_state: int):
        # for when a c0 meas target duplicates a c0 meas target
        self._c0_global_meas_idxs.append(self._num_global_meas + lookback_from_current_state)
        # don't increment num_global_meas

    def _dedup_c1_measurement(self, lookback_from_current_state: int):
        # for when a c1 meas target duplicates a c1 or c0 meas target
        self._c1_global_meas_idxs.append(self._num_global_meas + lookback_from_current_state)
        # don't increment num_global_meas

    def _global_lookback(self, local_lookback: int, global_meas_idxs: list[int]) -> int:
        """computes meas rec lookbacks in the combined circuit from ones in the local circuit."""
        return global_meas_idxs[local_lookback] - self._num_global_meas

    def _matching_instructions_generator(
        self, circuit_iter: Iterator, global_meas_idxs: list[int]
    ) -> Generator[tuple[int, stim.CircuitInstruction]]:
        while True:
            try:
                i, op = next(circuit_iter)
            except StopIteration:
                return  # ends the generator
            if op.name in self.NON_MATCHING_INSTRUCTIONS:
                self._handle_non_matching_operations(op=op, global_meas_idxs=global_meas_idxs)
            else:
                yield i, op

    def _weave(self):
        # we use generators so that we can only handle the matching case here:
        # the generators handle the nonmatching case internally

        c0_gen = self._matching_instructions_generator(self._c0_iter, self._c0_global_meas_idxs)
        c1_gen = self._matching_instructions_generator(self._c1_iter, self._c1_global_meas_idxs)

        for (i, op0), (j, op1) in zip(c0_gen, c1_gen):

            if op0.name != op1.name:
                raise ValueError(f"Mismatched ops at position {i}: {op0}, {j}: {op1}")

            if op0.gate_args_copy() != op1.gate_args_copy():
                raise ValueError(
                    "Mismatched op arguments at position "
                    f"{i}: {op0}({op0.gate_args_copy()}), {j}: {op1}({op1.gate_args_copy()})"
                )

            self._handle_matching_operations(op0, op1)

        # Make sure both generators are done
        # The fetch here is also what handles any trailing nonmatching instructions
        for i, op0 in c0_gen:
            raise ValueError(f"Unmatched operation in c0 {i}:{op0}")
        for j, op1 in c1_gen:
            raise ValueError(f"Unmatched operation in c1 {j}:{op1}")

    def _handle_matching_operations(
        self, op0: stim.CircuitInstruction, op1: stim.CircuitInstruction
    ):

        gd = stim.gate_data(op0.name)
        if gd.produces_measurements:
            if gd.is_single_qubit_gate:
                self._handle_sq_m_gates(op0, op1)
            elif gd.is_two_qubit_gate:
                raise NotImplementedError("multiqubit measurement are not supported")
            elif gd.takes_pauli_targets:
                raise NotImplementedError("arbitrary pauli measurements are not supported")
            elif op0.name == "MPAD":
                raise NotImplementedError("MPAD not supported")
            else:
                raise ValueError(f"Unrecognised measurement operation {op0.name}")
        else:
            if op0.name == "TICK":
                self.circuit.append("TICK")
            elif gd.is_single_qubit_gate:
                self._handle_sq_u_gates(op0, op1)
            elif gd.is_two_qubit_gate:
                self._handle_2q_u_gates(op0, op1)
            elif gd.takes_pauli_targets:
                raise NotImplementedError("arbitrary pauli gates are not supported")
            else:
                raise ValueError(f"Unrecognised operation {op0.name}")

    def _handle_sq_m_gates(self, op0: stim.CircuitInstruction, op1: stim.CircuitInstruction):

        op0_targets = op0.targets_copy()
        op1_targets = op1.targets_copy()

        targets = []

        for t in op0_targets:
            if t not in targets:
                targets.append(t)
                self._add_c0_measurement()
            else:
                raise ValueError(f"Duplicate gate target {t} in c0:{op0}")

        for ti, t in enumerate(op1_targets):
            if t not in targets:
                targets.append(t)
                self._add_c1_measurement()
            elif t in op1_targets[:ti]:
                raise ValueError(f"Duplicate gate target {t} in c0:{op0}")
            else:
                lookback = targets.index(t) - len(targets)  # lookback is -ve
                self._dedup_c1_measurement(lookback)

        self.circuit.append(name=op0.name, targets=targets, arg=op0.gate_args_copy())

    def _handle_sq_u_gates(self, op0: stim.CircuitInstruction, op1: stim.CircuitInstruction):
        # easy mode, dedup targets,
        op0_targets = op0.targets_copy()
        op1_targets = op1.targets_copy()

        targets = []
        for t in op0_targets:
            if t not in targets:
                targets.append(t)
            else:
                raise ValueError(f"Duplicate target {t} in SQ gate {op0} in circuit 0")
        for ti, t in enumerate(op1_targets):
            if t not in targets:
                targets.append(t)
            elif t in op1_targets[:ti]:
                raise ValueError(f"Duplicate target {t} in SQ gate {op1} in circuit 1")
            # otherwise it's in there already, leave it be

        self.circuit.append(name=op0.name, targets=targets, arg=op0.gate_args_copy())

    def _handle_2q_u_gates(self, op0: stim.CircuitInstruction, op1: stim.CircuitInstruction):
        # combine the targets in pairs, also check for rec or sweep targets
        op0_targets = op0.targets_copy()
        op1_targets = op1.targets_copy()

        touched_qubit_targets = set()
        target_pairs: list[tuple[stim.GateTarget, stim.GateTarget]] = []

        # OP0 TARGETS
        for pair in pairs(op0_targets):
            if pair in target_pairs:
                raise ValueError(f"Duplicate target pair {pair} in 2Q gate {op0} in circuit 0")
            if pair[::-1] in target_pairs:
                raise ValueError(
                    f"Duplicate reversed target pair {pair}/{pair[::-1]} "
                    f"in 2Q gate {op0} in circuit 0"
                )

            new_pair = []
            for t in pair:
                if t.is_qubit_target:
                    if t in touched_qubit_targets:
                        raise ValueError(f"Duplicate target {t} in 2Q gate {op0} in circuit 0")
                    touched_qubit_targets.add(t)
                    new_t = t
                elif t.is_measurement_record_target:
                    new_t = stim.target_rec(
                        self._global_lookback(
                            local_lookback=t.value, global_meas_idxs=self._c0_global_meas_idxs
                        )
                    )
                elif t.is_sweep_bit_target:
                    if self.sweep_bit_func is None:
                        raise ValueError(
                            f"Can't handle sweep bit target {t} in {op0} in circuit 0 "
                            "when sweep_bit_func is not provided."
                        )
                    new_t = stim.target_sweep_bit(self.sweep_bit_func(0, t.value))
                else:
                    raise ValueError(f"Unrecognised GateTarget {t} in {op0} in circuit 0")
                new_pair.append(new_t)

            target_pairs.append(tuple(new_pair))

        # OP1 TARGETS
        # this time, we have to use a list because we have to be able to index into it
        op1_target_pairs = list(pairs(op1_targets))
        for pi, pair in enumerate(op1_target_pairs):
            if pair in target_pairs:
                if pair in op1_target_pairs[:pi]:
                    raise ValueError(f"Duplicate target pair {pair} in 2Q gate {op1} in circuit 1")
                else:  # it was in op0
                    continue
            if pair[::-1] in target_pairs:
                raise ValueError(
                    f"Duplicate reversed target pair {pair}/{pair[::-1]}"
                    f" in 2Q gate {op0} in circuit 1"
                )

            new_pair = []
            for t in pair:
                if t.is_qubit_target:
                    if t in touched_qubit_targets:
                        raise ValueError(f"Duplicate target {t} in 2Q gate {op1} in circuit 1")
                    else:
                        touched_qubit_targets.add(t)
                        new_t = t
                elif t.is_measurement_record_target:
                    new_t = stim.target_rec(
                        self._global_lookback(
                            local_lookback=t.value, global_meas_idxs=self._c1_global_meas_idxs
                        )
                    )
                elif t.is_sweep_bit_target:
                    if self.sweep_bit_func is None:
                        raise ValueError(
                            f"Can't handle sweep bit target {t} in {op1} in circuit 1 "
                            "when sweep_bit_func is not provided."
                        )
                    new_t = stim.target_sweep_bit(self.sweep_bit_func(1, t.value))
                else:
                    raise ValueError(f"Unrecognised GateTarget {t} in {op1} in circuit 1")
                new_pair.append(new_t)

            target_pairs.append(tuple(new_pair))

        targets = list(itertools.chain.from_iterable(target_pairs))

        self.circuit.append(name=op0.name, targets=targets, arg=op0.gate_args_copy())

    def _handle_non_matching_operations(
        self, op: stim.CircuitInstruction, global_meas_idxs: list[int]
    ):
        targets = []
        for t in op.targets_copy():
            if t.is_measurement_record_target:
                targets.append(
                    stim.target_rec(
                        lookback_index=self._global_lookback(
                            local_lookback=t.value, global_meas_idxs=global_meas_idxs
                        )
                    )
                )
            else:
                raise ValueError(f"Unrecognised target {t} of non-matching op {op}")
        self.circuit.append(name=op.name, targets=targets, arg=op.gate_args_copy())
