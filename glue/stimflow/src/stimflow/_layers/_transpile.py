from __future__ import annotations

import stim

from stimflow._layers._layer_circuit import LayerCircuit


def transpile_to_z_basis_interaction_circuit(
    circuit: stim.Circuit, *, is_entire_circuit: bool = True
) -> stim.Circuit:
    """Converts to a circuit using CZ, iSWAP, and MZZ as appropriate.

    This method mostly focuses on inserting single qubit rotations to convert
    interactions into their Z basis variant. It also does some optimizations
    that remove redundant rotations which would tend to be introduced by this
    process.
    """
    c = LayerCircuit.from_stim_circuit(circuit)
    c = c.with_qubit_coords_at_start()
    c = c.with_locally_optimized_layers()
    c = c.with_ejected_loop_iterations()
    c = c.with_locally_merged_measure_layers()
    c = c.with_cleaned_up_loop_iterations()
    c = c.to_z_basis()
    c = c.with_rotations_rolled_from_end_of_loop_to_start_of_loop()
    c = c.with_locally_optimized_layers()
    c = c.with_clearable_rotation_layers_cleared()
    c = c.with_rotations_merged_earlier()
    c = c.with_rotations_before_resets_removed()
    if is_entire_circuit:
        c = c.with_irrelevant_tail_layers_removed()
    return c.to_stim_circuit()
