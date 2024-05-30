import {Circuit} from "../circuit/circuit.js";
import {Chorder} from "../keyboard/chord.js";
import {Layer, minXY} from "../circuit/layer.js";
import {Revision} from "../base/revision.js";
import {ObservableValue} from "../base/obs.js";
import {pitch, rad} from "../draw/config.js";
import {xyToPos} from "../draw/main_draw.js";
import {StateSnapshot} from "../draw/state_snapshot.js";
import {Operation} from "../circuit/operation.js";
import {GATE_MAP} from "../gates/gateset.js";

/**
 * @param {!int} steps
 * @return {!function(x: !number, y: !number): ![!number, !number]}
 */
function rotated45Transform(steps) {
    let vx = [1, 0];
    let vy = [0, 1];
    let s = (x, y) => [x - y, x + y];
    steps %= 8;
    steps += 8;
    steps %= 8;
    for (let k = 0; k < steps; k++) {
        vx = s(vx[0], vx[1]);
        vy = s(vy[0], vy[1]);
    }
    return (x, y) => [vx[0]*x + vy[0]*y, vx[1]*x + vy[1]*y];
}

class EditorState {
    /**
     * @param {!HTMLCanvasElement} canvas
     */
    constructor(canvas) {
        this.rev = Revision.startingAt('');
        this.canvas = canvas;
        this.curMouseY = /** @type {undefined|!number} */ undefined;
        this.curMouseX = /** @type {undefined|!number} */ undefined;
        this.chorder = new Chorder();
        this.curLayer = 0;
        this.focusedSet = /** @type {!Map<!string, ![!number, !number]>} */  new Map();
        this.timelineSet = /** @type {!Map<!string, ![!number, !number]>} */ new Map();
        this.mouseDownX = /** @type {undefined|!number} */ undefined;
        this.mouseDownY = /** @type {undefined|!number} */ undefined;
        this.obs_val_draw_state = /** @type {!ObservableValue<StateSnapshot>} */ new ObservableValue(this.toSnapshot(undefined));
    }

    /**
     * @return {!Circuit}
     */
    copyOfCurCircuit() {
        let result = Circuit.fromStimCircuit(this.rev.peekActiveCommit());
        while (result.layers.length <= this.curLayer) {
            result.layers.push(new Layer());
        }
        return result;
    }

    clearFocus() {
        this.focusedSet.clear();
        this.force_redraw();
    }

    /**
     * @param {!boolean} preview
     */
    deleteAtFocus(preview) {
        let newCircuit = this.copyOfCurCircuit();
        let c2q = newCircuit.coordToQubitMap();
        for (let key of this.focusedSet.keys()) {
            let q = c2q.get(key);
            if (q !== undefined) {
                newCircuit.layers[this.curLayer].id_pop_at(q);
            }
        }
        this.commit_or_preview(newCircuit, preview);
    }

    /**
     * @param {!boolean} preview
     */
    deleteCurLayer(preview) {
        let c = this.copyOfCurCircuit();
        c.layers.splice(this.curLayer, 1);
        this.commit_or_preview(c, preview);
    }

    /**
     * @param {!boolean} preview
     */
    insertLayer(preview) {
        let c = this.copyOfCurCircuit();
        c.layers.splice(this.curLayer, 0, new Layer());
        this.commit_or_preview(c, preview);
    }

    undo() {
        this.rev.undo();
    }

    redo() {
        this.rev.redo();
    }

    /**
     * @param {!Circuit} newCircuit
     * @param {!boolean} preview
     */
    commit_or_preview(newCircuit, preview) {
        if (preview) {
            this.preview(newCircuit);
        } else {
            this.commit(newCircuit);
        }
    }

    /**
     * @param {!Circuit} newCircuit
     */
    commit(newCircuit) {
        while (newCircuit.layers.length > 0 && newCircuit.layers[newCircuit.layers.length - 1].isEmpty()) {
            newCircuit.layers.pop();
        }
        this.rev.commit(newCircuit.toStimCircuit());
    }

    /**
     * @param {!Circuit} newCircuit
     */
    preview(newCircuit) {
        this.rev.startedWorkingOnCommit(newCircuit.toStimCircuit());
        this.obs_val_draw_state.set(this.toSnapshot(newCircuit));
    }

    /**
     * @param {undefined|!Circuit} previewCircuit
     * @returns {!StateSnapshot}
     */
    toSnapshot(previewCircuit) {
        if (previewCircuit === undefined) {
            previewCircuit = this.copyOfCurCircuit();
        }
        return new StateSnapshot(
            previewCircuit,
            this.curLayer,
            this.focusedSet,
            this.timelineSet,
            this.curMouseX,
            this.curMouseY,
            this.mouseDownX,
            this.mouseDownY,
            this.currentPositionsBoxesByMouseDrag(this.chorder.curModifiers.has("alt")),
        );
    }

