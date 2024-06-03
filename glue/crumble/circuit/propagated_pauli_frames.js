import {PauliFrame} from './pauli_frame.js';
import {equate} from '../base/equate.js';
import {Layer} from './layer.js';
import {Operation} from './operation.js';
import {GATE_MAP} from '../gates/gateset.js';

class PropagatedPauliFrameLayer {
    /**
     * @param {!Map<!int, !string>} bases
     * @param {!Set<!int>} errors
     * @param {!Array<!{q1: !int, q2: !int, color: !string}>} crossings
     */
    constructor(bases, errors, crossings) {
        this.bases = bases;
        this.errors = errors;
        this.crossings = crossings;
    }

    /**
     * @param {!Set<!int>} qids
     * @returns {!boolean}
     */
    touchesQidSet(qids) {
        for (let q of this.bases.keys()) {
            if (qids.has(q)) {
                return true;
            }
        }
        for (let q of this.errors.keys()) {
            if (qids.has(q)) {
                return true;
            }
        }
        return false;
    }

    /**
     * @param {!PropagatedPauliFrameLayer} other
     * @returns {!PropagatedPauliFrameLayer}
     */
    mergedWith(other) {
        return new PropagatedPauliFrameLayer(
            new Map([...this.bases.entries(), ...other.bases.entries()]),
            new Set([...this.errors, ...other.errors]),
            [...this.crossings, ...other.crossings],
        );
    }

    /**
     * @returns {!string}
     */
    toString() {
        let num_qubits = 0;
        for (let q of this.bases.keys()) {
            num_qubits = Math.max(num_qubits, q + 1);
        }
        for (let q of this.errors) {
            num_qubits = Math.max(num_qubits, q + 1);
        }
        for (let [q1, q2] of this.crossings) {
            num_qubits = Math.max(num_qubits, q1 + 1);
            num_qubits = Math.max(num_qubits, q2 + 1);
        }
        let result = '"';
        for (let q = 0; q < num_qubits; q++) {
            let b = this.bases.get(q);
            if (b === undefined) {
                b = '_';
            }
            if (this.errors.has(q)) {
                b = 'E';
            }
            result += b;
        }
        result += '"';
        return result;
    }
}

class PropagatedPauliFrames {
    /**
     * @param {!Map<!int, !PropagatedPauliFrameLayer>} layers
     */
    constructor(layers) {
        this.id_layers = layers;
    }

    /**
     * @param {*} other
     * @returns {!boolean}
     */
    isEqualTo(other) {
        return other instanceof PropagatedPauliFrames && equate(this.id_layers, other.id_layers);
    }

    /**
     * @returns {!string}
     */
    toString() {
        let layers = [...this.id_layers.keys()];
        layers.sort((a, b) => a - b);
        let lines = ['PropagatedPauliFrames {'];
        for (let k of layers) {
            lines.push(`    ${k}: ${this.id_layers.get(k)}`);
        }
        lines.push('}');
        return lines.join('\n');
    }

    /**
     * @param {!int} layer
     * @returns {!PropagatedPauliFrameLayer}
     */
    atLayer(layer) {
        let result = this.id_layers.get(layer);
        if (result === undefined) {
            result = new PropagatedPauliFrameLayer(new Map(), new Set(), []);
        }
        return result;
    }

