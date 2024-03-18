import {Operation} from "./operation.js"
import {GATE_ALIAS_MAP, GATE_MAP} from "../gates/gateset.js"
import {Layer} from "./layer.js"
import {make_mpp_gate, make_spp_gate} from '../gates/gateset_mpp.js';
import {describe} from "../base/describe.js";

/**
 * @param {!Iterator<TItem>}items
 * @param {!function(item: TItem): TKey} func
 * @returns {!Map<TKey, !Array<TItem>>}
 * @template TItem
 * @template TKey
 */
function groupBy(items, func) {
    let result = new Map();
    for (let item of items) {
        let key = func(item);
        let group = result.get(key);
        if (group === undefined) {
            result.set(key, [item]);
        } else {
            group.push(item);
        }
    }
    return result;
}

/**
 * @param {!string} targetText
 * @returns {!Array.<!string>}
 */
function processTargetsTextIntoTargets(targetText) {
    let targets = [];
    let flush = () => {
        if (curTarget !== '') {
            targets.push(curTarget)
            curTarget = '';
        }
    }
    let curTarget = '';
    for (let c of targetText) {
        if (c === ' ') {
            flush();
        } else if (c === '*') {
            flush();
            targets.push('*');
        } else {
            curTarget += c;
        }
    }
    flush();

    return targets;
}

/**
 * @param {!Array.<!string>} targets
 * @returns {!Array.<!Array.<!string>>}
 */
function splitUncombinedTargets(targets) {
    let result = [];
    let start = 0;
    while (start < targets.length) {
        let end = start + 1;
        while (end < targets.length && targets[end] === '*') {
            end += 2;
        }
        if (end > targets.length) {
            throw Error(`Dangling combiner in ${targets}.`);
        }
        let term = [];
        for (let k = start; k < end; k += 2) {
            if (targets[k] === '*') {
                if (k === 0) {
                    throw Error(`Leading combiner in ${targets}.`);
                }
                throw Error(`Adjacent combiners in ${targets}.`);
            }
            term.push(targets[k]);
        }
        result.push(term);
        start = end;
    }
    return result;
}

/**
 * @param {!Float32Array} args
 * @param {!Array.<!string>} combinedTargets
 * @returns {!Operation}
 */
function simplifiedMPP(args, combinedTargets) {
    let bases = '';
    let qubits = [];
    for (let t of combinedTargets) {
        if (t[0] === '!') {
            t = t.substring(1);
        }
        if (t[0] === 'X' || t[0] === 'Y' || t[0] === 'Z') {
            bases += t[0];
            let v = parseInt(t.substring(1));
            if (v !== v) {
                throw Error(`Non-Pauli target given to MPP: ${combinedTargets}`);
            }
            qubits.push(v);
        } else {
            throw Error(`Non-Pauli target given to MPP: ${combinedTargets}`);
        }
    }

    let gate = GATE_MAP.get('M' + bases);
    if (gate === undefined) {
        gate = GATE_MAP.get('MPP:' + bases);
    }
    if (gate === undefined) {
        gate = make_mpp_gate(bases);
    }
    return new Operation(gate, args, new Uint32Array(qubits));
}

/**
 * @param {!Float32Array} args
 * @param {!boolean} dag
 * @param {!Array.<!string>} combinedTargets
 * @returns {!Operation}
 */
function simplifiedSPP(args, dag, combinedTargets) {
    let bases = '';
    let qubits = [];
    for (let t of combinedTargets) {
        if (t[0] === '!') {
            t = t.substring(1);
        }
        if (t[0] === 'X' || t[0] === 'Y' || t[0] === 'Z') {
            bases += t[0];
            let v = parseInt(t.substring(1));
            if (v !== v) {
                throw Error(`Non-Pauli target given to SPP: ${combinedTargets}`);
            }
            qubits.push(v);
        } else {
            throw Error(`Non-Pauli target given to SPP: ${combinedTargets}`);
        }
    }

    let gate = GATE_MAP.get((dag ? 'SPP_DAG:' : 'SPP:') + bases);
    if (gate === undefined) {
        gate = make_spp_gate(bases, dag);
    }
    return new Operation(gate, args, new Uint32Array(qubits));
}


