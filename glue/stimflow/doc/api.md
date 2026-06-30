# stimflow (Development Version) API Reference

## Index
- [`stimflow.Chunk`](#stimflow.Chunk)
    - [`stimflow.Chunk.__init__`](#stimflow.Chunk.__init__)
    - [`stimflow.Chunk.end_code`](#stimflow.Chunk.end_code)
    - [`stimflow.Chunk.end_interface`](#stimflow.Chunk.end_interface)
    - [`stimflow.Chunk.end_patch`](#stimflow.Chunk.end_patch)
    - [`stimflow.Chunk.find_distance`](#stimflow.Chunk.find_distance)
    - [`stimflow.Chunk.find_logical_error`](#stimflow.Chunk.find_logical_error)
    - [`stimflow.Chunk.flattened`](#stimflow.Chunk.flattened)
    - [`stimflow.Chunk.from_circuit_with_mpp_boundaries`](#stimflow.Chunk.from_circuit_with_mpp_boundaries)
    - [`stimflow.Chunk.start_code`](#stimflow.Chunk.start_code)
    - [`stimflow.Chunk.start_interface`](#stimflow.Chunk.start_interface)
    - [`stimflow.Chunk.start_patch`](#stimflow.Chunk.start_patch)
    - [`stimflow.Chunk.then`](#stimflow.Chunk.then)
    - [`stimflow.Chunk.time_reversed`](#stimflow.Chunk.time_reversed)
    - [`stimflow.Chunk.to_closed_circuit`](#stimflow.Chunk.to_closed_circuit)
    - [`stimflow.Chunk.to_coord_circuit`](#stimflow.Chunk.to_coord_circuit)
    - [`stimflow.Chunk.to_html_viewer`](#stimflow.Chunk.to_html_viewer)
    - [`stimflow.Chunk.verify`](#stimflow.Chunk.verify)
    - [`stimflow.Chunk.verify_distance_is_at_least`](#stimflow.Chunk.verify_distance_is_at_least)
    - [`stimflow.Chunk.with_edits`](#stimflow.Chunk.with_edits)
    - [`stimflow.Chunk.with_flag_added_to_all_flows`](#stimflow.Chunk.with_flag_added_to_all_flows)
    - [`stimflow.Chunk.with_obs_flows_as_det_flows`](#stimflow.Chunk.with_obs_flows_as_det_flows)
    - [`stimflow.Chunk.with_repetitions`](#stimflow.Chunk.with_repetitions)
    - [`stimflow.Chunk.with_transformed_coords`](#stimflow.Chunk.with_transformed_coords)
    - [`stimflow.Chunk.with_xz_flipped`](#stimflow.Chunk.with_xz_flipped)
- [`stimflow.ChunkBuilder`](#stimflow.ChunkBuilder)
    - [`stimflow.ChunkBuilder.__init__`](#stimflow.ChunkBuilder.__init__)
    - [`stimflow.ChunkBuilder.add_discarded_flow_input`](#stimflow.ChunkBuilder.add_discarded_flow_input)
    - [`stimflow.ChunkBuilder.add_discarded_flow_output`](#stimflow.ChunkBuilder.add_discarded_flow_output)
    - [`stimflow.ChunkBuilder.add_flow`](#stimflow.ChunkBuilder.add_flow)
    - [`stimflow.ChunkBuilder.append`](#stimflow.ChunkBuilder.append)
    - [`stimflow.ChunkBuilder.append_feedback`](#stimflow.ChunkBuilder.append_feedback)
    - [`stimflow.ChunkBuilder.finish_chunk`](#stimflow.ChunkBuilder.finish_chunk)
    - [`stimflow.ChunkBuilder.has_measurement`](#stimflow.ChunkBuilder.has_measurement)
    - [`stimflow.ChunkBuilder.lookup_measurement_indices`](#stimflow.ChunkBuilder.lookup_measurement_indices)
- [`stimflow.ChunkCompiler`](#stimflow.ChunkCompiler)
    - [`stimflow.ChunkCompiler.__init__`](#stimflow.ChunkCompiler.__init__)
    - [`stimflow.ChunkCompiler.append`](#stimflow.ChunkCompiler.append)
    - [`stimflow.ChunkCompiler.append_magic_end_chunk`](#stimflow.ChunkCompiler.append_magic_end_chunk)
    - [`stimflow.ChunkCompiler.append_magic_init_chunk`](#stimflow.ChunkCompiler.append_magic_init_chunk)
    - [`stimflow.ChunkCompiler.copy`](#stimflow.ChunkCompiler.copy)
    - [`stimflow.ChunkCompiler.cur_circuit_html_viewer`](#stimflow.ChunkCompiler.cur_circuit_html_viewer)
    - [`stimflow.ChunkCompiler.cur_end_interface`](#stimflow.ChunkCompiler.cur_end_interface)
    - [`stimflow.ChunkCompiler.ensure_observables_included`](#stimflow.ChunkCompiler.ensure_observables_included)
    - [`stimflow.ChunkCompiler.ensure_qubits_included`](#stimflow.ChunkCompiler.ensure_qubits_included)
    - [`stimflow.ChunkCompiler.finish_circuit`](#stimflow.ChunkCompiler.finish_circuit)
- [`stimflow.ChunkInterface`](#stimflow.ChunkInterface)
    - [`stimflow.ChunkInterface.data_set`](#stimflow.ChunkInterface.data_set)
    - [`stimflow.ChunkInterface.partitioned_detector_flows`](#stimflow.ChunkInterface.partitioned_detector_flows)
    - [`stimflow.ChunkInterface.to_code`](#stimflow.ChunkInterface.to_code)
    - [`stimflow.ChunkInterface.to_patch`](#stimflow.ChunkInterface.to_patch)
    - [`stimflow.ChunkInterface.to_svg`](#stimflow.ChunkInterface.to_svg)
    - [`stimflow.ChunkInterface.used_set`](#stimflow.ChunkInterface.used_set)
    - [`stimflow.ChunkInterface.with_discards_as_ports`](#stimflow.ChunkInterface.with_discards_as_ports)
    - [`stimflow.ChunkInterface.with_edits`](#stimflow.ChunkInterface.with_edits)
    - [`stimflow.ChunkInterface.with_transformed_coords`](#stimflow.ChunkInterface.with_transformed_coords)
    - [`stimflow.ChunkInterface.without_discards`](#stimflow.ChunkInterface.without_discards)
    - [`stimflow.ChunkInterface.without_keyed`](#stimflow.ChunkInterface.without_keyed)
- [`stimflow.ChunkLoop`](#stimflow.ChunkLoop)
    - [`stimflow.ChunkLoop.end_interface`](#stimflow.ChunkLoop.end_interface)
    - [`stimflow.ChunkLoop.end_patch`](#stimflow.ChunkLoop.end_patch)
    - [`stimflow.ChunkLoop.find_distance`](#stimflow.ChunkLoop.find_distance)
    - [`stimflow.ChunkLoop.find_logical_error`](#stimflow.ChunkLoop.find_logical_error)
    - [`stimflow.ChunkLoop.flattened`](#stimflow.ChunkLoop.flattened)
    - [`stimflow.ChunkLoop.start_interface`](#stimflow.ChunkLoop.start_interface)
    - [`stimflow.ChunkLoop.start_patch`](#stimflow.ChunkLoop.start_patch)
    - [`stimflow.ChunkLoop.time_reversed`](#stimflow.ChunkLoop.time_reversed)
    - [`stimflow.ChunkLoop.to_closed_circuit`](#stimflow.ChunkLoop.to_closed_circuit)
    - [`stimflow.ChunkLoop.to_html_viewer`](#stimflow.ChunkLoop.to_html_viewer)
    - [`stimflow.ChunkLoop.verify`](#stimflow.ChunkLoop.verify)
    - [`stimflow.ChunkLoop.verify_distance_is_at_least`](#stimflow.ChunkLoop.verify_distance_is_at_least)
    - [`stimflow.ChunkLoop.with_repetitions`](#stimflow.ChunkLoop.with_repetitions)
- [`stimflow.ChunkReflow`](#stimflow.ChunkReflow)
    - [`stimflow.ChunkReflow.end_code`](#stimflow.ChunkReflow.end_code)
    - [`stimflow.ChunkReflow.end_interface`](#stimflow.ChunkReflow.end_interface)
    - [`stimflow.ChunkReflow.end_patch`](#stimflow.ChunkReflow.end_patch)
    - [`stimflow.ChunkReflow.flattened`](#stimflow.ChunkReflow.flattened)
    - [`stimflow.ChunkReflow.from_auto_rewrite`](#stimflow.ChunkReflow.from_auto_rewrite)
    - [`stimflow.ChunkReflow.from_auto_rewrite_transitions_using_stable`](#stimflow.ChunkReflow.from_auto_rewrite_transitions_using_stable)
    - [`stimflow.ChunkReflow.removed_inputs`](#stimflow.ChunkReflow.removed_inputs)
    - [`stimflow.ChunkReflow.start_code`](#stimflow.ChunkReflow.start_code)
    - [`stimflow.ChunkReflow.start_interface`](#stimflow.ChunkReflow.start_interface)
    - [`stimflow.ChunkReflow.start_patch`](#stimflow.ChunkReflow.start_patch)
    - [`stimflow.ChunkReflow.verify`](#stimflow.ChunkReflow.verify)
    - [`stimflow.ChunkReflow.with_obs_flows_as_det_flows`](#stimflow.ChunkReflow.with_obs_flows_as_det_flows)
    - [`stimflow.ChunkReflow.with_transformed_coords`](#stimflow.ChunkReflow.with_transformed_coords)
- [`stimflow.Flow`](#stimflow.Flow)
    - [`stimflow.Flow.__init__`](#stimflow.Flow.__init__)
    - [`stimflow.Flow.__mul__`](#stimflow.Flow.__mul__)
    - [`stimflow.Flow.fused_with_next_flow`](#stimflow.Flow.fused_with_next_flow)
    - [`stimflow.Flow.obs_name`](#stimflow.Flow.obs_name)
    - [`stimflow.Flow.to_stim_flow`](#stimflow.Flow.to_stim_flow)
    - [`stimflow.Flow.with_edits`](#stimflow.Flow.with_edits)
    - [`stimflow.Flow.with_transformed_coords`](#stimflow.Flow.with_transformed_coords)
    - [`stimflow.Flow.with_xz_flipped`](#stimflow.Flow.with_xz_flipped)
- [`stimflow.FlowMetadata`](#stimflow.FlowMetadata)
    - [`stimflow.FlowMetadata.__init__`](#stimflow.FlowMetadata.__init__)
- [`stimflow.LayerCircuit`](#stimflow.LayerCircuit)
    - [`stimflow.LayerCircuit.copy`](#stimflow.LayerCircuit.copy)
    - [`stimflow.LayerCircuit.from_stim_circuit`](#stimflow.LayerCircuit.from_stim_circuit)
    - [`stimflow.LayerCircuit.to_stim_circuit`](#stimflow.LayerCircuit.to_stim_circuit)
    - [`stimflow.LayerCircuit.to_z_basis`](#stimflow.LayerCircuit.to_z_basis)
    - [`stimflow.LayerCircuit.touched`](#stimflow.LayerCircuit.touched)
    - [`stimflow.LayerCircuit.with_cleaned_up_loop_iterations`](#stimflow.LayerCircuit.with_cleaned_up_loop_iterations)
    - [`stimflow.LayerCircuit.with_clearable_rotation_layers_cleared`](#stimflow.LayerCircuit.with_clearable_rotation_layers_cleared)
    - [`stimflow.LayerCircuit.with_ejected_loop_iterations`](#stimflow.LayerCircuit.with_ejected_loop_iterations)
    - [`stimflow.LayerCircuit.with_irrelevant_tail_layers_removed`](#stimflow.LayerCircuit.with_irrelevant_tail_layers_removed)
    - [`stimflow.LayerCircuit.with_locally_merged_measure_layers`](#stimflow.LayerCircuit.with_locally_merged_measure_layers)
    - [`stimflow.LayerCircuit.with_locally_optimized_layers`](#stimflow.LayerCircuit.with_locally_optimized_layers)
    - [`stimflow.LayerCircuit.with_qubit_coords_at_start`](#stimflow.LayerCircuit.with_qubit_coords_at_start)
    - [`stimflow.LayerCircuit.with_rotations_before_resets_removed`](#stimflow.LayerCircuit.with_rotations_before_resets_removed)
    - [`stimflow.LayerCircuit.with_rotations_merged_earlier`](#stimflow.LayerCircuit.with_rotations_merged_earlier)
    - [`stimflow.LayerCircuit.with_rotations_rolled_from_end_of_loop_to_start_of_loop`](#stimflow.LayerCircuit.with_rotations_rolled_from_end_of_loop_to_start_of_loop)
    - [`stimflow.LayerCircuit.with_whole_layers_slid_as_early_as_possible_for_merge_with_same_layer`](#stimflow.LayerCircuit.with_whole_layers_slid_as_early_as_possible_for_merge_with_same_layer)
    - [`stimflow.LayerCircuit.with_whole_layers_slid_as_to_merge_with_previous_layer_of_same_type`](#stimflow.LayerCircuit.with_whole_layers_slid_as_to_merge_with_previous_layer_of_same_type)
    - [`stimflow.LayerCircuit.with_whole_rotation_layers_slid_earlier`](#stimflow.LayerCircuit.with_whole_rotation_layers_slid_earlier)
    - [`stimflow.LayerCircuit.without_empty_layers`](#stimflow.LayerCircuit.without_empty_layers)
- [`stimflow.LineDataFor3DModel`](#stimflow.LineDataFor3DModel)
    - [`stimflow.LineDataFor3DModel.__init__`](#stimflow.LineDataFor3DModel.__init__)
    - [`stimflow.LineDataFor3DModel.fused`](#stimflow.LineDataFor3DModel.fused)
- [`stimflow.NoiseModel`](#stimflow.NoiseModel)
    - [`stimflow.NoiseModel.noisy_circuit`](#stimflow.NoiseModel.noisy_circuit)
    - [`stimflow.NoiseModel.noisy_circuit_skipping_mpp_boundaries`](#stimflow.NoiseModel.noisy_circuit_skipping_mpp_boundaries)
    - [`stimflow.NoiseModel.si1000`](#stimflow.NoiseModel.si1000)
    - [`stimflow.NoiseModel.uniform_depolarizing`](#stimflow.NoiseModel.uniform_depolarizing)
- [`stimflow.NoiseRule`](#stimflow.NoiseRule)
    - [`stimflow.NoiseRule.__init__`](#stimflow.NoiseRule.__init__)
    - [`stimflow.NoiseRule.append_noisy_version_of`](#stimflow.NoiseRule.append_noisy_version_of)
- [`stimflow.Patch`](#stimflow.Patch)
    - [`stimflow.Patch.data_set`](#stimflow.Patch.data_set)
    - [`stimflow.Patch.m2tile`](#stimflow.Patch.m2tile)
    - [`stimflow.Patch.measure_set`](#stimflow.Patch.measure_set)
    - [`stimflow.Patch.partitioned_tiles`](#stimflow.Patch.partitioned_tiles)
    - [`stimflow.Patch.to_svg`](#stimflow.Patch.to_svg)
    - [`stimflow.Patch.used_set`](#stimflow.Patch.used_set)
    - [`stimflow.Patch.with_edits`](#stimflow.Patch.with_edits)
    - [`stimflow.Patch.with_only_x_tiles`](#stimflow.Patch.with_only_x_tiles)
    - [`stimflow.Patch.with_only_y_tiles`](#stimflow.Patch.with_only_y_tiles)
    - [`stimflow.Patch.with_only_z_tiles`](#stimflow.Patch.with_only_z_tiles)
    - [`stimflow.Patch.with_remaining_degrees_of_freedom_as_logicals`](#stimflow.Patch.with_remaining_degrees_of_freedom_as_logicals)
    - [`stimflow.Patch.with_transformed_bases`](#stimflow.Patch.with_transformed_bases)
    - [`stimflow.Patch.with_transformed_coords`](#stimflow.Patch.with_transformed_coords)
    - [`stimflow.Patch.with_xz_flipped`](#stimflow.Patch.with_xz_flipped)
- [`stimflow.PauliMap`](#stimflow.PauliMap)
    - [`stimflow.PauliMap.__init__`](#stimflow.PauliMap.__init__)
    - [`stimflow.PauliMap.anticommutes`](#stimflow.PauliMap.anticommutes)
    - [`stimflow.PauliMap.commutes`](#stimflow.PauliMap.commutes)
    - [`stimflow.PauliMap.from_xs`](#stimflow.PauliMap.from_xs)
    - [`stimflow.PauliMap.from_ys`](#stimflow.PauliMap.from_ys)
    - [`stimflow.PauliMap.from_zs`](#stimflow.PauliMap.from_zs)
    - [`stimflow.PauliMap.get`](#stimflow.PauliMap.get)
    - [`stimflow.PauliMap.items`](#stimflow.PauliMap.items)
    - [`stimflow.PauliMap.keys`](#stimflow.PauliMap.keys)
    - [`stimflow.PauliMap.to_stim_pauli_string`](#stimflow.PauliMap.to_stim_pauli_string)
    - [`stimflow.PauliMap.to_stim_targets`](#stimflow.PauliMap.to_stim_targets)
    - [`stimflow.PauliMap.to_tile`](#stimflow.PauliMap.to_tile)
    - [`stimflow.PauliMap.values`](#stimflow.PauliMap.values)
    - [`stimflow.PauliMap.with_basis`](#stimflow.PauliMap.with_basis)
    - [`stimflow.PauliMap.with_obs_name`](#stimflow.PauliMap.with_obs_name)
    - [`stimflow.PauliMap.with_transformed_coords`](#stimflow.PauliMap.with_transformed_coords)
    - [`stimflow.PauliMap.with_xy_flipped`](#stimflow.PauliMap.with_xy_flipped)
    - [`stimflow.PauliMap.with_xz_flipped`](#stimflow.PauliMap.with_xz_flipped)
- [`stimflow.StabilizerCode`](#stimflow.StabilizerCode)
    - [`stimflow.StabilizerCode.__init__`](#stimflow.StabilizerCode.__init__)
    - [`stimflow.StabilizerCode.as_interface`](#stimflow.StabilizerCode.as_interface)
    - [`stimflow.StabilizerCode.concat_over`](#stimflow.StabilizerCode.concat_over)
    - [`stimflow.StabilizerCode.data_set`](#stimflow.StabilizerCode.data_set)
    - [`stimflow.StabilizerCode.find_distance`](#stimflow.StabilizerCode.find_distance)
    - [`stimflow.StabilizerCode.find_logical_error`](#stimflow.StabilizerCode.find_logical_error)
    - [`stimflow.StabilizerCode.flat_logicals`](#stimflow.StabilizerCode.flat_logicals)
    - [`stimflow.StabilizerCode.from_patch_with_inferred_observables`](#stimflow.StabilizerCode.from_patch_with_inferred_observables)
    - [`stimflow.StabilizerCode.get_observable_by_basis`](#stimflow.StabilizerCode.get_observable_by_basis)
    - [`stimflow.StabilizerCode.list_pure_basis_observables`](#stimflow.StabilizerCode.list_pure_basis_observables)
    - [`stimflow.StabilizerCode.make_code_capacity_circuit`](#stimflow.StabilizerCode.make_code_capacity_circuit)
    - [`stimflow.StabilizerCode.make_phenom_circuit`](#stimflow.StabilizerCode.make_phenom_circuit)
    - [`stimflow.StabilizerCode.measure_set`](#stimflow.StabilizerCode.measure_set)
    - [`stimflow.StabilizerCode.patch`](#stimflow.StabilizerCode.patch)
    - [`stimflow.StabilizerCode.physical_to_logical`](#stimflow.StabilizerCode.physical_to_logical)
    - [`stimflow.StabilizerCode.tiles`](#stimflow.StabilizerCode.tiles)
    - [`stimflow.StabilizerCode.to_svg`](#stimflow.StabilizerCode.to_svg)
    - [`stimflow.StabilizerCode.transversal_init_chunk`](#stimflow.StabilizerCode.transversal_init_chunk)
    - [`stimflow.StabilizerCode.transversal_measure_chunk`](#stimflow.StabilizerCode.transversal_measure_chunk)
    - [`stimflow.StabilizerCode.used_set`](#stimflow.StabilizerCode.used_set)
    - [`stimflow.StabilizerCode.verify`](#stimflow.StabilizerCode.verify)
    - [`stimflow.StabilizerCode.verify_distance_is_at_least`](#stimflow.StabilizerCode.verify_distance_is_at_least)
    - [`stimflow.StabilizerCode.with_edits`](#stimflow.StabilizerCode.with_edits)
    - [`stimflow.StabilizerCode.with_integer_coordinates`](#stimflow.StabilizerCode.with_integer_coordinates)
    - [`stimflow.StabilizerCode.with_observables_from_basis`](#stimflow.StabilizerCode.with_observables_from_basis)
    - [`stimflow.StabilizerCode.with_remaining_degrees_of_freedom_as_logicals`](#stimflow.StabilizerCode.with_remaining_degrees_of_freedom_as_logicals)
    - [`stimflow.StabilizerCode.with_transformed_coords`](#stimflow.StabilizerCode.with_transformed_coords)
    - [`stimflow.StabilizerCode.with_xz_flipped`](#stimflow.StabilizerCode.with_xz_flipped)
    - [`stimflow.StabilizerCode.x_basis_subset`](#stimflow.StabilizerCode.x_basis_subset)
    - [`stimflow.StabilizerCode.z_basis_subset`](#stimflow.StabilizerCode.z_basis_subset)
- [`stimflow.StimCircuitLoom`](#stimflow.StimCircuitLoom)
    - [`stimflow.StimCircuitLoom.weave`](#stimflow.StimCircuitLoom.weave)
    - [`stimflow.StimCircuitLoom.weaved_target_rec_from_c0`](#stimflow.StimCircuitLoom.weaved_target_rec_from_c0)
    - [`stimflow.StimCircuitLoom.weaved_target_rec_from_c1`](#stimflow.StimCircuitLoom.weaved_target_rec_from_c1)
- [`stimflow.TextDataFor3DModel`](#stimflow.TextDataFor3DModel)
    - [`stimflow.TextDataFor3DModel.__init__`](#stimflow.TextDataFor3DModel.__init__)
- [`stimflow.Tile`](#stimflow.Tile)
    - [`stimflow.Tile.__init__`](#stimflow.Tile.__init__)
    - [`stimflow.Tile.basis`](#stimflow.Tile.basis)
    - [`stimflow.Tile.center`](#stimflow.Tile.center)
    - [`stimflow.Tile.data_set`](#stimflow.Tile.data_set)
    - [`stimflow.Tile.to_pauli_map`](#stimflow.Tile.to_pauli_map)
    - [`stimflow.Tile.used_set`](#stimflow.Tile.used_set)
    - [`stimflow.Tile.with_bases`](#stimflow.Tile.with_bases)
    - [`stimflow.Tile.with_basis`](#stimflow.Tile.with_basis)
    - [`stimflow.Tile.with_data_qubit_cleared`](#stimflow.Tile.with_data_qubit_cleared)
    - [`stimflow.Tile.with_edits`](#stimflow.Tile.with_edits)
    - [`stimflow.Tile.with_transformed_bases`](#stimflow.Tile.with_transformed_bases)
    - [`stimflow.Tile.with_transformed_coords`](#stimflow.Tile.with_transformed_coords)
    - [`stimflow.Tile.with_xz_flipped`](#stimflow.Tile.with_xz_flipped)
- [`stimflow.TriangleDataFor3DModel`](#stimflow.TriangleDataFor3DModel)
    - [`stimflow.TriangleDataFor3DModel.__init__`](#stimflow.TriangleDataFor3DModel.__init__)
    - [`stimflow.TriangleDataFor3DModel.fused`](#stimflow.TriangleDataFor3DModel.fused)
    - [`stimflow.TriangleDataFor3DModel.rect`](#stimflow.TriangleDataFor3DModel.rect)
- [`stimflow.Viewable3dModelGLTF`](#stimflow.Viewable3dModelGLTF)
    - [`stimflow.Viewable3dModelGLTF.html_viewer`](#stimflow.Viewable3dModelGLTF.html_viewer)
- [`stimflow.append_reindexed_content_to_circuit`](#stimflow.append_reindexed_content_to_circuit)
- [`stimflow.circuit_to_dem_target_measurement_records_map`](#stimflow.circuit_to_dem_target_measurement_records_map)
- [`stimflow.circuit_with_xz_flipped`](#stimflow.circuit_with_xz_flipped)
- [`stimflow.count_measurement_layers`](#stimflow.count_measurement_layers)
- [`stimflow.find_d1_error`](#stimflow.find_d1_error)
- [`stimflow.find_d2_error`](#stimflow.find_d2_error)
- [`stimflow.gate_counts_for_circuit`](#stimflow.gate_counts_for_circuit)
- [`stimflow.gates_used_by_circuit`](#stimflow.gates_used_by_circuit)
- [`stimflow.html_viewer`](#stimflow.html_viewer)
- [`stimflow.html_viewer_for_gltf_model`](#stimflow.html_viewer_for_gltf_model)
- [`stimflow.make_3d_model`](#stimflow.make_3d_model)
- [`stimflow.min_max_complex`](#stimflow.min_max_complex)
- [`stimflow.sorted_complex`](#stimflow.sorted_complex)
- [`stimflow.stim_circuit_with_transformed_coords`](#stimflow.stim_circuit_with_transformed_coords)
- [`stimflow.stim_circuit_with_transformed_moments`](#stimflow.stim_circuit_with_transformed_moments)
- [`stimflow.str_html`](#stimflow.str_html)
    - [`stimflow.str_html.write_to`](#stimflow.str_html.write_to)
- [`stimflow.str_svg`](#stimflow.str_svg)
    - [`stimflow.str_svg.write_to`](#stimflow.str_svg.write_to)
- [`stimflow.svg_viewer`](#stimflow.svg_viewer)
- [`stimflow.transpile_to_z_basis_interaction_circuit`](#stimflow.transpile_to_z_basis_interaction_circuit)
- [`stimflow.transversal_code_transition_chunks`](#stimflow.transversal_code_transition_chunks)
- [`stimflow.verify_distance_is_at_least`](#stimflow.verify_distance_is_at_least)
- [`stimflow.xor_sorted`](#stimflow.xor_sorted)
```python
# Types used by the method definitions.
from __future__ import annotations
from typing import overload, TYPE_CHECKING, Any, Iterable
import io
import pathlib
import numpy as np
```

<a name="stimflow.Chunk"></a>
```python
# stimflow.Chunk

# (at top-level in the stimflow module)
class Chunk:
    """A circuit with accompanying stabilizer flow assertions.

    This object is intended to be immutable.
    Some of its fields are editable types, but it is assumed they do not change
    (e.g. computations may be cached).
    Don't do things like appending to the circuit of a chunk after the chunk is created.

    Example:
        >>> import stimflow as sf
        >>> import stim
        >>> chunk = sf.Chunk(
        ...     circuit=stim.Circuit('''
        ...         QUBIT_COORDS(1, 2) 0
        ...         H 0
        ...     '''),
        ...     flows=[
        ...         sf.Flow(start=sf.PauliMap({1+2j: "X"}), end=sf.PauliMap({1+2j: "Z"})),
        ...     ],
        ... )
        >>> chunk.verify()
    """
```

<a name="stimflow.Chunk.__init__"></a>
```python
# stimflow.Chunk.__init__

# (in class stimflow.Chunk)
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
    """Creates a `stimflow.Chunk` with the given values.

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
            and the next chunk. For example, this is useful when creating a
            transversal initialization chunk.
        wants_to_merge_with_prev: Defaults to False. When set to True,
            the chunk compiler won't insert a TICK between this chunk
            and the previous chunk. For example, this is useful when creating a
            transversal measurement chunk.
        q2i: Defaults to None (infer from QUBIT_COORDS instructions in circuit else
            raise an exception). The stimflow-qubit-coordinate-to-stim-qubit-index mapping
            used to translate between stimflow's qubit keys and stim's qubit keys.
        o2i: Defaults to None (raise an exception if observables present in circuit).
            The stimflow-observable-key-to-stim-observable-index mapping used to translate
            between stimflow's observable keys and stim's observable keys.

    Example:
        >>> import stimflow as sf
        >>> import stim
        >>> chunk = sf.Chunk(
        ...     circuit=stim.Circuit('''
        ...         QUBIT_COORDS(1, 2) 0
        ...         H 0
        ...     '''),
        ...     flows=[
        ...         sf.Flow(start=sf.PauliMap({1+2j: "X"}), end=sf.PauliMap({1+2j: "Z"})),
        ...     ],
        ... )
        >>> chunk.verify()
    """
```

<a name="stimflow.Chunk.end_code"></a>
```python
# stimflow.Chunk.end_code

# (in class stimflow.Chunk)
def end_code(
    self,
) -> StabilizerCode:
```

<a name="stimflow.Chunk.end_interface"></a>
```python
# stimflow.Chunk.end_interface

# (in class stimflow.Chunk)
def end_interface(
    self,
    *,
    skip_passthroughs: bool = False,
) -> ChunkInterface:
    """Returns a description of the flows that should exit from the chunk.
    """
```

<a name="stimflow.Chunk.end_patch"></a>
```python
# stimflow.Chunk.end_patch

# (in class stimflow.Chunk)
def end_patch(
    self,
) -> Patch:
```

<a name="stimflow.Chunk.find_distance"></a>
```python
# stimflow.Chunk.find_distance

# (in class stimflow.Chunk)
def find_distance(
    self,
    *,
    max_search_weight: int,
    noise: float | NoiseModel = 0.001,
    noiseless_qubits: Iterable[float | int | complex] = (),
    skip_adding_noise: bool = False,
) -> int:
```

<a name="stimflow.Chunk.find_logical_error"></a>
```python
# stimflow.Chunk.find_logical_error

# (in class stimflow.Chunk)
def find_logical_error(
    self,
    *,
    max_search_weight: int,
    noise: float | NoiseModel = 0.001,
    noiseless_qubits: Iterable[float | int | complex] = (),
    skip_adding_noise: bool = False,
) -> list[stim.ExplainedError]:
```

<a name="stimflow.Chunk.flattened"></a>
```python
# stimflow.Chunk.flattened

# (in class stimflow.Chunk)
def flattened(
    self,
) -> list[Chunk]:
    """This is here for duck-type compatibility with ChunkLoop.
    """
```

<a name="stimflow.Chunk.from_circuit_with_mpp_boundaries"></a>
```python
# stimflow.Chunk.from_circuit_with_mpp_boundaries

# (in class stimflow.Chunk)
def from_circuit_with_mpp_boundaries(
    circuit: stim.Circuit,
) -> Chunk:
```

<a name="stimflow.Chunk.start_code"></a>
```python
# stimflow.Chunk.start_code

# (in class stimflow.Chunk)
def start_code(
    self,
) -> StabilizerCode:
```

<a name="stimflow.Chunk.start_interface"></a>
```python
# stimflow.Chunk.start_interface

# (in class stimflow.Chunk)
def start_interface(
    self,
    *,
    skip_passthroughs: bool = False,
) -> ChunkInterface:
    """Returns a description of the flows that should enter into the chunk.
    """
```

<a name="stimflow.Chunk.start_patch"></a>
```python
# stimflow.Chunk.start_patch

# (in class stimflow.Chunk)
def start_patch(
    self,
) -> Patch:
```

<a name="stimflow.Chunk.then"></a>
```python
# stimflow.Chunk.then

# (in class stimflow.Chunk)
def then(
    self,
    other: Chunk | ChunkReflow | ChunkLoop,
) -> Chunk:
```

<a name="stimflow.Chunk.time_reversed"></a>
```python
# stimflow.Chunk.time_reversed

# (in class stimflow.Chunk)
def time_reversed(
    self,
) -> Chunk:
    """Checks that this chunk's circuit actually implements its flows.
    """
```

<a name="stimflow.Chunk.to_closed_circuit"></a>
```python
# stimflow.Chunk.to_closed_circuit

# (in class stimflow.Chunk)
def to_closed_circuit(
    self,
) -> stim.Circuit:
    """Compiles the chunk into a circuit by conjugating with mpp init/end chunks.
    """
```

<a name="stimflow.Chunk.to_coord_circuit"></a>
```python
# stimflow.Chunk.to_coord_circuit

# (in class stimflow.Chunk)
def to_coord_circuit(
    self,
) -> stim.Circuit:
```

<a name="stimflow.Chunk.to_html_viewer"></a>
```python
# stimflow.Chunk.to_html_viewer

# (in class stimflow.Chunk)
def to_html_viewer(
    self,
    *,
    background: Patch | StabilizerCode | ChunkInterface | dict[int, Patch | StabilizerCode | ChunkInterface] | None = None,
    tile_color_func: Callable[[Tile], tuple[float, float, float, float]] | None = None,
    known_error: Iterable[stim.ExplainedError] | None = None,
) -> str_html:
```

<a name="stimflow.Chunk.verify"></a>
```python
# stimflow.Chunk.verify

# (in class stimflow.Chunk)
def verify(
    self,
    *,
    expected_in: ChunkInterface | StabilizerCode | Patch | None = None,
    expected_out: ChunkInterface | StabilizerCode | Patch | None = None,
    should_measure_all_code_stabilizers: bool = False,
    allow_overlapping_flows: bool = False,
):
    """Checks that this chunk's circuit actually implements its flows.
    """
```

<a name="stimflow.Chunk.verify_distance_is_at_least"></a>
```python
# stimflow.Chunk.verify_distance_is_at_least

# (in class stimflow.Chunk)
def verify_distance_is_at_least(
    self,
    minimum_distance: int,
    *,
    noise: float | NoiseModel = 0.001,
):
    """Verifies undetected logical errors require at least the given number of physical errors.

    Args:
        minimum_distance: The minimum distance to verify. Currently this must be at most 3.
        noise: The noise model to use. Defaults to a uniform depolarizing circuit noise model
            that allows multiple operations per tick and where two qubit gates apply two qubit
            depolarizing noise.

    Example:
        >>> import stimflow as sf
        >>> import stim
        >>> lz = sf.PauliMap({0: "Z"}).with_obs_name("LZ")
        >>> zz01 = sf.PauliMap.from_zs([0, 1])
        >>> zz12 = sf.PauliMap.from_zs([1, 2])
        >>> zz23 = sf.PauliMap.from_zs([2, 3])
        >>> zz34 = sf.PauliMap.from_zs([3, 4])
        >>> chunk = sf.Chunk(
        ...     stim.Circuit('''
        ...         QUBIT_COORDS(0, 0) 0
        ...         QUBIT_COORDS(1, 0) 1
        ...         QUBIT_COORDS(2, 0) 2
        ...         QUBIT_COORDS(3, 0) 3
        ...         QUBIT_COORDS(4, 0) 4
        ...         MZZ 0 1 1 2 2 3 3 4
        ...     '''),
        ...     flows=[
        ...         sf.Flow(start=lz, end=lz),
        ...         sf.Flow(start=zz01, measurement_indices=[0]),
        ...         sf.Flow(start=zz12, measurement_indices=[1]),
        ...         sf.Flow(start=zz23, measurement_indices=[2]),
        ...         sf.Flow(start=zz34, measurement_indices=[3]),
        ...         sf.Flow(end=zz01, measurement_indices=[0]),
        ...         sf.Flow(end=zz12, measurement_indices=[1]),
        ...         sf.Flow(end=zz23, measurement_indices=[2]),
        ...         sf.Flow(end=zz34, measurement_indices=[3]),
        ...     ],
        ... )
        >>> chunk.verify_distance_is_at_least(3)
    """
```

<a name="stimflow.Chunk.with_edits"></a>
```python
# stimflow.Chunk.with_edits

# (in class stimflow.Chunk)
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
```

<a name="stimflow.Chunk.with_flag_added_to_all_flows"></a>
```python
# stimflow.Chunk.with_flag_added_to_all_flows

# (in class stimflow.Chunk)
def with_flag_added_to_all_flows(
    self,
    flag: str,
) -> Chunk:
```

<a name="stimflow.Chunk.with_obs_flows_as_det_flows"></a>
```python
# stimflow.Chunk.with_obs_flows_as_det_flows

# (in class stimflow.Chunk)
def with_obs_flows_as_det_flows(
    self,
) -> Chunk:
```

<a name="stimflow.Chunk.with_repetitions"></a>
```python
# stimflow.Chunk.with_repetitions

# (in class stimflow.Chunk)
def with_repetitions(
    self,
    repetitions: int,
) -> ChunkLoop:
```

<a name="stimflow.Chunk.with_transformed_coords"></a>
```python
# stimflow.Chunk.with_transformed_coords

# (in class stimflow.Chunk)
def with_transformed_coords(
    self,
    transform: Callable[[complex], complex],
) -> Chunk:
```

<a name="stimflow.Chunk.with_xz_flipped"></a>
```python
# stimflow.Chunk.with_xz_flipped

# (in class stimflow.Chunk)
def with_xz_flipped(
    self,
) -> Chunk:
```

<a name="stimflow.ChunkBuilder"></a>
```python
# stimflow.ChunkBuilder

# (at top-level in the stimflow module)
class ChunkBuilder:
    """A helper class for building chunks.

    This class takes care of details like converting qubit coordinates into qubit indices,
    storing and retrieving measurement indices, and accumulating flow data.

    Example:
        >>> import stimflow as sf

        >>> # Build a repetition code idling chunk.
        >>> d = 5
        >>> data_qubits = range(d)
        >>> measure_qubits = [q + 0.5 for q in data_qubits[::-1]]
        >>> builder = sf.ChunkBuilder()
        >>> builder.append("R", measure_qubits)
        >>> builder.append("TICK")
        >>> builder.append("CX", [(m-0.5, m) for m in measure_qubits])
        >>> builder.append("TICK")
        >>> builder.append("CX", [(m+0.5, m) for m in measure_qubits])
        >>> builder.append("TICK")
        >>> builder.append("M", measure_qubits)
        >>> for m in measure_qubits:
        ...     stabilizer = sf.PauliMap.from_zs([m-0.5, m+0.5])
        ...     builder.add_flow(start=stabilizer, measurements=[m])
        ...     builder.add_flow(end=stabilizer, measurements=[m])
        >>> obs = sf.PauliMap({data_qubits[0]: "Z"}).with_obs_name("LZ")
        >>> builder.add_flow(start=obs, end=obs)
        >>> chunk = builder.finish_chunk()

        >>> chunk.verify()
        >>> print(chunk.to_closed_circuit())
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(0.5, 0) 1
        QUBIT_COORDS(1, 0) 2
        QUBIT_COORDS(1.5, 0) 3
        QUBIT_COORDS(2, 0) 4
        QUBIT_COORDS(2.5, 0) 5
        QUBIT_COORDS(3, 0) 6
        QUBIT_COORDS(3.5, 0) 7
        QUBIT_COORDS(4, 0) 8
        QUBIT_COORDS(4.5, 0) 9
        QUBIT_COORDS(5, 0) 10
        OBSERVABLE_INCLUDE(0) Z0
        TICK
        MPP Z0*Z2 Z4*Z6 Z8*Z10
        TICK
        MPP Z2*Z4 Z6*Z8
        TICK
        R 9 7 5 3 1
        TICK
        CX 8 9 6 7 4 5 2 3 0 1
        TICK
        CX 8 7 6 5 4 3 2 1 10 9
        TICK
        M 9 7 5 3 1
        DETECTOR(4.5, 0, 0) rec[-8] rec[-5]
        DETECTOR(3.5, 0, 0) rec[-6] rec[-4]
        DETECTOR(2.5, 0, 0) rec[-9] rec[-3]
        DETECTOR(1.5, 0, 0) rec[-7] rec[-2]
        DETECTOR(0.5, 0, 0) rec[-10] rec[-1]
        SHIFT_COORDS(0, 0, 1)
        TICK
        MPP Z0*Z2 Z4*Z6 Z8*Z10
        TICK
        MPP Z2*Z4 Z6*Z8
        DETECTOR(0.5, 0, 0) rec[-6] rec[-5]
        DETECTOR(2.5, 0, 0) rec[-8] rec[-4]
        DETECTOR(4.5, 0, 0) rec[-10] rec[-3]
        DETECTOR(1.5, 0, 0) rec[-7] rec[-2]
        DETECTOR(3.5, 0, 0) rec[-9] rec[-1]
        TICK
        OBSERVABLE_INCLUDE(0) Z0
    """
```

<a name="stimflow.ChunkBuilder.__init__"></a>
```python
# stimflow.ChunkBuilder.__init__

# (in class stimflow.ChunkBuilder)
def __init__(
    self,
    allowed_qubits: Iterable[complex] | None = None,
):
    """Creates a Builder for creating a circuit over the given qubits.

    Args:
        allowed_qubits: Defaults to None (everything allowed). Specifies the qubit positions
            that the circuit is permitted to contain.

    Examples:
        >>> import stimflow as sf
        >>> data_qubits = range(5)
        >>> measure_qubits = [q + 0.5 for q in data_qubits[::-1]]
        >>> builder = sf.ChunkBuilder(allowed_qubits=[*data_qubits, *measure_qubits])
    """
```

<a name="stimflow.ChunkBuilder.add_discarded_flow_input"></a>
```python
# stimflow.ChunkBuilder.add_discarded_flow_input

# (in class stimflow.ChunkBuilder)
def add_discarded_flow_input(
    self,
    flow: PauliMap | Tile,
) -> None:
    """Annotates that an input stabilizer won't be used.

    When compiling chunks, it is normally an error if the output flows of one
    chunk don't match up with the input flows of the next. For example, a
    Z basis transversal measurement can't measure the X stabilizers of a code,
    so a chunk performing can't declare flows with X basis inputs. But this
    would cause an error during compilation, due the prior idling chunk having
    X basis output flows. Adding the X basis stabilizers as discarded flow inputs
    of the transversal chunk explicitly indicates that it is expected for this
    mismatch to occur, so that no error is raised.

    Example:
        >>> import stimflow as sf
        >>> xx = sf.PauliMap.from_xs([0, 1])
        >>> zz = sf.PauliMap.from_zs([0, 1])

        >>> init_builder = sf.ChunkBuilder()
        >>> init_builder.append("R", [0, 1])
        >>> init_builder.add_flow(end=zz)
        >>> init_builder.add_discarded_flow_output(sf.PauliMap.from_xs([0, 1]))
        >>> init_chunk = init_builder.finish_chunk()
        >>> init_chunk.verify()

        >>> idle_builder = sf.ChunkBuilder()
        >>> idle_builder.append("MXX", [(0, 1)], measure_key_func=lambda e: ('X', e))
        >>> idle_builder.append("TICK")
        >>> idle_builder.append("MZZ", [(0, 1)], measure_key_func=lambda e: ('Z', e))
        >>> idle_builder.add_flow(start=xx, measurements=[('X', (0, 1))])
        >>> idle_builder.add_flow(start=zz, measurements=[('Z', (0, 1))])
        >>> idle_builder.add_flow(end=xx, measurements=[('X', (0, 1))])
        >>> idle_builder.add_flow(end=zz, measurements=[('Z', (0, 1))])
        >>> idle_chunk = idle_builder.finish_chunk()
        >>> idle_chunk.verify()

        >>> end_builder = sf.ChunkBuilder()
        >>> end_builder.append("M", [0, 1])
        >>> end_builder.add_flow(start=zz, measurements=[0, 1])
        >>> end_builder.add_discarded_flow_input(sf.PauliMap.from_xs([0, 1]))
        >>> end_chunk = end_builder.finish_chunk()
        >>> end_chunk.verify()

        >>> compiler = sf.ChunkCompiler()
        >>> compiler.append(init_chunk)
        >>> compiler.append(idle_chunk)
        >>> compiler.append(end_chunk)
        >>> print(compiler.finish_circuit())
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(1, 0) 1
        R 0 1
        TICK
        MXX 0 1
        TICK
        MZZ 0 1
        DETECTOR(0.5, 0, 0) rec[-1]
        SHIFT_COORDS(0, 0, 1)
        TICK
        M 0 1
        DETECTOR(0.5, 0, 0) rec[-3] rec[-2] rec[-1]
    """
```

<a name="stimflow.ChunkBuilder.add_discarded_flow_output"></a>
```python
# stimflow.ChunkBuilder.add_discarded_flow_output

# (in class stimflow.ChunkBuilder)
def add_discarded_flow_output(
    self,
    flow: PauliMap | Tile,
) -> None:
    """Annotates that an output stabilizer won't be used.

    When compiling chunks, it is normally an error if the output flows of one
    chunk don't match up with the input flows of the next. For example, a
    Z basis transversal preparation can't prepare the X stabilizers of a code,
    so a chunk performing can't declare flows with X basis inputs. But this
    would cause an error during compilation, due the next idling chunk having
    X basis input flows. Adding the X basis stabilizers as discarded flow outputs
    of the transversal chunk explicitly indicates that it is expected for this
    mismatch to occur, so that no error is raised.

    Example:
        >>> import stimflow as sf
        >>> xx = sf.PauliMap.from_xs([0, 1])
        >>> zz = sf.PauliMap.from_zs([0, 1])

        >>> init_builder = sf.ChunkBuilder()
        >>> init_builder.append("R", [0, 1])
        >>> init_builder.add_flow(end=zz)
        >>> init_builder.add_discarded_flow_output(sf.PauliMap.from_xs([0, 1]))
        >>> init_chunk = init_builder.finish_chunk()
        >>> init_chunk.verify()

        >>> idle_builder = sf.ChunkBuilder()
        >>> idle_builder.append("MXX", [(0, 1)], measure_key_func=lambda e: ('X', e))
        >>> idle_builder.append("TICK")
        >>> idle_builder.append("MZZ", [(0, 1)], measure_key_func=lambda e: ('Z', e))
        >>> idle_builder.add_flow(start=xx, measurements=[('X', (0, 1))])
        >>> idle_builder.add_flow(start=zz, measurements=[('Z', (0, 1))])
        >>> idle_builder.add_flow(end=xx, measurements=[('X', (0, 1))])
        >>> idle_builder.add_flow(end=zz, measurements=[('Z', (0, 1))])
        >>> idle_chunk = idle_builder.finish_chunk()
        >>> idle_chunk.verify()

        >>> end_builder = sf.ChunkBuilder()
        >>> end_builder.append("M", [0, 1])
        >>> end_builder.add_flow(start=zz, measurements=[0, 1])
        >>> end_builder.add_discarded_flow_input(sf.PauliMap.from_xs([0, 1]))
        >>> end_chunk = end_builder.finish_chunk()
        >>> end_chunk.verify()

        >>> compiler = sf.ChunkCompiler()
        >>> compiler.append(init_chunk)
        >>> compiler.append(idle_chunk)
        >>> compiler.append(end_chunk)
        >>> print(compiler.finish_circuit())
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(1, 0) 1
        R 0 1
        TICK
        MXX 0 1
        TICK
        MZZ 0 1
        DETECTOR(0.5, 0, 0) rec[-1]
        SHIFT_COORDS(0, 0, 1)
        TICK
        M 0 1
        DETECTOR(0.5, 0, 0) rec[-3] rec[-2] rec[-1]
    """
```

<a name="stimflow.ChunkBuilder.add_flow"></a>
```python
# stimflow.ChunkBuilder.add_flow

# (in class stimflow.ChunkBuilder)
def add_flow(
    self,
    *,
    start: "PauliMap | Tile | Literal['auto'] | None" = None,
    end: "PauliMap | Tile | Literal['auto'] | None" = None,
    measurements: "Iterable[Any] | Literal['auto']" = (),
    ignore_unknown_measurements: bool = False,
    center: "complex | None | Literal['infer']" = 'infer,
    flags: Iterable[str] = frozenset(),
    sign: bool | None = None,
) -> None:
    """Declares that the circuit being built should have a given stabilizer flow.

    When chunks are concatenated, their flows are matched up in order to form detectors.
    At most one of `start`, `end`, and `measurements` can be set to "auto" in order to
    infer it.

    Args:
        start: Defaults to None (empty). The stabilizer that the flow starts as, at the
            beginning of the circuit. If the flow begins within the circuit, this should
            be set to None or an empty PauliMap. If this is set to "auto", it will be
            inferred  by backpropagation from `end` and `measurements`.
        end: Defaults to None (empty). The stabilizer that the flow ends as, at the
            end of the circuit. If the flow ends within the circuit, this should
            be set to None or an empty PauliMap. If this is set to "auto", it will be
            inferred by forward propagating from `start` and measurements` (no resets will
            be included in the forward propagation).
        measurements: Defaults to empty. The keys identifying measurements mediate the flow.
            For example, if a stabilizer is measured by a circuit then this would
            typically be a singleton list containing the measurement that reveals
            the stabilizer's value. If this is set to "auto", it will be inferred from
            `start` and `end` by Gaussian elimination via `stim.Circuit.flow_generators`.

            Caution: beware using "auto" when the solution isn't unique (e.g. this is
            common if the circuit includes multiple rounds of stabilizer measurement), as
            it may select a solution you don't expect.
        ignore_unknown_measurements: Defaults to False. When set to False, unrecognized measurement
            ids cause the method to raise an exception instead of adding the flow. When set
            to True, unrecognized measurements are silently discarded.
        center: Defaults to None (unused). Optional metadata specifying coordinates for the
            flow. Typically, these coordinates will end up being exposed as the parens args
            on the DETECTOR instruction created when producing a stim circuit. When not
            specified, the coordinates will instead be inferred in some heuristic way.
        flags: Defaults to empty. Hashable equatable values associated with the flow. When
            flows are combined, the result will contain the union of their flags. When compiling
            chunks into a circuit, the optional `metadata_func` argument can use these flags
            to produce better metadata.
        sign: Defaults to None (unsigned). When not set, the circuit having the flow with either
            a positive or negative sign are both acceptable. When set to False or True, the sign
            implemented by the circuit must match.

    Examples:
        >>> import stimflow as sf
        >>> builder = sf.ChunkBuilder()

        >>> # 0 ───────@─────────── 0
        >>> #          │
        >>> # 1 ───R───X───X───M─── 1
        >>> #              │
        >>> # 2 ───────────@─────── 2
        >>> builder.append('R', [1])
        >>> builder.append('TICK')
        >>> builder.append('CX', [(0, 1)])
        >>> builder.append('TICK')
        >>> builder.append('CX', [(2, 1)])
        >>> builder.append('TICK')
        >>> builder.append('M', [1])

        >>> # 0 z━━━━━━━@─────────── 0
        >>> #           ┃
        >>> # 1  ───R━━━X━━━X━━[M]── 1
        >>> #               ┃
        >>> # 2 z━━━━━━━━━━━@─────── 2
        >>> builder.add_flow(
        ...     start=sf.PauliMap({0: 'Z', 2: 'Z'}),
        ...     measurements=[1],
        ... )

        >>> # 0 ───────@━━━━━━━━━━━z 0
        >>> #          ┃
        >>> # 1 ───R━━━X━━━X━━[M]──  1
        >>> #              ┃
        >>> # 2 ───────────@━━━━━━━z 2
        >>> builder.add_flow(
        ...     measurements=[1],
        ...     end=sf.PauliMap({0: 'Z', 2: 'Z'}),
        ... )

        >>> # 0 x═══════@═══════════x 0
        >>> #           ║
        >>> # 1  ───R───X═══X───M───  1
        >>> #               ║
        >>> # 2 x═══════════@═══════x 2
        >>> builder.add_flow(
        ...     start=sf.PauliMap({0: 'X', 2: 'X'}),
        ...     end=sf.PauliMap({0: 'X', 2: 'X'}),
        ... )

        >>> # 0 z━━━━━━━@───────────  0
        >>> #           ┃
        >>> # 1  ───R━━━X━━━X━━[M]──  1
        >>> #               ┃
        >>> # 2  ───────────@━━━━━━━z 2
        >>> builder.add_flow(
        ...     start=sf.PauliMap({0: 'Z'}),
        ...     measurements=[1],
        ...     end=sf.PauliMap({2: 'Z'}),
        ... )

        >>> builder.finish_chunk().verify()
    """
```

<a name="stimflow.ChunkBuilder.append"></a>
```python
# stimflow.ChunkBuilder.append

# (in class stimflow.ChunkBuilder)
def append(
    self,
    gate: str,
    targets: Iterable[complex | Sequence[complex] | PauliMap | Tile | Any] = (),
    *,
    arg: float | Iterable[float] | None = None,
    measure_key_func: Callable[[complex], Any] | Callable[[tuple[complex, complex]], Any] | Callable[[PauliMap | Tile], Any] | None = lambda e: e,
    tag: str = ',
    unknown_qubit_append_mode: "Literal['auto, 'error, 'skip, 'include']" = 'auto,
) -> None:
    """Appends an instruction to the builder's circuit.

    This method differs from `stim.Circuit.append` in the following ways:

    1) It targets qubits by position instead of by index. Also, it takes two
    qubit targets as pairs instead of interleaved. For example, instead of
    saying

        a = builder.q2i[5 + 1j]
        b = builder.q2i[5]
        c = builder.q2i[0]
        d = builder.q2i[1j]
        builder.circuit.append('CZ', [a, b, c, d])

    you would say

        builder.append('CZ', [(5+1j, 5), (0, 1j)])

    2) It canonicalizes. In particular, it will:
        - Sort targets. For example:
            `H 3 1 2` -> `H 1 2 3`
            `CX 2 3 1 0` -> `CX 1 0 2 3`
            `CZ 2 3 6 0` -> `CZ 0 6 2 3`
        - Replace rare gates with common gates. For example:
            `XCZ 1 2` -> `CX 2 1`
        - Not append target-less gates at all. For example:
            `CX      ` -> ``

        Canonicalization makes the form of the final circuit stable,
        despite things like python's `set` data structure having
        inconsistent iteration orders. This makes the output easier
        to unit test, and more viable to store under source control.

    3) It tracks measurements. When appending a measurement, its index is
    stored in the measurement tracker keyed by the position of the qubit
    being measured (or by a custom key, if `measure_key_func` is specified).
    The indices of the measurements can be looked up later via
    `builder.lookup_measurement_indices([key1, key2, ...])`.

    Args:
        gate: The name of the gate to append, such as "H" or "M" or "CX".
        targets: The qubit positions that the gate operates on. For single
            qubit gates like H or M this should be an iterable of complex
            numbers. For two qubit gates like CX or MXX it should be an
            iterable of pairs of complex numbers. For MPP it should be an
            iterable of stimflow.PauliMap instances.
        arg: Optional. The parens argument or arguments used for the gate
            instruction. For example, for a measurement gate, this is the
            probability of the incorrect result being reported.
        measure_key_func: Customizes the keys used to track the indices of
            measurement results. By default, measurements are keyed by
            position, but thus won't work if a circuit measures the same
            qubit multiple times. This function can transform that position
            into a different value (for example, you might set
            `measure_key_func=lambda pos: (pos, 'first_cycle')` for
            measurements during the first cycle of the circuit).
        tag: Defaults to "" (no tag). A custom tag to attach to the
            instruction(s) appended into the stim circuit.
        unknown_qubit_append_mode: Defaults to 'auto'. The available options are:
            - 'auto':  Replace by 'include' if the builder's `allowed_qubits` field is
                empty, else replace by 'error'.
            - 'error': When a qubit position outside `allowed_qubits` is encountered,
                raise an exception.
            - 'include': When a qubit position outside `allowed_qubits` is encountered,
                automatically include it into `builder.q2i` and `builder.allowed_qubits`.
            - 'skip': When a qubit position outside `allowed_qubits` is encountered,
                ignore it. Note that, for two-qubit and multi-qubit operations, this
                will ignore the pair or group of targets containing the skipped position.
    """
```

<a name="stimflow.ChunkBuilder.append_feedback"></a>
```python
# stimflow.ChunkBuilder.append_feedback

# (in class stimflow.ChunkBuilder)
def append_feedback(
    self,
    *,
    control_keys: Iterable[Any],
    targets: Iterable[complex],
    basis: str,
    unknown_qubit_append_mode: "Literal['auto, 'error, 'skip, 'include']" = 'auto,
) -> None:
    """Appends the tensor product of the given controls and targets into the circuit.
    """
```

<a name="stimflow.ChunkBuilder.finish_chunk"></a>
```python
# stimflow.ChunkBuilder.finish_chunk

# (in class stimflow.ChunkBuilder)
def finish_chunk(
    self,
    *,
    wants_to_merge_with_prev: bool = False,
    wants_to_merge_with_next: bool = False,
    failure_mode: "Literal['error, 'ignore, 'print']" = 'error,
) -> Chunk:
    """Finishes producing the circuit.
    """
```

<a name="stimflow.ChunkBuilder.has_measurement"></a>
```python
# stimflow.ChunkBuilder.has_measurement

# (in class stimflow.ChunkBuilder)
def has_measurement(
    self,
    key: Any,
) -> bool:
    """Determines if a measurement with the given key has been performed.

    Args:
        key: The measurement key.

    Returns:
        Whether a measurement with the given key has been performed.

    Examples:
        >>> import stimflow as sf
        >>> builder = sf.ChunkBuilder()
        >>> builder.append("M", [1 + 2j])
        >>> builder.has_measurement(1 + 2j)
        True
        >>> builder.has_measurement(1 + 3j)
        False
    """
```

<a name="stimflow.ChunkBuilder.lookup_measurement_indices"></a>
```python
# stimflow.ChunkBuilder.lookup_measurement_indices

# (in class stimflow.ChunkBuilder)
def lookup_measurement_indices(
    self,
    keys: Iterable[Any],
    *,
    ignore_unknown_measurements: bool = False,
) -> list[int]:
    """Looks up measurement indices by key.

    Measurement keys are created automatically by the `append` method when appending
    measurement operations (optionally tweaked by the `measure_key_func` argument).

    Args:
        keys: The measurement keys to lookup.
        ignore_unknown_measurements: Defaults to False. If set to True, keys that don't correspond
            to measurements are ignored instead of raising an error.

    Returns:
        A list of offsets indicating when the measurements occurred.

    Examples:
        >>> import stimflow as sf
        >>> builder = sf.ChunkBuilder()
        >>> builder.append("M", [1 + 2j])
        >>> builder.append("MX", [2j, 3j], measure_key_func=lambda e: str(e) + "test")

        >>> builder.lookup_measurement_indices([1 + 2j])
        [0]
        >>> builder.lookup_measurement_indices(["2jtest"])
        [1]
        >>> builder.lookup_measurement_indices(["2jtest", 1 + 2j])
        [0, 1]

        >>> builder.append("MZZ", [(0, 1)])
        >>> builder.lookup_measurement_indices([(1, 0)])
        [3]
    """
```

<a name="stimflow.ChunkCompiler"></a>
```python
# stimflow.ChunkCompiler

# (at top-level in the stimflow module)
class ChunkCompiler:
    """Compiles appended chunks into a unified circuit.

    Examples:
        >>> import stim
        >>> import stimflow as sf

        >>> zz = sf.PauliMap({0: 'Z', 1 + 1j: 'Z'})
        >>> idle_chunk = sf.Chunk(
        ...     stim.Circuit('''
        ...         QUBIT_COORDS(0, 0) 0
        ...         QUBIT_COORDS(0, 1) 1
        ...         QUBIT_COORDS(1, 1) 2
        ...         R 1
        ...         TICK
        ...         CX 0 1
        ...         TICK
        ...         CX 2 1
        ...         TICK
        ...         M 1
        ...     '''),
        ...     flows=[
        ...         sf.Flow(start=zz, measurement_indices=[0]),
        ...         sf.Flow(end=zz, measurement_indices=[0]),
        ...     ]
        ... )

        >>> compiler = sf.ChunkCompiler()
        >>> compiler.append_magic_init_chunk()
        >>> compiler.append(idle_chunk)
        >>> compiler.append(idle_chunk)
        >>> compiler.append_magic_end_chunk()
        >>> compiler.finish_circuit()
        stim.Circuit('''
            QUBIT_COORDS(0, 0) 0
            QUBIT_COORDS(0, 1) 1
            QUBIT_COORDS(1, 1) 2
            MPP Z0*Z2
            TICK
            R 1
            TICK
            CX 0 1
            TICK
            CX 2 1
            TICK
            M 1
            DETECTOR(0.5, 0.5, 0) rec[-2] rec[-1]
            SHIFT_COORDS(0, 0, 1)
            TICK
            R 1
            TICK
            CX 0 1
            TICK
            CX 2 1
            TICK
            M 1
            DETECTOR(0.5, 0.5, 0) rec[-2] rec[-1]
            SHIFT_COORDS(0, 0, 1)
            TICK
            MPP Z0*Z2
            DETECTOR(0.5, 0.5, 0) rec[-2] rec[-1]
        ''')
    """
```

<a name="stimflow.ChunkCompiler.__init__"></a>
```python
# stimflow.ChunkCompiler.__init__

# (in class stimflow.ChunkCompiler)
def __init__(
    self,
    *,
    metadata_func: Callable[[Flow], FlowMetadata] | None = None,
    skip_verification_before_append: bool = False,
):
    """

    Args:
        metadata_func: Determines coordinate data appended to detectors
            (after x, y, and t). Defaults to None (no extra metadata).
        skip_verification_before_append: Defaults to False. When False, the
            `verify` method if chunks (or other objects being appended) are
            verified before being appended. When True, this verification step
            is skipped. Setting to True will improve performance at the cost
            of safety.

    Examples:
        >>> import stim
        >>> import stimflow as sf

        >>> zz = sf.PauliMap({0: 'Z', 1 + 1j: 'Z'})
        >>> idle_chunk = sf.Chunk(
        ...     stim.Circuit('''
        ...         QUBIT_COORDS(0, 0) 0
        ...         QUBIT_COORDS(0, 1) 1
        ...         QUBIT_COORDS(1, 1) 2
        ...         R 1
        ...         TICK
        ...         CX 0 1
        ...         TICK
        ...         CX 2 1
        ...         TICK
        ...         M 1
        ...     '''),
        ...     flows=[
        ...         sf.Flow(start=zz, measurement_indices=[0]),
        ...         sf.Flow(end=zz, measurement_indices=[0]),
        ...     ]
        ... )

        >>> compiler = sf.ChunkCompiler()
        >>> compiler.append_magic_init_chunk()
        >>> compiler.append(idle_chunk)
        >>> compiler.append(idle_chunk)
        >>> compiler.append_magic_end_chunk()
        >>> compiler.finish_circuit()
        stim.Circuit('''
            QUBIT_COORDS(0, 0) 0
            QUBIT_COORDS(0, 1) 1
            QUBIT_COORDS(1, 1) 2
            MPP Z0*Z2
            TICK
            R 1
            TICK
            CX 0 1
            TICK
            CX 2 1
            TICK
            M 1
            DETECTOR(0.5, 0.5, 0) rec[-2] rec[-1]
            SHIFT_COORDS(0, 0, 1)
            TICK
            R 1
            TICK
            CX 0 1
            TICK
            CX 2 1
            TICK
            M 1
            DETECTOR(0.5, 0.5, 0) rec[-2] rec[-1]
            SHIFT_COORDS(0, 0, 1)
            TICK
            MPP Z0*Z2
            DETECTOR(0.5, 0.5, 0) rec[-2] rec[-1]
        ''')
    """
```

<a name="stimflow.ChunkCompiler.append"></a>
```python
# stimflow.ChunkCompiler.append

# (in class stimflow.ChunkCompiler)
def append(
    self,
    appended: Chunk | ChunkLoop | ChunkReflow,
) -> None:
    """Appends a chunk to the circuit being built.

    The input flows of the appended chunk must exactly match the open outgoing flows of the
    circuit so far.

    Args:
        appended: The object to append to the circuit.

            Unless `skip_verification_before_append=True` was specified when constructing the
            compiler, the `verify` method of this object will be called in order to ensure it
            is well form. If verification is skipped and the object is not well-formed, the
            compiler may output an invalid Stim circuit (e.g. with non-deterministic detectors).
    """
```

<a name="stimflow.ChunkCompiler.append_magic_end_chunk"></a>
```python
# stimflow.ChunkCompiler.append_magic_end_chunk

# (in class stimflow.ChunkCompiler)
def append_magic_end_chunk(
    self,
    expected: ChunkInterface | None = None,
) -> None:
    """Appends a non-physical chunk that terminates the circuit, regardless of open flows.

    Args:
        expected: Defaults to None (unused). If set to None, no extra checks are performed.
            If set to a ChunkInterface, it is verified that the open flows actually
            correspond to this interface.
    """
```

<a name="stimflow.ChunkCompiler.append_magic_init_chunk"></a>
```python
# stimflow.ChunkCompiler.append_magic_init_chunk

# (in class stimflow.ChunkCompiler)
def append_magic_init_chunk(
    self,
    expected: ChunkInterface | None = None,
) -> None:
    """Appends a non-physical chunk that outputs the flows expected by the next chunk.

    Args:
        expected: Defaults to None (unused). If set to a ChunkInterface, it will be
            verified that the next appended chunk actually has a start interface
            matching the given expected interface. If set to None, then no checks are
            performed; no constraints are placed on the next chunk.
    """
```

<a name="stimflow.ChunkCompiler.copy"></a>
```python
# stimflow.ChunkCompiler.copy

# (in class stimflow.ChunkCompiler)
def copy(
    self,
) -> ChunkCompiler:
    """Returns a deep copy of the compiler's state.
    """
```

<a name="stimflow.ChunkCompiler.cur_circuit_html_viewer"></a>
```python
# stimflow.ChunkCompiler.cur_circuit_html_viewer

# (in class stimflow.ChunkCompiler)
def cur_circuit_html_viewer(
    self,
) -> stimflow.str_html:
```

<a name="stimflow.ChunkCompiler.cur_end_interface"></a>
```python
# stimflow.ChunkCompiler.cur_end_interface

# (in class stimflow.ChunkCompiler)
def cur_end_interface(
    self,
) -> ChunkInterface:
```

<a name="stimflow.ChunkCompiler.ensure_observables_included"></a>
```python
# stimflow.ChunkCompiler.ensure_observables_included

# (in class stimflow.ChunkCompiler)
def ensure_observables_included(
    self,
    observable_names: Iterable[Any],
):
```

<a name="stimflow.ChunkCompiler.ensure_qubits_included"></a>
```python
# stimflow.ChunkCompiler.ensure_qubits_included

# (in class stimflow.ChunkCompiler)
def ensure_qubits_included(
    self,
    qubits: Iterable[complex],
):
    """Adds the given qubit positions to the indexed positions, if they aren't already.
    """
```

<a name="stimflow.ChunkCompiler.finish_circuit"></a>
```python
# stimflow.ChunkCompiler.finish_circuit

# (in class stimflow.ChunkCompiler)
def finish_circuit(
    self,
) -> stim.Circuit:
    """Returns the circuit built by the compiler.

    Performs some final translation steps:
    - Re-indexing the qubits to be in a sorted order.
    - Re-indexing the observables to omit discarded observable flows.
    """
```

<a name="stimflow.ChunkInterface"></a>
```python
# stimflow.ChunkInterface

# (at top-level in the stimflow module)
class ChunkInterface:
    """Specifies a set of stabilizers and observables that a chunk can consume or prepare.
    """
```

<a name="stimflow.ChunkInterface.data_set"></a>
```python
# stimflow.ChunkInterface.data_set

# (in class stimflow.ChunkInterface)
class data_set:
```

<a name="stimflow.ChunkInterface.partitioned_detector_flows"></a>
```python
# stimflow.ChunkInterface.partitioned_detector_flows

# (in class stimflow.ChunkInterface)
def partitioned_detector_flows(
    self,
) -> list[list[PauliMap]]:
    """Returns the stabilizers of the interface, split into non-overlapping groups.
    """
```

<a name="stimflow.ChunkInterface.to_code"></a>
```python
# stimflow.ChunkInterface.to_code

# (in class stimflow.ChunkInterface)
def to_code(
    self,
) -> StabilizerCode:
    """Returns a stimflow.StabilizerCode with an equivalent interface.
    """
```

<a name="stimflow.ChunkInterface.to_patch"></a>
```python
# stimflow.ChunkInterface.to_patch

# (in class stimflow.ChunkInterface)
def to_patch(
    self,
) -> Patch:
    """Returns a stimflow.Patch with tiles equal to the chunk interface's stabilizers.
    """
```

<a name="stimflow.ChunkInterface.to_svg"></a>
```python
# stimflow.ChunkInterface.to_svg

# (in class stimflow.ChunkInterface)
def to_svg(
    self,
    *,
    show_order: bool = False,
    show_measure_qubits: bool = False,
    show_data_qubits: bool = True,
    system_qubits: Iterable[complex] = (),
    opacity: float = 1,
    show_coords: bool = True,
    show_obs: bool = True,
    other: StabilizerCode | Patch | Iterable[StabilizerCode | Patch] | None = None,
    tile_color_func: Callable[[Tile], str] | None = None,
    rows: int | None = None,
    cols: int | None = None,
    find_logical_err_max_weight: int | None = None,
) -> str_svg:
```

<a name="stimflow.ChunkInterface.used_set"></a>
```python
# stimflow.ChunkInterface.used_set

# (in class stimflow.ChunkInterface)
class used_set:
    """Returns the set of qubits used in any flow mentioned by the chunk interface.
    """
```

<a name="stimflow.ChunkInterface.with_discards_as_ports"></a>
```python
# stimflow.ChunkInterface.with_discards_as_ports

# (in class stimflow.ChunkInterface)
def with_discards_as_ports(
    self,
) -> ChunkInterface:
    """Returns the same chunk interface, but with discarded flows turned into normal flows.
    """
```

<a name="stimflow.ChunkInterface.with_edits"></a>
```python
# stimflow.ChunkInterface.with_edits

# (in class stimflow.ChunkInterface)
def with_edits(
    self,
    *,
    ports: Iterable[PauliMap] | None = None,
    discards: Iterable[PauliMap] | None = None,
) -> ChunkInterface:
    """Returns an equivalent chunk interface but with the given values replaced.
    """
```

<a name="stimflow.ChunkInterface.with_transformed_coords"></a>
```python
# stimflow.ChunkInterface.with_transformed_coords

# (in class stimflow.ChunkInterface)
def with_transformed_coords(
    self,
    transform: Callable[[complex], complex],
) -> ChunkInterface:
    """Returns the same interface, but with coordinates transformed by the given function.
    """
```

<a name="stimflow.ChunkInterface.without_discards"></a>
```python
# stimflow.ChunkInterface.without_discards

# (in class stimflow.ChunkInterface)
def without_discards(
    self,
) -> ChunkInterface:
    """Returns the same chunk interface, but with discarded flows not included.
    """
```

<a name="stimflow.ChunkInterface.without_keyed"></a>
```python
# stimflow.ChunkInterface.without_keyed

# (in class stimflow.ChunkInterface)
def without_keyed(
    self,
) -> ChunkInterface:
    """Returns the same chunk interface, but without logical flows (named flows).
    """
```

<a name="stimflow.ChunkLoop"></a>
```python
# stimflow.ChunkLoop

# (at top-level in the stimflow module)
class ChunkLoop:
    """Specifies a series of chunks to repeat a fixed number of times.

    The loop invariant is that the last chunk's end interface should match the
    first chunk's start interface (unless the number of repetitions is less than
    2).

    For duck typing purposes, many methods supported by Chunk are supported by
    ChunkLoop.
    """
```

<a name="stimflow.ChunkLoop.end_interface"></a>
```python
# stimflow.ChunkLoop.end_interface

# (in class stimflow.ChunkLoop)
def end_interface(
    self,
) -> ChunkInterface:
    """Returns the end interface of the last chunk in the loop.
    """
```

<a name="stimflow.ChunkLoop.end_patch"></a>
```python
# stimflow.ChunkLoop.end_patch

# (in class stimflow.ChunkLoop)
def end_patch(
    self,
) -> Patch:
```

<a name="stimflow.ChunkLoop.find_distance"></a>
```python
# stimflow.ChunkLoop.find_distance

# (in class stimflow.ChunkLoop)
def find_distance(
    self,
    *,
    max_search_weight: int,
    noise: float | NoiseModel = 0.001,
    noiseless_qubits: Iterable[float | int | complex] = (),
) -> int:
```

<a name="stimflow.ChunkLoop.find_logical_error"></a>
```python
# stimflow.ChunkLoop.find_logical_error

# (in class stimflow.ChunkLoop)
def find_logical_error(
    self,
    *,
    max_search_weight: int,
    noise: float | NoiseModel = 0.001,
    noiseless_qubits: Iterable[float | int | complex] = (),
) -> list[stim.ExplainedError]:
    """Searches for a minium distance undetected logical error.

    By default, searches using a uniform depolarizing circuit noise model.
    """
```

<a name="stimflow.ChunkLoop.flattened"></a>
```python
# stimflow.ChunkLoop.flattened

# (in class stimflow.ChunkLoop)
def flattened(
    self,
) -> list[Chunk | ChunkReflow]:
    """Unrolls the loop, and any sub-loops, into a series of chunks.
    """
```

<a name="stimflow.ChunkLoop.start_interface"></a>
```python
# stimflow.ChunkLoop.start_interface

# (in class stimflow.ChunkLoop)
def start_interface(
    self,
) -> ChunkInterface:
    """Returns the start interface of the first chunk in the loop.
    """
```

<a name="stimflow.ChunkLoop.start_patch"></a>
```python
# stimflow.ChunkLoop.start_patch

# (in class stimflow.ChunkLoop)
def start_patch(
    self,
) -> Patch:
```

<a name="stimflow.ChunkLoop.time_reversed"></a>
```python
# stimflow.ChunkLoop.time_reversed

# (in class stimflow.ChunkLoop)
def time_reversed(
    self,
) -> ChunkLoop:
    """Returns the same loop, but time reversed.

    The time reversed loop has reversed flows, implemented by performs operations in the
    reverse order and exchange measurements for resets (and vice versa) as appropriate.
    It has exactly the same fault tolerant structure, just mirrored in time.
    """
```

<a name="stimflow.ChunkLoop.to_closed_circuit"></a>
```python
# stimflow.ChunkLoop.to_closed_circuit

# (in class stimflow.ChunkLoop)
def to_closed_circuit(
    self,
) -> stim.Circuit:
    """Compiles the chunk into a circuit by conjugating with mpp init/end chunks.
    """
```

<a name="stimflow.ChunkLoop.to_html_viewer"></a>
```python
# stimflow.ChunkLoop.to_html_viewer

# (in class stimflow.ChunkLoop)
def to_html_viewer(
    self,
    *,
    patch: Patch | StabilizerCode | ChunkInterface | None = None,
    tile_color_func: Callable[[Tile], tuple[float, float, float, float]] | None = None,
    known_error: Iterable[stim.ExplainedError] | None = None,
) -> str_html:
    """Returns an HTML document containing a viewer for the chunk loop's circuit.
    """
```

<a name="stimflow.ChunkLoop.verify"></a>
```python
# stimflow.ChunkLoop.verify

# (in class stimflow.ChunkLoop)
def verify(
    self,
    *,
    expected_in: ChunkInterface | None = None,
    expected_out: ChunkInterface | None = None,
):
```

<a name="stimflow.ChunkLoop.verify_distance_is_at_least"></a>
```python
# stimflow.ChunkLoop.verify_distance_is_at_least

# (in class stimflow.ChunkLoop)
def verify_distance_is_at_least(
    self,
    minimum_distance: int,
    *,
    noise: float | NoiseModel = 0.001,
):
    """Verifies undetected logical errors require at least the given number of physical errors.

    Verifies using a uniform depolarizing circuit noise model.
    """
```

<a name="stimflow.ChunkLoop.with_repetitions"></a>
```python
# stimflow.ChunkLoop.with_repetitions

# (in class stimflow.ChunkLoop)
def with_repetitions(
    self,
    new_repetitions: int,
) -> ChunkLoop:
    """Returns the same loop, but with a different number of repetitions.
    """
```

<a name="stimflow.ChunkReflow"></a>
```python
# stimflow.ChunkReflow

# (at top-level in the stimflow module)
class ChunkReflow:
    """An adapter chunk for attaching chunks describing the same thing in different ways.

    (This class is still a work in progress; it is not simple to use and it
    doesn't achieve all the desired functionality.)

    For example, consider two surface code idle round chunks where one has the logical
    operator on the left side and the other has the logical operator on the right side.
    They can't be directly concatenated, because their flows don't match. But a reflow
    chunk can be placed in between, mapping the left logical operator to the right
    logical operator times a set of stabilizers, in order to bridge the incompatibility.
    """
```

<a name="stimflow.ChunkReflow.end_code"></a>
```python
# stimflow.ChunkReflow.end_code

# (in class stimflow.ChunkReflow)
def end_code(
    self,
) -> StabilizerCode:
```

<a name="stimflow.ChunkReflow.end_interface"></a>
```python
# stimflow.ChunkReflow.end_interface

# (in class stimflow.ChunkReflow)
def end_interface(
    self,
) -> ChunkInterface:
```

<a name="stimflow.ChunkReflow.end_patch"></a>
```python
# stimflow.ChunkReflow.end_patch

# (in class stimflow.ChunkReflow)
def end_patch(
    self,
) -> Patch:
```

<a name="stimflow.ChunkReflow.flattened"></a>
```python
# stimflow.ChunkReflow.flattened

# (in class stimflow.ChunkReflow)
def flattened(
    self,
) -> list[ChunkReflow]:
    """This is here for duck-type compatibility with ChunkLoop.
    """
```

<a name="stimflow.ChunkReflow.from_auto_rewrite"></a>
```python
# stimflow.ChunkReflow.from_auto_rewrite

# (in class stimflow.ChunkReflow)
def from_auto_rewrite(
    *,
    inputs: Iterable[PauliMap],
    out2in: "dict[PauliMap, list[PauliMap] | Literal['auto']]",
) -> ChunkReflow:
```

<a name="stimflow.ChunkReflow.from_auto_rewrite_transitions_using_stable"></a>
```python
# stimflow.ChunkReflow.from_auto_rewrite_transitions_using_stable

# (in class stimflow.ChunkReflow)
def from_auto_rewrite_transitions_using_stable(
    *,
    stable: Iterable[PauliMap],
    transitions: Iterable[tuple[PauliMap, PauliMap]],
) -> ChunkReflow:
    """Bridges the given transitions using products from the given stable values.
    """
```

<a name="stimflow.ChunkReflow.removed_inputs"></a>
```python
# stimflow.ChunkReflow.removed_inputs

# (in class stimflow.ChunkReflow)
class removed_inputs:
```

<a name="stimflow.ChunkReflow.start_code"></a>
```python
# stimflow.ChunkReflow.start_code

# (in class stimflow.ChunkReflow)
def start_code(
    self,
) -> StabilizerCode:
```

<a name="stimflow.ChunkReflow.start_interface"></a>
```python
# stimflow.ChunkReflow.start_interface

# (in class stimflow.ChunkReflow)
def start_interface(
    self,
) -> ChunkInterface:
```

<a name="stimflow.ChunkReflow.start_patch"></a>
```python
# stimflow.ChunkReflow.start_patch

# (in class stimflow.ChunkReflow)
def start_patch(
    self,
) -> Patch:
```

<a name="stimflow.ChunkReflow.verify"></a>
```python
# stimflow.ChunkReflow.verify

# (in class stimflow.ChunkReflow)
def verify(
    self,
    *,
    expected_in: StabilizerCode | ChunkInterface | None = None,
    expected_out: StabilizerCode | ChunkInterface | None = None,
):
    """Verifies that the ChunkReflow is well-formed.
    """
```

<a name="stimflow.ChunkReflow.with_obs_flows_as_det_flows"></a>
```python
# stimflow.ChunkReflow.with_obs_flows_as_det_flows

# (in class stimflow.ChunkReflow)
def with_obs_flows_as_det_flows(
    self,
):
```

<a name="stimflow.ChunkReflow.with_transformed_coords"></a>
```python
# stimflow.ChunkReflow.with_transformed_coords

# (in class stimflow.ChunkReflow)
def with_transformed_coords(
    self,
    transform: Callable[[complex], complex],
) -> ChunkReflow:
```

<a name="stimflow.Flow"></a>
```python
# stimflow.Flow

# (at top-level in the stimflow module)
class Flow:
    """A rule for how a stabilizer travels into, through, and/or out of a circuit.
    """
```

<a name="stimflow.Flow.__init__"></a>
```python
# stimflow.Flow.__init__

# (in class stimflow.Flow)
def __init__(
    self,
    *,
    start: PauliMap | Tile | None = None,
    end: PauliMap | Tile | None = None,
    measurement_indices: Iterable[int] = (),
    center: "complex | None | Literal['infer']" = 'infer,
    flags: Iterable[Any] = frozenset(),
    sign: bool | None = None,
):
    """Initializes a Flow.

    Args:
        start: Defaults to None (empty). The Pauli product operator at the beginning of
            the circuit (before *all* operations, including resets).
        end: Defaults to None (empty). The Pauli product operator at the end of the
            circuit (after *all* operations, including measurements).
        measurement_indices: Defaults to empty. Indices of measurements that mediate
            the flow (that multiply into it as it traverses the circuit).
        center: Defaults to 'infer' (attempt to infer). Specifies a 2d coordinate to
            use in metadata, when the flow is completed into a detector. Can be set to a
            complex number or to None.
        flags: Defaults to empty. Custom information about the flow, that can be used by
            code operating on chunks for a variety of purposes. For example, this could
            identify the "color" of the flow in a color code.
        sign: Defaults to None (unsigned).
    """
```

<a name="stimflow.Flow.__mul__"></a>
```python
# stimflow.Flow.__mul__

# (in class stimflow.Flow)
def __mul__(
    self,
    other: Flow,
) -> Flow:
    """Computes the product of two flows.

    The product of two flows sends the product of their inputs to the product of their
    outputs. For example, (A -> B) * (C -> D) = (A*C) -> (B*D).

    Starts are multiplied. Ends are multiplied. Measurement sets are xored. Centers are
    averaged. Signs are xored. flags are union'd.

    Args:
        other: The other flow in the multiplication.

    Raises:
        ValueError:
            The flows have incompatible observable names.

            OR

            The flows disagree on whether they're unsigned.

    Examples:
        >>> import stimflow as sf
        >>> a = sf.Flow(
        ...     start=sf.PauliMap({1: 'X'}),
        ...     end=sf.PauliMap({2: 'Y'}),
        ...     measurement_indices=[-1, 2],
        ... )
        >>> b = sf.Flow(
        ...     start=sf.PauliMap({2: 'Y'}),
        ...     end=sf.PauliMap({3: 'Z'}),
        ...     measurement_indices=[-10, 20],
        ... )
        >>> a * b
        stimflow.Flow(
            start=stimflow.PauliMap({(1+0j): 'X', (2+0j): 'Y'}),
            end=stimflow.PauliMap({(2+0j): 'Y', (3+0j): 'Z'}),
            measurement_indices=(-10, -1, 2, 20),
            center=(2+0j),
        )
    """
```

<a name="stimflow.Flow.fused_with_next_flow"></a>
```python
# stimflow.Flow.fused_with_next_flow

# (in class stimflow.Flow)
def fused_with_next_flow(
    self,
    next_flow: Flow,
    *,
    next_flow_measure_offset: int,
) -> Flow:
    """Combines flows tail-to-head.

    For example, fusing X1 -> Y2 with Y2 -> Z3 produces X1 -> Z3.

    Measurement sets are xored, adjusting for the offset. Centers are
    taken as is, preferring the center of the prior flow. Signs are xored.
    flags are union'd.

    Args:
        next_flow: The flow that occurs after this flow. Must have a start
            that matches the end of this flow.
        next_flow_measure_offset: What offset to add into measurement indices
            used by the other flow.

    Returns:
        The fused flow.

    Examples:
        >>> import stimflow as sf
        >>> a = sf.Flow(
        ...     start=sf.PauliMap({1: 'X'}),
        ...     end=sf.PauliMap({2: 'Y'}),
        ...     measurement_indices=[-1, 2],
        ... )
        >>> b = sf.Flow(
        ...     start=sf.PauliMap({2: 'Y'}),
        ...     end=sf.PauliMap({3: 'Z'}),
        ...     measurement_indices=[-10, 20],
        ... )
        >>> a.fused_with_next_flow(b, next_flow_measure_offset=100)
        stimflow.Flow(
            start=stimflow.PauliMap({(1+0j): 'X'}),
            end=stimflow.PauliMap({(3+0j): 'Z'}),
            measurement_indices=(2, 90, 99, 120),
            center=(2+0j),
        )
    """
```

<a name="stimflow.Flow.obs_name"></a>
```python
# stimflow.Flow.obs_name

# (in class stimflow.Flow)
@property
def obs_name(
    self,
):
```

<a name="stimflow.Flow.to_stim_flow"></a>
```python
# stimflow.Flow.to_stim_flow

# (in class stimflow.Flow)
def to_stim_flow(
    self,
    *,
    q2i: dict[complex, int],
    o2i: Mapping[Any, int | None] | None = None,
) -> stim.Flow:
    """Converts this `stimflow.Flow` into a `stim.Flow`.

    Args:
        q2i: A mapping from stimflow qubit positions to stim qubit indices.
        o2i: A mapping from stimflow obs names to stim obs indices.
            This argument can be skipped if the flow has no obs_name.

    Returns:
        The stim flow.

    Raise:
        ValueError:
            The flow has an `obs_name` but `o2i` wasn't specified.

    Examples:
        >>> import stimflow as sf
        >>> flow = sf.Flow(
        ...     start=sf.PauliMap({'Z': 1j}, obs_name="test"),
        ...     end=sf.PauliMap({'X': 1 + 1j}, obs_name="test"),
        ...     measurement_indices=[1, 2],
        ...     sign=True,
        ... )
        >>> flow.to_stim_flow(q2i={1j: 2, 1 + 1j: 3}, o2i={"test": 0})
        stim.Flow("__Z -> -___X xor rec[1] xor rec[2] xor obs[0]")
    """
```

<a name="stimflow.Flow.with_edits"></a>
```python
# stimflow.Flow.with_edits

# (in class stimflow.Flow)
def with_edits(
    self,
    *,
    start: PauliMap = _UNSPECIFIED,
    end: PauliMap = _UNSPECIFIED,
    measurement_indices: Iterable[int] = _UNSPECIFIED,
    center: complex | None = _UNSPECIFIED,
    flags: Iterable[str] = _UNSPECIFIED,
    sign: Any = _UNSPECIFIED,
    obs_name: None | str = _UNSPECIFIED,
) -> Flow:
    """Returns the same flow but with specified edits.

    Args:
        start: If specified, the returned flow has the specified start instead of the
            start used by the original flow. Note: if `obs_name` is also specified,
            the obs_name of this argument must be consistent with the given `obs_name`.
        end: If specified, the returned flow has the specified end instead of the
            end used by the original flow. Note: if `obs_name` is also specified,
            the obs_name of this argument must be consistent with the given `obs_name`.
        measurement_indices: If specified, the returned flow has the specified
            measurement_indices instead of the measurement_indices used by the original
            flow.
        center: If specified, the returned flow has the specified center instead of the
            center used by the original flow.
        flags: If specified, the returned flow has the specified flags instead of the
            flags used by the original flow.
        sign: If specified, the returned flow has the specified sign instead of the
            sign used by the original flow.
        obs_name: If specified, the returned flow has the obs_name of both its start and
            end changed to the given value. If `start` or `end` are specified alongside
            this argument, they must use the same observable name.

    Returns:
        The edited flow.

    Raises:
        ValueError:
            Specified contradictory `obs_name=` and `start=` values.

            OR

            Specified contradictory `obs_name=` and `end=` values.

            OR

            The edits produced an invalid flow (stimflow.Flow.__init__ raised an error).
    """
```

<a name="stimflow.Flow.with_transformed_coords"></a>
```python
# stimflow.Flow.with_transformed_coords

# (in class stimflow.Flow)
def with_transformed_coords(
    self,
    transform: Callable[[complex], complex],
) -> Flow:
```

<a name="stimflow.Flow.with_xz_flipped"></a>
```python
# stimflow.Flow.with_xz_flipped

# (in class stimflow.Flow)
def with_xz_flipped(
    self,
) -> Flow:
```

<a name="stimflow.FlowMetadata"></a>
```python
# stimflow.FlowMetadata

# (at top-level in the stimflow module)
class FlowMetadata:
    """Metadata, based on a flow, to use during circuit generation.
    """
```

<a name="stimflow.FlowMetadata.__init__"></a>
```python
# stimflow.FlowMetadata.__init__

# (in class stimflow.FlowMetadata)
def __init__(
    self,
    *,
    extra_coords: Iterable[float] = (),
    tag: str | None = ',
):
    """

    Args:
        extra_coords: Extra numbers to add to DETECTOR coordinate arguments. By default stimflow
            gives each detector an X, Y, and T coordinate. These numbers go afterward.
        tag: A tag to attach to DETECTOR or OBSERVABLE_INCLUDE instructions.
    """
```

<a name="stimflow.LayerCircuit"></a>
```python
# stimflow.LayerCircuit

# (at top-level in the stimflow module)
@dataclasses.dataclass
class LayerCircuit:
    """A stabilizer circuit represented as a series of typed layers.

    For example, the circuit could be a `LayerReset`, then a `LayerRotation`,
    then a few `LayerInteract`s, then a `LayerMeasure`.
    """
    layers: list[Layer]
```

<a name="stimflow.LayerCircuit.copy"></a>
```python
# stimflow.LayerCircuit.copy

# (in class stimflow.LayerCircuit)
def copy(
    self,
) -> LayerCircuit:
```

<a name="stimflow.LayerCircuit.from_stim_circuit"></a>
```python
# stimflow.LayerCircuit.from_stim_circuit

# (in class stimflow.LayerCircuit)
def from_stim_circuit(
    circuit: stim.Circuit,
) -> LayerCircuit:
```

<a name="stimflow.LayerCircuit.to_stim_circuit"></a>
```python
# stimflow.LayerCircuit.to_stim_circuit

# (in class stimflow.LayerCircuit)
def to_stim_circuit(
    self,
) -> stim.Circuit:
    """Compiles the layer circuit into a stim circuit and returns it.
    """
```

<a name="stimflow.LayerCircuit.to_z_basis"></a>
```python
# stimflow.LayerCircuit.to_z_basis

# (in class stimflow.LayerCircuit)
def to_z_basis(
    self,
) -> LayerCircuit:
```

<a name="stimflow.LayerCircuit.touched"></a>
```python
# stimflow.LayerCircuit.touched

# (in class stimflow.LayerCircuit)
def touched(
    self,
) -> set[int]:
```

<a name="stimflow.LayerCircuit.with_cleaned_up_loop_iterations"></a>
```python
# stimflow.LayerCircuit.with_cleaned_up_loop_iterations

# (in class stimflow.LayerCircuit)
def with_cleaned_up_loop_iterations(
    self,
) -> LayerCircuit:
    """Attempts to roll up partially unrolled loops.

    Checks if the instructions before a loop correspond to the instruction inside a loop. If so,
    removes the matching instructions beforehand and increases the iteration count by 1. Same
    for instructions after the loop.

    This essentially undoes the effect of `with_ejected_loop_iterations`. A common pattern is
    to do `with_ejected_loop_iterations`, then an optimization, then
    `with_cleaned_up_loop_iterations`. This gives the optimization the chance to optimize across
    a loop boundary, but cleans up after itself if no optimization occurs.

    In some cases this method is useful because of circuit generation code being overly cautious
    about how quickly loop invariants are established, and so emitting the first iteration of a
    loop in a special way. If it happens to be identical, despite the different code path that
    produced it, this method will roll it into the rest of the loop.

    For example, this method would turn this circuit fragment:

        X 0
        MR 0
        REPEAT 98 {
            X 0
            MR 0
        }
        X 0
        MR 0

    into this circuit fragment:

        REPEAT 100 {
            X 0
            MR 0
        }
    """
```

<a name="stimflow.LayerCircuit.with_clearable_rotation_layers_cleared"></a>
```python
# stimflow.LayerCircuit.with_clearable_rotation_layers_cleared

# (in class stimflow.LayerCircuit)
def with_clearable_rotation_layers_cleared(
    self,
) -> LayerCircuit:
    """Removes rotation layers where every rotation in the layer can be moved to another layer.

    Each individual rotation can move through intermediate non-rotation layers as long as those
    layers don't touch the qubit being rotated.
    """
```

<a name="stimflow.LayerCircuit.with_ejected_loop_iterations"></a>
```python
# stimflow.LayerCircuit.with_ejected_loop_iterations

# (in class stimflow.LayerCircuit)
def with_ejected_loop_iterations(
    self,
) -> LayerCircuit:
    """Partially unrolls loops, placing one iteration before and one iteration after.

    This is useful for ensuring the transition into and out of a loop is optimized correctly.
    For example, if a circuit begins with a transversal initialization of data qubits and then
    immediately starts a memory loop, the resets from the data initialization should be merged
    into the same layer as the resets from the measurement initialization at the beginning of
    the loop. But the reset-merging optimization might not see that this is possible across the
    loop boundary. Ejecting an iteration fixes this issue.

    For example, this method would turn this circuit fragment:

        REPEAT 100 {
            X 0
            MR 0
        }

    into this circuit fragment:

        X 0
        MR 0
        REPEAT 98 {
            X 0
            MR 0
        }
        X 0
        MR 0
    """
```

<a name="stimflow.LayerCircuit.with_irrelevant_tail_layers_removed"></a>
```python
# stimflow.LayerCircuit.with_irrelevant_tail_layers_removed

# (in class stimflow.LayerCircuit)
def with_irrelevant_tail_layers_removed(
    self,
) -> LayerCircuit:
```

<a name="stimflow.LayerCircuit.with_locally_merged_measure_layers"></a>
```python
# stimflow.LayerCircuit.with_locally_merged_measure_layers

# (in class stimflow.LayerCircuit)
def with_locally_merged_measure_layers(
    self,
) -> LayerCircuit:
    """Merges measurement layers together, despite intervening annotation layers.

    For example, this method would turn this circuit fragment:

        M 0
        DETECTOR(0, 0) rec[-1]
        OBSERVABLE_INCLUDE(5) rec[-1]
        SHIFT_COORDS(0, 1)
        M 1
        DETECTOR(0, 0) rec[-1]

    into this circuit fragment:

        M 0 1
        DETECTOR(0, 0) rec[-2]
        OBSERVABLE_INCLUDE(5) rec[-2]
        SHIFT_COORDS(0, 1)
        DETECTOR(0, 0) rec[-1]
    """
```

<a name="stimflow.LayerCircuit.with_locally_optimized_layers"></a>
```python
# stimflow.LayerCircuit.with_locally_optimized_layers

# (in class stimflow.LayerCircuit)
def with_locally_optimized_layers(
    self,
) -> LayerCircuit:
    """Iterates over the circuit aggregating layer.optimized(second_layer).
    """
```

<a name="stimflow.LayerCircuit.with_qubit_coords_at_start"></a>
```python
# stimflow.LayerCircuit.with_qubit_coords_at_start

# (in class stimflow.LayerCircuit)
def with_qubit_coords_at_start(
    self,
) -> LayerCircuit:
```

<a name="stimflow.LayerCircuit.with_rotations_before_resets_removed"></a>
```python
# stimflow.LayerCircuit.with_rotations_before_resets_removed

# (in class stimflow.LayerCircuit)
def with_rotations_before_resets_removed(
    self,
    loop_boundary_resets: set[int] | None = None,
) -> LayerCircuit:
```

<a name="stimflow.LayerCircuit.with_rotations_merged_earlier"></a>
```python
# stimflow.LayerCircuit.with_rotations_merged_earlier

# (in class stimflow.LayerCircuit)
def with_rotations_merged_earlier(
    self,
) -> LayerCircuit:
```

<a name="stimflow.LayerCircuit.with_rotations_rolled_from_end_of_loop_to_start_of_loop"></a>
```python
# stimflow.LayerCircuit.with_rotations_rolled_from_end_of_loop_to_start_of_loop

# (in class stimflow.LayerCircuit)
def with_rotations_rolled_from_end_of_loop_to_start_of_loop(
    self,
) -> LayerCircuit:
    """Rewrites loops so that they only have rotations at the start, not the end.

    This is useful for ensuring loops don't redundantly rotate at the loop boundary,
    by merging the rotations at the end with the rotations at the start or by
    making it clear rotations at the end were not needed because of the
    operations coming next.

    For example, this:

        REPEAT 5 {
            S 2 3 4
            R 0 1
            ...
            M 0 1
            H 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14
            DETECTOR rec[-1]
        }

    will become this:

        REPEAT 5 {
            H 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14
            S 2 3 4
            R 0 1
            ...
            M 0 1
            DETECTOR rec[-1]
        }

    which later optimization passes can then reduce further.
    """
```

<a name="stimflow.LayerCircuit.with_whole_layers_slid_as_early_as_possible_for_merge_with_same_layer"></a>
```python
# stimflow.LayerCircuit.with_whole_layers_slid_as_early_as_possible_for_merge_with_same_layer

# (in class stimflow.LayerCircuit)
def with_whole_layers_slid_as_early_as_possible_for_merge_with_same_layer(
    self,
    layer_types: type | tuple[type, ...],
) -> LayerCircuit:
```

<a name="stimflow.LayerCircuit.with_whole_layers_slid_as_to_merge_with_previous_layer_of_same_type"></a>
```python
# stimflow.LayerCircuit.with_whole_layers_slid_as_to_merge_with_previous_layer_of_same_type

# (in class stimflow.LayerCircuit)
def with_whole_layers_slid_as_to_merge_with_previous_layer_of_same_type(
    self,
    layer_types: type | tuple[type, ...],
) -> LayerCircuit:
```

<a name="stimflow.LayerCircuit.with_whole_rotation_layers_slid_earlier"></a>
```python
# stimflow.LayerCircuit.with_whole_rotation_layers_slid_earlier

# (in class stimflow.LayerCircuit)
def with_whole_rotation_layers_slid_earlier(
    self,
) -> LayerCircuit:
```

<a name="stimflow.LayerCircuit.without_empty_layers"></a>
```python
# stimflow.LayerCircuit.without_empty_layers

# (in class stimflow.LayerCircuit)
def without_empty_layers(
    self,
) -> LayerCircuit:
    """Removes empty layers from the circuit.

    Empty layers are sometimes created as a byproduct of certain optimizations, or may have been
    present in the original circuit. Usually they are unwanted, and this method removes them.
    """
```

<a name="stimflow.LineDataFor3DModel"></a>
```python
# stimflow.LineDataFor3DModel

# (at top-level in the stimflow module)
class LineDataFor3DModel:
    """Coordinates and colors of lines to draw in a 3d model.

    Example:
        >>> import stimflow as sf
        >>> red_square_outline = sf.LineDataFor3DModel(
        ...     rgba=(1, 0, 0, 1),
        ...     edge_list=[
        ...         # A square made of four lines.
        ...         [(0, 0, 0), (0, 1, 0)],
        ...         [(0, 1, 0), (1, 1, 0)],
        ...         [(1, 1, 0), (1, 0, 0)],
        ...         [(1, 0, 0), (0, 0, 0)],
        ...     ],
        ... )
        >>> model = sf.make_3d_model([red_square_outline])
        >>> assert model.html_viewer() is not None
    """
```

<a name="stimflow.LineDataFor3DModel.__init__"></a>
```python
# stimflow.LineDataFor3DModel.__init__

# (in class stimflow.LineDataFor3DModel)
def __init__(
    self,
    *,
    rgba: tuple[float, float, float, float],
    edge_list: np.ndarray | Iterable[Sequence[Sequence[float]]],
):
    """Lines with associated color information.

    Args:
        rgba: Red, green, blue, and alpha data to associate with all the lines.
            Each value should range from 0 to 1.
            (The alpha data is ignored in most viewers, but needed by the 3d model format.)
        edge_list: A 3d float32 numpy array with shape == (*, 2, 3).
            Axis 0 is the triangle axis (each entry is a triangle).
            Axis 1 is the AB vertex axis (each entry is a vertex from the edge).
            Axis 2 is the XYZ coordinate axis (each entry is a coordinate from the vertex).
    """
```

<a name="stimflow.LineDataFor3DModel.fused"></a>
```python
# stimflow.LineDataFor3DModel.fused

# (in class stimflow.LineDataFor3DModel)
def fused(
    data: Iterable[LineDataFor3DModel],
) -> list[LineDataFor3DModel]:
    """Attempts to combine line data instances into fewer instances.
    """
```

<a name="stimflow.NoiseModel"></a>
```python
# stimflow.NoiseModel

# (at top-level in the stimflow module)
class NoiseModel:
    """Converts circuits into noisy circuits according to rules.
    """
```

<a name="stimflow.NoiseModel.noisy_circuit"></a>
```python
# stimflow.NoiseModel.noisy_circuit

# (in class stimflow.NoiseModel)
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
```

<a name="stimflow.NoiseModel.noisy_circuit_skipping_mpp_boundaries"></a>
```python
# stimflow.NoiseModel.noisy_circuit_skipping_mpp_boundaries

# (in class stimflow.NoiseModel)
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
```

<a name="stimflow.NoiseModel.si1000"></a>
```python
# stimflow.NoiseModel.si1000

# (in class stimflow.NoiseModel)
def si1000(
    p: float,
) -> NoiseModel:
    """Superconducting inspired noise.

    As defined in "A Fault-Tolerant Honeycomb Memory" https://arxiv.org/abs/2108.10457

    Small tweak when measurements aren't immediately followed by a reset: the measurement result
    is probabilistically flipped instead of the input qubit. The input qubit is depolarized
    after the measurement.
    """
```

<a name="stimflow.NoiseModel.uniform_depolarizing"></a>
```python
# stimflow.NoiseModel.uniform_depolarizing

# (in class stimflow.NoiseModel)
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
```

<a name="stimflow.NoiseRule"></a>
```python
# stimflow.NoiseRule

# (at top-level in the stimflow module)
class NoiseRule:
    """Describes how to add noise to an operation.
    """
```

<a name="stimflow.NoiseRule.__init__"></a>
```python
# stimflow.NoiseRule.__init__

# (in class stimflow.NoiseRule)
def __init__(
    self,
    *,
    before: dict[str, float | tuple[float, ...]] | None = None,
    after: dict[str, float | tuple[float, ...]] | None = None,
    flip_result: float = 0,
):
    """

    Args:
        after: A dictionary mapping noise rule names to their probability argument.
            For example, {"DEPOLARIZE2": 0.01, "X_ERROR": 0.02} will add two qubit
            depolarization with parameter 0.01 and also add 2% bit flip noise. These
            noise channels occur after all other operations in the moment and are applied
            to the same targets as the relevant operation.
        flip_result: The probability that a measurement result should be reported incorrectly.
            Only valid when applied to operations that produce measurement results.
    """
```

<a name="stimflow.NoiseRule.append_noisy_version_of"></a>
```python
# stimflow.NoiseRule.append_noisy_version_of

# (in class stimflow.NoiseRule)
def append_noisy_version_of(
    self,
    *,
    split_op: stim.CircuitInstruction,
    out_during_moment: stim.Circuit,
    before_moments: collections.defaultdict[Any, stim.Circuit],
    after_moments: collections.defaultdict[Any, stim.Circuit],
    immune_qubit_indices: Set[int],
) -> None:
```

<a name="stimflow.Patch"></a>
```python
# stimflow.Patch

# (at top-level in the stimflow module)
class Patch:
    """A collection of annotated stabilizers.
    """
```

<a name="stimflow.Patch.data_set"></a>
```python
# stimflow.Patch.data_set

# (in class stimflow.Patch)
class data_set:
    """Returns the set of all data qubits used by tiles in the patch.
    """
```

<a name="stimflow.Patch.m2tile"></a>
```python
# stimflow.Patch.m2tile

# (in class stimflow.Patch)
class m2tile:
```

<a name="stimflow.Patch.measure_set"></a>
```python
# stimflow.Patch.measure_set

# (in class stimflow.Patch)
class measure_set:
    """Returns the set of all measure qubits used by tiles in the patch.
    """
```

<a name="stimflow.Patch.partitioned_tiles"></a>
```python
# stimflow.Patch.partitioned_tiles

# (in class stimflow.Patch)
class partitioned_tiles:
    """Returns the tiles of the patch, but split into non-overlapping groups.
    """
```

<a name="stimflow.Patch.to_svg"></a>
```python
# stimflow.Patch.to_svg

# (in class stimflow.Patch)
def to_svg(
    self,
    *,
    title: str | list[str] | None = None,
    other: Patch | StabilizerCode | Iterable[Patch | StabilizerCode] = (),
    show_order: bool = False,
    show_measure_qubits: bool = False,
    show_data_qubits: bool = True,
    system_qubits: Iterable[complex] = (),
    show_coords: bool = True,
    opacity: float = 1,
    show_obs: bool = False,
    rows: int | None = None,
    cols: int | None = None,
    tile_color_func: Callable[[Tile], str] | None = None,
) -> str_svg:
```

<a name="stimflow.Patch.used_set"></a>
```python
# stimflow.Patch.used_set

# (in class stimflow.Patch)
class used_set:
    """Returns the set of all data and measure qubits used by tiles in the patch.
    """
```

<a name="stimflow.Patch.with_edits"></a>
```python
# stimflow.Patch.with_edits

# (in class stimflow.Patch)
def with_edits(
    self,
    *,
    tiles: Iterable[Tile] | None = None,
) -> Patch:
```

<a name="stimflow.Patch.with_only_x_tiles"></a>
```python
# stimflow.Patch.with_only_x_tiles

# (in class stimflow.Patch)
def with_only_x_tiles(
    self,
) -> Patch:
```

<a name="stimflow.Patch.with_only_y_tiles"></a>
```python
# stimflow.Patch.with_only_y_tiles

# (in class stimflow.Patch)
def with_only_y_tiles(
    self,
) -> Patch:
```

<a name="stimflow.Patch.with_only_z_tiles"></a>
```python
# stimflow.Patch.with_only_z_tiles

# (in class stimflow.Patch)
def with_only_z_tiles(
    self,
) -> Patch:
```

<a name="stimflow.Patch.with_remaining_degrees_of_freedom_as_logicals"></a>
```python
# stimflow.Patch.with_remaining_degrees_of_freedom_as_logicals

# (in class stimflow.Patch)
def with_remaining_degrees_of_freedom_as_logicals(
    self,
) -> StabilizerCode:
    """Solves for the logical observables, given only the stabilizers.
    """
```

<a name="stimflow.Patch.with_transformed_bases"></a>
```python
# stimflow.Patch.with_transformed_bases

# (in class stimflow.Patch)
def with_transformed_bases(
    self,
    basis_transform: "Callable[[Literal['X, 'Y, 'Z']], Literal['X, 'Y, 'Z']]",
) -> Patch:
```

<a name="stimflow.Patch.with_transformed_coords"></a>
```python
# stimflow.Patch.with_transformed_coords

# (in class stimflow.Patch)
def with_transformed_coords(
    self,
    coord_transform: Callable[[complex], complex],
) -> Patch:
```

<a name="stimflow.Patch.with_xz_flipped"></a>
```python
# stimflow.Patch.with_xz_flipped

# (in class stimflow.Patch)
def with_xz_flipped(
    self,
) -> Patch:
```

<a name="stimflow.PauliMap"></a>
```python
# stimflow.PauliMap

# (at top-level in the stimflow module)
class PauliMap:
    """An immutable qubit-to-pauli mapping.

    Similar to a stim.PauliString, but sparse instead of dense and also PauliMap
    doesn't track signs (i.e. X*Y produces Z instead of i*Z).

    The mapping can also be given a name. In some contexts, stimflow requires that Pauli mappings
    have a name (e.g. when specifying the Pauli mapping of a logical operator for a stabilizer code).

    Examples:
        >>> import stimflow as sf
        >>> p1 = sf.PauliMap({0: "X", 1: "Y", 2: "Z"})
        >>> p2 = sf.PauliMap.from_xs([1, 2, 3])
        >>> p3 = sf.PauliMap({"Z": [3, 4j]})
        >>> print(p1 * p2 * p3)
        X0*Z4j*Z1*Y2*Y3
    """
```

<a name="stimflow.PauliMap.__init__"></a>
```python
# stimflow.PauliMap.__init__

# (in class stimflow.PauliMap)
def __init__(
    self,
    mapping: "dict[complex, Literal['X, 'Y, 'Z'] | str] | dict[Literal['X, 'Y, 'Z'] | str, complex | Iterable[complex]] | PauliMap | Tile | stim.PauliString | None" = None,
    *,
    obs_name: Any = None,
):
    """Initializes a PauliMap using maps of Paulis to/from qubits.

    Args:
        mapping: The association between qubits and paulis, specifiable in a variety of ways.
        obs_name: Defaults to None (no name). Can be set to an arbitrary hashable equatable value,
            in order to identify the Pauli map. A common convention used in the library is that
            named Pauli maps correspond to logical operators.

    Examples:
        >>> import stimflow as sf
        >>> import stim

        >>> print(sf.PauliMap())
        I

        >>> print(sf.PauliMap({0: "X", 1: "Y", 2: "Z"}))
        X0*Y1*Z2

        >>> print(sf.PauliMap({"X": [1, 2], "Y": 1+1j}))
        X1*Y(1+1j)*X2

        >>> print(sf.PauliMap(stim.PauliString("XYZ_X")))
        X0*Y1*Z2*X4

        >>> print(sf.PauliMap(sf.Tile(data_qubits=[1, 2, 3], bases="X")))
        X1*X2*X3

        >>> print(sf.PauliMap({0: "X", "Y": [0, 1]}))
        Z0*Y1

        >>> print(sf.PauliMap({0: "X", 1: "Y", 2: "Z"}, obs_name="test"))
        (obs_name='test') X0*Y1*Z2
    """
```

<a name="stimflow.PauliMap.anticommutes"></a>
```python
# stimflow.PauliMap.anticommutes

# (in class stimflow.PauliMap)
def anticommutes(
    self,
    other: PauliMap,
) -> bool:
    """Determines if the pauli map anticommutes with another pauli map.
    """
```

<a name="stimflow.PauliMap.commutes"></a>
```python
# stimflow.PauliMap.commutes

# (in class stimflow.PauliMap)
def commutes(
    self,
    other: PauliMap,
) -> bool:
    """Determines if the pauli map commutes with another pauli map.
    """
```

<a name="stimflow.PauliMap.from_xs"></a>
```python
# stimflow.PauliMap.from_xs

# (in class stimflow.PauliMap)
def from_xs(
    xs: Iterable[complex],
    *,
    obs_name: Any = None,
) -> PauliMap:
    """Returns a PauliMap mapping the given qubits to the X basis.
    """
```

<a name="stimflow.PauliMap.from_ys"></a>
```python
# stimflow.PauliMap.from_ys

# (in class stimflow.PauliMap)
def from_ys(
    ys: Iterable[complex],
    *,
    obs_name: Any = None,
) -> PauliMap:
    """Returns a PauliMap mapping the given qubits to the Y basis.
    """
```

<a name="stimflow.PauliMap.from_zs"></a>
```python
# stimflow.PauliMap.from_zs

# (in class stimflow.PauliMap)
def from_zs(
    zs: Iterable[complex],
    *,
    obs_name: Any = None,
) -> PauliMap:
    """Returns a PauliMap mapping the given qubits to the Z basis.
    """
```

<a name="stimflow.PauliMap.get"></a>
```python
# stimflow.PauliMap.get

# (in class stimflow.PauliMap)
def get(
    self,
    key: complex,
    default: Any = None,
) -> Any:
```

<a name="stimflow.PauliMap.items"></a>
```python
# stimflow.PauliMap.items

# (in class stimflow.PauliMap)
def items(
    self,
) -> "Iterable[tuple[complex, Literal['X', 'Y', 'Z']]]":
    """Returns the (qubit, basis) pairs of the PauliMap.
    """
```

<a name="stimflow.PauliMap.keys"></a>
```python
# stimflow.PauliMap.keys

# (in class stimflow.PauliMap)
def keys(
    self,
) -> Set[complex]:
    """Returns the qubits of the PauliMap.
    """
```

<a name="stimflow.PauliMap.to_stim_pauli_string"></a>
```python
# stimflow.PauliMap.to_stim_pauli_string

# (in class stimflow.PauliMap)
def to_stim_pauli_string(
    self,
    q2i: dict[complex, int],
    *,
    num_qubits: int | None = None,
) -> stim.PauliString:
    """Converts into a stim.PauliString.
    """
```

<a name="stimflow.PauliMap.to_stim_targets"></a>
```python
# stimflow.PauliMap.to_stim_targets

# (in class stimflow.PauliMap)
def to_stim_targets(
    self,
    q2i: dict[complex, int],
) -> list[stim.GateTarget]:
    """Converts into a stim combined pauli target like 'X1*Y2*Z3'.
    """
```

<a name="stimflow.PauliMap.to_tile"></a>
```python
# stimflow.PauliMap.to_tile

# (in class stimflow.PauliMap)
def to_tile(
    self,
) -> Tile:
    """Converts the PauliMap into a stimflow.Tile.
    """
```

<a name="stimflow.PauliMap.values"></a>
```python
# stimflow.PauliMap.values

# (in class stimflow.PauliMap)
def values(
    self,
) -> "Iterable[Literal['X', 'Y', 'Z']]":
    """Returns the bases used by the PauliMap.
    """
```

<a name="stimflow.PauliMap.with_basis"></a>
```python
# stimflow.PauliMap.with_basis

# (in class stimflow.PauliMap)
def with_basis(
    self,
    basis: "Literal['X, 'Y, 'Z']",
) -> PauliMap:
    """Returns the same PauliMap, but with all its qubits mapped to the given basis.
    """
```

<a name="stimflow.PauliMap.with_obs_name"></a>
```python
# stimflow.PauliMap.with_obs_name

# (in class stimflow.PauliMap)
def with_obs_name(
    self,
    name: Any,
) -> PauliMap:
    """Returns the same PauliMap, but with the given name.

    Names are used to identify logical operators.
    """
```

<a name="stimflow.PauliMap.with_transformed_coords"></a>
```python
# stimflow.PauliMap.with_transformed_coords

# (in class stimflow.PauliMap)
def with_transformed_coords(
    self,
    transform: Callable[[complex], complex],
) -> PauliMap:
    """Returns the same PauliMap but with coordinates transformed by the given function.
    """
```

<a name="stimflow.PauliMap.with_xy_flipped"></a>
```python
# stimflow.PauliMap.with_xy_flipped

# (in class stimflow.PauliMap)
def with_xy_flipped(
    self,
) -> PauliMap:
    """Returns the same PauliMap, but with all qubits conjugated by H_XY.
    """
```

<a name="stimflow.PauliMap.with_xz_flipped"></a>
```python
# stimflow.PauliMap.with_xz_flipped

# (in class stimflow.PauliMap)
def with_xz_flipped(
    self,
) -> PauliMap:
    """Returns the same PauliMap, but with all qubits conjugated by H.
    """
```

<a name="stimflow.StabilizerCode"></a>
```python
# stimflow.StabilizerCode

# (at top-level in the stimflow module)
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
```

<a name="stimflow.StabilizerCode.__init__"></a>
```python
# stimflow.StabilizerCode.__init__

# (in class stimflow.StabilizerCode)
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
```

<a name="stimflow.StabilizerCode.as_interface"></a>
```python
# stimflow.StabilizerCode.as_interface

# (in class stimflow.StabilizerCode)
def as_interface(
    self,
) -> stimflow.ChunkInterface:
```

<a name="stimflow.StabilizerCode.concat_over"></a>
```python
# stimflow.StabilizerCode.concat_over

# (in class stimflow.StabilizerCode)
def concat_over(
    self,
    under: StabilizerCode,
    *,
    skip_inner_stabilizers: bool = False,
) -> StabilizerCode:
    """Computes the interleaved concatenation of two stabilizer codes.
    """
```

<a name="stimflow.StabilizerCode.data_set"></a>
```python
# stimflow.StabilizerCode.data_set

# (in class stimflow.StabilizerCode)
class data_set:
```

<a name="stimflow.StabilizerCode.find_distance"></a>
```python
# stimflow.StabilizerCode.find_distance

# (in class stimflow.StabilizerCode)
def find_distance(
    self,
    *,
    max_search_weight: int,
) -> int:
```

<a name="stimflow.StabilizerCode.find_logical_error"></a>
```python
# stimflow.StabilizerCode.find_logical_error

# (in class stimflow.StabilizerCode)
def find_logical_error(
    self,
    *,
    max_search_weight: int,
    return_stim_explained_error: bool = False,
) -> PauliMap | list[stim.ExplainedError]:
```

<a name="stimflow.StabilizerCode.flat_logicals"></a>
```python
# stimflow.StabilizerCode.flat_logicals

# (in class stimflow.StabilizerCode)
class flat_logicals:
    """Returns a list of the logical operators defined by the stabilizer code.

    It's "flat" because paired X/Z logicals are returned separately instead of
    as a tuple.
    """
```

<a name="stimflow.StabilizerCode.from_patch_with_inferred_observables"></a>
```python
# stimflow.StabilizerCode.from_patch_with_inferred_observables

# (in class stimflow.StabilizerCode)
def from_patch_with_inferred_observables(
    patch: Patch,
) -> StabilizerCode:
```

<a name="stimflow.StabilizerCode.get_observable_by_basis"></a>
```python
# stimflow.StabilizerCode.get_observable_by_basis

# (in class stimflow.StabilizerCode)
def get_observable_by_basis(
    self,
    index: int,
    basis: "Literal['X, 'Y, 'Z'] | str",
    *,
    default: Any = '__!not_specified,
) -> PauliMap:
```

<a name="stimflow.StabilizerCode.list_pure_basis_observables"></a>
```python
# stimflow.StabilizerCode.list_pure_basis_observables

# (in class stimflow.StabilizerCode)
def list_pure_basis_observables(
    self,
    basis: "Literal['X, 'Y, 'Z']",
) -> list[PauliMap]:
```

<a name="stimflow.StabilizerCode.make_code_capacity_circuit"></a>
```python
# stimflow.StabilizerCode.make_code_capacity_circuit

# (in class stimflow.StabilizerCode)
def make_code_capacity_circuit(
    self,
    *,
    noise: float | NoiseRule,
    metadata_func: Callable[[stimflow.Flow], stimflow.FlowMetadata] = lambda _: FlowMetadata(),
) -> stim.Circuit:
    """Produces a code capacity noisy memory experiment circuit for the stabilizer code.
    """
```

<a name="stimflow.StabilizerCode.make_phenom_circuit"></a>
```python
# stimflow.StabilizerCode.make_phenom_circuit

# (in class stimflow.StabilizerCode)
def make_phenom_circuit(
    self,
    *,
    noise: float | NoiseRule,
    rounds: int,
    metadata_func: Callable[[stimflow.Flow], stimflow.FlowMetadata] = lambda _: FlowMetadata(),
) -> stim.Circuit:
    """Produces a phenomenological noise memory experiment circuit for the stabilizer code.
    """
```

<a name="stimflow.StabilizerCode.measure_set"></a>
```python
# stimflow.StabilizerCode.measure_set

# (in class stimflow.StabilizerCode)
class measure_set:
```

<a name="stimflow.StabilizerCode.patch"></a>
```python
# stimflow.StabilizerCode.patch

# (in class stimflow.StabilizerCode)
@property
def patch(
    self,
):
    """Returns the stimflow.Patch storing the stabilizers of the code.
    """
```

<a name="stimflow.StabilizerCode.physical_to_logical"></a>
```python
# stimflow.StabilizerCode.physical_to_logical

# (in class stimflow.StabilizerCode)
def physical_to_logical(
    self,
    ps: stim.PauliString,
) -> PauliMap:
    """Maps a physical qubit string into a logical qubit string.

    Requires that all logicals be specified as X/Z tuples.
    """
```

<a name="stimflow.StabilizerCode.tiles"></a>
```python
# stimflow.StabilizerCode.tiles

# (in class stimflow.StabilizerCode)
@property
def tiles(
    self,
):
    """Returns the tiles of the code's stabilizer patch.
    """
```

<a name="stimflow.StabilizerCode.to_svg"></a>
```python
# stimflow.StabilizerCode.to_svg

# (in class stimflow.StabilizerCode)
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
    tile_color_func: Callable[[stimflow.Tile], str | tuple[float, float, float] | tuple[float, float, float, float] | None] | None = None,
    rows: int | None = None,
    cols: int | None = None,
    find_logical_err_max_weight: int | None = None,
    stabilizer_style: "Literal['polygon, 'circles'] | None" = 'polygon,
    observable_style: "Literal['label, 'polygon, 'circles']" = 'label,
) -> str_svg:
    """Returns an SVG diagram of the stabilizer code.
    """
```

<a name="stimflow.StabilizerCode.transversal_init_chunk"></a>
```python
# stimflow.StabilizerCode.transversal_init_chunk

# (in class stimflow.StabilizerCode)
def transversal_init_chunk(
    self,
    *,
    basis: "Literal['X, 'Y, 'Z'] | str | stimflow.PauliMap | dict[complex, str | Literal['X, 'Y, 'Z']]",
) -> stimflow.Chunk:
    """Returns a chunk that describes initializing the stabilizer code with given reset bases.

    Stabilizers that anticommute with the resets will be discarded flows.

    The returned chunk isn't guaranteed to be fault tolerant.
    """
```

<a name="stimflow.StabilizerCode.transversal_measure_chunk"></a>
```python
# stimflow.StabilizerCode.transversal_measure_chunk

# (in class stimflow.StabilizerCode)
def transversal_measure_chunk(
    self,
    *,
    basis: "Literal['X, 'Y, 'Z'] | str | stimflow.PauliMap | dict[complex, str | Literal['X, 'Y, 'Z']]",
) -> stimflow.Chunk:
    """Returns a chunk that describes measuring the stabilizer code with given measure bases.

    Stabilizers that anticommute with the measurements will be discarded flows.

    The returned chunk isn't guaranteed to be fault tolerant.
    """
```

<a name="stimflow.StabilizerCode.used_set"></a>
```python
# stimflow.StabilizerCode.used_set

# (in class stimflow.StabilizerCode)
class used_set:
```

<a name="stimflow.StabilizerCode.verify"></a>
```python
# stimflow.StabilizerCode.verify

# (in class stimflow.StabilizerCode)
def verify(
    self,
) -> None:
    """Verifies observables and stabilizers relate as a stabilizer code.

    All stabilizers should commute with each other.
    All stabilizers should commute with all observables.
    Same-index X and Z observables should anti-commute.
    All other observable pairs should commute.
    """
```

<a name="stimflow.StabilizerCode.verify_distance_is_at_least"></a>
```python
# stimflow.StabilizerCode.verify_distance_is_at_least

# (in class stimflow.StabilizerCode)
def verify_distance_is_at_least(
    self,
    minimum_distance: int,
):
```

<a name="stimflow.StabilizerCode.with_edits"></a>
```python
# stimflow.StabilizerCode.with_edits

# (in class stimflow.StabilizerCode)
def with_edits(
    self,
    *,
    stabilizers: Iterable[Tile | PauliMap] | Patch | None = None,
    logicals: Iterable[PauliMap | tuple[PauliMap, PauliMap]] | None = None,
) -> StabilizerCode:
```

<a name="stimflow.StabilizerCode.with_integer_coordinates"></a>
```python
# stimflow.StabilizerCode.with_integer_coordinates

# (in class stimflow.StabilizerCode)
def with_integer_coordinates(
    self,
) -> StabilizerCode:
    """Returns an equivalent stabilizer code, but with all qubit on Gaussian integers.
    """
```

<a name="stimflow.StabilizerCode.with_observables_from_basis"></a>
```python
# stimflow.StabilizerCode.with_observables_from_basis

# (in class stimflow.StabilizerCode)
def with_observables_from_basis(
    self,
    basis: "Literal['X, 'Y, 'Z']",
) -> StabilizerCode:
```

<a name="stimflow.StabilizerCode.with_remaining_degrees_of_freedom_as_logicals"></a>
```python
# stimflow.StabilizerCode.with_remaining_degrees_of_freedom_as_logicals

# (in class stimflow.StabilizerCode)
def with_remaining_degrees_of_freedom_as_logicals(
    self,
) -> StabilizerCode:
    """Solves for the logical observables, given only the stabilizers.
    """
```

<a name="stimflow.StabilizerCode.with_transformed_coords"></a>
```python
# stimflow.StabilizerCode.with_transformed_coords

# (in class stimflow.StabilizerCode)
def with_transformed_coords(
    self,
    coord_transform: Callable[[complex], complex],
) -> StabilizerCode:
    """Returns the same stabilizer code, but with coordinates transformed by the given
    function.
    """
```

<a name="stimflow.StabilizerCode.with_xz_flipped"></a>
```python
# stimflow.StabilizerCode.with_xz_flipped

# (in class stimflow.StabilizerCode)
def with_xz_flipped(
    self,
) -> StabilizerCode:
    """Returns the same stabilizer code, but with all qubits Hadamard conjugated.
    """
```

<a name="stimflow.StabilizerCode.x_basis_subset"></a>
```python
# stimflow.StabilizerCode.x_basis_subset

# (in class stimflow.StabilizerCode)
def x_basis_subset(
    self,
) -> StabilizerCode:
```

<a name="stimflow.StabilizerCode.z_basis_subset"></a>
```python
# stimflow.StabilizerCode.z_basis_subset

# (in class stimflow.StabilizerCode)
def z_basis_subset(
    self,
) -> StabilizerCode:
```

<a name="stimflow.StimCircuitLoom"></a>
```python
# stimflow.StimCircuitLoom

# (at top-level in the stimflow module)
class StimCircuitLoom:
    """Class for combining stim circuits running in parallel at separate locations.

    for standard usage, call StimCircuitLoom.weave(...), which returns the weaved circuit
    for usage details, see the docstring to that function

    for complex usage, you can instantiate a loom StimCircuitLoom(...)
    This is lets you access details of the weaving afterward, such as the measurement mapping
    """
```

<a name="stimflow.StimCircuitLoom.weave"></a>
```python
# stimflow.StimCircuitLoom.weave

# (in class stimflow.StimCircuitLoom)
class weave:
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
```

<a name="stimflow.StimCircuitLoom.weaved_target_rec_from_c0"></a>
```python
# stimflow.StimCircuitLoom.weaved_target_rec_from_c0

# (in class stimflow.StimCircuitLoom)
def weaved_target_rec_from_c0(
    self,
    target_rec: int,
) -> int:
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
```

<a name="stimflow.StimCircuitLoom.weaved_target_rec_from_c1"></a>
```python
# stimflow.StimCircuitLoom.weaved_target_rec_from_c1

# (in class stimflow.StimCircuitLoom)
def weaved_target_rec_from_c1(
    self,
    target_rec: int,
) -> int:
    """given a target rec in circuit_1, return the equiv rec in the weaved circuit.
    """
```

<a name="stimflow.TextDataFor3DModel"></a>
```python
# stimflow.TextDataFor3DModel

# (at top-level in the stimflow module)
class TextDataFor3DModel:
    """Details about text to draw in a 3d model.

    The intent is to draw the text as a filled rectangle containing the text.
    The data specifies the orientation of the rectangle and the text to place inside of it.

    Example:
        >>> import stimflow as sf
        >>> hello_banner = sf.TextDataFor3DModel(
        ...     text='hello',
        ...     start=(0, 0, 0),
        ...     forward=(1, 0, 0),
        ...     up=(0, 1, 0),
        ... )
        >>> model = sf.make_3d_model([hello_banner])
        >>> assert model.html_viewer() is not None
    """
```

<a name="stimflow.TextDataFor3DModel.__init__"></a>
```python
# stimflow.TextDataFor3DModel.__init__

# (in class stimflow.TextDataFor3DModel)
def __init__(
    self,
    *,
    text: str,
    start: tuple[float, float, float] | Sequence[float],
    forward: tuple[float, float, float] | Sequence[float],
    up: tuple[float, float, float] | Sequence[float],
    mirror_backside: bool = True,
):
    """Describes a rectangle showing text.

    Args:
        text: The text to draw in the rectangle.
        start: The 3d point where the rectangle and text starts.
            This is the `bottom_left` of the rectangle, in 3d.
        forward: The 3d direction along which the text grows as the message gets longer.
            This is the `bottom_right - bottom_left` of the rectangle, in 3d.
            The length of this vector is ignored; the length of the rectangle is determined
            automatically from the desired text.
        up: The 3d direction along which the text is oriented.
            This is the `top_left - bottom_left` of the rectangle, in 3d.
            The length of this vector is ignored; the height of the rectangle is determined
            automatically from the text.
            Should be perpendicular to `forward`.
        mirror_backside: Determines whether the text on the back of the rectangle
            is mirrored (making it readable) or not (keeping the forward direction consistent).
            Defaults to True (readable on both sides).
    """
```

<a name="stimflow.Tile"></a>
```python
# stimflow.Tile

# (at top-level in the stimflow module)
class Tile:
    """A stabilizer with some associated metadata.

    The exact meaning of the tile's fields are often context dependent. For example,
    different circuits will use the measure qubit in different ways (or not at all)
    and the flags could be essentially anything at all. Tile is intended to be useful
    as an intermediate step in the production of a circuit.

    For example, it's much easier to create a color code circuit when you have a list
    of the hexagonal and trapezoidal shapes making up the color code. So it's natural to
    split the color code circuit generation problem into two steps: (1) making the shapes
    then (2) making the circuit given the shapes. In other words, deal with the spatial
    complexities first then deal with the temporal complexities second. The Tile class
    is a reasonable representation for the shapes, because:

    - The X/Z basis of the stabilizer can be stored in the `bases` field.
    - The red/green/blue coloring can be stored as flags.
    - The ancilla qubits for the shapes be stored as measure_qubit values.
    - You can get diagrams of the shapes by passing the tiles into a `stimflow.Patch`.
    - You can verify the tiles form a code by passing the patch into a `stimflow.StabilizerCode`.
    """
```

<a name="stimflow.Tile.__init__"></a>
```python
# stimflow.Tile.__init__

# (in class stimflow.Tile)
def __init__(
    self,
    *,
    bases: str,
    data_qubits: Iterable[complex | None],
    measure_qubit: complex | None = None,
    flags: Iterable[str] = (),
):
    """

    Args:
        bases: Basis of the stabilizer. A string of XYZ characters the same
            length as the data_qubits argument. It is permitted to
            give a single-character string, which will automatically be
            expanded to the full length. For example, 'X' will become 'XXXX'
            if there are four data qubits.
        measure_qubit: The ancilla qubit used to measure the stabilizer.
        data_qubits: The data qubits in the stabilizer, in the order
            that they are interacted with. Some entries may be None,
            indicating that no data qubit is interacted with during the
            corresponding interaction layer.
    """
```

<a name="stimflow.Tile.basis"></a>
```python
# stimflow.Tile.basis

# (in class stimflow.Tile)
class basis:
```

<a name="stimflow.Tile.center"></a>
```python
# stimflow.Tile.center

# (in class stimflow.Tile)
def center(
    self,
) -> complex:
```

<a name="stimflow.Tile.data_set"></a>
```python
# stimflow.Tile.data_set

# (in class stimflow.Tile)
class data_set:
```

<a name="stimflow.Tile.to_pauli_map"></a>
```python
# stimflow.Tile.to_pauli_map

# (in class stimflow.Tile)
def to_pauli_map(
    self,
) -> PauliMap:
```

<a name="stimflow.Tile.used_set"></a>
```python
# stimflow.Tile.used_set

# (in class stimflow.Tile)
class used_set:
```

<a name="stimflow.Tile.with_bases"></a>
```python
# stimflow.Tile.with_bases

# (in class stimflow.Tile)
def with_bases(
    self,
    bases: str,
) -> Tile:
```

<a name="stimflow.Tile.with_basis"></a>
```python
# stimflow.Tile.with_basis

# (in class stimflow.Tile)
def with_basis(
    self,
    bases: str,
) -> Tile:
```

<a name="stimflow.Tile.with_data_qubit_cleared"></a>
```python
# stimflow.Tile.with_data_qubit_cleared

# (in class stimflow.Tile)
def with_data_qubit_cleared(
    self,
    q: complex,
) -> Tile:
```

<a name="stimflow.Tile.with_edits"></a>
```python
# stimflow.Tile.with_edits

# (in class stimflow.Tile)
def with_edits(
    self,
    *,
    bases: str | None = None,
    measure_qubit: "complex | None | Literal['unspecified']" = 'unspecified,
    data_qubits: Iterable[complex | None] | None = None,
    flags: Iterable[str] | None = None,
) -> Tile:
```

<a name="stimflow.Tile.with_transformed_bases"></a>
```python
# stimflow.Tile.with_transformed_bases

# (in class stimflow.Tile)
def with_transformed_bases(
    self,
    basis_transform: "Callable[[Literal['X, 'Y, 'Z']], Literal['X, 'Y, 'Z']]",
) -> Tile:
```

<a name="stimflow.Tile.with_transformed_coords"></a>
```python
# stimflow.Tile.with_transformed_coords

# (in class stimflow.Tile)
def with_transformed_coords(
    self,
    coord_transform: Callable[[complex], complex],
) -> Tile:
```

<a name="stimflow.Tile.with_xz_flipped"></a>
```python
# stimflow.Tile.with_xz_flipped

# (in class stimflow.Tile)
def with_xz_flipped(
    self,
) -> Tile:
```

<a name="stimflow.TriangleDataFor3DModel"></a>
```python
# stimflow.TriangleDataFor3DModel

# (at top-level in the stimflow module)
class TriangleDataFor3DModel:
    """Coordinates and colors of triangles to draw in a 3d model.

    Example:
        >>> import stimflow as sf
        >>> red_square = sf.TriangleDataFor3DModel(
        ...     rgba=(1, 0, 0, 1),
        ...     triangle_list=[
        ...         # A square made of two triangles.
        ...         [(0, 0, 0), (0, 1, 0), (1, 0, 0)],
        ...         [(1, 1, 0), (0, 1, 0), (1, 0, 0)],
        ...     ],
        ... )
        >>> model = sf.make_3d_model([red_square])
        >>> assert model.html_viewer() is not None
    """
```

<a name="stimflow.TriangleDataFor3DModel.__init__"></a>
```python
# stimflow.TriangleDataFor3DModel.__init__

# (in class stimflow.TriangleDataFor3DModel)
def __init__(
    self,
    *,
    rgba: tuple[float, float, float, float],
    triangle_list: np.ndarray | Iterable[Sequence[Sequence[float]]],
):
    """Triangles with associated color information.

    Args:
        rgba: Red, green, blue, and alpha data to associate with all the triangles.
            Each value should range from 0 to 1.
            (The alpha data is ignored in most viewers, but needed by the 3d model format.)
        triangle_list: A 3d float32 numpy array with shape == (*, 3, 3).
            Axis 0 is the triangle axis (each entry is a triangle).
            Axis 1 is the ABC vertex axis (each entry is a vertex from the triangle).
            Axis 2 is the XYZ coordinate axis (each entry is a coordinate from the vertex).

    Example:
        >>> import stimflow as sf
        >>> red_square = sf.TriangleDataFor3DModel(
        ...     rgba=(1, 0, 0, 1),
        ...     triangle_list=[
        ...         # A square made of two triangles.
        ...         [(0, 0, 0), (0, 1, 0), (1, 0, 0)],
        ...         [(1, 1, 0), (0, 1, 0), (1, 0, 0)],
        ...     ],
        ... )
        >>> model = sf.make_3d_model([red_square])
        >>> assert model.html_viewer() is not None
    """
```

<a name="stimflow.TriangleDataFor3DModel.fused"></a>
```python
# stimflow.TriangleDataFor3DModel.fused

# (in class stimflow.TriangleDataFor3DModel)
def fused(
    data: Iterable[TriangleDataFor3DModel],
) -> list[TriangleDataFor3DModel]:
    """Attempts to combine triangle data instances into fewer instances.
    """
```

<a name="stimflow.TriangleDataFor3DModel.rect"></a>
```python
# stimflow.TriangleDataFor3DModel.rect

# (in class stimflow.TriangleDataFor3DModel)
def rect(
    *,
    rgba: tuple[float, float, float, float],
    origin: Iterable[float],
    d1: Iterable[float],
    d2: Iterable[float],
) -> TriangleDataFor3DModel:
    """Creates a pair of triangles forming a rectangle.

    Args:
        rgba: Color of the rectangle.
        origin: Bottom-left corner of the rectangle.
        d1: The right - left displacement.
        d2: The top - bottom displacement.
    """
```

<a name="stimflow.Viewable3dModelGLTF"></a>
```python
# stimflow.Viewable3dModelGLTF

# (at top-level in the stimflow module)
@dataclasses.dataclass
class Viewable3dModelGLTF:
    """A pygltflib.GLTF2 augmented with the ability to create a simple 3d viewer for the model.
    """
    extensions: Optional[Dict[str, Any]]
    extras: Optional[Dict[str, Any]]
    accessors: List[pygltflib.Accessor]
    animations: List[pygltflib.Animation]
    asset: <class 'pygltflib.Asset'>
    bufferViews: List[pygltflib.BufferView]
    buffers: List[pygltflib.Buffer]
    cameras: List[pygltflib.Camera]
    extensionsUsed: List[str]
    extensionsRequired: List[str]
    images: List[pygltflib.Image]
    materials: List[pygltflib.Material]
    meshes: List[pygltflib.Mesh]
    nodes: List[pygltflib.Node]
    samplers: List[pygltflib.Sampler]
    scene: <class 'int'> = None
    scenes: List[pygltflib.Scene]
    skins: List[pygltflib.Skin]
    textures: List[pygltflib.Texture]
```

<a name="stimflow.Viewable3dModelGLTF.html_viewer"></a>
```python
# stimflow.Viewable3dModelGLTF.html_viewer

# (in class stimflow.Viewable3dModelGLTF)
def html_viewer(
    self,
) -> str_html:
    """Returns an HTML document that embeds the 3d model within a 3d viewer.
    """
```

<a name="stimflow.append_reindexed_content_to_circuit"></a>
```python
# stimflow.append_reindexed_content_to_circuit

# (at top-level in the stimflow module)
def append_reindexed_content_to_circuit(
    *,
    out_circuit: stim.Circuit,
    content: stim.Circuit,
    qubit_i2i: dict[int, int],
    obs_i2i: "dict[int, int | Literal['discard']]",
    rewrite_detector_time_coordinates: bool = False,
) -> None:
    """Reindexes content and appends it to a circuit.

    Note that QUBIT_COORDS instructions are skipped.

    Args:
        out_circuit: The output circuit. The circuit being edited.
        content: The circuit to be appended to the output circuit.
        qubit_i2i: A dictionary specifying how qubit indices are remapped. Indices outside the
            map are not changed.
        obs_i2i: A dictionary specifying how observable indices are remapped. Indices outside the
            map are not changed.
        rewrite_detector_time_coordinates: Defaults to False. When set to True, SHIFT_COORD and
            DETECTOR instructions are automatically rewritten to track the passage of time without
            using the same detector position twice at the same time.
    """
```

<a name="stimflow.circuit_to_dem_target_measurement_records_map"></a>
```python
# stimflow.circuit_to_dem_target_measurement_records_map

# (at top-level in the stimflow module)
def circuit_to_dem_target_measurement_records_map(
    circuit: stim.Circuit,
) -> dict[stim.DemTarget, list[int]]:
```

<a name="stimflow.circuit_with_xz_flipped"></a>
```python
# stimflow.circuit_with_xz_flipped

# (at top-level in the stimflow module)
def circuit_with_xz_flipped(
    circuit: stim.Circuit,
) -> stim.Circuit:
```

<a name="stimflow.count_measurement_layers"></a>
```python
# stimflow.count_measurement_layers

# (at top-level in the stimflow module)
def count_measurement_layers(
    circuit: stim.Circuit,
) -> int:
```

<a name="stimflow.find_d1_error"></a>
```python
# stimflow.find_d1_error

# (at top-level in the stimflow module)
def find_d1_error(
    obj: stim.Circuit | stim.DetectorErrorModel,
) -> stim.ExplainedError | stim.DemInstruction | None:
```

<a name="stimflow.find_d2_error"></a>
```python
# stimflow.find_d2_error

# (at top-level in the stimflow module)
def find_d2_error(
    obj: stim.Circuit | stim.DetectorErrorModel,
) -> list[stim.ExplainedError] | stim.DetectorErrorModel | None:
```

<a name="stimflow.gate_counts_for_circuit"></a>
```python
# stimflow.gate_counts_for_circuit

# (at top-level in the stimflow module)
def gate_counts_for_circuit(
    circuit: stim.Circuit,
) -> collections.Counter[str]:
    """Determines gates used by a circuit, disambiguating MPP/feedback cases.

    MPP instructions are expanded into what they actually measure, such as
    "MXX" for MPP X1*X2 and "MXYZ" for MPP X4*Y5*Z7.

    Feedback instructions like `CX rec[-1] 0` become the gate "feedback".

    Sweep instructions like `CX sweep[2] 0` become the gate "sweep".
    """
```

<a name="stimflow.gates_used_by_circuit"></a>
```python
# stimflow.gates_used_by_circuit

# (at top-level in the stimflow module)
def gates_used_by_circuit(
    circuit: stim.Circuit,
) -> set[str]:
    """Determines gates used by a circuit, disambiguating MPP/feedback cases.

    MPP instructions are expanded into what they actually measure, such as
    "MXX" for MPP X1*X2 and "MXYZ" for MPP X4*Y5*Z7.

    Feedback instructions like `CX rec[-1] 0` become the gate "feedback".

    Sweep instructions like `CX sweep[2] 0` become the gate "sweep".
    """
```

<a name="stimflow.html_viewer"></a>
```python
# stimflow.html_viewer

# (at top-level in the stimflow module)
def html_viewer(
    obj: stim.Circuit | Any,
    *,
    background: stimflow.Patch | stimflow.StabilizerCode | stimflow.ChunkInterface | dict[int, stimflow.Patch | stimflow.StabilizerCode | stimflow.ChunkInterface] | None = None,
    tile_color_func: Callable[[stimflow.Tile], tuple[float, float, float, float] | tuple[float, float, float] | str] | None = None,
    width: int = 500,
    height: int = 500,
    known_error: Iterable[stim.ExplainedError] | None = None,
) -> str_html:
    """Creates an HTML page for viewing the given object.

    Args:
        obj: The object to be visualized.
        background: Something to draw in the background of the viewer (e.g. the
            stimflow.StabilizerCode implemented by the circuit being viewed).
        tile_color_func: Customizes how stabilizers and other operators are drawn.
        width: The width of the viewer.
        height: The height of the viewer.
        known_error: An error (e.g. returned from stim.Circuit.shortest_graphlike_error)
            to show as part of the object.

    Returns:
        The HTML string (as a stimflow.str_html, which inherits from python's `str`).

        (The result is of type stimflow.str_html so that its viewer is shown automatically
        in Jupyter notebooks, and also for convenience methods like `write_to`.)
    """
```

<a name="stimflow.html_viewer_for_gltf_model"></a>
```python
# stimflow.html_viewer_for_gltf_model

# (at top-level in the stimflow module)
def html_viewer_for_gltf_model(
    model: pygltflib.GLTF2,
) -> str_html:
```

<a name="stimflow.make_3d_model"></a>
```python
# stimflow.make_3d_model

# (at top-level in the stimflow module)
def make_3d_model(
    elements: Iterable[TriangleDataFor3DModel | LineDataFor3DModel | TextDataFor3DModel],
) -> Viewable3dModelGLTF:
    """Creates a 3d model containing the given elements.

    Args:
        elements: A list of objects to include in the model. The list can include triangles
            (TriangleDataFor3DModel), lines (LineDataFor3DModel), and text (TextDataFor3DModel).

    Returns:
        The 3d model, as a `stimflow.gltf_model`.

        `stimflow.gltf_model` inherits from `pygltflib.GLTF2` but adds a `_repr_html_` class
        (creating a 3d viewer in Jupyter notebooks) and a `write_viewer_to` method for
        saving a standalone HTML viewer.

    Example:
        >>> import stimflow as sf
        >>> red_square = sf.TriangleDataFor3DModel(
        ...     rgba=(1, 0, 0, 1),
        ...     triangle_list=[
        ...         # A square made of two triangles.
        ...         [(0, 0, 0), (0, 1, 0), (1, 0, 0)],
        ...         [(1, 1, 0), (0, 1, 0), (1, 0, 0)],
        ...     ],
        ... )
        >>> blue_square_outline = sf.LineDataFor3DModel(
        ...     rgba=(0, 0, 1, 1),
        ...     edge_list=[
        ...         # A square made of four lines.
        ...         [(0, 0, 2), (0, 1, 2)],
        ...         [(0, 1, 2), (1, 1, 2)],
        ...         [(1, 1, 2), (1, 0, 2)],
        ...         [(1, 0, 2), (0, 0, 2)],
        ...     ],
        ... )
        >>> hello_banner = sf.TextDataFor3DModel(
        ...     text='hello',
        ...     start=(0, 0, 5),
        ...     forward=(1, 0, 0),
        ...     up=(0, 1, 0),
        ... )
        >>> model: sf.Viewable3dModelGLTF = sf.make_3d_model([
        ...     red_square,
        ...     hello_banner,
        ...     blue_square_outline,
        ... ])
        >>> viewer: sf.str_html = model.html_viewer()
        >>>
        >>> # This line is commented out so that running doctest doesn't create a file
        >>> # The 'write_to' method writes a file and also announces the written file:// URL to stderr.
        >>> # viewer.write_to('tmp.html')
        >>>
        >>> print(viewer[:162] + "...")
        <!DOCTYPE html>
        <html>
        <head>
          <meta charset="UTF-8" />
        </head>
        <body>
          <a download="model.gltf" id="stim-3d-viewer-download-link" href="data:text/plain;base64,...
    """
```

<a name="stimflow.min_max_complex"></a>
```python
# stimflow.min_max_complex

# (at top-level in the stimflow module)
def min_max_complex(
    coords: Iterable[complex],
    *,
    default: complex | None = None,
) -> tuple[complex, complex]:
    """Computes the bounding box of a collection of complex numbers.

    Args:
        coords: The complex numbers to place a bounding box around.
        default: If no elements are included, the returned minimum and maximum
            will be equal to this value. If this argument isn't set (or is set to None),
            an exception will be raised instead when given an empty collection. The
            default value is not used when coords is not empty.

    Returns:
        A pair of complex values (c_min, c_max) where c_min is the minimum corner of
        the bounding box and c_max is the maximum corner of the bounding box.

    Raises:
        ValueError:
            An empty list of coords was given, and a default value wasn't specified.
    Examples:
        >>> import stimflow as sf
        >>> sf.min_max_complex([1+2j, 2+1j])
        ((1+1j), (2+2j))
        >>> sf.min_max_complex([1+2j, 2+1j, 1+3j])
        ((1+1j), (2+3j))
        >>> sf.min_max_complex([], default=4+3j)
        ((4+3j), (4+3j))
        >>> sf.min_max_complex([1])
        ((1+0j), (1+0j))
        >>> sf.min_max_complex([1, 3, 2])
        ((1+0j), (3+0j))
        >>> sf.min_max_complex([2j, 1j, 3j])
        (1j, 3j)
    """
```

<a name="stimflow.sorted_complex"></a>
```python
# stimflow.sorted_complex

# (at top-level in the stimflow module)
def sorted_complex(
    values: Iterable[complex],
) -> list[complex]:
    """Sorts complex numbers by real then imaginary coordinate.

    Args:
        values: The complex numbers to sort.

    Returns:
        The sorted list.

    Examples:
        >>> import stimflow as sf
        >>> sf.sorted_complex([0, 1, 1j, 1 + 1j])
        [0, 1j, 1, (1+1j)]
    """
```

<a name="stimflow.stim_circuit_with_transformed_coords"></a>
```python
# stimflow.stim_circuit_with_transformed_coords

# (at top-level in the stimflow module)
def stim_circuit_with_transformed_coords(
    circuit: stim.Circuit,
    transform: Callable[[complex], complex],
) -> stim.Circuit:
    """Returns an equivalent circuit, but with the qubit and detector position metadata modified.
    The "position" is assumed to be the first two coordinates. These are mapped to the real and
    imaginary values of a complex number which is then transformed.

    Note that `SHIFT_COORDS` instructions that modify the first two coordinates are not supported.
    This is because supporting them requires flattening loops, or promising that the given
    transformation is affine.

    Args:
        circuit: The circuit with qubits to reposition.
        transform: The transformation to apply to the positions. The positions are given one by one
            to this method, as complex numbers. The method returns the new complex number for the
            position.

    Returns:
        The transformed circuit.
    """
```

<a name="stimflow.stim_circuit_with_transformed_moments"></a>
```python
# stimflow.stim_circuit_with_transformed_moments

# (at top-level in the stimflow module)
def stim_circuit_with_transformed_moments(
    circuit: stim.Circuit,
    *,
    moment_func: Callable[[stim.Circuit], stim.Circuit],
) -> stim.Circuit:
    """Applies a transformation to regions of a circuit separated by TICKs and blocks.

    For example, in this circuit:

        H 0
        X 0
        TICK

        H 1
        X 1
        REPEAT 100 {
            H 2
            X 2
        }
        H 3
        X 3

        TICK
        H 4
        X 4

    `moment_func` would be called five times, each time with one of the H and X instruction pairs.
    The result from the method would then be substituted into the circuit, replacing each of the H
    and X instruction pairs.

    Args:
        circuit: The circuit to return a transformed result of.
        moment_func: The transformation to apply to regions of the circuit. Returns a new circuit
            for the result.

    Returns:
        A transformed circuit.
    """
```

<a name="stimflow.str_html"></a>
```python
# stimflow.str_html

# (at top-level in the stimflow module)
class str_html:
    """A string that will display as an HTML widget in Jupyter notebooks.

    It's expected that the contents of the string will correspond to the
    contents of an HTML file.
    """
```

<a name="stimflow.str_html.write_to"></a>
```python
# stimflow.str_html.write_to

# (in class stimflow.str_html)
def write_to(
    self,
    path: str | pathlib.Path | io.IOBase,
):
    """Write the contents to a file, and announce that it was done.

    This method exists for quick debugging. In many contexts, such as
    in a bash terminal or in PyCharm, the printed path can be clicked
    on to open the file.
    """
```

<a name="stimflow.str_svg"></a>
```python
# stimflow.str_svg

# (at top-level in the stimflow module)
class str_svg:
    """A string that will display as an SVG image in Jupyter notebooks.

    It's expected that the contents of the string will correspond to the
    contents of an SVG file.
    """
```

<a name="stimflow.str_svg.write_to"></a>
```python
# stimflow.str_svg.write_to

# (in class stimflow.str_svg)
def write_to(
    self,
    path: str | pathlib.Path | io.IOBase,
):
    """Write the contents to a file, and announce that it was done.

    This method exists for quick debugging. In many contexts, such as
    in a bash terminal or in PyCharm, the printed path can be clicked
    on to open the file.
    """
```

<a name="stimflow.svg_viewer"></a>
```python
# stimflow.svg_viewer

# (at top-level in the stimflow module)
def svg_viewer(
    obj: stimflow.Patch | stimflow.StabilizerCode | stimflow.ChunkInterface | stim.Circuit | PauliMap | Any | Iterable[stimflow.Patch | stimflow.StabilizerCode | stimflow.ChunkInterface | stim.Circuit | PauliMap | Any],
    *,
    background: stimflow.Patch | stimflow.StabilizerCode | stimflow.ChunkInterface | stim.Circuit | PauliMap | Any | None = None,
    title: str | list[str] | None = None,
    canvas_height: int | None = None,
    show_order: bool = False,
    show_obs: bool = True,
    opacity: float = 1,
    show_measure_qubits: bool = True,
    show_data_qubits: bool = False,
    system_qubits: Iterable[complex] = (),
    show_all_qubits: bool = False,
    extra_used_coords: Iterable[complex] = (),
    show_coords: bool = True,
    find_logical_err_max_weight: int | None = None,
    rows: int | None = None,
    cols: int | None = None,
    tile_color_func: Callable[[stimflow.Tile], str | tuple[float, float, float] | tuple[float, float, float, float] | None] | None = None,
    stabilizer_style: "Literal['polygon, 'circles'] | None" = 'polygon,
    observable_style: "Literal['label, 'polygon, 'circles']" = 'label,
    show_frames: bool = True,
    pad: float | None = None,
) -> stimflow.str_svg:
    """Returns an SVG image of the given objects.
    """
```

<a name="stimflow.transpile_to_z_basis_interaction_circuit"></a>
```python
# stimflow.transpile_to_z_basis_interaction_circuit

# (at top-level in the stimflow module)
def transpile_to_z_basis_interaction_circuit(
    circuit: stim.Circuit,
    *,
    is_entire_circuit: bool = True,
) -> stim.Circuit:
    """Converts to a circuit using CZ, iSWAP, and MZZ as appropriate.

    This method mostly focuses on inserting single qubit rotations to convert
    interactions into their Z basis variant. It also does some optimizations
    that remove redundant rotations which would tend to be introduced by this
    process.
    """
```

<a name="stimflow.transversal_code_transition_chunks"></a>
```python
# stimflow.transversal_code_transition_chunks

# (at top-level in the stimflow module)
def transversal_code_transition_chunks(
    *,
    prev_code: StabilizerCode,
    next_code: StabilizerCode,
    measured: PauliMap,
    reset: PauliMap,
) -> tuple[Chunk, ChunkReflow, Chunk]:
```

<a name="stimflow.verify_distance_is_at_least"></a>
```python
# stimflow.verify_distance_is_at_least

# (at top-level in the stimflow module)
def verify_distance_is_at_least(
    obj: stim.Circuit | stim.DetectorErrorModel | StabilizerCode,
    minimum_distance: int,
):
```

<a name="stimflow.xor_sorted"></a>
```python
# stimflow.xor_sorted

# (at top-level in the stimflow module)
def xor_sorted(
    vals: Iterable[TItem],
    *,
    key: Callable[[TItem], Any] | None = None,
) -> list[TItem]:
    """Sorts items and then cancels pairs of equal items.

    An item will be in the result once if it appeared an odd number of times.
    An item won't be in the result if it appeared an even number of times.

    Args:
        vals: The items to sort.
        key: An optional key function, mapping the items to keys that determine the
            sorted order. Unequal items with the same key don't cancel.

    Examples:
        >>> import stimflow as sf
        >>> sf.xor_sorted([1])
        [1]
        >>> sf.xor_sorted([1, 1])
        []
        >>> sf.xor_sorted([1, 1, 1])
        [1]
        >>> sf.xor_sorted([1, 1, 1, 1])
        []
        >>> sf.xor_sorted([3, 1, 2, 1])
        [2, 3]
        >>> sf.xor_sorted([3, 1, 2, 1, 3])
        [2]
        >>> sf.xor_sorted([5, 4, 3, 2, 1, 4])
        [1, 2, 3, 5]
        >>> sf.xor_sorted([*range(10), *range(2, 6)])
        [0, 1, 6, 7, 8, 9]
        >>> sf.xor_sorted([61, 91, 83, 72, 61], key=lambda e: e % 10)
        [91, 72, 83]
    """
```