    /**
     * @param {!Circuit} circuit
     * @param {!int} marker_index
     * @returns {!PropagatedPauliFrames}
     */
    static fromCircuit(circuit, marker_index) {
        let result = new PropagatedPauliFrames(new Map());

        let bases = /** @type {!Map<!int, !string>} */ new Map();
        for (let k = 0; k < circuit.layers.length; k++) {
            let layer = circuit.layers[k];
            let prevBases = bases;
            bases = layer.id_pauliFrameAfter(bases, marker_index);

            let errors = new Set();
            for (let key of [...bases.keys()]) {
                let val = bases.get(key);
                if (val.startsWith('ERR:')) {
                    errors.add(key);
                    bases.set(key, val.substring(4));
                }
                if (bases.get(key) === 'I') {
                    bases.delete(key);
                }
            }

            let crossings = /** @type {!Array<!{q1: !int, q2: !int, color: !string}>} */ [];
            for (let op of layer.iter_gates_and_markers()) {
                if (op.gate.num_qubits === 2 && !op.gate.is_marker) {
                    let [q1, q2] = op.id_targets;
                    let differences = new Set();
                    for (let t of op.id_targets) {
                        let b1 = bases.get(t);
                        let b2 = prevBases.get(t);
                        if (b1 !== b2) {
                            if (b1 !== undefined) {
                                differences.add(b1);
                            }
                            if (b2 !== undefined) {
                                differences.add(b2);
                            }
                        }
                    }
                    if (differences.size > 0) {
                        let color = 'I';
                        if (differences.size === 1) {
                            color = [...differences][0];
                        }
                        crossings.push({q1, q2, color});
                    }
                }
            }

            if (bases.size > 0) {
                result.id_layers.set(k + 0.5, new PropagatedPauliFrameLayer(bases, new Set(), []));
            }
            if (errors.size > 0 || crossings.length > 0) {
                result.id_layers.set(k, new PropagatedPauliFrameLayer(new Map(), errors, crossings));
            }
        }
        return result;
    }

    /**
     * @param {!Circuit} circuit
     * @param {!Array<!int>} measurements
     * @returns {!PropagatedPauliFrames}
     */
    static fromMeasurements(circuit, measurements) {
        let result = new PropagatedPauliFrames(new Map());
        let frame = new PauliFrame(1, circuit.allQubits().size);
        let measurementsBack = 0;

        for (let k = circuit.layers.length - 1; k >= -1; k--) {
            let layer = k >= 0 ? circuit.layers[k] : new Layer();
            let targets = [...layer.id_ops.keys()];
            targets.reverse();

            for (let id of targets) {
                let op = layer.id_ops.get(id);
                if (op.id_targets[0] === id) {
                    let m = op.countMeasurements();
                    frame.undo_gate(op.gate, [...op.id_targets]);
                    measurementsBack -= m;
                    if (m > 0 && measurements.indexOf(measurementsBack) !== -1) {
                        for (let t_id = 0; t_id < op.id_targets.length; t_id++) {
                            let t = op.id_targets[t_id];
                            let basis;
                            if (op.gate.name === 'MX' || op.gate.name === 'MRX' || op.gate.name === 'MXX') {
                                basis = 'X';
                            } else if (op.gate.name === 'MY' || op.gate.name === 'MRY' || op.gate.name === 'MYY') {
                                basis = 'Y';
                            } else if (op.gate.name === 'M' || op.gate.name === 'MR' || op.gate.name === 'MZZ') {
                                basis = 'Z';
                            } else if (op.gate.name === 'MPAD') {
                                continue;
                            } else if (op.gate.name.startsWith('MPP:')) {
                                basis = op.gate.name[t_id + 4];
                            } else {
                                throw new Error('Unhandled measurement gate: ' + op.gate.name);
                            }
                            if (basis === 'X') {
                                frame.xs[t] ^= 1;
                            } else if (basis === 'Y') {
                                frame.xs[t] ^= 1;
                                frame.zs[t] ^= 1;
                            } else if (basis === 'Z') {
                                frame.zs[t] ^= 1;
                            } else {
                                throw new Error('Unhandled measurement gate: ' + op.gate.name);
                            }
                        }
                    }
                }
            }

            let bases = new Map();
            let errors = new Set();
            for (let q = 0; q < frame.xs.length; q++) {
                if (frame.xs[q] || frame.zs[q]) {
                    bases.set(q, '_XZY'[frame.xs[q] + 2*frame.zs[q]]);
                }
                if (frame.flags[q]) {
                    frame.flags[q] = 0;
                    errors.add(q);
                }
            }
            if (bases.size > 0) {
                result.id_layers.set(k - 0.5, new PropagatedPauliFrameLayer(bases, new Set(), []));
            }
             if (errors.size > 0) {
                result.id_layers.set(k, new PropagatedPauliFrameLayer(new Map(), errors, []));
            }
       }
        return result;
    }
}

export {PropagatedPauliFrames, PropagatedPauliFrameLayer};
