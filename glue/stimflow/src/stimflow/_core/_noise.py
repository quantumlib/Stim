from __future__ import annotations

import collections
from collections.abc import Iterable, Iterator, Set
from typing import Any

import stim

ANNOTATION_OPS = {"DETECTOR", "OBSERVABLE_INCLUDE", "QUBIT_COORDS", "SHIFT_COORDS", "TICK", "MPAD"}
OP_TO_MEASURE_BASES = {
    "M": "Z",
    "MR": "Z",
    "MX": "X",
    "MY": "Y",
    "MZ": "Z",
    "MRX": "X",
    "MRY": "Y",
    "MRZ": "Z",
    "MXX": "XX",
    "MYY": "YY",
    "MZZ": "ZZ",
    "MPP": "*",
}


class NoiseRule:
    """Describes how to add noise to an operation."""

    def __init__(
        self,
        *,
        before: dict[str, float | tuple[float, ...]] | None = None,
        after: dict[str, float | tuple[float, ...]] | None = None,
        flip_result: float = 0,
    ):
        """Initializes a NoiseRule.

        Args:
            before: A name-to-argument mapping of noise instructions to add before some
                instruction that is being made noisy. For example,
                    after={"DEPOLARIZE2": 0.01, "X_ERROR": 0.02}
                will add two qubit depolarization with parameter 0.01 and also add 2%
                bit flip noise. These noise channels occur before all other operations
                in the moment and are applied to the same targets as the relevant operation.
            after: A name-to-argument mapping of noise instructions to add after some
                instruction that is being made noisy. For example,
                    after={"DEPOLARIZE2": 0.01, "X_ERROR": 0.02}
                will add two qubit depolarization with parameter 0.01 and also add 2%
                bit flip noise. These noise channels occur after all other operations
                in the moment and are applied to the same targets as the relevant operation.
            flip_result: The probability that a measurement result should be reported incorrectly.
                Only valid when applied to operations that produce measurement results.

        Examples:
            >>> import stim
            >>> import stimflow as sf
            >>> noise = sf.NoiseModel(gate_rules={
            ...     'R': sf.NoiseRule(after={"X_ERROR": 5e-3}),
            ...     'M': sf.NoiseRule(flip_result=1e-3),
            ...     'CZ': sf.NoiseRule(after={"Z_ERROR": 3e-3, "DEPOLARIZE2": 1e-3}),
            ...     'H': sf.NoiseRule(before={"PAULI_CHANNEL_1": (1e-3, 1e-2, 1e-3)}),
            ... })
            >>> noise.noisy_circuit(stim.Circuit('''
            ...     R 1
            ...     TICK
            ...     H 1
            ...     TICK
            ...     CZ 0 1
            ...     TICK
            ...     CZ 2 1
            ...     TICK
            ...     H 1
            ...     TICK
            ...     M 1
            ... '''))
            stim.Circuit('''
                R 1
                X_ERROR(0.005) 1
                TICK
                PAULI_CHANNEL_1(0.001, 0.01, 0.001) 1
                H 1
                TICK
                CZ 0 1
                DEPOLARIZE2(0.001) 0 1
                Z_ERROR(0.003) 0 1
                TICK
                CZ 2 1
                DEPOLARIZE2(0.001) 2 1
                Z_ERROR(0.003) 2 1
                TICK
                PAULI_CHANNEL_1(0.001, 0.01, 0.001) 1
                H 1
                TICK
                M(0.001) 1
            ''')
        """
        if after is None:
            after = {}
        if before is None:
            before = {}
        if not (0 <= flip_result <= 1):
            raise ValueError(f"not (0 <= {flip_result=} <= 1)")
        for k, p_args in [*after.items(), *before.items()]:
            gate_data = stim.gate_data(k)
            if gate_data.produces_measurements or not gate_data.is_noisy_gate:
                raise ValueError(f"not a pure noise channel: {k} from {after=}")
            if gate_data.num_parens_arguments_range == range(1, 2):
                if not isinstance(p_args, (int, float)) or not (0 <= p_args <= 1):
                    raise ValueError(f"not a probability: {p_args!r}")
            else:
                if not isinstance(p_args, (list, tuple)) or not (0 <= sum(p_args) <= 1):
                    raise ValueError(f"not a tuple of disjoint probabilities: {p_args!r}")
                if len(p_args) not in gate_data.num_parens_arguments_range:
                    raise ValueError(f"Wrong number of arguments {p_args!r} for gate {k!r}")
        self.before: dict[str, float | tuple[float, ...]] = before
        self.after: dict[str, float | tuple[float, ...]] = after
        self.flip_result: float = flip_result

    def append_noisy_version_of(
        self,
        *,
        split_op: stim.CircuitInstruction,
        out_during_moment: stim.Circuit,
        before_moments: collections.defaultdict[Any, stim.Circuit],
        after_moments: collections.defaultdict[Any, stim.Circuit],
        immune_qubit_indices: Set[int],
    ) -> None:
        targets = split_op.targets_copy()
        if immune_qubit_indices and any(
            (t.is_qubit_target or t.is_x_target or t.is_y_target or t.is_z_target)
            and t.value in immune_qubit_indices
            for t in targets
        ):
            out_during_moment.append(split_op)
            return

        args = split_op.gate_args_copy()
        if self.flip_result:
            gate_data = stim.gate_data(split_op.name)
            assert gate_data.produces_measurements
            assert gate_data.is_noisy_gate
            assert gate_data.num_parens_arguments_range == range(0, 2)
            assert len(args) == 0
            args = [self.flip_result]

        out_during_moment.append(split_op.name, targets, args, tag=split_op.tag)
        raw_targets = [t.value for t in targets if not t.is_combiner]
        for op_name, arg in self.before.items():
            before_moments[(op_name, arg)].append(op_name, raw_targets, arg)
        for op_name, arg in self.after.items():
            after_moments[(op_name, arg)].append(op_name, raw_targets, arg)