    force_redraw() {
        let previewedCircuit = this.obs_val_draw_state.get().circuit;
        this.obs_val_draw_state.set(this.toSnapshot(previewedCircuit));
    }

    clearCircuit() {
        this.commit(new Circuit(new Float64Array([]), []));
    }

    clearMarkers() {
        let c = this.copyOfCurCircuit();
        for (let layer of c.layers) {
            layer.markers = layer.markers.filter(e => e.gate.name !== 'MARKX' && e.gate.name !== 'MARKY' && e.gate.name !== 'MARKZ');
        }
        this.commit(c);
    }

    /**
     * @param {!boolean} parityLock
     * @returns {!Array<![!int, !int]>}
     */
    currentPositionsBoxesByMouseDrag(parityLock) {
        let curMouseX = this.curMouseX;
        let curMouseY = this.curMouseY;
        let mouseDownX = this.mouseDownX;
        let mouseDownY = this.mouseDownY;
        let result = [];
        if (curMouseX !== undefined && mouseDownX !== undefined) {
            let [sx, sy] = xyToPos(mouseDownX, mouseDownY);
            let x1 = Math.min(curMouseX, mouseDownX);
            let x2 = Math.max(curMouseX, mouseDownX);
            let y1 = Math.min(curMouseY, mouseDownY);
            let y2 = Math.max(curMouseY, mouseDownY);
            let gap = pitch/4 - rad;
            x1 += gap;
            x2 -= gap;
            y1 += gap;
            y2 -= gap;
            x1 = Math.floor(x1 * 2 / pitch + 0.5) / 2;
            x2 = Math.floor(x2 * 2 / pitch + 0.5) / 2;
            y1 = Math.floor(y1 * 2 / pitch + 0.5) / 2;
            y2 = Math.floor(y2 * 2 / pitch + 0.5) / 2;
            let b = 1;
            if (x1 === x2 || y1 === y2) {
                b = 2;
            }
            for (let x = x1; x <= x2; x += 0.5) {
                for (let y = y1; y <= y2; y += 0.5) {
                    if (x % 1 === y % 1) {
                        if (!parityLock || (sx % b === x % b && sy % b === y % b)) {
                            result.push([x, y]);
                        }
                    }
                }
            }
        }
        return result;
    }

    /**
     * @param {!function(!number, !number): ![!number, !number]} coordTransform
     * @param {!boolean} preview
     * @param {!boolean} moveFocus
     */
    applyCoordinateTransform(coordTransform, preview, moveFocus) {
        let c = this.copyOfCurCircuit();
        c = c.afterCoordTransform(coordTransform);
        if (!preview && moveFocus) {
            let trans = m => {
                let new_m = new Map();
                for (let [x, y] of m.values()) {
                    [x, y] = coordTransform(x, y);
                    new_m.set(`${x},${y}`, [x, y]);
                }
                return new_m;
            }
            this.timelineSet = trans(this.timelineSet);
            this.focusedSet = trans(this.focusedSet);
        }
        this.commit_or_preview(c, preview);
    }

    /**
     * @param {!int} steps
     * @param {!boolean} preview
     */
    rotate45(steps, preview) {
        let t1 = rotated45Transform(steps);
        let t2 = this.copyOfCurCircuit().afterCoordTransform(t1).coordTransformForRectification();
        this.applyCoordinateTransform((x, y) => {
            [x, y] = t1(x, y);
            return t2(x, y);
        }, preview, true);
    }

    /**
     * @param {!int} newLayer
     */
    changeCurLayerTo(newLayer) {
        this.curLayer = Math.max(newLayer, 0);
        this.force_redraw();
    }

    /**
     * @param {!Array<![!number, !number]>} newFocus
     * @param {!boolean} unionMode
     * @param {!boolean} xorMode
     */
    changeFocus(newFocus, unionMode, xorMode) {
        if (!unionMode && !xorMode) {
            this.focusedSet.clear();
        }
        for (let [x, y] of newFocus) {
            let k = `${x},${y}`;
            if (xorMode && this.focusedSet.has(k)) {
                this.focusedSet.delete(k);
            } else {
                this.focusedSet.set(k, [x, y]);
            }
        }
        this.force_redraw();
    }