class Circuit {
    /**
     * @param {!Float64Array} qubitCoordData
     * @param {!Array<!Layer>} layers
     */
    constructor(qubitCoordData, layers = []) {
        if (!(qubitCoordData instanceof Float64Array)) {
            throw new Error('!(qubitCoords instanceof Float64Array)');
        }
        if (!Array.isArray(layers)) {
            throw new Error('!Array.isArray(layers)');
        }
        if (!layers.every(e => e instanceof Layer)) {
            throw new Error('!layers.every(e => e instanceof Layer)');
        }
        this.qubitCoordData = qubitCoordData;
        this.layers = layers;
    }

    /**
     * @param {!string} stimCircuit
     * @returns {!Circuit}
     */
    static fromStimCircuit(stimCircuit) {
        let lines = stimCircuit.replaceAll(';', '\n').
            replaceAll('_', ' ').
            replaceAll('Q(', 'QUBIT_COORDS(').
            replaceAll('DT', 'DETECTOR').
            replaceAll(' COORDS', '_COORDS').
            replaceAll(' ERROR', '_ERROR').
            replaceAll('C XYZ', 'C_XYZ').
            replaceAll('H XY', 'H_XY').
            replaceAll('H YZ', 'H_YZ').
            replaceAll(' INCLUDE', '_INCLUDE').
            replaceAll('SQRT ', 'SQRT_').
            replaceAll(' DAG ', '_DAG ').
            replaceAll('C ZYX', 'C_ZYX').split('\n');
        let layers = [new Layer()];
        let i2q = new Map();
        let used_positions = new Set();

        let findEndOfBlock = (lines, startIndex, endIndex) => {
            let nestLevel = 0;
            for (let k = startIndex; k < endIndex; k++) {
                let line = lines[k];
                line = line.split('#')[0].trim();
                if (line.toLowerCase().startsWith("repeat ")) {
                    nestLevel++;
                } else if (line === '}') {
                    nestLevel--;
                    if (nestLevel === 0) {
                        return k;
                    }
                }
            }
            throw Error("Repeat block didn't end");
        };

        let processLineChunk = (lines, startIndex, endIndex, repetitions) => {
            if (!layers[layers.length - 1].empty()) {
                layers.push(new Layer());
            }
            for (let rep = 0; rep < repetitions; rep++) {
                for (let k = startIndex; k < endIndex; k++) {
                    let line = lines[k];
                    line = line.split('#')[0].trim();
                    if (line.toLowerCase().startsWith("repeat ")) {
                        let reps = parseInt(line.split(" ")[1]);
                        let k2 = findEndOfBlock(lines, k, endIndex);
                        processLineChunk(lines, k + 1, k2, reps);
                        k = k2;
                    } else {
                        processLine(line);
                    }
                }
                if (!layers[layers.length - 1].empty()) {
                    layers.push(new Layer());
                }
            }
        };

        let processLine = line => {
            let args = [];
            let targets = [];
            let name = '';
            if (line.indexOf(')') !== -1) {
                let [ab, c] = line.split(')');
                let [a, b] = ab.split('(');
                name = a.trim();
                args = b.split(',').map(e => e.trim()).map(parseFloat);
                targets = processTargetsTextIntoTargets(c);
            } else {
                let ab = line.split(' ').map(e => e.trim()).filter(e => e !== '');
                if (ab.length === 0) {
                    return;
                }
                let [a, ...b] = ab;
                name = a.trim();
                args = [];
                targets = b.flatMap(processTargetsTextIntoTargets);
            }
            let reverse_pairs = false;
            if (name === '') {
                return;
            }
            let alias = GATE_ALIAS_MAP.get(name);
            if (alias !== undefined) {
                if (alias.ignore) {
                    return;
                } else if (alias.name !== undefined) {
                    reverse_pairs = alias.rev_pair !== undefined && alias.rev_pair;
                    name = alias.name;
                } else {
                    throw new Error(`Unimplemented alias ${name}: ${describe(alias)}.`);
                }
            } else if (name === 'TICK') {
                layers.push(new Layer());
                return;
            } else if (name === 'MPP') {
                let combinedTargets = splitUncombinedTargets(targets);
                let layer = layers[layers.length - 1]
                for (let combo of combinedTargets) {
                    try {
                        layer.put(simplifiedMPP(new Float32Array(args), combo), false);
                    } catch (_) {
                        layers.push(new Layer());
                        layer = layers[layers.length - 1];
                        layer.put(simplifiedMPP(new Float32Array(args), combo), false);
                    }
                }
                return;
            } else if (name === 'SPP' || name === 'SPP_DAG') {
                let dag = name === 'SPP_DAG';
                let combinedTargets = splitUncombinedTargets(targets);
                let layer = layers[layers.length - 1]
                for (let combo of combinedTargets) {
                    try {
                        layer.put(simplifiedSPP(new Float32Array(args), dag, combo), false);
                    } catch (_) {
                        layers.push(new Layer());
                        layer = layers[layers.length - 1];
                        layer.put(simplifiedSPP(new Float32Array(args), dag, combo), false);
                    }
                }
                return;
            } else if (name.startsWith('QUBIT_COORDS')) {
                let x = args.length < 1 ? 0 : args[0];
                let y = args.length < 2 ? 0 : args[1];
                for (let targ of targets) {
                    let t = parseInt(targ);
                    if (i2q.has(t)) {
                        console.warn(`Ignoring "${line}" because there's already coordinate data for qubit ${t}.`);
                    } else if (used_positions.has(`${x},${y}`)) {
                        console.warn(`Ignoring "${line}" because there's already a qubit placed at ${x},${y}.`);
                    } else {
                        i2q.set(t, [x, y]);
                        used_positions.add(`${x},${y}`);
                    }
                }
                return;
            }

            let ignored = false;
            for (let targ of targets) {
                if (targ.startsWith("rec[")) {
                    if (name === "CX" || name === "CY" || name === "CZ" || name === "ZCX" || name === "ZCY") {
                        ignored = true;
                        break;
                    }
                }
                if (typeof parseInt(targ) !== 'number') {
                    throw new Error(line);
                }
            }
            if (ignored) {
                console.warn("IGNORED", name);
                return;
            }

            let gate = GATE_MAP.get(name);
            if (gate === undefined) {
                console.warn("Unrecognized gate name in " + line);
                return;
            }
            let a = new Float32Array(args);

            let layer = layers[layers.length - 1];
            if (gate.num_qubits === undefined) {
                layer.put(new Operation(gate, a, new Uint32Array(targets)));
            } else {
                if (targets.length % gate.num_qubits !== 0) {
                    throw new Error("Incorrect number of targets in line " + line);
                }
                for (let k = 0; k < targets.length; k += gate.num_qubits) {
                    let sub_targets = targets.slice(k, k + gate.num_qubits);
                    if (reverse_pairs) {
                        sub_targets.reverse();
                    }
                    let qs = new Uint32Array(sub_targets);
                    try {
                        layer.put(new Operation(gate, a, qs), false);
                    } catch (_) {
                        layers.push(new Layer());
                        layer = layers[layers.length - 1];
                        layer.put(new Operation(gate, a, qs), false);
                    }
                }
            }
        };

        processLineChunk(lines, 0, lines.length, 1);
        if (layers.length > 0 && layers[layers.length - 1].isEmpty()) {
            layers.pop();
        }

        let next_auto_position_x = 0;
        let ensure_has_coords = (t) => {
            let b = true;
            while (!i2q.has(t)) {
                let x = b ? t : next_auto_position_x;
                let k = `${x},0`;
                if (!used_positions.has(k)) {
                    used_positions.add(k);
                    i2q.set(t, [x, 0]);
                }
                next_auto_position_x += !b;
                b = false;
            }
        };

        for (let layer of layers) {
            for (let op of layer.iter_gates_and_markers()) {
                for (let t of op.id_targets) {
                    ensure_has_coords(t);
                }
            }
        }

        let numQubits = Math.max(...i2q.keys(), 0) + 1;
        let qubitCoords = new Float64Array(numQubits*2);
        for (let q = 0; q < numQubits; q++) {
            ensure_has_coords(q);
            let [x, y] = i2q.get(q);
            qubitCoords[2*q] = x;
            qubitCoords[2*q + 1] = y;
        }

        return new Circuit(qubitCoords, layers);
    }