class NoiseModel:
    """Converts circuits into noisy circuits according to rules."""

    def __init__(
        self,
        idle_depolarization: float = 0,
        tick_noise: NoiseRule | None = None,
        additional_depolarization_waiting_for_m_or_r: float = 0,
        gate_rules: dict[str, NoiseRule] | None = None,
        measure_rules: dict[str, NoiseRule] | None = None,
        any_measurement_rule: NoiseRule | None = None,
        any_clifford_1q_rule: NoiseRule | float | None = None,
        any_clifford_2q_rule: NoiseRule | float | None = None,
        allow_multiple_uses_of_a_qubit_in_one_tick: bool = False,
    ):
        if isinstance(any_clifford_1q_rule, float):
            any_clifford_1q_rule = NoiseRule(after={"DEPOLARIZE1": any_clifford_1q_rule})
        if isinstance(any_clifford_2q_rule, float):
            any_clifford_2q_rule = NoiseRule(after={"DEPOLARIZE2": any_clifford_2q_rule})
        self.idle_depolarization = idle_depolarization
        self.tick_noise = tick_noise
        self.additional_depolarization_waiting_for_m_or_r = (
            additional_depolarization_waiting_for_m_or_r
        )
        self.gate_rules = {} if gate_rules is None else gate_rules
        self.measure_rules = measure_rules
        self.any_measurement_rule = any_measurement_rule
        self.any_clifford_1q_rule = any_clifford_1q_rule
        self.any_clifford_2q_rule = any_clifford_2q_rule
        self.allow_multiple_uses_of_a_qubit_in_one_tick = allow_multiple_uses_of_a_qubit_in_one_tick
        assert self.tick_noise is None or not self.tick_noise.flip_result

    @staticmethod
    def si1000(p: float) -> NoiseModel:
        """Superconducting inspired noise.

        As defined in "A Fault-Tolerant Honeycomb Memory" https://arxiv.org/abs/2108.10457

        Small tweak when measurements aren't immediately followed by a reset: the measurement result
        is probabilistically flipped instead of the input qubit. The input qubit is depolarized
        after the measurement.
        """
        return NoiseModel(
            idle_depolarization=p / 10,
            additional_depolarization_waiting_for_m_or_r=2 * p,
            any_clifford_1q_rule=NoiseRule(after={"DEPOLARIZE1": p / 10}),
            any_clifford_2q_rule=NoiseRule(after={"DEPOLARIZE2": p}),
            measure_rules={
                "Z": NoiseRule(after={"DEPOLARIZE1": p}, flip_result=p * 5),
                "ZZ": NoiseRule(after={"DEPOLARIZE2": p}, flip_result=p * 5),
            },
            gate_rules={"R": NoiseRule(after={"X_ERROR": p * 2})},
        )

    @staticmethod
    def uniform_depolarizing(
            p: float,
            *,
            single_qubit_only: bool = False,
            allow_multiple_uses_of_a_qubit_in_one_tick: bool = False,
    ) -> NoiseModel:
        """Near-standard circuit depolarizing noise.

        Everything has the same parameter p.
        Single qubit clifford gates get single qubit depolarization.
        Two qubit clifford gates get single qubit depolarization.
        Dissipative gates have their result probabilistically bit flipped (or phase flipped if
        appropriate).

        Non-demolition measurement is treated a bit unusually in that it is the result that is
        flipped instead of the input qubit. The input qubit is depolarized.

        Args:
            single_qubit_only: Defaults to False. When False, two qubit gates apply two
                qubit depolarizing noise (DEPOLARIZE2). When True, they instead apply single qubit
                depolarizing noise (DEPOLARIZE1).
            allow_multiple_uses_of_a_qubit_in_one_tick: Defaults to False. When False, an error will be
                raised if attempting to add noise to a circuit that operates on a qubit
                multiple times between TICK operations. When set to True, no error is raised.
        """
        dep2 = "DEPOLARIZE1" if single_qubit_only else "DEPOLARIZE2"
        return NoiseModel(
            idle_depolarization=p,
            any_clifford_1q_rule=NoiseRule(after={"DEPOLARIZE1": p}),
            any_clifford_2q_rule=NoiseRule(after={dep2: p}),
            measure_rules={
                "X": NoiseRule(after={"DEPOLARIZE1": p}, flip_result=p),
                "Y": NoiseRule(after={"DEPOLARIZE1": p}, flip_result=p),
                "Z": NoiseRule(after={"DEPOLARIZE1": p}, flip_result=p),
                "XX": NoiseRule(after={dep2: p}, flip_result=p),
                "XY": NoiseRule(after={dep2: p}, flip_result=p),
                "XZ": NoiseRule(after={dep2: p}, flip_result=p),
                "YX": NoiseRule(after={dep2: p}, flip_result=p),
                "YY": NoiseRule(after={dep2: p}, flip_result=p),
                "YZ": NoiseRule(after={dep2: p}, flip_result=p),
                "ZX": NoiseRule(after={dep2: p}, flip_result=p),
                "ZY": NoiseRule(after={dep2: p}, flip_result=p),
                "ZZ": NoiseRule(after={dep2: p}, flip_result=p),
            },
            gate_rules={
                "RX": NoiseRule(after={"Z_ERROR": p}),
                "RY": NoiseRule(after={"X_ERROR": p}),
                "R": NoiseRule(after={"X_ERROR": p}),
            },
            allow_multiple_uses_of_a_qubit_in_one_tick=allow_multiple_uses_of_a_qubit_in_one_tick,
        )

    def _noise_rule_for_split_operation(
        self, *, split_op: stim.CircuitInstruction
    ) -> NoiseRule | None:
        if occurs_in_classical_control_system(split_op):
            return None

        rule = self.gate_rules.get(split_op.name)
        if rule is not None:
            return rule

        gate_data = stim.gate_data(split_op.name)

        if (
            self.any_clifford_1q_rule is not None
            and gate_data.is_unitary
            and gate_data.is_single_qubit_gate
        ):
            return self.any_clifford_1q_rule
        if (
            self.any_clifford_2q_rule is not None
            and gate_data.is_unitary
            and gate_data.is_two_qubit_gate
        ):
            return self.any_clifford_2q_rule
        if self.measure_rules is not None:
            rule = self.measure_rules.get(_measure_basis(split_op=split_op))
            if rule is not None:
                return rule
        if self.any_measurement_rule is not None and gate_data.produces_measurements:
            return self.any_measurement_rule
        if gate_data.is_reset and gate_data.produces_measurements:
            m_name, r_name = {"MRX": ("MX", "RX"), "MRY": ("MY", "RY"), "MR": ("M", "R")}[
                gate_data.name
            ]
            r_noise = self._noise_rule_for_split_operation(
                split_op=stim.CircuitInstruction(r_name, split_op.targets_copy(), tag=split_op.tag)
            )
            m_noise = self._noise_rule_for_split_operation(
                split_op=stim.CircuitInstruction(m_name, split_op.targets_copy(), tag=split_op.tag)
            )
            return NoiseRule(
                before=r_noise.before if r_noise is not None else {},
                after=r_noise.after if r_noise is not None else {},
                flip_result=m_noise.flip_result if m_noise is not None else 0,
            )

        raise ValueError(f"No noise (or lack of noise) specified for '{split_op}'.")

    def _append_idle_error(
        self,
        *,
        moment_split_ops: list[stim.CircuitInstruction],
        out: stim.Circuit,
        system_qubit_indices: Set[int],
        immune_qubit_indices: Set[int],
    ) -> None:
        collapse_qubits: list[int] = []
        clifford_qubits: list[int] = []
        pauli_qubits: list[int] = []
        for split_op in moment_split_ops:
            if occurs_in_classical_control_system(split_op):
                continue
            gate_data = stim.gate_data(split_op.name)
            qubits_out: list[int]
            if gate_data.is_reset or gate_data.produces_measurements:
                qubits_out = collapse_qubits
            elif split_op.name in "IXYZ":
                qubits_out = pauli_qubits
            elif gate_data.is_unitary:
                qubits_out = clifford_qubits
            else:
                raise NotImplementedError(f"{split_op=}")
            for target in split_op.targets_copy():
                if not target.is_combiner:
                    qubits_out.append(target.value)

        # Safety check for operation collisions.
        usage_counts = collections.Counter(collapse_qubits + clifford_qubits)
        for pauli_qubit in pauli_qubits:
            if usage_counts[pauli_qubit] == 0:
                usage_counts[pauli_qubit] += 1
        qubits_used_multiple_times = {q for q, c in usage_counts.items() if c != 1}
        if qubits_used_multiple_times and not self.allow_multiple_uses_of_a_qubit_in_one_tick:
            moment = stim.Circuit()
            for op in moment_split_ops:
                moment.append(op)
            raise ValueError(
                f"Qubits were operated on multiple times without a TICK in between:\n"
                f"multiple uses: {sorted(qubits_used_multiple_times)}\n"
                f"moment:\n"
                f"{moment}"
            )

        collapse_qubits_set = set(collapse_qubits)
        clifford_qubits_set = set(clifford_qubits + pauli_qubits)
        idle = sorted(
            system_qubit_indices - collapse_qubits_set - clifford_qubits_set - immune_qubit_indices
        )
        if idle and self.idle_depolarization:
            out.append("DEPOLARIZE1", idle, self.idle_depolarization)

        waiting_for_mr = sorted(system_qubit_indices - collapse_qubits_set - immune_qubit_indices)
        if (
            collapse_qubits_set
            and waiting_for_mr
            and self.additional_depolarization_waiting_for_m_or_r
        ):
            out.append("DEPOLARIZE1", idle, self.additional_depolarization_waiting_for_m_or_r)

        if self.tick_noise is not None:
            for k, p in self.tick_noise.before.items():
                out.append(k, system_qubit_indices - immune_qubit_indices, p)
            for k, p in self.tick_noise.after.items():
                out.append(k, system_qubit_indices - immune_qubit_indices, p)

    def _append_noisy_moment(
        self,
        *,
        moment_split_ops: list[stim.CircuitInstruction],
        out: stim.Circuit,
        system_qubits_indices: Set[int],
        immune_qubit_indices: Set[int],
    ) -> None:
        skip_pauli_targets: set[int] = set()
        for split_op in moment_split_ops:
            gate_data = stim.gate_data(split_op.name)
            if (
                gate_data.is_unitary
                and gate_data.is_single_qubit_gate
                and not split_op.name in "IXYZ"
            ):
                skip_pauli_targets.update(t.qubit_value for t in split_op.targets_copy())

        before: collections.defaultdict[Any, stim.Circuit] = collections.defaultdict(stim.Circuit)
        after: collections.defaultdict[Any, stim.Circuit] = collections.defaultdict(stim.Circuit)
        grow = stim.Circuit()
        for split_op in moment_split_ops:
            rule = self._noise_rule_for_split_operation(split_op=split_op)
            if rule is None:
                grow.append(split_op)
            elif split_op.name in "IXYZ":
                new_targets = []
                skipped_targets = []
                for t in split_op.targets_copy():
                    if t.qubit_value in skip_pauli_targets:
                        skipped_targets.append(t)
                    else:
                        new_targets.append(t)
                        skip_pauli_targets.add(t.qubit_value)
                if skipped_targets:
                    grow.append(
                        stim.CircuitInstruction(
                            split_op.name, skipped_targets, split_op.gate_args_copy(), tag=split_op.tag,
                        )
                    )
                if new_targets:
                    rule.append_noisy_version_of(
                        split_op=stim.CircuitInstruction(
                            split_op.name, new_targets, split_op.gate_args_copy(), tag=split_op.tag,
                        ),
                        out_during_moment=grow,
                        before_moments=before,
                        after_moments=after,
                        immune_qubit_indices=immune_qubit_indices,
                    )
            else:
                rule.append_noisy_version_of(
                    split_op=split_op,
                    out_during_moment=grow,
                    before_moments=before,
                    after_moments=after,
                    immune_qubit_indices=immune_qubit_indices,
                )
        for k in sorted(before.keys()):
            out += before[k]
        out += grow
        for k in sorted(after.keys()):
            out += after[k]

        self._append_idle_error(
            moment_split_ops=moment_split_ops,
            out=out,
            system_qubit_indices=system_qubits_indices,
            immune_qubit_indices=immune_qubit_indices,
        )

    def noisy_circuit_skipping_mpp_boundaries(
        self,
        circuit: stim.Circuit,
        *,
        immune_qubit_indices: Set[int] | None = None,
        immune_qubit_coords: Iterable[complex | float | int | Iterable[float | int]] | None = None,
    ) -> stim.Circuit:
        """Adds noise to the circuit except for MPP operations at the start/end.

        Divides the circuit into three parts: mpp_start, body, mpp_end. The mpp
        sections grow from the ends of the circuit until they hit an instruction
        that's not an annotation or an MPP. Then body is the remaining circuit
        between the two ends. Noise is added to the body, and then the pieces
        are reassembled.
        """
        allowed = {"TICK", "OBSERVABLE_INCLUDE", "DETECTOR", "MPP", "QUBIT_COORDS", "SHIFT_COORDS"}
        start = 0
        end = len(circuit)
        while start < len(circuit) and circuit[start].name in allowed:
            start += 1
        while end > 0 and circuit[end - 1].name in allowed:
            end -= 1
        if end <= start:
            raise ValueError("end <= start")
        noisy = self.noisy_circuit(
            circuit[start:end],
            immune_qubit_indices=_immune_indices(
                circuit, immune_qubit_indices, immune_qubit_coords
            ),
        )
        return circuit[:start] + noisy + circuit[end:]

    def noisy_circuit(
        self,
        circuit: stim.Circuit,
        *,
        system_qubit_indices: set[int] | None = None,
        immune_qubit_indices: Iterable[int] | None = None,
        immune_qubit_coords: Iterable[complex | float | int | Iterable[float | int]] | None = None,
    ) -> stim.Circuit:
        """Returns a noisy version of the given circuit, by applying the receiving noise model.

        Args:
            circuit: The circuit to layer noise over.
            system_qubit_indices: All qubits used by the circuit. These are the qubits eligible for
                idling noise.
            immune_qubit_indices: Qubits to not apply noise to, even if they are operated on.
            immune_qubit_coords: Qubit coordinates to not apply noise to, even if they are operated
                on.

        Returns:
            The noisy version of the circuit.
        """
        if system_qubit_indices is None:
            system_qubit_indices = set(range(circuit.num_qubits))
        immune_qubit_indices = _immune_indices(circuit, immune_qubit_indices, immune_qubit_coords)

        result = stim.Circuit()

        first = True
        for moment_split_ops in _iter_split_op_moments(
            circuit, immune_qubit_indices=immune_qubit_indices
        ):
            if first:
                first = False
            elif result and isinstance(result[-1], stim.CircuitRepeatBlock):
                pass
            else:
                result.append("TICK")
            if isinstance(moment_split_ops, stim.CircuitRepeatBlock):
                noisy_body = self.noisy_circuit(
                    moment_split_ops.body_copy(),
                    system_qubit_indices=system_qubit_indices,
                    immune_qubit_indices=immune_qubit_indices,
                )
                noisy_body.append("TICK")
                result.append(
                    stim.CircuitRepeatBlock(
                        repeat_count=moment_split_ops.repeat_count, body=noisy_body
                    )
                )
            else:
                self._append_noisy_moment(
                    moment_split_ops=moment_split_ops,
                    out=result,
                    system_qubits_indices=system_qubit_indices,
                    immune_qubit_indices=immune_qubit_indices,
                )

        return result