    /**
     * @param {!Iterable<int>} affectedQubits
     * @returns {!Map<!int, !string>}
     * @private
     */
    _inferBases(affectedQubits) {
        let inferredBases = new Map();
        let layer = this.copyOfCurCircuit().layers[this.curLayer];
        for (let q of [...affectedQubits]) {
            let op = layer.id_ops.get(q);
            if (op !== undefined) {
                if (op.gate.name === 'RX' || op.gate.name === 'MX' || op.gate.name === 'MRX') {
                    inferredBases.set(q, 'X');
                } else if (op.gate.name === 'RY' || op.gate.name === 'MY' || op.gate.name === 'MRY') {
                    inferredBases.set(q, 'Y');
                } else if (op.gate.name === 'R' || op.gate.name === 'M' || op.gate.name === 'MR') {
                    inferredBases.set(q, 'Z');
                } else if (op.gate.name === 'MXX' || op.gate.name === 'MYY' || op.gate.name === 'MZZ') {
                    let opBasis = op.gate.name[1];
                    for (let q of op.id_targets) {
                        inferredBases.set(q, opBasis);
                    }
                } else if (op.gate.name.startsWith('MPP:') && op.gate.tableau_map === undefined && op.id_targets.length === op.gate.name.length - 4) {
                    // MPP special case.
                    let bases = op.gate.name.substring(4);
                    for (let k = 0; k < op.id_targets.length; k++) {
                        let q = op.id_targets[k];
                        inferredBases.set(q, bases[k]);
                    }
                }
            }
        }
        return inferredBases;
    }

    /**
     * @param {!boolean} preview
     * @param {!int} markIndex
     */
    markFocusInferBasis(preview, markIndex) {
        let newCircuit = this.copyOfCurCircuit().withCoordsIncluded(this.focusedSet.values());
        let c2q = newCircuit.coordToQubitMap();
        let affectedQubits = new Set();
        for (let key of this.focusedSet.keys()) {
            affectedQubits.add(c2q.get(key));
        }

        // Determine which qubits have forced basis based on their operation.
        let forcedBases = this._inferBases(affectedQubits);
        for (let q of forcedBases.keys()) {
            affectedQubits.add(q);
        }

        // Pick a default basis for unforced qubits.
        let seenBases = new Set(forcedBases.values());
        seenBases.delete(undefined);
        let defaultBasis;
        if (seenBases.size === 1) {
            defaultBasis = [...seenBases][0];
        } else {
            defaultBasis = 'Z';
        }

        // Mark each qubit with its inferred basis.
        let layer = newCircuit.layers[this.curLayer];
        for (let q of affectedQubits) {
            let basis = forcedBases.get(q);
            if (basis === undefined) {
                basis = defaultBasis;
            }
            let gate = GATE_MAP.get(`MARK${basis}`).withDefaultArgument(markIndex);
            layer.put(new Operation(
                gate,
                new Float32Array([markIndex]),
                new Uint32Array([q]),
            ));
        }

        this.commit_or_preview(newCircuit, preview);
    }

    /**
     * @param {!boolean} preview
     */
    unmarkFocusInferBasis(preview) {
        let newCircuit = this.copyOfCurCircuit().withCoordsIncluded(this.focusedSet.values());
        let c2q = newCircuit.coordToQubitMap();
        let affectedQubits = new Set();
        for (let key of this.focusedSet.keys()) {
            affectedQubits.add(c2q.get(key));
        }

        let inferredBases = this._inferBases(affectedQubits);
        for (let q of inferredBases.keys()) {
            affectedQubits.add(q);
        }

        for (let q of affectedQubits) {
            if (q !== undefined) {
                newCircuit.layers[this.curLayer].id_dropMarkersAt(q);
            }
        }

        this.commit_or_preview(newCircuit, preview);
    }

    /**
     * @param {!boolean} preview
     * @param {!Gate} gate
     * @param {!Array<!number>} gate_args
     */
    _writeSingleQubitGateToFocus(preview, gate, gate_args) {
        let newCircuit = this.copyOfCurCircuit().withCoordsIncluded(this.focusedSet.values());
        let c2q = newCircuit.coordToQubitMap();
        for (let key of this.focusedSet.keys()) {
            newCircuit.layers[this.curLayer].put(new Operation(
                gate,
                new Float32Array(gate_args),
                new Uint32Array([c2q.get(key)]),
            ));
        }
        this.commit_or_preview(newCircuit, preview);
    }

