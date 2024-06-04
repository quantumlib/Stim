import {Gate} from "../gates/gate.js"

/**
 * @param {!string} base
 * @returns {!Array<!string>}
 */
function expandBase(base) {
    let result = [];
    for (let k = 0; k < base.length; k++) {
        let prefix = 'I'.repeat(k);
        let suffix = 'I'.repeat(base.length - k - 1);
        if (base[k] === 'X' || base[k] === 'Y') {
            result.push(prefix + 'X' + suffix);
        }
        if (base[k] === 'Z' || base[k] === 'Y') {
            result.push(prefix + 'Z' + suffix);
        }
    }
    return result;
}

class Operation {
    /**
     * @param {!Gate} gate
     * @param {!Float32Array} args
     * @param {!Uint32Array} targets
     */
    constructor(gate, args, targets) {
        if (!(gate instanceof Gate)) {
            throw new Error('!(gate instanceof Gate)');
        }
        if (!(args instanceof Float32Array)) {
            throw new Error('!(args instanceof Float32Array)');
        }
        if (!(targets instanceof Uint32Array)) {
            throw new Error('!(targets instanceof Uint32Array)');
        }
        this.gate = gate;
        this.args = args;
        this.id_targets = targets;
    }

    /**
     * @returns {!string}
     */
    toString() {
        return `${this.gate.name}(${[...this.args].join(', ')}) ${[...this.id_targets].join(' ')}`;
    }

    /**
     * @returns {!int}
     */
    countMeasurements() {
        if (this.gate.name === 'M' || this.gate.name === 'MX' || this.gate.name === 'MY' || this.gate.name === 'MR' || this.gate.name === 'MRX' || this.gate.name === 'MRY') {
            return this.id_targets.length;
        }
        if (this.gate.name === 'MXX' || this.gate.name === 'MYY' || this.gate.name === 'MZZ') {
            return this.id_targets.length / 2;
        }
        if (this.gate.name.startsWith('MPP:')) {
            return 1;
        }
        return 0;
    }

    /**
     * @param {!string} before
     * @returns {!string}
     */
    pauliFrameAfter(before) {
        let m = this.gate.tableau_map;
        if (m === undefined) {
            if (this.gate.name.startsWith('M')) {
                let bases;
                if (this.gate.name.startsWith('MPP:')) {
                    bases = this.gate.name.substring(4);
                } else {
                    bases = this.gate.name.substring(1);
                }
                let differences = 0;
                for (let k = 0; k < before.length; k++) {
                    let a = 'XYZ'.indexOf(before[k]);
                    let b = 'XYZ'.indexOf(bases[k]);
                    if (a >= 0 && b >= 0 && a !== b) {
                        differences++;
                    }
                }
                if (differences % 2 !== 0) {
                    return 'ERR:' + before;
                }
                return before;
            } else if (this.gate.name.startsWith('SPP:') || this.gate.name.startsWith('SPP_DAG:')) {
                let dag = this.gate.name.startsWith('SPP_DAG:');
                let bases = this.gate.name.substring(dag ? 8 : 4);
                let differences = 0;
                let flipped = '';
                for (let k = 0; k < before.length; k++) {
                    let a = 'IXYZ'.indexOf(before[k]);
                    let b = 'IXYZ'.indexOf(bases[k]);
                    if (a > 0 && b > 0 && a !== b) {
                        differences++;
                    }
                    flipped += 'IXYZ'[a ^ b]
                }
                if (differences % 2 !== 0) {
                    return flipped;
                }
                return before;
            } else if (this.gate.name === 'POLYGON') {
                // Do nothing.
                return before;
            } else {
                throw new Error(this.gate.name);
            }
        }
        if (before.length !== this.gate.num_qubits) {
            throw new Error(`before.length !== this.gate.num_qubits`);
        }
        if (m.has(before)) {
            return m.get(before);
        }
        let bases = expandBase(before);
        bases = bases.map(e => m.get(e));
        let out = [0, 0];
        for (let b of bases) {
            for (let k = 0; k < before.length; k++) {
                if (b[k] === 'X') {
                    out[k] ^= 1;
                }
                if (b[k] === 'Y') {
                    out[k] ^= 3;
                }
                if (b[k] === 'Z') {
                    out[k] ^= 2;
                }
            }
        }
        let result = '';
        for (let k = 0; k < before.length; k++) {
            result += 'IXZY'[out[k]];
        }
        return result;
    }

    /**
     * @param {!function(qubit: !int): ![!number, !number]} qubitCoordsFunc
     * @param {!CanvasRenderingContext2D} ctx
     */
    id_draw(qubitCoordsFunc, ctx) {
        ctx.save();
        try {
            this.gate.drawer(this, qubitCoordsFunc, ctx);
        } finally {
            ctx.restore();
        }
    }
}

export {Operation};
