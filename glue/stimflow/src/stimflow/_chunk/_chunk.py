from __future__ import annotations

import collections
from collections.abc import Callable, Iterable
from typing import Any, cast, Literal, TYPE_CHECKING

import stim

from stimflow._chunk._code_util import (
    verify_distance_is_at_least_2,
    verify_distance_is_at_least_3,
)
from stimflow._chunk._patch import Patch
from stimflow._chunk._stabilizer_code import StabilizerCode
from stimflow._chunk._test_util import assert_has_same_set_of_items_as
from stimflow._core import (
    circuit_to_dem_target_measurement_records_map,
    circuit_with_xz_flipped,
    Flow,
    NoiseModel,
    PauliMap,
    stim_circuit_with_transformed_coords,
    str_html,
    Tile,
)

if TYPE_CHECKING:
    from stimflow._chunk._chunk_interface import ChunkInterface
    from stimflow._chunk._chunk_loop import ChunkLoop
    from stimflow._chunk._chunk_reflow import ChunkReflow


class Chunk:
    """A circuit chunk with accompanying stabilizer flow assertions."""

    def __init__(
        self,
        circuit: stim.Circuit,
        *,
        flows: Iterable[Flow],
        discarded_inputs: Iterable[PauliMap | Tile] = (),
        discarded_outputs: Iterable[PauliMap | Tile] = (),
        wants_to_merge_with_next: bool = False,
        wants_to_merge_with_prev: bool = False,
        q2i: dict[complex, int] | None = None,
        o2i: dict[Any, int] | None = None,
    ):
        """

        Args:
            circuit: The circuit implementing the chunk's functionality.
            flows: A series of stabilizer flows that the circuit implements.
            discarded_inputs: Explicitly rejected in flows. For example, a data
                measurement chunk might reject flows for stabilizers from the
                anticommuting basis. If they are not rejected, then compilation
                will fail when attempting to combine this chunk with a preceding
                chunk that has those stabilizers from the anticommuting basis
                flowing out.
            discarded_outputs: Explicitly rejected out flows. For example, an
                initialization chunk might reject flows for stabilizers from the
                anticommuting basis. If they are not rejected, then compilation
                will fail when attempting to combine this chunk with a following
                chunk that has those stabilizers from the anticommuting basis
                flowing in.
            wants_to_merge_with_next: Defaults to False. When set to True,
                the chunk compiler won't insert a TICK between this chunk
                and the next chunk.
            wants_to_merge_with_prev: Defaults to False. When set to True,
                the chunk compiler won't insert a TICK between this chunk
                and the previous chunk.
            q2i: Defaults to None (infer from QUBIT_COORDS instructions in circuit else
                raise an exception). The stimflow-qubit-coordinate-to-stim-qubit-index mapping
                used to translate between stimflow's qubit keys and stim's qubit keys.
            o2i: Defaults to None (raise an exception if observables present in circuit).
                The stimflow-observable-key-to-stim-observable-index mapping used to translate
                between stimflow's observable keys and stim's observable keys.
        """
        if q2i is None:
            q2i = {x + 1j * y: i for i, (x, y) in circuit.get_final_qubit_coordinates().items()}
            if len(q2i) != circuit.num_qubits:
                raise ValueError(
                    "The given circuit doesn't have enough `QUBIT_COORDS` instructions to "
                    "determine the stimflow-coordinate-to-stim-qubit-index mapping. You must manually "
                    "specify it by passing a `q2i={...}` argument, or add the missing "
                    "`QUBIT_COORDS`."
                )
        flows = tuple(flows)
        if o2i is None:
            if circuit.num_observables:
                raise ValueError(
                    "The given circuit has `OBSERVABLE_INCLUDE` instructions. You must specify "
                    "the stimflow-observable-key-to-stim-observable-index mapping by passing an"
                    "`o2k={...}` argument."
                )
            o2i = {}
            for flow in flows:
                if flow.obs_key is not None and flow.obs_key not in o2i:
                    o2i[flow.obs_key] = len(o2i)

        self.q2i: dict[complex, int] = q2i
        self.o2i: dict[Any, int] = o2i
        self.circuit: stim.Circuit = circuit
        self.flows: tuple[Flow, ...] = flows

        self.discarded_inputs: tuple[PauliMap, ...] = tuple(
            e.to_pauli_map() if isinstance(e, Tile) else e for e in discarded_inputs
        )
        self.discarded_outputs: tuple[PauliMap, ...] = tuple(
            e.to_pauli_map() if isinstance(e, Tile) else e for e in discarded_outputs
        )
        self.wants_to_merge_with_next = wants_to_merge_with_next
        self.wants_to_merge_with_prev = wants_to_merge_with_prev
        assert all(isinstance(e, PauliMap) for e in self.discarded_inputs)
        assert all(isinstance(e, PauliMap) for e in self.discarded_outputs)

    def __add__(self, other: Chunk | ChunkReflow | ChunkLoop) -> Chunk:
        return self.then(other)

    def then(self, other: Chunk | ChunkReflow | ChunkLoop) -> Chunk:
        from stimflow._chunk._chunk_loop import ChunkLoop
        from stimflow._chunk._chunk_reflow import ChunkReflow

        if isinstance(other, Chunk):
            return self._then_chunk(other)
        elif isinstance(other, ChunkReflow):
            return self._then_reflow(other)
        elif isinstance(other, ChunkLoop):
            acc = self
            for k in range(other.repetitions):
                for c in other.chunks:
                    acc = self.then(c)
            return acc
        else:
            raise NotImplementedError(f"{other=}")

    def _then_reflow(self, other: ChunkReflow) -> Chunk:
        new_flows: list[Flow] = []
        new_discarded_outputs: list[PauliMap] = []

        must_match_outputs: set[PauliMap] = set()
        used_outputs: set[PauliMap] = set()

        i2f = {}
        old_discarded_outputs = set(self.discarded_outputs)
        must_match_outputs.update(self.discarded_outputs)
        for flow in self.flows:
            if flow.end:
                assert flow.end not in i2f
                i2f[flow.end] = flow
                must_match_outputs.add(flow.end)
            else:
                new_flows.append(flow)
        for out, inputs in other.out2in.items():
            acc = None
            used_outputs.update(inputs)
            for inp in inputs:
                if inp in old_discarded_outputs:
                    new_discarded_outputs.append(out)
                    break
                f = i2f[inp].with_edits(obs_key=None)
                if acc is None:
                    acc = f
                else:
                    acc *= f
            else:
                assert acc is not None
                assert acc.end == out
                new_flows.append(acc.with_edits(obs_key=out.name))
        if used_outputs != must_match_outputs:
            lines = ["Unmatched reflows."]
            for e in must_match_outputs - used_outputs:
                lines.append(f"    missing: {e}")
            for e in used_outputs - must_match_outputs:
                lines.append(f"    extra: {e}")
            raise ValueError("\n".join(lines))

        result = Chunk(
            circuit=self.circuit.copy(),
            flows=new_flows,
            discarded_inputs=self.discarded_inputs,
            discarded_outputs=new_discarded_outputs,
            wants_to_merge_with_prev=self.wants_to_merge_with_prev,
            wants_to_merge_with_next=self.wants_to_merge_with_next,
        )
        return result

    def _then_chunk(self, other: Chunk) -> Chunk:
        from stimflow._chunk._chunk_compiler import ChunkCompiler

        compiler = ChunkCompiler()
        compiler.append(self.with_edits(flows=[], discarded_inputs=[], discarded_outputs=[]))
        compiler.append(other.with_edits(flows=[], discarded_inputs=[], discarded_outputs=[]))
        combined_circuit = compiler.finish_circuit()

        nm1 = self.circuit.num_measurements
        nm2 = other.circuit.num_measurements

        new_flows: list[Flow] = []
        new_discarded_outputs: list[PauliMap] = list(other.discarded_outputs)
        new_discarded_inputs: list[PauliMap] = list(self.discarded_inputs)

        mid2flow: dict[PauliMap, Flow | Literal["discard"]] = {}
        for flow in self.flows:
            if flow.end:
                mid2flow[flow.end] = flow
            else:
                new_flows.append(flow)
        for key in self.discarded_outputs:
            mid2flow[key] = "discard"

        for key in other.discarded_inputs:
            prev_flow = mid2flow.pop(key, None)
            if prev_flow is None:
                lines = []
                lines.append("Incompatible chunks.")
                lines.append(f"The second chunk has the discarded input {key}")
                lines.append("But the first chunk has no matching flow output:")
                e = self.end_interface()
                for v in sorted(e.ports):
                    lines.append(f"    {v}")
                for v in sorted(e.discards):
                    lines.append(f"    {v} [discard]")
                raise ValueError("\n".join(lines))
            if isinstance(prev_flow, Flow):
                if prev_flow.start:
                    new_discarded_inputs.append(prev_flow.start)

        for flow in other.flows:
            if not flow.start:
                new_flows.append(
                    flow.with_edits(
                        measurement_indices=[m % nm2 + nm1 for m in flow.measurement_indices]
                    )
                )
                continue

            prev_flow = mid2flow.pop(flow.start, None)
            if prev_flow is None:
                lines = []
                lines.append("Incompatible chunks.")
                lines.append(f"The second chunk has the flow {flow}")
                lines.append("But the first chunk has no matching flow output:")
                e = self.end_interface()
                for v in sorted(e.ports):
                    lines.append(f"    {v}")
                for v in sorted(e.discards):
                    lines.append(f"    {v} [discard]")
                raise ValueError("\n".join(lines))

            if isinstance(prev_flow, Flow):
                new_flows.append(
                    flow.with_edits(
                        start=prev_flow.start,
                        measurement_indices=[m % nm1 for m in prev_flow.measurement_indices]
                        + [m % nm2 + nm1 for m in flow.measurement_indices],
                        flags=flow.flags | prev_flow.flags,
                    )
                )
            else:
                assert prev_flow == "discard"
                if flow.end:
                    new_discarded_outputs.append(flow.end)

        for flow in new_flows:
            if flow.obs_key is not None:
                compiler.o2i.setdefault(flow.obs_key, len(compiler.o2i))
        result = Chunk(
            circuit=combined_circuit,
            q2i=compiler.q2i,
            o2i=compiler.o2i,
            flows=new_flows,
            discarded_inputs=new_discarded_inputs,
            discarded_outputs=new_discarded_outputs,
            wants_to_merge_with_prev=self.wants_to_merge_with_prev,
            wants_to_merge_with_next=other.wants_to_merge_with_next,
        )
        return result

    def __repr__(self) -> str:
        lines = ["stimflow.Chunk("]
        lines.append(f"    q2i={self.q2i!r},")
        lines.append(f"    circuit={self.circuit!r},".replace("\n", "\n    "))
        if self.flows:
            lines.append(f"    flows={self.flows!r},")
        if self.discarded_inputs:
            lines.append(f"    discarded_inputs={self.discarded_inputs!r},")
        if self.discarded_outputs:
            lines.append(f"    discarded_outputs={self.discarded_outputs!r},")
        if self.wants_to_merge_with_prev:
            lines.append(f"    wants_to_merge_with_prev={self.wants_to_merge_with_prev!r},")
        if self.wants_to_merge_with_next:
            lines.append(f"    discarded_outputs={self.wants_to_merge_with_next!r},")
        lines.append(")")
        return "\n".join(lines)

    def with_obs_flows_as_det_flows(self) -> Chunk:
        return self.with_edits(flows=[flow.with_edits(obs_key=None) for flow in self.flows])

    def with_flag_added_to_all_flows(self, flag: str) -> Chunk:
        return self.with_edits(
            flows=[flow.with_edits(flags={*flow.flags, flag}) for flow in self.flows]
        )

    @staticmethod
    def from_circuit_with_mpp_boundaries(circuit: stim.Circuit) -> Chunk:
        allowed = {"TICK", "OBSERVABLE_INCLUDE", "DETECTOR", "MPP", "QUBIT_COORDS", "SHIFT_COORDS"}
        start = 0
        end = len(circuit)
        while start < len(circuit) and circuit[start].name in allowed:
            start += 1
        while end > 0 and circuit[end - 1].name in allowed:
            end -= 1
        while end < len(circuit) and circuit[end].name != "MPP":
            end += 1
        while end > 0 and circuit[end - 1].name == "TICK":
            end -= 1
        if end <= start:
            raise ValueError("end <= start")

        prefix, body, suffix = circuit[:start], circuit[start:end], circuit[end:]
        start_tick = prefix.num_ticks
        end_tick = start_tick + body.num_ticks + 1
        c = stim.Circuit()
        c += prefix
        c.append("TICK")
        c += body
        c.append("TICK")
        c += suffix
        det_regions = c.detecting_regions(ticks=[start_tick, end_tick])
        records = circuit_to_dem_target_measurement_records_map(c)
        pn = prefix.num_measurements
        record_range = range(pn, pn + body.num_measurements)

        q2i = {qr + qi * 1j: i for i, (qr, qi) in circuit.get_final_qubit_coordinates().items()}
        i2q = {i: q for q, i in q2i.items()}
        dropped_detectors = set()

        flows = []
        for target, items in det_regions.items():
            if target.is_relative_detector_id():
                dropped_detectors.add(target.val)
            start_ps: stim.PauliString = items.get(start_tick, stim.PauliString(0))
            end_ps: stim.PauliString = items.get(end_tick, stim.PauliString(0))

            start_pm = PauliMap(start_ps).with_transformed_coords(lambda i: i2q[i])
            end_pm = PauliMap(end_ps).with_transformed_coords(lambda i: i2q[i])

            flows.append(
                Flow(
                    start=start_pm,
                    end=end_pm,
                    mids=[m - record_range.start for m in records[target] if m in record_range],
                    obs_key=None if target.is_relative_detector_id() else target.val,
                    center=(sum(start_pm.keys()) + sum(end_pm.keys()))
                    / (len(start_pm) + len(end_pm)),
                )
            )

        kept = stim.Circuit()
        num_d = prefix.num_detectors
        for inst in body.flattened():
            if inst.name == "DETECTOR":
                if num_d not in dropped_detectors:
                    kept.append(inst)
                num_d += 1
            elif inst.name != "OBSERVABLE_INCLUDE":
                kept.append(inst)
        o2i = {i: i for i in range(circuit.num_observables)}

        return Chunk(q2i=q2i, o2i=o2i, flows=flows, circuit=kept)

    def _interface(
        self,
        side: Literal["start", "end"],
        *,
        skip_discards: bool = False,
        skip_passthroughs: bool = False,
    ) -> tuple[PauliMap, ...]:
        if side == "start":
            include_start = True
            include_end = False
        elif side == "end":
            include_start = False
            include_end = True
        else:
            raise NotImplementedError(f"{side=}")

        result: list[PauliMap] = []
        for flow in self.flows:
            if include_start and flow.start and not (skip_passthroughs and flow.end):
                result.append(flow.start)
            if include_end and flow.end and not (skip_passthroughs and flow.start):
                result.append(flow.end)
        if include_start and not skip_discards:
            result.extend(self.discarded_inputs)
        if include_end and not skip_discards:
            result.extend(self.discarded_outputs)

        result_set: set[PauliMap] = set()
        collisions: set[PauliMap] = set()
        for item in result:
            if item in result_set:
                collisions.add(item)
            result_set.add(item)

        if collisions:
            msg = [f"{side} interface had collisions:"]
            for a, b in sorted(collisions):
                msg.append(f"    {a}, obs_key={b}")
            raise ValueError("\n".join(msg))

        return tuple(sorted(result_set))

    def with_edits(
        self,
        *,
        circuit: stim.Circuit | None = None,
        q2i: dict[complex, int] | None = None,
        flows: Iterable[Flow] | None = None,
        discarded_inputs: Iterable[PauliMap] | None = None,
        discarded_outputs: Iterable[PauliMap] | None = None,
        wants_to_merge_with_prev: bool | None = None,
        wants_to_merge_with_next: bool | None = None,
    ) -> Chunk:
        return Chunk(
            circuit=self.circuit if circuit is None else circuit,
            q2i=self.q2i if q2i is None else q2i,
            flows=self.flows if flows is None else flows,
            discarded_inputs=(
                self.discarded_inputs if discarded_inputs is None else discarded_inputs
            ),
            discarded_outputs=(
                self.discarded_outputs if discarded_outputs is None else discarded_outputs
            ),
            wants_to_merge_with_prev=(
                self.wants_to_merge_with_prev
                if wants_to_merge_with_prev is None
                else wants_to_merge_with_prev
            ),
            wants_to_merge_with_next=(
                self.wants_to_merge_with_next
                if wants_to_merge_with_next is None
                else wants_to_merge_with_next
            ),
        )

    def __eq__(self, other):
        if not isinstance(other, Chunk):
            return NotImplemented
        return (
            self.q2i == other.q2i
            and self.circuit == other.circuit
            and self.flows == other.flows
            and self.discarded_inputs == other.discarded_inputs
            and self.discarded_outputs == other.discarded_outputs
            and self.wants_to_merge_with_prev == other.wants_to_merge_with_prev
            and self.wants_to_merge_with_next == other.wants_to_merge_with_next
        )

    def to_html_viewer(
        self,
        *,
        background: (
            Patch
            | StabilizerCode
            | ChunkInterface
            | dict[int, Patch | StabilizerCode | ChunkInterface]
            | None
        ) = None,
        tile_color_func: Callable[[Tile], tuple[float, float, float, float]] | None = None,
        known_error: Iterable[stim.ExplainedError] | None = None,
    ) -> str_html:
        from stimflow._viz import stim_circuit_html_viewer

        circuit = self.to_closed_circuit()
        if background is None:
            start = self.start_patch()
            end = self.end_patch()
            if len(start.tiles) == 0:
                background = end
            elif len(end.tiles) == 0:
                background = start
            else:
                background = {0: start, circuit.num_ticks: end}
        return stim_circuit_html_viewer(
            circuit, background=background, tile_color_func=tile_color_func, known_error=known_error
        )

    def __mul__(self, other: int) -> ChunkLoop:
        from stimflow._chunk._chunk_loop import ChunkLoop

        return ChunkLoop([self], repetitions=other)

    def with_repetitions(self, repetitions: int) -> ChunkLoop:
        from stimflow._chunk._chunk_loop import ChunkLoop

        return ChunkLoop([self], repetitions=repetitions)

    def find_distance(
        self,
        *,
        max_search_weight: int,
        noise: float | NoiseModel = 1e-3,
        noiseless_qubits: Iterable[float | int | complex] = (),
        skip_adding_noise: bool = False,
    ) -> int:
        err = self.find_logical_error(
            max_search_weight=max_search_weight,
            noise=noise,
            noiseless_qubits=noiseless_qubits,
            skip_adding_noise=skip_adding_noise,
        )
        return len(err)

    def to_closed_circuit(self) -> stim.Circuit:
        """Compiles the chunk into a circuit by conjugating with mpp init/end chunks."""
        from stimflow._chunk._chunk_compiler import ChunkCompiler

        compiler = ChunkCompiler()
        compiler.append_magic_init_chunk(self.start_interface())
        compiler.append(self)
        compiler.append_magic_end_chunk(self.end_interface())
        return compiler.finish_circuit()

    def verify_distance_is_at_least_2(self, *, noise: float | NoiseModel = 1e-3):
        """Verifies undetected logical errors require at least 2 physical errors.

        By default, verifies using a uniform depolarizing circuit noise model.
        """
        __tracebackhide__ = True
        circuit = self.to_closed_circuit()
        if isinstance(noise, float):
            noise = NoiseModel.uniform_depolarizing(1e-3)
        circuit = noise.noisy_circuit_skipping_mpp_boundaries(circuit)
        verify_distance_is_at_least_2(circuit)

    def to_coord_circuit(self) -> stim.Circuit:
        coords = stim.Circuit()
        for q, i in self.q2i.items():
            coords.append("QUBIT_COORDS", [i], [q.real, q.imag])
        return coords + self.circuit

    def verify_distance_is_at_least_3(self, *, noise: float | NoiseModel = 1e-3):
        """Verifies undetected logical errors require at least 3 physical errors.

        By default, verifies using a uniform depolarizing circuit noise model.
        """
        __tracebackhide__ = True
        circuit = self.to_closed_circuit()
        if isinstance(noise, float):
            noise = NoiseModel.uniform_depolarizing(1e-3)
        circuit = noise.noisy_circuit_skipping_mpp_boundaries(circuit)
        verify_distance_is_at_least_3(circuit)

    def find_logical_error(
        self,
        *,
        max_search_weight: int,
        noise: float | NoiseModel = 1e-3,
        noiseless_qubits: Iterable[float | int | complex] = (),
        skip_adding_noise: bool = False,
    ) -> list[stim.ExplainedError]:
        circuit = self.to_closed_circuit()
        if not skip_adding_noise:
            if isinstance(noise, float):
                noise = NoiseModel.uniform_depolarizing(1e-3)
            circuit = noise.noisy_circuit_skipping_mpp_boundaries(
                circuit, immune_qubit_coords=noiseless_qubits
            )
        if max_search_weight == 2:
            return circuit.shortest_graphlike_error(canonicalize_circuit_errors=True)
        return circuit.search_for_undetectable_logical_errors(
            dont_explore_edges_with_degree_above=max_search_weight,
            dont_explore_detection_event_sets_with_size_above=max_search_weight,
            dont_explore_edges_increasing_symptom_degree=False,
            canonicalize_circuit_errors=True,
        )

    def verify(
        self,
        *,
        expected_in: ChunkInterface | StabilizerCode | Patch | None = None,
        expected_out: ChunkInterface | StabilizerCode | Patch | None = None,
        should_measure_all_code_stabilizers: bool = False,
        allow_overlapping_flows: bool = False,
    ):
        """Checks that this chunk's circuit actually implements its flows."""
        __tracebackhide__ = True

        # Check basic types.
        assert (
            not should_measure_all_code_stabilizers
            or expected_in is not None
            or should_measure_all_code_stabilizers is not None
        )
        assert isinstance(self.circuit, stim.Circuit)
        assert isinstance(self.q2i, dict)
        assert isinstance(self.o2i, dict)
        assert isinstance(self.flows, tuple)
        assert isinstance(self.discarded_inputs, tuple)
        assert isinstance(self.discarded_outputs, tuple)
        assert all(isinstance(e, Flow) for e in self.flows)
        assert all(isinstance(e, PauliMap) for e in self.discarded_inputs)
        assert all(isinstance(e, PauliMap) for e in self.discarded_outputs)

        # Check observable mapping.
        i2o = {i: o for o, i in self.o2i.items()}
        if len(i2o) < len(self.o2i):
            raise ValueError(f"{self.o2i=} maps multiple observables to the same index.")
        obs_indices_present: set[int] = set()
        _accumulate_observable_indices_used_by_circuit(self.circuit, out=obs_indices_present)
        unexplained_indices = obs_indices_present - i2o.keys()
        if unexplained_indices:
            raise ValueError(
                f"The chunk's circuit has {obs_indices_present=}, but {self.o2i=} doesn't map to "
                f"any of {unexplained_indices=}."
            )

        # Check for flow collisions.
        if not allow_overlapping_flows:
            groups: collections.defaultdict[PauliMap, list[Flow]]

            groups = collections.defaultdict(list)
            for flow in self.flows:
                groups[flow.start].append(flow)
            for key, group in groups.items():
                if key and len(group) > 1:
                    lines = ["Multiple flows with same non-empty start:"]
                    for g in group:
                        lines.append(f"    {g}")
                    raise ValueError("\n".join(lines))

            groups = collections.defaultdict(list)
            for flow in self.flows:
                groups[flow.end].append(flow)
            for key, group in groups.items():
                if key and len(group) > 1:
                    raise ValueError(f"Multiple flows with same non-empty end: {group}")

        # Verify flows are actually satisfied by the circuit.
        unsigned_stim_flows: list[stim.Flow] = []
        unsigned_indices: list[int] = []
        signed_stim_flows: list[stim.Flow] = []
        signed_indices: list[int] = []
        o2i_def: collections.defaultdict[Any, int | None] = collections.defaultdict(lambda: None)
        o2i_def.update(self.o2i)
        for k, flow in enumerate(self.flows):
            stim_flow = flow.to_stim_flow(q2i=self.q2i, o2i=o2i_def)
            if flow.sign is None:
                unsigned_stim_flows.append(stim_flow)
                unsigned_indices.append(k)
            else:
                signed_stim_flows.append(stim_flow)
                signed_indices.append(k)
        if not self.circuit.has_all_flows(
            unsigned_stim_flows, unsigned=True
        ) or not self.circuit.has_all_flows(signed_stim_flows):
            msg = []
            for k in range(len(unsigned_stim_flows)):
                if not self.circuit.has_flow(unsigned_stim_flows[k], unsigned=True):
                    msg.append("    (unsigned) " + str(self.flows[unsigned_indices[k]]))
            for k in range(len(signed_stim_flows)):
                if not self.circuit.has_flow(signed_stim_flows[k], unsigned=True):
                    msg.append(
                        "    (wanted signed, not even unsigned present) "
                        + str(self.flows[signed_indices[k]])
                    )
                elif not self.circuit.has_flow(signed_stim_flows[k]):
                    msg.append("    (signed) " + str(self.flows[signed_indices[k]]))
            msg.insert(0, f"Circuit lacks the following {len(msg)} flows:")
            raise ValueError("\n".join(msg))

        if expected_in is not None:
            if isinstance(expected_in, Patch):
                expected_in = StabilizerCode(expected_in).as_interface()
            if isinstance(expected_in, StabilizerCode):
                expected_in = expected_in.as_interface()
            if should_measure_all_code_stabilizers:
                assert_has_same_set_of_items_as(
                    self.start_interface(skip_passthroughs=True)
                    .without_discards()
                    .without_keyed()
                    .ports,
                    expected_in.without_discards().without_keyed().ports,
                    "actual_measured_operators",
                    "expected_measured_operators",
                )
            assert_has_same_set_of_items_as(
                self.start_interface().with_discards_as_ports().ports,
                expected_in.with_discards_as_ports().ports,
                "actual_start_interface",
                "expected_start_interface",
            )
        else:
            # Creating the interface checks for collisions
            self.start_interface()

        if expected_out is not None:
            if isinstance(expected_out, Patch):
                expected_out = StabilizerCode(expected_out).as_interface()
            if isinstance(expected_out, StabilizerCode):
                expected_out = expected_out.as_interface()
            if should_measure_all_code_stabilizers:
                assert_has_same_set_of_items_as(
                    self.end_interface(skip_passthroughs=True)
                    .without_discards()
                    .without_keyed()
                    .ports,
                    expected_out.without_discards().without_keyed().ports,
                    "actual_prepared_operators",
                    "expected_prepared_operators",
                )
            assert_has_same_set_of_items_as(
                self.end_interface().with_discards_as_ports().ports,
                expected_out.with_discards_as_ports().ports,
                "actual_end_interface",
                "expected_end_interface",
            )
        else:
            # Creating the interface checks for collisions
            self.end_interface()

    def time_reversed(self) -> Chunk:
        """Checks that this chunk's circuit actually implements its flows."""

        stim_flows = []
        for flow in self.flows:
            inp = stim.PauliString(len(self.q2i))
            out = stim.PauliString(len(self.q2i))
            for q, p in flow.start.qubits.items():
                inp[self.q2i[q]] = p
            for q, p in flow.end.qubits.items():
                out[self.q2i[q]] = p
            stim_flows.append(
                stim.Flow(input=inp, output=out, measurements=cast(Any, flow.measurement_indices))
            )
        rev_circuit, rev_flows = self.circuit.time_reversed_for_flows(stim_flows)
        nm = rev_circuit.num_measurements
        return Chunk(
            circuit=rev_circuit,
            q2i=self.q2i,
            flows=[
                Flow(
                    center=flow.center,
                    start=flow.end,
                    end=flow.start,
                    mids=[m + nm for m in rev_flow.measurements_copy()],
                    flags=flow.flags,
                    obs_key=flow.obs_key,
                )
                for flow, rev_flow in zip(self.flows, rev_flows, strict=True)
            ],
            discarded_inputs=self.discarded_outputs,
            discarded_outputs=self.discarded_inputs,
            wants_to_merge_with_prev=self.wants_to_merge_with_next,
            wants_to_merge_with_next=self.wants_to_merge_with_prev,
        )

    def with_xz_flipped(self) -> Chunk:
        return self.with_edits(
            circuit=circuit_with_xz_flipped(self.circuit),
            flows=[flow.with_xz_flipped() for flow in self.flows],
            discarded_inputs=[p.with_xz_flipped() for p in self.discarded_inputs],
            discarded_outputs=[p.with_xz_flipped() for p in self.discarded_outputs],
        )

    def with_transformed_coords(self, transform: Callable[[complex], complex]) -> Chunk:
        return self.with_edits(
            q2i={transform(q): i for q, i in self.q2i.items()},
            circuit=stim_circuit_with_transformed_coords(self.circuit, transform),
            flows=[flow.with_transformed_coords(transform) for flow in self.flows],
            discarded_inputs=[p.with_transformed_coords(transform) for p in self.discarded_inputs],
            discarded_outputs=[
                p.with_transformed_coords(transform) for p in self.discarded_outputs
            ],
        )

    def flattened(self) -> list[Chunk]:
        """This is here for duck-type compatibility with ChunkLoop."""
        return [self]

    def start_interface(self, *, skip_passthroughs: bool = False) -> ChunkInterface:
        """Returns a description of the flows that should enter into the chunk."""
        from stimflow._chunk._chunk_interface import ChunkInterface

        return ChunkInterface(
            ports=[
                flow.start
                for flow in self.flows
                if flow.start
                if not (skip_passthroughs and flow.end)
            ],
            discards=self.discarded_inputs,
        )

    def end_interface(self, *, skip_passthroughs: bool = False) -> ChunkInterface:
        """Returns a description of the flows that should exit from the chunk."""
        from stimflow._chunk._chunk_interface import ChunkInterface

        return ChunkInterface(
            ports=[
                flow.end
                for flow in self.flows
                if flow.end
                if not (skip_passthroughs and flow.start)
            ],
            discards=self.discarded_outputs,
        )

    def start_code(self) -> StabilizerCode:
        return StabilizerCode(
            self.start_patch(),
            logicals=[flow.start for flow in self.flows if flow.obs_key is not None],
        )

    def end_code(self) -> StabilizerCode:
        return StabilizerCode(
            self.end_patch(),
            logicals=[flow.end for flow in self.flows if flow.obs_key is not None],
        )

    def start_patch(self) -> Patch:
        from stimflow._chunk._patch import Patch

        return Patch(
            [
                Tile(
                    bases="".join(flow.start.values()),
                    data_qubits=flow.start.keys(),
                    measure_qubit=flow.center,
                    flags=flow.flags,
                )
                for flow in self.flows
                if flow.start
                if flow.obs_key is None
            ]
        )

    def end_patch(self) -> Patch:
        from stimflow._chunk._patch import Patch

        return Patch(
            [
                Tile(
                    bases="".join(flow.end.values()),
                    data_qubits=flow.end.keys(),
                    measure_qubit=flow.center,
                    flags=flow.flags,
                )
                for flow in self.flows
                if flow.end
                if flow.obs_key is None
            ]
        )


def _accumulate_observable_indices_used_by_circuit(circuit: stim.Circuit, *, out: set[int]):
    for inst in circuit:
        if inst.name == "OBSERVABLE_INCLUDE":
            out.add(int(inst.gate_args_copy()[0]))
        elif inst.name == "REPEAT":
            _accumulate_observable_indices_used_by_circuit(inst.body_copy(), out=out)
