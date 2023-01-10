import {Operation} from "./operation.js"

class Layer {
    constructor() {
        this.id_ops = /** @type {!Map<!int, !Operation>} */ new Map();
        this.markers /** @type {!Array<!Operation>} */ = [];
    }

    /**
     * @returns {!Layer}
     */
    copy() {
        let result = new Layer();
        result.id_ops = new Map(this.id_ops);
        result.markers = [...this.markers];
        return result;
    }

    /**
     * @return {!boolean}
     */
    empty() {
        return this.id_ops.size === 0 && this.markers.length === 0;
    }

    /**
     * @param {!function(op: !Operation): !boolean} predicate
     * @returns {!Layer}
     */
    id_filtered(predicate) {
        let newLayer = new Layer();
        for (let op of this.id_ops.values()) {
            if (predicate(op)) {
                newLayer.put(op);
            }
        }
        for (let op of this.markers) {
            if (predicate(op)) {
                newLayer.markers.push(op);
            }
        }
        return newLayer;
    }

    /**
     * @param {!function(qubit: !int): !boolean} predicate
     * @returns {!Layer}
     */
    id_filteredByQubit(predicate) {
        return this.id_filtered(op => !op.id_targets.every(q => !predicate(q)));
    }

    /**
     * @param {!Map<!int, !string>} before
     * @param {!int} marker_index
     * @returns {!Map<!int, !string>}
     */
    id_pauliFrameAfter(before, marker_index) {
        let after = new Map();
        let handled = new Set();

        for (let k of before.keys()) {
            let v = before.get(k);
            let op = this.id_ops.get(k);
            if (op !== undefined) {
                let already_done = false;
                let b = '';
                for (let q of op.id_targets) {
                    if (handled.has(q)) {
                        already_done = true;
                    }
                    handled.add(q);
                    let r = before.get(q);
                    if (r === undefined) {
                        r = 'I';
                    }
                    b += r;
                }
                let a = op.pauliFrameAfter(b);
                let hasErr = a.startsWith('ERR:');
                for (let qi = 0; qi < op.id_targets.length; qi++) {
                    let q = op.id_targets[qi];
                    if (hasErr) {
                        after.set(q, 'ERR:' + a[4 + qi]);
                    } else {
                        after.set(q, a[qi]);
                    }
                }
            } else {
                after.set(k, v);
            }
        }

        for (let op of this.markers) {
            if (op.gate.name === 'MARKX' && op.args[0] === marker_index) {
                let key = op.id_targets[0];
                let pauli = after.get(key);
                if (pauli === undefined || pauli === 'I') {
                    pauli = 'X';
                } else if (pauli === 'X') {
                    pauli = 'I';
                } else if (pauli === 'Y') {
                    pauli = 'Z';
                } else if (pauli === 'Z') {
                    pauli = 'Y';
                }
                after.set(key, pauli);
            } else if (op.gate.name === 'MARKY' && op.args[0] === marker_index) {
                let key = op.id_targets[0];
                let pauli = after.get(key);
                if (pauli === undefined || pauli === 'I') {
                    pauli = 'Y';
                } else if (pauli === 'X') {
                    pauli = 'Z';
                } else if (pauli === 'Y') {
                    pauli = 'I';
                } else if (pauli === 'Z') {
                    pauli = 'X';
                }
                after.set(key, pauli);
            } else if (op.gate.name === 'MARKZ' && op.args[0] === marker_index) {
                let key = op.id_targets[0];
                let pauli = after.get(key);
                if (pauli === undefined || pauli === 'I') {
                    pauli = 'Z';
                } else if (pauli === 'X') {
                    pauli = 'Y';
                } else if (pauli === 'Y') {
                    pauli = 'X';
                } else if (pauli === 'Z') {
                    pauli = 'I';
                }
                after.set(key, pauli);
            }
        }

        return after;
    }

    /**
     * @returns {!boolean}
     */
    isEmpty() {
        return this.id_ops.size === 0 && this.markers.length === 0;
    }

    /**
     * @param {!int} qubit
     * @returns {!Operation|undefined}
     */
    id_pop_at(qubit) {
        this.markers = this.markers.filter(op => op.id_targets.indexOf(qubit) === -1);
        if (this.id_ops.has(qubit)) {
            let op = this.id_ops.get(qubit);
            for (let t of op.id_targets) {
                this.id_ops.delete(t);
            }
            return op;
        }
        return undefined;
    }

    /**
     * @param {!int} q
     * @param {undefined|!int} index
     */
    id_dropMarkersAt(q, index=undefined) {
        this.markers = this.markers.filter(op => {
            if (index !== undefined && op.args[0] !== index) {
                return true;
            }
            if (op.gate.name !== 'MARKX' && op.gate.name !== 'MARKY' && op.gate.name !== 'MARKZ') {
                return true;
            }
            return op.id_targets[0] !== q;
        });
    }

    /**
     * @param {!Operation} op
     * @param {!boolean=true} allow_overwrite
     */
    put(op, allow_overwrite=true) {
        if (op.gate.is_marker) {
            if (op.gate.name === 'MARKX' || op.gate.name === 'MARKY' || op.gate.name === 'MARKZ') {
                this.id_dropMarkersAt(op.id_targets[0], op.args[0]);
            }
            this.markers.push(op);
            return;
        }

        for (let t of op.id_targets) {
            if (this.id_ops.has(t)) {
                if (allow_overwrite) {
                    this.id_pop_at(t);
                } else {
                    throw new Error("Collision");
                }
            }
        }
        for (let t of op.id_targets) {
            this.id_ops.set(t, op);
        }
    }

    /**
     * @returns {!Iterator<!Operation>}
     */
    *iter_gates_and_markers() {
        for (let t of this.id_ops.keys()) {
            let op = this.id_ops.get(t);
            if (op.id_targets[0] === t) {
                yield op;
            }
        }
        yield *this.markers;
    }
}

/**
 * @param {!Iterable<![!number, !number]>} xys
 * @returns {![undefined | !number, undefined | !number]}
 */
function minXY(xys) {
    let minX = undefined;
    let minY = undefined;
    for (let [vx, vy] of xys) {
        if (minX === undefined || vx < minX || (vx === minX && vy < minY)) {
            minX = vx;
            minY = vy;
        }
    }
    return [minX, minY];
}

export {Layer, minXY};
