import {Gate} from "./gate.js"
import {iter_gates_controlled_paulis} from "./gateset_controlled_paulis.js";
import {iter_gates_demolition_measurements} from "./gateset_demolition_measurements.js";
import {iter_gates_hadamard_likes} from "./gateset_hadamard_likes.js";
import {iter_gates_markers} from "./gateset_markers.js";
import {iter_gates_pair_measurements} from "./gateset_pair_measurements.js";
import {iter_gates_paulis} from "./gateset_paulis.js";
import {iter_gates_quarter_turns} from "./gateset_quarter_turns.js";
import {iter_gates_resets} from "./gateset_resets.js";
import {iter_gates_solo_measurements} from "./gateset_solo_measurements.js";
import {iter_gates_sqrt_pauli_pairs} from "./gateset_sqrt_pauli_pairs.js";
import {iter_gates_swaps} from "./gateset_swaps.js";
import {iter_gates_third_turns} from "./gateset_third_turns.js";

function *iter_gates() {
    yield *iter_gates_controlled_paulis();
    yield *iter_gates_demolition_measurements();
    yield *iter_gates_hadamard_likes();
    yield *iter_gates_markers();
    yield *iter_gates_pair_measurements();
    yield *iter_gates_paulis();
    yield *iter_gates_quarter_turns();
    yield *iter_gates_resets();
    yield *iter_gates_solo_measurements();
    yield *iter_gates_sqrt_pauli_pairs();
    yield *iter_gates_swaps();
    yield *iter_gates_third_turns();
}

/**
 * @returns {!Map<!string, !Gate>}
 */
function make_gate_map() {
    let result = new Map();
    for (let gate of iter_gates()) {
        result.set(gate.name, gate);
    }
    return result;
}
const GATE_MAP = /** @type {Map<!string, !Gate>} */ make_gate_map();

export {GATE_MAP};