    /**
     * @param {!boolean} preview
     * @param {!Gate} gate
     * @param {!Array<!number>} gate_args
     */
    _writeTwoQubitGateToFocus(preview, gate, gate_args) {
        let newCircuit = this.copyOfCurCircuit();
        let [x, y] = xyToPos(this.curMouseX, this.curMouseY);
        let [minX, minY] = minXY(this.focusedSet.values());
        let coords = [];
        if (x !== undefined && minX !== undefined && !this.focusedSet.has(`${x},${y}`)) {
            let dx = x - minX;
            let dy = y - minY;

            for (let [vx, vy] of this.focusedSet.values()) {
                coords.push([vx, vy]);
                coords.push([vx + dx, vy + dy]);
            }
        } else if (this.focusedSet.size === 2) {
            for (let [vx, vy] of this.focusedSet.values()) {
                coords.push([vx, vy]);
            }
        }
        if (coords.length > 0) {
            newCircuit = newCircuit.withCoordsIncluded(coords)
            let c2q = newCircuit.coordToQubitMap();
            for (let k = 0; k < coords.length; k += 2) {
                let [x0, y0] = coords[k];
                let [x1, y1] = coords[k + 1];
                let q0 = c2q.get(`${x0},${y0}`);
                let q1 = c2q.get(`${x1},${y1}`);
                newCircuit.layers[this.curLayer].put(new Operation(
                    gate,
                    new Float32Array(gate_args),
                    new Uint32Array([q0, q1]),
                ));
            }
        }

        this.commit_or_preview(newCircuit, preview);
    }

    /**
     * @param {!boolean} preview
     * @param {!Gate} gate
     * @param {!Array<!number>} gate_args
     */
    _writeVariableQubitGateToFocus(preview, gate, gate_args) {
        if (this.focusedSet.size === 0) {
            return;
        }

        let pairs = [];
        let cx = 0;
        let cy = 0;
        for (let xy of this.focusedSet.values()) {
            pairs.push(xy);
            cx += xy[0];
            cy += xy[1];
        }
        cx /= pairs.length;
        cy /= pairs.length;
        pairs.sort((a, b) => {
            let [x1, y1] = a;
            let [x2, y2] = b;
            return Math.atan2(y1 - cy, x1 - cx) - Math.atan2(y2 - cy, x2 - cx);
        });

        let newCircuit = this.copyOfCurCircuit().withCoordsIncluded(this.focusedSet.values());
        let c2q = newCircuit.coordToQubitMap();
        let qs = new Uint32Array(this.focusedSet.size);
        for (let k = 0; k < pairs.length; k++) {
            let [x, y] = pairs[k];
            qs[k] = c2q.get(`${x},${y}`);
        }

        newCircuit.layers[this.curLayer].put(new Operation(gate, new Float32Array(gate_args), qs));
        this.commit_or_preview(newCircuit, preview);
    }

    /**
     * @param {!boolean} preview
     * @param {!Gate} gate
     * @param {undefined|!Array<!number>=} gate_args
     */
    writeGateToFocus(preview, gate, gate_args=undefined) {
        if (gate_args === undefined) {
            if (gate.defaultArgument === undefined) {
                gate_args = [];
            } else {
                gate_args = [gate.defaultArgument];
            }
        }
        if (gate.num_qubits === 1) {
            this._writeSingleQubitGateToFocus(preview, gate, gate_args);
        } else if (gate.num_qubits === 2) {
            this._writeTwoQubitGateToFocus(preview, gate, gate_args);
        } else {
            this._writeVariableQubitGateToFocus(preview, gate, gate_args);
        }
    }

    writeMarkerToObservable(preview, marker_index) {
        this._writeMarkerToDetOrObs(preview, marker_index, false);
    }

    writeMarkerToDetector(preview, marker_index) {
        this._writeMarkerToDetOrObs(preview, marker_index, true);
    }

    _writeMarkerToDetOrObs(preview, marker_index, isDet) {
        let newCircuit = this.copyOfCurCircuit().withCoordsIncluded(this.focusedSet.values());
        let argIndex = isDet ? newCircuit.collectDetectorsAndObservables(false).dets.length : marker_index;
        for (let k = 0; k < newCircuit.layers.length; k++) {
            let layer = newCircuit.layers[k];
            for (let k2 = 0; k2 < layer.markers.length; k2++) {
                let op = /** @type {!Operation} */ layer.markers[k2];
                if (op.args[0] === marker_index && ['MARKX', 'MARKY', 'MARKZ'].indexOf(op.gate.name) !== -1) {
                    layer.markers[k2] = new Operation(
                        GATE_MAP.get(isDet ? 'DETECTOR' : 'OBSERVABLE_INCLUDE'),
                        new Float32Array([argIndex]),
                        op.id_targets,
                    );
                }
            }
        }
        this.commit_or_preview(newCircuit, preview);
    }
}

export {EditorState, StateSnapshot}
