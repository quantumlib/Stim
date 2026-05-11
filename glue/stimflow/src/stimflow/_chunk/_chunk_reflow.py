from __future__ import annotations

import functools
from collections.abc import Callable, Iterable
from typing import Any, cast, Literal, TYPE_CHECKING

from stimflow._chunk._patch import Patch
from stimflow._chunk._stabilizer_code import StabilizerCode
from stimflow._chunk._test_util import assert_has_same_set_of_items_as
from stimflow._core import PauliMap, sorted_complex, Tile

if TYPE_CHECKING:
    from stimflow._chunk._chunk_interface import ChunkInterface


class ChunkReflow:
    """An adapter chunk for attaching chunks describing the same thing in different ways.

    For example, consider two surface code idle round chunks where one has the logical
    operator on the left side and the other has the logical operator on the right side.
    They can't be directly concatenated, because their flows don't match. But a reflow
    chunk can be placed in between, mapping the left logical operator to the right
    logical operator times a set of stabilizers, in order to bridge the incompatibility.
    """

    def __init__(self, out2in: dict[PauliMap, list[PauliMap]], discard_in: Iterable[PauliMap] = ()):
        self.out2in = out2in
        self.discard_in = tuple(discard_in)
        assert isinstance(self.out2in, dict)
        for k, vs in self.out2in.items():
            assert isinstance(k, PauliMap), k
            assert isinstance(vs, list)
            for v in vs:
                assert isinstance(v, PauliMap)

    @staticmethod
    def from_auto_rewrite(
        *, inputs: Iterable[PauliMap], out2in: dict[PauliMap, list[PauliMap] | Literal["auto"]]
    ) -> ChunkReflow:
        new_out2in: dict[PauliMap, list[PauliMap]] = {}
        unsolved: list[PauliMap] = []
        for pk, pv in out2in.items():
            if pv == "auto":
                unsolved.append(pk)
            else:
                new_out2in[pk] = cast(Any, pv)
        if not unsolved:
            return ChunkReflow(out2in=new_out2in)

        rows: list[tuple[set[int], PauliMap]] = []
        inputs = list(inputs)
        qs: set[complex] = set()
        for index in range(len(inputs)):
            rows.append(({index}, inputs[index]))
            qs |= inputs[index].keys()
        for pv2 in unsolved:
            rows.append((set(), pv2))
        num_solved = 0
        for q in sorted_complex(qs):
            for b in "ZX":
                for pivot in range(num_solved, len(inputs)):
                    p = rows[pivot][1][q]
                    if p != b and p != "I":
                        break
                else:
                    continue
                for row in range(len(rows)):
                    p = rows[row][1][q]
                    if row != pivot and p != b and p != "I":
                        a1, b1 = rows[row]
                        a2, b2 = rows[pivot]
                        rows[row] = (a1 ^ a2, b1 * b2)
                if pivot != num_solved:
                    rows[num_solved], rows[pivot] = rows[pivot], rows[num_solved]
                num_solved += 1
        for index in range(len(unsolved)):
            v = rows[index + len(inputs)]
            if v[1]:
                raise ValueError(f"Failed to solve for {unsolved[index]}.")
            new_out2in[unsolved[index]] = [inputs[v2] for v2 in v[0]]

        return ChunkReflow(out2in=new_out2in)

    @staticmethod
    def from_auto_rewrite_transitions_using_stable(
        *, stable: Iterable[PauliMap], transitions: Iterable[tuple[PauliMap, PauliMap]]
    ) -> ChunkReflow:
        """Bridges the given transitions using products from the given stable values."""
        new_out2in: dict[PauliMap, list[PauliMap]] = {}

        stable = list(stable)
        rows: list[tuple[set[int], PauliMap]] = []
        used_qubits: set[complex] = set()
        for index, s in enumerate(stable):
            new_out2in[s] = [s]
            used_qubits |= s.keys()
            rows.append(({index}, s))
        num_stable_rows = len(rows)

        unsolved: list[tuple[PauliMap, PauliMap]] = []
        for inp, out in transitions:
            assert inp.name == out.name
            if inp == out:
                new_out2in[out] = [inp]
            else:
                unsolved.append((inp, out))
        if not unsolved:
            return ChunkReflow(out2in=new_out2in)

        for inp, out in unsolved:
            rows.append((set(), PauliMap(inp) * PauliMap(out)))
        num_solved = 0
        for q in sorted_complex(used_qubits):
            for b in "ZX":
                for pivot in range(num_solved, num_stable_rows):
                    p = rows[pivot][1][q]
                    if p != b and p != "I":
                        break
                else:
                    continue
                for row in range(len(rows)):
                    p = rows[row][1][q]
                    if row != pivot and p != b and p != "I":
                        a1, b1 = rows[row]
                        a2, b2 = rows[pivot]
                        rows[row] = (a1 ^ a2, b1 * b2)
                if pivot != num_solved:
                    rows[num_solved], rows[pivot] = rows[pivot], rows[num_solved]
                num_solved += 1
        for index in range(len(unsolved)):
            inp, out = unsolved[index]
            used_indices, remainder = rows[index + num_stable_rows]
            if remainder:
                raise ValueError(f"Failed to solve for {inp} -> {out}.")
            new_out2in[out] = [inp] + [stable[k] for k in used_indices]

        return ChunkReflow(out2in=new_out2in)

    def with_obs_flows_as_det_flows(self):
        return ChunkReflow(
            out2in={PauliMap(k): [PauliMap(v) for v in vs] for k, vs in self.out2in.items()},
            discard_in=[PauliMap(k) for k in self.discard_in],
        )

    def with_transformed_coords(self, transform: Callable[[complex], complex]) -> ChunkReflow:
        return ChunkReflow(
            out2in={
                kp.with_transformed_coords(transform): [
                    vp.with_transformed_coords(transform) for vp in vs
                ]
                for kp, vs in self.out2in.items()
            },
            discard_in=[kp.with_transformed_coords(transform) for kp in self.discard_in],
        )

    def start_interface(self) -> ChunkInterface:
        from stimflow._chunk._chunk_interface import ChunkInterface

        return ChunkInterface(
            ports={v for vs in self.out2in.values() for v in vs}, discards=self.discard_in
        )

    def end_interface(self) -> ChunkInterface:
        from stimflow._chunk._chunk_interface import ChunkInterface

        return ChunkInterface(ports=self.out2in.keys(), discards=self.discard_in)

    def start_code(self) -> StabilizerCode:
        tiles: list[Tile] = []
        observables: list[PauliMap] = []
        for obs in self.removed_inputs:
            if obs.name is None:
                tiles.append(Tile(data_qubits=obs.keys(), bases="".join(obs.values())))
            else:
                observables.append(obs)
        return StabilizerCode(stabilizers=Patch(tiles), logicals=observables)

    def start_patch(self) -> Patch:
        return self.start_code().patch

    def end_code(self) -> StabilizerCode:
        tiles: list[Tile] = []
        observables: list[PauliMap] = []
        for obs in self.out2in.keys():
            if obs.name is None:
                tiles.append(Tile(data_qubits=obs.keys(), bases="".join(obs.values())))
            else:
                observables.append(obs)
        return StabilizerCode(stabilizers=Patch(tiles), logicals=observables)

    def end_patch(self) -> Patch:
        return self.end_code().patch

    @functools.cached_property
    def removed_inputs(self) -> frozenset[PauliMap]:
        return frozenset(v for vs in self.out2in.values() for v in vs) | frozenset(self.discard_in)

    def verify(
        self,
        *,
        expected_in: StabilizerCode | ChunkInterface | None = None,
        expected_out: StabilizerCode | ChunkInterface | None = None,
    ):
        """Verifies that the ChunkReflow is well-formed."""
        from stimflow._chunk._chunk_interface import ChunkInterface

        assert isinstance(self.out2in, dict)
        for k, vs in self.out2in.items():
            assert isinstance(k, PauliMap), k
            assert isinstance(vs, list)
            for v in vs:
                assert isinstance(v, PauliMap)

        for k, vs in self.out2in.items():
            acc = PauliMap({})
            for v in vs:
                acc *= PauliMap(v)
            if acc != PauliMap(k):
                lines = [
                    "A reflow output wasn't equal to the product of its inputs.",
                    f"   Output: {k}",
                    f"   Difference: {PauliMap(k) * acc}",
                    "   Inputs:",
                ]
                for v in vs:
                    lines.append(f"        {v}")
                raise ValueError("\n".join(lines))

        if expected_in is not None:
            if isinstance(expected_in, StabilizerCode):
                expected_in = expected_in.as_interface()
            assert isinstance(expected_in, ChunkInterface)
            assert_has_same_set_of_items_as(
                self.start_interface().with_discards_as_ports().ports,
                expected_in.with_discards_as_ports().ports,
                "self.start_interface().with_discards_as_ports().ports",
                "expected_in.with_discards_as_ports().ports",
            )

        if expected_out is not None:
            if isinstance(expected_out, StabilizerCode):
                expected_out = expected_out.as_interface()
            assert isinstance(expected_out, ChunkInterface)
            assert_has_same_set_of_items_as(
                self.end_interface().with_discards_as_ports().ports,
                expected_out.with_discards_as_ports().ports,
                "self.end_interface().with_discards_as_ports().ports",
                "expected_out.with_discards_as_ports().ports",
            )

        if len(self.out2in) != len(self.removed_inputs):
            msg = ["Number of outputs != number of distinct inputs.", "Outputs {"]
            for ps, obs in self.out2in:
                msg.append(f"    {ps}, obs={obs}")
            msg.append("}")
            msg.append("Distinct inputs {")
            for ps, obs in self.removed_inputs:
                msg.append(f"    {ps}, obs={obs}")
            msg.append("}")
            raise ValueError("\n".join(msg))

    def __eq__(self, other) -> bool:
        if isinstance(other, ChunkReflow):
            kv1 = {k: set(v) for k, v in self.out2in.items()}
            kv2 = {k: set(v) for k, v in other.out2in.items()}
            return kv1 == kv2 and self.discard_in == other.discard_in
        return False

    def __ne__(self, other) -> bool:
        return not (self == other)

    def __repr__(self) -> str:
        lines = []
        lines.append("stimflow.ChunkReflow(")
        lines.append("    out2in={")
        for k, v in self.out2in.items():
            if len(v) == 1 and v[0] == k:
                lines.append(f"        {k!r}: {v!r},")
            else:
                lines.append(f"        {k!r}: [")
                for v2 in v:
                    lines.append(f"            {v2!r},")
                lines.append("        ],")
        lines.append("    },")
        lines.append("    discard_in=(")
        for discarded_in in self.discard_in:
            lines.append(f"        {discarded_in!r},")
        lines.append("    ),")
        return "\n".join(lines)

    def __str__(self) -> str:
        lines = ["Reflow {"]
        for k, v in self.out2in.items():
            if [k] != v:
                lines.append(f"    gather {k} {{")
                for v2 in v:
                    lines.append(f"        {v2}")
                lines.append("    }")
        for k, v in self.out2in.items():
            if [k] == v:
                lines.append(f"    keep {k}")
        for k in self.discard_in:
            lines.append(f"    discard {k}")
        lines.append("}")
        return "\n".join(lines)

    def flattened(self) -> list[ChunkReflow]:
        """This is here for duck-type compatibility with ChunkLoop."""
        return [self]