def occurs_in_classical_control_system(op: stim.CircuitInstruction) -> bool:
    """Determines if an operation is an annotation or a classical control system update."""
    if op.tag == 'noiseless-virtual':
        return True
    if op.name in ANNOTATION_OPS:
        return True

    gate_data = stim.gate_data(op.name)
    if gate_data.is_unitary and gate_data.is_two_qubit_gate:
        targets = op.targets_copy()
        for k in range(0, len(targets), 2):
            a = targets[k]
            b = targets[k + 1]
            classical_0 = a.is_measurement_record_target or a.is_sweep_bit_target
            classical_1 = b.is_measurement_record_target or b.is_sweep_bit_target
            if not (classical_0 or classical_1):
                return False
        return True
    return False


def _split_targets_if_needed(
    op: stim.CircuitInstruction, immune_qubit_indices: Set[int]
) -> Iterator[stim.CircuitInstruction]:
    """Splits operations into pieces as needed (e.g. MPP into each product, classical control away
    from quantum ops)."""
    gate_data = stim.gate_data(op.name)
    if gate_data.is_unitary and gate_data.is_two_qubit_gate:
        yield from _split_targets_if_needed_clifford_2q(op, immune_qubit_indices)
    elif op.name == "MPP":
        yield from _split_targets_if_needed_m_basis(op)
    elif op.name in ANNOTATION_OPS:
        yield op
    elif gate_data.is_noisy_gate and not gate_data.produces_measurements:
        yield op
    elif gate_data.is_single_qubit_gate:
        yield from _split_out_immune_targets_assuming_single_qubit_gate(op, immune_qubit_indices)
    elif gate_data.is_two_qubit_gate:
        yield from _split_out_immune_targets_assuming_two_qubit_gate(op, immune_qubit_indices)
    else:
        raise NotImplementedError(f"{op=}")


