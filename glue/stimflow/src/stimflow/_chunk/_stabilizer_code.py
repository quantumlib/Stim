from __future__ import annotations

import collections
import functools
from collections.abc import Callable, Iterable, Sequence
from typing import Any, cast, Literal, TYPE_CHECKING

import stim

from stimflow._chunk._flow_metadata import FlowMetadata
from stimflow._chunk._patch import Patch
from stimflow._core import min_max_complex, NoiseRule, PauliMap, sorted_complex, str_svg, Tile

if TYPE_CHECKING:
    from stimflow._chunk._chunk_builder import ChunkBuilder
    import stimflow


class StabilizerCode:
    """This class stores the stabilizers and logicals of a stabilizer code.

    The exact semantics of the class are somewhat loose. For example, by default
    this class doesn't verify that its fields actually form a valid stabilizer
    code. This is so that the class can be used as a sort of useful data dumping
    ground even in cases where what is being built isn't a stabilizer code. For
    example, you can store a gauge code in the fields... it's just that methods
    like 'make_code_capacity_circuit' will no longer work.

    The stabilizers are stored as a `stimflow.Patch`. A patch is like a list of `stimflow.PauliMap`,
    except it actually stores `stimflow.Tile` instances so additional annotations can be added
    and additional utility methods are easily available.
    """

    def __init__(
        self,
        stabilizers: Iterable[Tile | PauliMap] | Patch | None = None,
        *,
        logicals: Iterable[PauliMap | tuple[PauliMap, PauliMap]] = (),
        scattered_logicals: Iterable[PauliMap] = (),
    ):
        """

        Args:
            stabilizers: The stabilizers of the code, specified as a Patch
            logicals: The logical qubits and/or observables of the code. Each entry should be
                either a pair of anti-commuting stimflow.PauliMap (e.g. the X and Z observables of the
                logical qubit) or a single stimflow.PauliMap (e.g. just the X observable).
            scattered_logicals: Logical operators with arbitrary commutation relationships to each
                other. Still expected to commute with the stabilizers.
        """
        __tracebackhide__ = True
        packed_obs: list[PauliMap | tuple[PauliMap, PauliMap]] = []
        for obs in logicals:
            if isinstance(obs, PauliMap):
                packed_obs.append(obs)
            elif len(obs) == 2 and isinstance(obs[0], PauliMap) and isinstance(obs[1], PauliMap):
                packed_obs.append(cast(tuple[PauliMap, PauliMap], tuple(obs)))
            else:
                raise NotImplementedError(
                    f"{obs=} isn't a Pauli product or anti-commuting pair of Pauli products."
                )
        if stabilizers is None:
            stabilizers = Patch([])
        elif not isinstance(stabilizers, Patch):
            stabilizers = Patch(stabilizers)

        self.stabilizers: Patch = stabilizers
        self.logicals: tuple[PauliMap | tuple[PauliMap, PauliMap], ...] = tuple(packed_obs)
        self.scattered_logicals: tuple[PauliMap, ...] = tuple(scattered_logicals)

        seen_names = set()
        for obs in self.flat_logicals:
            if obs.obs_name is None:
                raise ValueError(f"Unnamed logical operator: {obs!r}")
            if obs.obs_name in seen_names:
                raise ValueError(f"Name collision {obs.obs_name=}")
            seen_names.add(obs.obs_name)

    @property
    def patch(self) -> Patch:
        """Returns the stimflow.Patch storing the stabilizers of the code."""
        return self.stabilizers

    @functools.cached_property
    def flat_logicals(self) -> tuple[PauliMap, ...]:
        """Returns a list of the logical operators defined by the stabilizer code.

        It's "flat" because paired X/Z logicals are returned separately instead of
        as a tuple.
        """
        result: list[PauliMap] = []
        for logical in self.logicals:
            if isinstance(logical, tuple):
                result.extend(logical)
            else:
                result.append(logical)
        result.extend(self.scattered_logicals)
        return tuple(result)

    def with_remaining_degrees_of_freedom_as_logicals(self) -> StabilizerCode:
        """Solves for the logical observables, given only the stabilizers."""

        # Collect constraints.
        gen_pauli_maps: list[PauliMap] = []
        for stabilizer in self.stabilizers:
            gen_pauli_maps.append(stabilizer.to_pauli_map())
        for logical in self.logicals:
            if isinstance(logical, tuple):
                raise NotImplementedError(
                    "Logical (X, Z) pairs. Result might change the destabilizer."
                )
        gen_pauli_maps.extend(self.flat_logicals)

        # Convert to stim types.
        q2i = {q: i for i, q in enumerate(sorted_complex(self.patch.data_set))}
        i2q = {i: q for q, i in q2i.items()}
        stim_pauli_strings: list[stim.PauliString] = []
        for pm in gen_pauli_maps:
            ps = stim.PauliString(len(q2i))
            for q, p in pm.items():
                ps[q2i[q]] = p
            stim_pauli_strings.append(ps)

        # Solve remaining degrees of freedom with stim.
        full_tableau = stim.Tableau.from_stabilizers(
            stim_pauli_strings, allow_underconstrained=True, allow_redundant=True
        )

        # Convert back to stimflow.
        new_logicals = []
        for k in range(len(full_tableau)):
            z = full_tableau.z_output(k)
            if z in stim_pauli_strings:
                continue
            x = full_tableau.x_output(k)
            x2 = PauliMap(x).with_transformed_coords(cast(Any, i2q.__getitem__))
            z2 = PauliMap(z).with_transformed_coords(cast(Any, i2q.__getitem__))
            new_logicals.append((x2.with_obs_name(f"inferred_X{k}"), z2.with_obs_name(f"inferred_Z{k}")))

        return StabilizerCode(
            stabilizers=self.patch,
            logicals=(*self.logicals, *new_logicals),
            scattered_logicals=self.scattered_logicals,
        )

    def with_integer_coordinates(self) -> StabilizerCode:
        """Returns an equivalent stabilizer code, but with all qubit on Gaussian integers."""

        r2r = {v: i for i, v in enumerate(sorted({e.real for e in self.used_set}))}
        i2i = {v: i for i, v in enumerate(sorted({e.imag for e in self.used_set}))}
        return self.with_transformed_coords(lambda e: r2r[e.real] + i2i[e.imag] * 1j)

    def physical_to_logical(self, ps: stim.PauliString) -> PauliMap:
        """Maps a physical qubit string into a logical qubit string.

        Requires that all logicals be specified as X/Z tuples.
        """
        result: PauliMap = PauliMap()
        for q in ps.pauli_indices():
            if q >= len(self.logicals):
                raise ValueError("More qubits than logicals.")
            obs = self.logicals[q]
            if isinstance(obs, PauliMap):
                raise ValueError(
                    "Need logicals to be pairs of observables to map physical to logical."
                )
            p = ps[q]
            if p == 1:
                result *= obs[0]
            elif p == 2:
                result *= obs[0]
                result *= obs[1]
            elif p == 3:
                result *= obs[1]
            else:
                assert False
        return result

    def concat_over(
        self, under: StabilizerCode, *, skip_inner_stabilizers: bool = False
    ) -> StabilizerCode:
        """Computes the interleaved concatenation of two stabilizer codes."""
        over = self.with_integer_coordinates()
        c_min, c_max = min_max_complex(under.data_set)
        pitch = c_max - c_min + 1 + 1j

        def concatenated_obs(over_obs: PauliMap, under_index: int) -> PauliMap:
            total = PauliMap()
            for q, p in over_obs.items():
                obs_pair = under.logicals[under_index]
                if not isinstance(obs_pair, tuple):
                    raise NotImplementedError("Partial observable")
                logical_x, logical_z = obs_pair
                if p == "X":
                    obs = logical_x
                elif p == "Y":
                    obs = logical_x * logical_z
                elif p == "Z":
                    obs = logical_z
                else:
                    raise NotImplementedError(f"{q=}, {p=}")
                total *= obs.with_transformed_coords(
                    lambda e: q.real * pitch.real + q.imag * pitch.imag * 1j + e
                )
            return total.with_obs_name((over_obs.obs_name, under_index))

        new_stabilizers = []
        for stabilizer in over.stabilizers:
            ps = stabilizer.to_pauli_map()
            for k in range(len(under.logicals)):
                new_stabilizers.append(
                    concatenated_obs(ps, k).to_tile().with_edits(flags=stabilizer.flags)
                )
        if not skip_inner_stabilizers:
            for stabilizer in under.stabilizers:
                for q in over.data_set:
                    new_stabilizers.append(
                        stabilizer.with_transformed_coords(
                            lambda e: q.real * pitch.real + q.imag * pitch.imag * 1j + e
                        )
                    )

        new_logicals: list[PauliMap | tuple[PauliMap, PauliMap]] = []
        for logical in over.logicals:
            for k in range(len(under.logicals)):
                if isinstance(logical, PauliMap):
                    new_logicals.append(concatenated_obs(logical, k))
                else:
                    x, z = logical
                    new_logicals.append((concatenated_obs(x, k), concatenated_obs(z, k)))

        return StabilizerCode(stabilizers=new_stabilizers, logicals=new_logicals)

    def get_observable_by_basis(
        self, index: int, basis: Literal["X", "Y", "Z"] | str, *, default: Any = "__!not_specified"
    ) -> PauliMap:
        obs = self.logicals[index]
        if isinstance(obs, PauliMap) and set(obs.values()) == {basis}:
            return obs
        elif isinstance(obs, tuple):
            a1, a2 = obs
            b1 = frozenset(a1.values())
            b2 = frozenset(a2.values())
            if b1 == {basis}:
                return a1
            if b2 == {basis}:
                return a2
            if len(b1) == 1 and len(b2) == 1:
                # For example, we have X and Z specified and the user asked for Y.
                # Note that this works even if the X doesn't exactly overlap the Z.
                return (a1 * a2).with_obs_name((a1.obs_name, a2.obs_name))
        if default != "__!not_specified":
            return default
        raise ValueError(f"Couldn't return a basis {basis} observable from {obs=}.")

    def list_pure_basis_observables(self, basis: Literal["X", "Y", "Z"]) -> list[PauliMap]:
        result = []
        for k in range(len(self.logicals)):
            obs = self.get_observable_by_basis(k, basis, default=None)
            if obs is not None:
                result.append(obs)
        return result

    def x_basis_subset(self) -> StabilizerCode:
        return StabilizerCode(
            stabilizers=self.stabilizers.with_only_x_tiles(),
            logicals=self.list_pure_basis_observables("X"),
        )

    def z_basis_subset(self) -> StabilizerCode:
        return StabilizerCode(
            stabilizers=self.stabilizers.with_only_x_tiles(),
            logicals=self.list_pure_basis_observables("Z"),
        )

    @property
    def tiles(self) -> tuple[stimflow.Tile, ...]:
        """Returns the tiles of the code's stabilizer patch."""
        return self.stabilizers.tiles

    def verify_distance_is_at_least(self, minimum_distance: int):
        if minimum_distance == 2:
            self._verify_distance_is_at_least_2()
        elif minimum_distance == 3:
            self._verify_distance_is_at_least_3()
        elif minimum_distance < 2:
            return
        else:
            raise NotImplementedError("Only minimum_distance=2 and minimum_distance=3 are implemented efficiently.")

    def _verify_distance_is_at_least_2(self):
        """Verifies undetected logical errors require at least 2 physical errors.

        Verifies using a code capacity noise model.
        """
        __tracebackhide__ = True
        self.verify()

        circuit = self.make_code_capacity_circuit(noise=1e-3)
        for inst in circuit.detector_error_model():
            if inst.type == "error":
                dets = set()
                obs = set()
                for target in inst.targets_copy():
                    if target.is_relative_detector_id():
                        dets ^= {target.val}
                    elif target.is_logical_observable_id():
                        obs ^= {target.val}
                dets = frozenset(dets)
                obs = frozenset(obs)
                if obs and not dets:
                    filter_det = stim.DetectorErrorModel()
                    filter_det.append(inst)
                    err = circuit.explain_detector_error_model_errors(dem_filter=filter_det)
                    loc = err[0].circuit_error_locations[0].flipped_pauli_product[0]
                    raise ValueError(
                        f"Code has a distance 1 error:"
                        f"\n    {loc.gate_target.pauli_type} at {loc.coords}"
                    )

    def _verify_distance_is_at_least_3(self):
        """Verifies undetected logical errors require at least 3 physical errors.

        Verifies using a code capacity noise model.
        """
        __tracebackhide__ = True
        self._verify_distance_is_at_least_2()
        seen = {}
        circuit = self.make_code_capacity_circuit(noise=1e-3)
        for inst in circuit.detector_error_model().flattened():
            if inst.type == "error":
                dets = set()
                obs = set()
                for target in inst.targets_copy():
                    if target.is_relative_detector_id():
                        dets ^= {target.val}
                    elif target.is_logical_observable_id():
                        obs ^= {target.val}
                dets = frozenset(dets)
                obs = frozenset(obs)
                if dets not in seen:
                    seen[dets] = (obs, inst)
                elif seen[dets][0] != obs:
                    filter_det = stim.DetectorErrorModel()
                    filter_det.append(inst)
                    filter_det.append(seen[dets][1])
                    err = circuit.explain_detector_error_model_errors(
                        dem_filter=filter_det, reduce_to_one_representative_error=True
                    )
                    loc1 = err[0].circuit_error_locations[0].flipped_pauli_product[0]
                    loc2 = err[1].circuit_error_locations[0].flipped_pauli_product[0]
                    raise ValueError(
                        f"Code has a distance 2 error:"
                        f"\n    {loc1.gate_target.pauli_type} at {loc1.coords}"
                        f"\n    {loc2.gate_target.pauli_type} at {loc2.coords}"
                    )

    def find_distance(self, *, max_search_weight: int) -> int:
        return len(self.find_logical_error(max_search_weight=max_search_weight))

    def find_logical_error(self, *, max_search_weight: int) -> list[stim.ExplainedError]:
        circuit = self.make_code_capacity_circuit(noise=1e-3)
        if max_search_weight == 2:
            return circuit.shortest_graphlike_error(canonicalize_circuit_errors=True)
        return circuit.search_for_undetectable_logical_errors(
            dont_explore_edges_with_degree_above=max_search_weight,
            dont_explore_detection_event_sets_with_size_above=max_search_weight,
            dont_explore_edges_increasing_symptom_degree=False,
            canonicalize_circuit_errors=True,
        )

    def with_observables_from_basis(self, basis: Literal["X", "Y", "Z"]) -> StabilizerCode:
        if basis == "X":
            return StabilizerCode(
                stabilizers=self.stabilizers, logicals=self.list_pure_basis_observables("X")
            )
        elif basis == "Y":
            return StabilizerCode(
                stabilizers=self.stabilizers, logicals=self.list_pure_basis_observables("Y")
            )
        elif basis == "Z":
            return StabilizerCode(
                stabilizers=self.stabilizers, logicals=self.list_pure_basis_observables("Z")
            )
        else:
            raise NotImplementedError(f"{basis=}")

    def as_interface(self) -> stimflow.ChunkInterface:
        from stimflow._chunk._chunk_interface import ChunkInterface

        ports: list[PauliMap] = []
        for tile in self.stabilizers.tiles:
            if tile.data_set:
                ports.append(tile.to_pauli_map())
        ports.extend(self.flat_logicals)
        return ChunkInterface(ports=ports, discards=[])

    def with_edits(
        self,
        *,
        stabilizers: Iterable[Tile | PauliMap] | Patch | None = None,
        logicals: Iterable[PauliMap | tuple[PauliMap, PauliMap]] | None = None,
    ) -> StabilizerCode:
        return StabilizerCode(
            stabilizers=self.stabilizers if stabilizers is None else stabilizers,
            logicals=self.logicals if logicals is None else logicals,
        )

    @functools.cached_property
    def data_set(self) -> frozenset[complex]:
        result = set(self.stabilizers.data_set)
        for obs in self.logicals:
            if isinstance(obs, PauliMap):
                result |= obs.keys()
            else:
                a, b = obs
                result |= a.keys()
                result |= b.keys()
        return frozenset(result)

    @functools.cached_property
    def measure_set(self) -> frozenset[complex]:
        return self.stabilizers.measure_set

    @functools.cached_property
    def used_set(self) -> frozenset[complex]:
        result = set(self.stabilizers.used_set)
        for obs in self.logicals:
            if isinstance(obs, PauliMap):
                result |= obs.keys()
            else:
                a, b = obs
                result |= a.keys()
                result |= b.keys()
        return frozenset(result)

    @staticmethod
    def from_patch_with_inferred_observables(patch: Patch) -> StabilizerCode:
        return StabilizerCode(patch).with_remaining_degrees_of_freedom_as_logicals()

    def verify(self) -> None:
        """Verifies observables and stabilizers relate as a stabilizer code.

        All stabilizers should commute with each other.
        All stabilizers should commute with all observables.
        Same-index X and Z observables should anti-commute.
        All other observable pairs should commute.
        """
        __tracebackhide__ = True

        q2tiles: collections.defaultdict[complex, list[Tile]] = collections.defaultdict(list)
        for tile in self.stabilizers.tiles:
            for q in tile.data_set:
                q2tiles[q].append(tile)
        for tile1 in self.stabilizers.tiles:
            overlapping = {tile2 for q in tile1.data_set for tile2 in q2tiles[q]}
            for tile2 in overlapping:
                t1 = tile1.to_pauli_map()
                t2 = tile2.to_pauli_map()
                if not t1.commutes(t2):
                    raise ValueError(
                        f"Tile stabilizer {t1=} anticommutes with tile stabilizer {t2=}."
                    )

        for tile in self.stabilizers.tiles:
            ps = tile.to_pauli_map()
            for obs in self.flat_logicals:
                if not ps.commutes(obs):
                    raise ValueError(f"Tile stabilizer {tile=} anticommutes with {obs=}.")

        for entry in self.logicals:
            if not isinstance(entry, PauliMap):
                a, b = entry
                if a.commutes(b):
                    raise ValueError(f"The observable pair {a} vs {b} didn't anticommute.")

        packed_obs: list[Sequence[PauliMap]] = []
        for entry in self.logicals:
            if isinstance(entry, PauliMap):
                packed_obs.append([entry])
            else:
                packed_obs.append(entry)
        for k1 in range(len(packed_obs)):
            for k2 in range(k1 + 1, len(packed_obs)):
                for obs1 in packed_obs[k1]:
                    for obs2 in packed_obs[k2]:
                        if not obs1.commutes(obs2):
                            raise ValueError(
                                f"Unpaired observables didn't commute: {obs1=}, {obs2=}."
                            )

    def with_xz_flipped(self) -> StabilizerCode:
        """Returns the same stabilizer code, but with all qubits Hadamard conjugated."""
        new_observables: list[PauliMap | tuple[PauliMap, PauliMap]] = []
        for entry in self.logicals:
            if isinstance(entry, PauliMap):
                new_observables.append(entry.with_xz_flipped())
            else:
                a, b = entry
                new_observables.append((a.with_xz_flipped(), b.with_xz_flipped()))
        return StabilizerCode(
            stabilizers=self.stabilizers.with_xz_flipped(), logicals=new_observables
        )

    def _repr_svg_(self) -> str:
        return self.to_svg()

    def to_svg(
        self,
        *,
        title: str | list[str] | None = None,
        canvas_height: int | None = None,
        show_order: bool = False,
        show_measure_qubits: bool = False,
        show_data_qubits: bool = True,
        system_qubits: Iterable[complex] = (),
        opacity: float = 1,
        show_coords: bool = True,
        show_obs: bool = True,
        other: stimflow.StabilizerCode | Patch | Iterable[stimflow.StabilizerCode | Patch] | None = None,
        tile_color_func: (
            Callable[
                [stimflow.Tile],
                str | tuple[float, float, float] | tuple[float, float, float, float] | None,
            ]
            | None
        ) = None,
        rows: int | None = None,
        cols: int | None = None,
        find_logical_err_max_weight: int | None = None,
        stabilizer_style: Literal["polygon", "circles"] | None = "polygon",
        observable_style: Literal["label", "polygon", "circles"] = "label",
    ) -> str_svg:
        """Returns an SVG diagram of the stabilizer code."""
        flat: list[StabilizerCode | Patch] = [self] if self is not None else []
        if isinstance(other, (StabilizerCode, Patch)):
            flat.append(other)
        elif other is not None:
            flat.extend(other)

        from stimflow._viz import svg

        return svg(
            objects=flat,
            title=title,
            show_obs=show_obs,
            canvas_height=canvas_height,
            show_measure_qubits=show_measure_qubits,
            show_data_qubits=show_data_qubits,
            show_order=show_order,
            find_logical_err_max_weight=find_logical_err_max_weight,
            system_qubits=system_qubits,
            opacity=opacity,
            show_coords=show_coords,
            tile_color_func=tile_color_func,
            cols=cols,
            rows=rows,
            stabilizer_style=stabilizer_style,
            observable_style=observable_style,
        )

    def with_transformed_coords(
        self, coord_transform: Callable[[complex], complex]
    ) -> StabilizerCode:
        """Returns the same stabilizer code, but with coordinates transformed by the given
        function."""
        new_observables: list[PauliMap | tuple[PauliMap, PauliMap]] = []
        for entry in self.logicals:
            if isinstance(entry, PauliMap):
                new_observables.append(entry.with_transformed_coords(coord_transform))
            else:
                a, b = entry
                new_observables.append(
                    (
                        a.with_transformed_coords(coord_transform),
                        b.with_transformed_coords(coord_transform),
                    )
                )
        return StabilizerCode(
            stabilizers=self.stabilizers.with_transformed_coords(coord_transform),
            logicals=new_observables,
            scattered_logicals=[
                e.with_transformed_coords(coord_transform) for e in self.scattered_logicals
            ],
        )

    def make_code_capacity_circuit(
        self,
        *,
        noise: float | NoiseRule,
        metadata_func: Callable[[stimflow.Flow], stimflow.FlowMetadata] = lambda _: FlowMetadata(),
    ) -> stim.Circuit:
        """Produces a code capacity noisy memory experiment circuit for the stabilizer code."""
        if isinstance(noise, (int, float)):
            noise = NoiseRule(after={"DEPOLARIZE1": noise})
        if noise.flip_result:
            raise ValueError(f"{noise=} includes measurement noise.")
        from stimflow._chunk._chunk_compiler import ChunkCompiler

        compiler = ChunkCompiler(metadata_func=metadata_func)
        interface = self.as_interface()
        compiler.append_magic_init_chunk(interface)
        all_qs = sorted(compiler.q2i.values())
        for gate, strength in noise.before.items():
            compiler.circuit.append(gate, targets=all_qs, arg=[strength])
        for gate, strength in noise.after.items():
            compiler.circuit.append(gate, targets=all_qs, arg=[strength])
        compiler.append_magic_end_chunk(interface)
        return compiler.finish_circuit()

    def make_phenom_circuit(
        self,
        *,
        noise: float | NoiseRule,
        rounds: int,
        metadata_func: Callable[[stimflow.Flow], stimflow.FlowMetadata] = lambda _: FlowMetadata(),
    ) -> stim.Circuit:
        """Produces a phenomenological noise memory experiment circuit for the stabilizer code."""
        if isinstance(noise, (int, float)):
            noise = NoiseRule(after={"DEPOLARIZE1": noise}, flip_result=noise)
        from stimflow._chunk._chunk_compiler import ChunkCompiler
        from stimflow._chunk._chunk_loop import ChunkLoop

        from stimflow._chunk._chunk_builder import ChunkBuilder
        builder = ChunkBuilder(self.data_set)
        for obs in self.flat_logicals:
            builder.add_flow(start=obs, end=obs)
        for gate, strength in noise.before.items():
            builder.append(gate, self.data_set, arg=strength)
        for k, tile in enumerate(self.tiles):
            builder.add_flow(start=tile, end=tile)
        before_noise_chunk = builder.finish_chunk(wants_to_merge_with_next=True)

        builder = ChunkBuilder(self.data_set)
        for obs in self.flat_logicals:
            builder.add_flow(start=obs, end=obs)
        for gate, strength in noise.after.items():
            builder.append(gate, self.data_set, arg=strength)
        for k, tile in enumerate(self.tiles):
            builder.add_flow(start=tile, end=tile)
        after_noise_chunk = builder.finish_chunk(wants_to_merge_with_prev=True)

        builder = ChunkBuilder(self.data_set)
        for obs in self.flat_logicals:
            builder.add_flow(start=obs, end=obs)
        for k1, layer in enumerate(self.patch.partitioned_tiles):
            if k1 > 0:
                builder.append("TICK")
            for k2, tile in enumerate(layer):
                builder.append(
                    "MPP", [tile], measure_key_func=lambda _: f"det{k1},{k2}", arg=noise.flip_result
                )
                builder.add_flow(end=tile, ms=[f"det{k1},{k2}"])
                builder.add_flow(start=tile, ms=[f"det{k1},{k2}"])

        measure_chunk = builder.finish_chunk()

        compiler = ChunkCompiler(metadata_func=metadata_func)
        compiler.append_magic_init_chunk(measure_chunk.start_interface())
        compiler.append(before_noise_chunk.with_edits(wants_to_merge_with_next=False))
        compiler.append(ChunkLoop([before_noise_chunk, measure_chunk, after_noise_chunk], rounds))
        compiler.append(after_noise_chunk.with_edits(wants_to_merge_with_prev=False))
        compiler.append_magic_end_chunk()
        return compiler.finish_circuit()

    def __repr__(self) -> str:
        def indented(x: str) -> str:
            return x.replace("\n", "\n    ")

        def indented_repr(x: Any) -> str:
            if isinstance(x, tuple):
                return indented(indented("[\n" + ",\n".join(indented_repr(e) for e in x)) + ",\n]")
            return indented(repr(x))

        return f"""stimflow.StabilizerCode(
    stabilizers={indented_repr(self.stabilizers)},
    logicals={indented_repr(self.logicals)},
    scattered_logicals={indented_repr(self.scattered_logicals)},
)"""

    def __eq__(self, other) -> bool:
        if not isinstance(other, StabilizerCode):
            return NotImplemented
        return self.stabilizers == other.stabilizers and self.logicals == other.logicals

    def __ne__(self, other) -> bool:
        return not (self == other)

    @functools.lru_cache(maxsize=1)
    def __hash__(self) -> int:
        return hash((StabilizerCode, self.stabilizers, self.logicals))

    def transversal_init_chunk(
        self,
        *,
        basis: (
            Literal["X", "Y", "Z"]
            | str
            | stimflow.PauliMap
            | dict[complex, str | Literal["X", "Y", "Z"]]
        ),
    ) -> stimflow.Chunk:
        """Returns a chunk that describes initializing the stabilizer code with given reset bases.

        Stabilizers that anticommute with the resets will be discarded flows.

        The returned chunk isn't guaranteed to be fault tolerant.
        """
        from stimflow._chunk._chunk_builder import ChunkBuilder

        basis_map: PauliMap
        if isinstance(basis, str):
            basis_map = PauliMap({cast(Any, basis): self.data_set})
        elif isinstance(basis, PauliMap):
            basis_map = basis
        elif isinstance(basis, dict):
            basis_map = PauliMap(basis)
        else:
            raise NotImplementedError(f"{basis=}")
        builder = ChunkBuilder(self.data_set)
        for b in "XYZ":
            builder.append(f"R{b}", [q for q in self.data_set if basis_map[q] == b])
        for tile in self.tiles:
            if not tile.data_set:
                continue
            if all(basis_map[q] == p for q, p in tile.to_pauli_map().items()):
                builder.add_flow(end=tile)
            else:
                builder.add_discarded_flow_output(tile)
        for obs in self.flat_logicals:
            if all(basis_map[q] == p for q, p in obs.items()):
                builder.add_flow(end=obs)
            else:
                builder.add_discarded_flow_output(obs)

        return builder.finish_chunk(wants_to_merge_with_next=True)

    def transversal_measure_chunk(
        self,
        *,
        basis: (
            Literal["X", "Y", "Z"]
            | str
            | stimflow.PauliMap
            | dict[complex, str | Literal["X", "Y", "Z"]]
        ),
    ) -> stimflow.Chunk:
        """Returns a chunk that describes measuring the stabilizer code with given measure bases.

        Stabilizers that anticommute with the measurements will be discarded flows.

        The returned chunk isn't guaranteed to be fault tolerant.
        """
        return self.transversal_init_chunk(basis=basis).time_reversed()
