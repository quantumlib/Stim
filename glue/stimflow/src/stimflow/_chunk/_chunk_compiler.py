from __future__ import annotations

import collections
from collections.abc import Callable, Iterable
from typing import Any, cast, Literal, TYPE_CHECKING

import stim

from stimflow._chunk._chunk import Chunk
from stimflow._chunk._chunk_interface import ChunkInterface
from stimflow._chunk._chunk_loop import ChunkLoop
from stimflow._chunk._chunk_reflow import ChunkReflow
from stimflow._chunk._flow_metadata import FlowMetadata
from stimflow._core import append_reindexed_content_to_circuit, Flow, PauliMap, sorted_complex

if TYPE_CHECKING:
    import stimflow


class ChunkCompiler:
    """Compiles appended chunks into a unified circuit."""

    def __init__(self, *, metadata_func: Callable[[Flow], FlowMetadata] | None = None):
        """

        Args:
            metadata_func: Determines coordinate data appended to detectors
                (after x, y, and t). Defaults to None (no extra metadata).
        """
        if metadata_func is None:
            metadata_func = lambda _: FlowMetadata()
        self.open_flows: dict[PauliMap, Flow | Literal["discard"]] = {}
        self.num_measurements: int = 0
        self.waiting_for_magic_init = False
        self.circuit: stim.Circuit = stim.Circuit()
        self.q2i: dict[complex, int] = {}
        self.o2i: dict[Any, int] = {}
        self.discarded_observables: set[int] = set()
        self.metadata_func: Callable[[Flow], FlowMetadata] = cast(Any, metadata_func)
        self.prev_chunk_wants_to_merge_with_next: bool = False

    def ensure_qubits_included(self, qubits: Iterable[complex]):
        """Adds the given qubit positions to the indexed positions, if they aren't already."""
        for q in sorted_complex(qubits):
            if q not in self.q2i:
                self.q2i[q] = len(self.q2i)

    def ensure_observables_included(self, observable_names: Iterable[Any]):
        for name in observable_names:
            if name is not None and name not in self.o2i:
                self.o2i[name] = len(self.o2i)

    def copy(self) -> ChunkCompiler:
        """Returns a deep copy of the compiler's state."""
        result = ChunkCompiler(metadata_func=self.metadata_func)
        result.open_flows = dict(self.open_flows)
        result.num_measurements = self.num_measurements
        result.circuit = self.circuit.copy()
        result.q2i = dict(self.q2i)
        result.o2i = dict(self.o2i)
        result.discarded_observables = set(self.discarded_observables)
        return result

    def __str__(self) -> str:
        lines = ["ChunkCompiler {", "    discard_flows {"]

        for key, flow in self.open_flows.items():
            if flow == "discard":
                lines.append(f"        {key}")
        lines.append("    }")

        lines.append("    det_flows {")
        for key, flow in self.open_flows.items():
            if isinstance(flow, Flow) and flow.obs_key is None:
                lines.append(f"        {flow.end}, ms={flow.measurement_indices}")
        lines.append("    }")

        lines.append("    obs_flows {")
        for key, flow in self.open_flows.items():
            if isinstance(flow, Flow) and flow.obs_key is not None:
                lines.append(f"        {flow.key_end}: ms={flow.measurement_indices}")
        lines.append("    }")

        lines.append(f"    num_measurements = {self.num_measurements}")
        lines.append("}")
        return "\n".join(lines)

    def cur_circuit_html_viewer(self) -> stimflow.str_html:
        copy = self.copy()
        if copy.open_flows:
            copy.append_magic_end_chunk()
        from stimflow._viz import stim_circuit_html_viewer

        return stim_circuit_html_viewer(
            circuit=copy.finish_circuit(), background=self.cur_end_interface()
        )

    def finish_circuit(self) -> stim.Circuit:
        """Returns the circuit built by the compiler.

        Performs some final translation steps:
        - Re-indexing the qubits to be in a sorted order.
        - Re-indexing the observables to omit discarded observable flows.
        """

        if self.open_flows or self.waiting_for_magic_init:
            raise ValueError(f"Some flows were unterminated when finishing the circuit:\n\n{self}")
        if len(set(self.q2i.values())) < len(self.q2i):
            raise NotImplementedError(
                "The qubit indexing map was inconsistent, probably due to a mix of manual and automatic indices."
            )
        if len(set(self.o2i.values())) < len(self.o2i):
            raise NotImplementedError(
                "The observable indexing map was inconsistent, probably due to a mix of manual and automatic indices."
            )

        obs2i: dict[int, int | Literal["discard"]] = {}
        next_obs_index = 0
        for obs_key, obs_index in sorted(self.o2i.items(), key=lambda e: e[1]):
            if obs_key in self.discarded_observables:
                obs2i[obs_index] = "discard"
            elif isinstance(obs_key, int):
                obs2i[obs_index] = next_obs_index
                next_obs_index += 1
        for obs_key, obs_index in sorted(self.o2i.items(), key=lambda e: e[1]):
            if obs_index not in obs2i:
                obs2i[obs_index] = next_obs_index
                next_obs_index += 1

        new_q2i = {q: i for i, q in enumerate(sorted_complex(self.q2i.keys()))}
        final_circuit = stim.Circuit()
        for q, i in new_q2i.items():
            final_circuit.append("QUBIT_COORDS", [i], [q.real, q.imag])
        qubit2i: dict[int, int] = {i: new_q2i[q] for q, i in self.q2i.items()}
        append_reindexed_content_to_circuit(
            content=self.circuit,
            out_circuit=final_circuit,
            qubit_i2i=qubit2i,
            obs_i2i=obs2i,
            rewrite_detector_time_coordinates=False,
        )
        while len(final_circuit) > 0 and (
            final_circuit[-1].name == "SHIFT_COORDS" or final_circuit[-1].name == "TICK"
        ):
            final_circuit.pop()
        return final_circuit

    def append_magic_init_chunk(self, expected: ChunkInterface | None = None) -> None:
        """Appends a non-physical chunk that outputs the flows expected by the next chunk.

        Args:
            expected: Defaults to None (unused). If set to a ChunkInterface, it will be
                verified that the next appended chunk actually has a start interface
                matching the given expected interface. If set to None, then no checks are
                performed; no constraints are placed on the next chunk.
        """
        if expected is None:
            self.waiting_for_magic_init = True
            return
        self.waiting_for_magic_init = False

        self.ensure_qubits_included(expected.used_set)
        self.ensure_observables_included(port.name for port in sorted(expected.ports))
        self.ensure_observables_included(port.name for port in sorted(expected.discards))
        obs_ports = sorted(port for port in expected.ports if port.name is not None)
        for obs_port in obs_ports:
            assert obs_port not in self.open_flows
            assert obs_port.name is not None
            targets = [stim.target_pauli(self.q2i[q], p) for q, p in obs_port.items()]
            self.open_flows[obs_port] = Flow(end=obs_port)
            obs_index = self.o2i.setdefault(obs_port.name, len(self.o2i))
            metadata = self.metadata_func(Flow(start=PauliMap(name=obs_port.name)))
            self.circuit.append("OBSERVABLE_INCLUDE", targets, arg=obs_index, tag=metadata.tag)
            self.circuit.append("TICK")

        for layer in expected.partitioned_detector_flows():
            for port in layer:
                assert port not in self.open_flows
                targets = [stim.target_pauli(self.q2i[q], p) for q, p in port.items()]
                self.open_flows[port] = Flow(end=port, measurement_indices=[self.num_measurements])
                self.circuit.append("MPP", stim.target_combined_paulis(targets))
                self.num_measurements += 1
            self.circuit.append("TICK")

    def append_magic_end_chunk(self, expected: ChunkInterface | None = None) -> None:
        """Appends a non-physical chunk that terminates the circuit, regardless of open flows.

        Args:
            expected: Defaults to None (unused). If set to None, no extra checks are performed.
                If set to a ChunkInterface, it is verified that the open flows actually
                correspond to this interface.
        """
        if self.waiting_for_magic_init:
            self.waiting_for_magic_init = False
        if expected is None:
            expected = self.cur_end_interface()
        self.ensure_qubits_included(expected.used_set)
        self.ensure_observables_included(port.name for port in sorted(expected.ports))
        self.ensure_observables_included(port.name for port in sorted(expected.discards))
        obs_ports: list[PauliMap] = sorted(port for port in expected.ports if port.name is not None)
        completed_flows = []
        for discarded in expected.discards:
            v = self.open_flows.pop(discarded)
            assert v == "discard"
        for layer in expected.partitioned_detector_flows():
            if self.circuit and self.circuit[-1].name != "TICK":
                skip = False
                if self.circuit[-1].name == 'REPEAT':
                    body = self.circuit[-1].body_copy()
                    if body and body[-1].name == 'TICK':
                        skip = True
                if not skip:
                    self.circuit.append("TICK")
            for port in layer:
                targets = [stim.target_pauli(self.q2i[q], p) for q, p in port.items()]
                flow = self.open_flows.pop(port)
                assert flow != "discard"
                flow = cast(Flow, flow).fused_with_next_flow(
                    Flow(start=port, measurement_indices=[self.num_measurements]), next_flow_measure_offset=0
                )
                self.circuit.append("MPP", stim.target_combined_paulis(targets))
                self.num_measurements += 1
                completed_flows.append(flow)
        for obs_port in obs_ports:
            if self.circuit and self.circuit[-1].name != "TICK":
                self.circuit.append("TICK")
            targets = [stim.target_pauli(self.q2i[q], p) for q, p in obs_port.items()]
            flow = self.open_flows.pop(obs_port)
            assert flow != "discard"
            flow = flow.fused_with_next_flow(
                Flow(start=obs_port), next_flow_measure_offset=0
            )
            obs_index = self.o2i.setdefault(obs_port.name, len(self.o2i))
            metadata = self.metadata_func(Flow(start=PauliMap(name=obs_port.name)))
            self.circuit.append("OBSERVABLE_INCLUDE", targets, arg=obs_index, tag=metadata.tag)
            completed_flows.append(flow)
        self._append_detectors(completed_flows=completed_flows)

    def cur_end_interface(self) -> ChunkInterface:
        ports = []
        discards = []
        for pauli_map, flow in self.open_flows.items():
            if flow == "discard":
                discards.append(pauli_map)
            else:
                ports.append(pauli_map)
        return ChunkInterface(ports, discards=discards)

    def append(self, appended: Chunk | ChunkLoop | ChunkReflow) -> None:
        """Appends a chunk to the circuit being built.

        The input flows of the appended chunk must exactly match the open outgoing flows of the
        circuit so far.
        """
        __tracebackhide__ = True

        if self.waiting_for_magic_init:
            self.append_magic_init_chunk(appended.start_interface())

        if isinstance(appended, Chunk):
            self._append_chunk(chunk=appended)
        elif isinstance(appended, ChunkReflow):
            self._append_chunk_reflow(chunk_reflow=appended)
        elif isinstance(appended, ChunkLoop):
            self._append_chunk_loop(chunk_loop=appended)
        else:
            raise NotImplementedError(f"{appended=}")

    def _append_chunk_reflow(self, *, chunk_reflow: ChunkReflow) -> None:
        for ps in chunk_reflow.discard_in:
            if ps.name is not None:
                self.discarded_observables.add(ps.name)
        next_flows: dict[PauliMap, Flow | Literal["discard"]] = {}
        for output, inputs in chunk_reflow.out2in.items():
            measurements: set[int] = set()
            centers: list[complex] = []
            flags: set[str] = set()
            discarded = False
            for inp_key in inputs:
                if inp_key not in self.open_flows:
                    msg = [f"Missing reflow input: {inp_key=}", "Needed inputs {"]
                    for ps in inputs:
                        msg.append(f"    {ps}")
                    msg.append("}")
                    msg.append("Actual inputs {")
                    for ps in self.open_flows.keys():
                        msg.append(f"    {ps}")
                    msg.append("}")
                    raise ValueError("\n".join(msg))
                inp = self.open_flows[inp_key]
                if inp == "discard":
                    discarded = True
                else:
                    assert isinstance(inp, Flow)
                    assert not inp.start
                    measurements ^= frozenset(inp.measurement_indices)
                    if inp.center is not None:
                        centers.append(inp.center)
                    flags |= inp.flags

            next_flows[output] = (
                "discard"
                if discarded
                else Flow(
                    start=None,
                    end=output,
                    measurement_indices=tuple(sorted(measurements)),
                    flags=flags,
                    center=sum(centers) / len(centers) if centers else None,
                )
            )

        for k, v in self.open_flows.items():
            if k in chunk_reflow.removed_inputs:
                continue
            assert k not in next_flows
            next_flows[k] = v

        self.open_flows = next_flows

    def _force_tick_separator(self, *, want_tick: bool) -> None:
        if want_tick:
            if (
                self.circuit
                and self.circuit[-1].name != "TICK"
                and self.circuit[-1].name != "REPEAT"
            ):
                self.circuit.append("TICK")
        else:
            # To make merging possible, break an iteration off the ending loop (if there is one).
            while self.circuit and self.circuit[-1].name == "REPEAT":
                block = self.circuit.pop()
                body = block.body_copy()
                if block.repeat_count > 1:
                    self.circuit.append(
                        stim.CircuitRepeatBlock(block.repeat_count - 1, body, tag=block.tag)
                    )
                self.circuit += body
            if self.circuit and self.circuit[-1].name == "TICK":
                self.circuit.pop()

    def _append_chunk(self, *, chunk: Chunk) -> None:
        __tracebackhide__ = True

        # Index any new locations.
        self.ensure_qubits_included(chunk.q2i.keys())
        self.ensure_observables_included(chunk.o2i.keys())

        # Ensure chunks are correctly separated by a TICK.
        self._force_tick_separator(
            want_tick=not (
                self.prev_chunk_wants_to_merge_with_next or chunk.wants_to_merge_with_prev
            )
        )
        self.prev_chunk_wants_to_merge_with_next = chunk.wants_to_merge_with_next

        # Attach new flows to existing flows.
        next_flows, completed_flows = self._compute_next_flows(chunk=chunk)
        for ps in chunk.discarded_inputs:
            if ps.name is not None:
                self.discarded_observables.add(ps.name)
        for ps in chunk.discarded_outputs:
            if ps.name is not None:
                self.discarded_observables.add(ps.name)
        self.num_measurements += chunk.circuit.num_measurements
        self.open_flows = next_flows

        # Grow the compiled circuit.
        qubit2i: dict[int, int] = {i: self.q2i[q] for q, i in chunk.q2i.items()}
        obs2i: dict[int, int | Literal["discard"]] = {
            i: self.o2i[name] for name, i in chunk.o2i.items()
        }
        append_reindexed_content_to_circuit(
            content=chunk.circuit,
            out_circuit=self.circuit,
            qubit_i2i=qubit2i,
            obs_i2i=obs2i,
            rewrite_detector_time_coordinates=False,
        )
        self._append_detectors(completed_flows=completed_flows)

    def _append_chunk_loop(self, *, chunk_loop: ChunkLoop) -> None:
        __tracebackhide__ = True
        past_circuit = self.circuit

        def compute_relative_flow_state():
            return {
                k: (
                    v.with_edits(
                        measurement_indices=[
                            self._canonicalize_measurement_index_to_negative(m)
                            for m in v.measurement_indices
                        ]
                    )
                    if isinstance(v, Flow)
                    else "discard"
                )
                for k, v in self.open_flows.items()
            }

        if self.prev_chunk_wants_to_merge_with_next:
            if chunk_loop.repetitions > 0:
                for chunk in chunk_loop.chunks:
                    self.append(chunk)
                chunk_loop = chunk_loop.with_repetitions(chunk_loop.repetitions - 1)

        self._force_tick_separator(want_tick=True)

        iteration_circuits: list[stim.Circuit] = []
        measure_offset_start_of_loop = self.num_measurements
        prev_rel_flow_state = compute_relative_flow_state()
        while len(iteration_circuits) < chunk_loop.repetitions:
            # Perform an iteration the hard way.
            self.circuit = stim.Circuit()
            for chunk in chunk_loop.chunks:
                self.append(chunk)
            self._force_tick_separator(want_tick=True)
            iteration_circuits.append(self.circuit)

            # Check if we can fold the rest.
            new_rel_flow_state = compute_relative_flow_state()
            has_pre_loop_measurement = any(
                m < measure_offset_start_of_loop
                for flow in self.open_flows.values()
                if isinstance(flow, Flow)
                for m in flow.measurement_indices
            )
            have_reached_steady_state = (
                not has_pre_loop_measurement and new_rel_flow_state == prev_rel_flow_state
            )
            if have_reached_steady_state:
                break
            prev_rel_flow_state = new_rel_flow_state

        # Found a repeating iteration.
        leftover_reps = chunk_loop.repetitions - len(iteration_circuits)
        if leftover_reps > 0:
            measurements_skipped = iteration_circuits[-1].num_measurements * leftover_reps

            # Fold identical repetitions at the end.
            while len(iteration_circuits) > 1 and iteration_circuits[-1] == iteration_circuits[-2]:
                leftover_reps += 1
                iteration_circuits.pop()
            iteration_circuits[-1] *= leftover_reps + 1

            self.num_measurements += measurements_skipped
            self.open_flows = {
                k: (
                    v.with_edits(
                        measurement_indices=[
                            m + measurements_skipped for m in v.measurement_indices
                        ]
                    )
                    if isinstance(v, Flow)
                    else "discard"
                )
                for k, v in self.open_flows.items()
            }

        # Fuse iterations that happened to be equal.
        self.circuit = past_circuit
        if (
            self.circuit
            and self.circuit[-1].name != "TICK"
            and self.circuit[-1].name != "REPEAT"
            and iteration_circuits
            and iteration_circuits[0]
            and iteration_circuits[0][0].name != "TICK"
        ):
            self.circuit.append("TICK")
        k = 0
        while k < len(iteration_circuits):
            k2 = k + 1
            while k2 < len(iteration_circuits) and iteration_circuits[k2] == iteration_circuits[k]:
                k2 += 1
            self.circuit += iteration_circuits[k] * (k2 - k)
            k = k2

    def _canonicalize_measurement_index_to_negative(self, m: int) -> int:
        if m >= 0:
            m -= self.num_measurements
        assert -self.num_measurements <= m < 0
        return m

    def _append_detectors(self, *, completed_flows: list[Flow]):
        inserted_ops = stim.Circuit()

        # Dump observable changes.
        for key, flow in list(self.open_flows.items()):
            if key.name is not None and isinstance(flow, Flow) and flow.measurement_indices:
                dump_targets: list[stim.GateTarget] = []
                for m in flow.measurement_indices:
                    dump_targets.append(stim.target_rec(m - self.num_measurements))
                obs_index = self.o2i.setdefault(flow.obs_key, len(self.o2i))
                inserted_ops.append("OBSERVABLE_INCLUDE", dump_targets, obs_index)
                self.open_flows[key] = flow.with_edits(measurement_indices=[])

        # Append detector and observable annotations for the completed flows.
        detector_pos_usage_counts: collections.Counter[complex] = collections.Counter()
        for flow in completed_flows:
            rec_targets: list[stim.GateTarget] = []
            for m in flow.measurement_indices:
                rec_targets.append(
                    stim.target_rec(self._canonicalize_measurement_index_to_negative(m))
                )
            metadata = self.metadata_func(flow)
            if flow.obs_key is None:
                dt = detector_pos_usage_counts[flow.center]
                detector_pos_usage_counts[flow.center] += 1
                coord = flow.center if flow.center is not None else -1
                coords = (coord.real, coord.imag, dt, *metadata.extra_coords)
                inserted_ops.append("DETECTOR", rec_targets, coords, tag=metadata.tag)
            else:
                obs_index = self.o2i.setdefault(flow.obs_key, len(self.o2i))
                if rec_targets:
                    if metadata.extra_coords:
                        raise ValueError(
                            f"{metadata=} for {flow=} has extra_coords, "
                            "but OBSERVABLE_INCLUDE instructions can't specify coordinates."
                        )
                    inserted_ops.append(
                        "OBSERVABLE_INCLUDE", rec_targets, obs_index, tag=metadata.tag
                    )

        if inserted_ops:
            insert_index = len(self.circuit)
            while insert_index > 0 and self.circuit[insert_index - 1].num_measurements == 0:
                insert_index -= 1
            self.circuit.insert(insert_index, inserted_ops)

        # Shift the time coordinate so future chunks' detectors are further along the time axis.
        det_offset = max(detector_pos_usage_counts.values(), default=0)
        if det_offset > 0:
            self.circuit.append("SHIFT_COORDS", [], (0, 0, det_offset))

    def _compute_next_flows(
        self, *, chunk: Chunk
    ) -> tuple[dict[PauliMap, Flow | Literal["discard"]], list[Flow]]:
        __tracebackhide__ = True
        attached_flows, outgoing_discards = self._compute_attached_flows_and_discards(chunk=chunk)

        next_flows: dict[PauliMap, Flow | Literal["discard"]] = {}
        completed_flows: list[Flow] = []
        for flow in attached_flows:
            assert not flow.start
            if flow.end:
                next_flows[flow.end] = flow
            else:
                completed_flows.append(flow)

        for discarded in outgoing_discards:
            next_flows[discarded] = "discard"
        for discarded in chunk.discarded_outputs:
            if discarded in next_flows:
                raise ValueError(
                    f"Chunk said to discard {discarded=}, but it was already in next_flows."
                )
            next_flows[discarded] = "discard"

        return next_flows, completed_flows

    def _compute_attached_flows_and_discards(
        self, *, chunk: Chunk
    ) -> tuple[list[Flow], list[PauliMap]]:
        __tracebackhide__ = True

        result: list[Flow] = []
        old_flows = dict(self.open_flows)

        # Drop existing flows explicitly discarded by the chunk.
        for discarded in chunk.discarded_inputs:
            old_flows.pop(discarded, None)
        outgoing_discards = []

        # Attach the chunk's flows to the existing flows.
        for new_flow in chunk.flows:
            prev = old_flows.pop(new_flow.start, None)
            if prev == "discard":
                # Okay, discard it.
                if new_flow.end:
                    outgoing_discards.append(new_flow.end)
            elif isinstance(prev, Flow):
                # Matched! Fuse them together.
                result.append(
                    prev.fused_with_next_flow(
                        new_flow, next_flow_measure_offset=self.num_measurements
                    )
                )
            elif not new_flow.start:
                # Flow started inside the new chunk, so doesn't need to be matched.
                result.append(
                    new_flow.with_edits(
                        measurement_indices=[
                            (m + self.num_measurements if m >= 0 else m)
                            for m in new_flow.measurement_indices
                        ]
                    )
                )
            else:
                # Failed to match. Describe the problem.
                lines = [
                    "A flow input wasn't satisfied.",
                    f"   Expected input: {new_flow.start}",
                    "   Available inputs:",
                ]
                for prev_avail in old_flows.keys():
                    lines.append(f"       {prev_avail}")
                raise ValueError("\n".join(lines))

        # Check for any unmatched flows.
        dangling_flows: list[Flow] = [val for val in old_flows.values() if isinstance(val, Flow)]
        if dangling_flows:
            lines = ["Some flow outputs were unmatched when appending a new chunk:"]
            for flow in dangling_flows:
                lines.append(f"   {flow.end}")
            raise ValueError("\n".join(lines))

        return result, outgoing_discards