def _split_out_immune_targets_assuming_single_qubit_gate(
    op: stim.CircuitInstruction, immune_qubit_indices: Set[int]
) -> Iterator[stim.CircuitInstruction]:
    if immune_qubit_indices:
        args = op.gate_args_copy()
        for t in op.targets_copy():
            yield stim.CircuitInstruction(op.name, [t], args, tag=op.tag)
    else:
        yield op


def _split_out_immune_targets_assuming_two_qubit_gate(
    op: stim.CircuitInstruction, immune_qubit_indices: Set[int]
) -> Iterator[stim.CircuitInstruction]:
    if immune_qubit_indices:
        args = op.gate_args_copy()
        targets = op.targets_copy()
        for k in range(len(targets)):
            t1 = targets[k]
            t2 = targets[k + 1]
            yield stim.CircuitInstruction(op.name, [t1, t2], args, tag=op.tag)
    else:
        yield op


def _split_targets_if_needed_clifford_2q(
    op: stim.CircuitInstruction, immune_qubit_indices: Set[int]
) -> Iterator[stim.CircuitInstruction]:
    """Splits classical control system operations away from things actually happening on the quantum
    computer."""
    gate_data = stim.gate_data(op.name)
    assert gate_data.is_unitary and gate_data.is_two_qubit_gate
    targets = op.targets_copy()
    if immune_qubit_indices or any(t.is_measurement_record_target for t in targets):
        args = op.gate_args_copy()
        for k in range(0, len(targets), 2):
            yield stim.CircuitInstruction(op.name, targets[k : k + 2], args, tag=op.tag)
    else:
        yield op