    /**
     * @returns {!Set<!int>}
     */
    allQubits() {
        let result = new Set();
        for (let layer of this.layers) {
            for (let op of layer.iter_gates_and_markers()) {
                for (let t of op.id_targets) {
                    result.add(t);
                }
            }
         }
        return result;
    }

    /**
     * @returns {!Circuit}
     */
    rotated45() {
        return this.afterCoordTransform((x, y) => [x - y, x + y]);
    }

    coordTransformForRectification() {
        let coordSet = new Map();
        for (let k = 0; k < this.qubitCoordData.length; k += 2) {
            let x = this.qubitCoordData[k];
            let y = this.qubitCoordData[k+1];
            coordSet.set(`${x},${y}`, [x, y]);
        }
        let minX = Infinity;
        let minY = Infinity;
        let step = 256;
        for (let [x, y] of coordSet.values()) {
            minX = Math.min(x, minX);
            minY = Math.min(y, minY);
            while ((x % step !== 0 || y % step !== 0) && step > 1 / 256) {
                step /= 2;
            }
        }
        let scale;
        if (step <= 1 / 256) {
            scale = 1;
        } else {
            scale = 1 / step;
            let mask = 0;
            for (let [x, y] of coordSet.values()) {
                let b1 = (x - minX + y - minY) % (2 * step);
                let b2 = (x - minX - y + minY) % (2 * step);
                mask |= b1 === 0 ? 1 : 2;
                mask |= b2 === 0 ? 4 : 8;
            }
            if (mask === (1 | 4)) {
                scale /= 2;
            } else if (mask === (2 | 8)) {
                minX -= step;
                scale /= 2;
            }
        }

        let offsetX = -minX;
        let offsetY = -minY;
        return (x, y) => [(x + offsetX) * scale, (y + offsetY) * scale];
    }

