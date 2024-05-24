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

/**
 * @returns {!Map<!string, !{name: undefined|!string, rev_pair: undefined|!boolean, ignore: undefined|!boolean}>}
 */
function make_gate_alias_map() {
    let result = new Map();

    result.set("CNOT", {name: "CX"});
    result.set("MZ", {name: "M"});
    result.set("MRZ", {name: "MR"});
    result.set("RZ", {name: "R"});
    result.set("H_XZ", {name: "H"});
    result.set("SQRT_Z", {name: "S"});
    result.set("SQRT_Z_DAG", {name: "S_DAG"});
    result.set("ZCX", {name: "CX"});
    result.set("ZCY", {name: "CY"});
    result.set("ZCZ", {name: "CZ"});
    result.set("SWAPCZ", {name: "CZSWAP"});

    // Ordered-flipped aliases.
    result.set("XCZ", {name: "CX", rev_pair: true});
    result.set("YCX", {name: "XCY", rev_pair: true});
    result.set("YCZ", {name: "CY", rev_pair: true});
    result.set("SWAPCX", {name: "CXSWAP", rev_pair: true});

    // Noise.
    result.set("CORRELATED_ERROR", {ignore: true});
    result.set("DEPOLARIZE1", {ignore: true});
    result.set("DEPOLARIZE2", {ignore: true});
    result.set("E", {ignore: true});
    result.set("ELSE_CORRELATED_ERROR", {ignore: true});
    result.set("PAULI_CHANNEL_1", {ignore: true});
    result.set("PAULI_CHANNEL_2", {ignore: true});
    result.set("X_ERROR", {ignore: true});
    result.set("Y_ERROR", {ignore: true});
    result.set("Z_ERROR", {ignore: true});
    result.set("HERALDED_ERASE", {ignore: true});
    result.set("HERALDED_PAULI_CHANNEL_1", {ignore: true});

    // Annotations.
    result.set("MPAD", {ignore: true});
    result.set("OBSERVABLE_INCLUDE", {ignore: true});
    result.set("SHIFT_COORDS", {ignore: true});

    return result;
}

const GATE_MAP = /** @type {Map<!string, !Gate>} */ make_gate_map();
const GATE_ALIAS_MAP = /** @type {!Map<!string, !{name: undefined|!string, rev_pair: undefined|!boolean, ignore: undefined|!boolean}>} */ make_gate_alias_map();

export {GATE_MAP, GATE_ALIAS_MAP};
