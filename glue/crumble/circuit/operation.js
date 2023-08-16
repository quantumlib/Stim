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
     * @param {!string} before
     * @returns {!string}
     */
    pauliFrameAfter(before) {
        let m = this.gate.tableau_map;
        if (m === undefined) {
            if (this.gate.name.startsWith('M')) {
                let differences = 0;
                for (let k = 0; k < before.length; k++) {
                    let a = 'XYZ'.indexOf(before[k]);
                    let b = 'XYZ'.indexOf(this.gate.name[k + 1]);
                    if (a >= 0 && b >= 0 && a !== b) {
                        differences++;
                    }
                }
                if (differences % 2 !== 0) {
                    return 'ERR:' + before;
                }
            }
            return before;
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