    /**
     * @returns {!Circuit}
     */
    afterRectification() {
        return this.afterCoordTransform(this.coordTransformForRectification());
    }

    /**
     * @param {!number} dx
     * @param {!number} dy
     * @returns {!Circuit}
     */
    shifted(dx, dy) {
        return this.afterCoordTransform((x, y) => [x + dx, y + dy]);
    }

    /**
     * @return {!Circuit}
     */
    copy() {
        return this.shifted(0, 0);
    }

    /**
     * @param {!function(!number, !number): ![!number, !number]} coordTransform
     * @returns {!Circuit}
     */
    afterCoordTransform(coordTransform) {
        let newCoords = new Float64Array(this.qubitCoordData.length);
        for (let k = 0; k < this.qubitCoordData.length; k += 2) {
            let x = this.qubitCoordData[k];
            let y = this.qubitCoordData[k + 1];
            let [x2, y2] = coordTransform(x, y);
            newCoords[k] = x2;
            newCoords[k + 1] = y2;
        }
        let newLayers = this.layers.map(e => e.copy());
        return new Circuit(newCoords, newLayers);
    }

    /**
     * @returns {!string}
     */
    toStimCircuit() {
        let usedQubits = new Set();
        for (let layer of this.layers) {
            for (let op of layer.iter_gates_and_markers()) {
                for (let t of op.id_targets) {
                    usedQubits.add(t);
                }
            }
        }

        let packedQubitCoords = [];
        for (let q of usedQubits) {
            let x = this.qubitCoordData[2*q];
            let y = this.qubitCoordData[2*q+1];
            packedQubitCoords.push({q, x, y});
        }
        packedQubitCoords.sort((a, b) => {
            if (a.x !== b.x) {
                return a.x - b.x;
            }
            if (a.y !== b.y) {
                return a.y - b.y;
            }
            return a.q - b.q;
        });
        let old2new = new Map();
        let out = [];
        for (let q = 0; q < packedQubitCoords.length; q++) {
            let {q: old_q, x, y} = packedQubitCoords[q];
            old2new.set(old_q, q);
            out.push(`QUBIT_COORDS(${x}, ${y}) ${q}`);
        }

        for (let layer of this.layers) {
            let opsByName = groupBy(layer.iter_gates_and_markers(), op => {
                let key = op.gate.name;
                if (key.startsWith('MPP:') && !GATE_MAP.has(key)) {
                    key = 'MPP';
                }
                if (key.startsWith('SPP:') && !GATE_MAP.has(key)) {
                    key = 'SPP';
                }
                if (key.startsWith('SPP_DAG:') && !GATE_MAP.has(key)) {
                    key = 'SPP_DAG';
                }
                if (op.args.length > 0) {
                    key += '(' + [...op.args].join(',') + ')';
                }
                return key;
            });
            let namesWithArgs = [...opsByName.keys()];
            namesWithArgs.sort((a, b) => {
                let ma = a.startsWith('MARK') || a.startsWith('POLY');
                let mb = b.startsWith('MARK') || b.startsWith('POLY');
                if (ma !== mb) {
                    return ma < mb ? -1 : +1;
                }
                return a < b ? -1 : a > b ? +1 : 0;
            });

            for (let nameWithArgs of namesWithArgs) {
                let group = opsByName.get(nameWithArgs);
                let targetGroups = [];

                let gateName = nameWithArgs.split('(')[0];

                let gate = GATE_MAP.get(gateName);
                if (gate === undefined && (gateName === 'MPP' || gateName === 'SPP' || gateName === 'SPP_DAG')) {
                    let line = [gateName + ' '];
                    for (let op of group) {
                        let bases = op.gate.name.substring(4);
                        for (let k = 0; k < op.id_targets.length; k++) {
                            line.push(bases[k] + old2new.get(op.id_targets[k]));
                            line.push('*');
                        }
                        line.pop();
                        line.push(' ');
                    }
                    out.push(line.join('').trim());
                } else {
                    if (gate !== undefined && gate.can_fuse) {
                        let flatTargetGroups = [];
                        for (let op of group) {
                            flatTargetGroups.push(...op.id_targets)
                        }
                        targetGroups.push(flatTargetGroups);
                    } else {
                        for (let op of group) {
                            targetGroups.push([...op.id_targets])
                        }
                    }

                    for (let targetGroup of targetGroups) {
                        let line = [nameWithArgs];
                        for (let t of targetGroup) {
                            line.push(old2new.get(t));
                        }
                        out.push(line.join(' '));
                    }
                }
            }
            out.push(`TICK`);
        }
        while (out.length > 0 && out[out.length - 1] === 'TICK') {
            out.pop();
        }

        return out.join('\n');
    }