def _split_targets_if_needed_m_basis(
    op: stim.CircuitInstruction,
) -> Iterator[stim.CircuitInstruction]:
    """Splits an MPP operation into one operation for each Pauli product it measures."""
    targets = op.targets_copy()
    args = op.gate_args_copy()
    k = 0
    start = k
    while k < len(targets):
        if k + 1 == len(targets) or not targets[k + 1].is_combiner:
            yield stim.CircuitInstruction(op.name, targets[start : k + 1], args, tag=op.tag)
            k += 1
            start = k
        else:
            k += 2
    assert k == len(targets)


def _iter_split_op_moments(
    circuit: stim.Circuit, *, immune_qubit_indices: Set[int]
) -> Iterator[stim.CircuitRepeatBlock | list[stim.CircuitInstruction]]:
    """Splits a circuit into moments and some operations into pieces.

    Classical control system operations like CX rec[-1] 0 are split apart from quantum operations
    like CX 1 0.

    MPP operations are split into one operation per Pauli product.

    Yields:
        Lists of operations corresponding to one moment in the circuit, with any problematic
        operations like MPPs split into pieces.

        (A moment is the time between two TICKs.)
    """
    cur_moment: list[stim.CircuitInstruction] = []

    for op in circuit:
        if op.tag == 'noiseless-virtual':
            cur_moment.append(op)
        elif isinstance(op, stim.CircuitRepeatBlock):
            if cur_moment:
                yield cur_moment
                cur_moment = []
            yield op
        elif isinstance(op, stim.CircuitInstruction):
            if op.name == "TICK":
                yield cur_moment
                cur_moment = []
            else:
                cur_moment.extend(
                    _split_targets_if_needed(op, immune_qubit_indices=immune_qubit_indices)
                )
    if cur_moment:
        yield cur_moment


