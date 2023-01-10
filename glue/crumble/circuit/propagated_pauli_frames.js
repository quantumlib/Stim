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
}

class PropagatedPauliFrames {
    /**
     * @param {!Map<!int, !PropagatedPauliFrameLayer>} layers
     */
    constructor(layers) {
        this.id_layers = layers;
    }

    /**
     * @param {!int} layer
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

            if (bases.size > 0 || errors.size > 0 || crossings.size > 0) {
                result.id_layers.set(k, new PropagatedPauliFrameLayer(bases, errors, crossings));
            }
        }
        return result;
    }
}

export {PropagatedPauliFrames};