    /**
     * @param {!Iterable<![!number, !number]>} coords
     */
    withCoordsIncluded(coords) {
        let coordMap = this.coordToQubitMap();
        let extraCoordData = [];
        for (let [x, y] of coords) {
            let key = `${x},${y}`;
            if (!coordMap.has(key)) {
                coordMap.set(key, coordMap.size);
                extraCoordData.push(x, y);
            }
        }
        return new Circuit(
            new Float64Array([...this.qubitCoordData, ...extraCoordData]),
            this.layers.map(e => e.copy()),
        );
    }

    /**
     * @returns {!Map<!string, !int>}
     */
    coordToQubitMap() {
        let result = new Map();
        for (let q = 0; q < this.qubitCoordData.length; q += 2) {
            let x = this.qubitCoordData[q];
            let y = this.qubitCoordData[q + 1];
            result.set(`${x},${y}`, q / 2);
        }
        return result;
    }

    /**
     * @returns {!string}
     */
    toString() {
        return this.toStimCircuit();
    }

    /**
     * @param {*} other
     * @returns {!boolean}
     */
    isEqualTo(other) {
        if (!(other instanceof Circuit)) {
            return false;
        }
        return this.toStimCircuit() === other.toStimCircuit();
    }
}

export {Circuit, processTargetsTextIntoTargets, splitUncombinedTargets};