def _measure_basis(*, split_op: stim.CircuitInstruction) -> str | None:
    """Converts an operation into a string describing the Pauli product basis it measures.

    Returns:
        None: This is not a measurement (or not *just* a measurement).
        str: Pauli product string that the operation measures (e.g. "XX" or "Y").
    """
    result = OP_TO_MEASURE_BASES.get(split_op.name)
    if result == "*":
        result = ""
        targets = split_op.targets_copy()
        for k in range(0, len(targets), 2):
            t = targets[k]
            if t.is_x_target:
                result += "X"
            elif t.is_y_target:
                result += "Y"
            elif t.is_z_target:
                result += "Z"
            else:
                raise NotImplementedError(f"{targets=}")
    return result


def _immune_indices(
    circuit: stim.Circuit,
    immune_qubit_indices: Iterable[int] | None = None,
    immune_qubit_coords: Iterable[complex | float | int | Iterable[float | int]] | None = None,
) -> frozenset[int]:
    result: set[int] = set()
    if immune_qubit_indices is not None:
        result.update(immune_qubit_indices)
    if immune_qubit_coords is not None:
        # Canonicalize the immune coordinates.
        immune_qubit_coords = frozenset(immune_qubit_coords)
        if immune_qubit_coords:
            immune_tuples: set[tuple[float, ...]] = set()
            for c in immune_qubit_coords:
                if isinstance(c, (int, float, complex)):
                    immune_tuples.add((c.real, c.imag))
                else:
                    immune_tuples.add(tuple(c))

            # Convert to indices.
            for k, coord in circuit.get_final_qubit_coordinates().items():
                if tuple(coord) in immune_tuples:
                    result.add(k)
    return frozenset(result)
