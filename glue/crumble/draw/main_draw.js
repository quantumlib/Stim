import {pitch, rad, OFFSET_X, OFFSET_Y} from "./config.js"
import {marker_placement} from "../gates/gateset_markers.js";
import {drawTimeline} from "./timeline_viewer.js";
import {PropagatedPauliFrames} from "../circuit/propagated_pauli_frames.js";
import {stroke_connector_to} from "../gates/gate_draw_util.js"
import {beginPathPolygon} from './draw_util.js';

/**
 * @param {!number|undefined} x
 * @param {!number|undefined} y
 * @return {![undefined, undefined]|![!number, !number]}
 */
function xyToPos(x, y) {
    if (x === undefined || y === undefined) {
        return [undefined, undefined];
    }
    let focusX = x / pitch;
    let focusY = y / pitch;
    let roundedX = Math.floor(focusX * 2 + 0.5) / 2;
    let roundedY = Math.floor(focusY * 2 + 0.5) / 2;
    let centerX = roundedX*pitch;
    let centerY = roundedY*pitch;
    if (Math.abs(centerX - x) <= rad && Math.abs(centerY - y) <= rad && roundedX % 1 === roundedY % 1) {
        return [roundedX, roundedY];
    }
    return [undefined, undefined];
}

/**
 * @param {!CanvasRenderingContext2D} ctx
 * @param {!StateSnapshot} snap
 * @param {!function(q: !int): ![!number, !number]} qubitCoordsFunc
 * @param {!PropagatedPauliFrames} propagatedMarkers
 * @param {!int} mi
 */
function drawCrossMarkers(ctx, snap, qubitCoordsFunc, propagatedMarkers, mi) {
    let crossings = propagatedMarkers.atLayer(snap.curLayer).crossings;
    if (crossings !== undefined) {
        for (let {q1, q2, color} of crossings) {
            let [x1, y1] = qubitCoordsFunc(q1);
            let [x2, y2] = qubitCoordsFunc(q2);
            if (color === 'X') {
                ctx.strokeStyle = 'red';
            } else if (color === 'Y') {
                ctx.strokeStyle = 'green';
            } else if (color === 'Z') {
                ctx.strokeStyle = 'blue';
            } else {
                ctx.strokeStyle = 'purple'
            }
            ctx.lineWidth = 8;
            stroke_connector_to(ctx, x1, y1, x2, y2);
            ctx.lineWidth = 1;
        }
    }
}

/**
 * @param {!CanvasRenderingContext2D} ctx
 * @param {!StateSnapshot} snap
 * @param {!function(q: !int): ![!number, !number]} qubitCoordsFunc
 * @param {!Map<!int, !PropagatedPauliFrames>} propagatedMarkerLayers
 */
function drawMarkers(ctx, snap, qubitCoordsFunc, propagatedMarkerLayers) {
    let obsCount = new Map();
    let detCount = new Map();
    for (let [mi, p] of propagatedMarkerLayers.entries()) {
        drawSingleMarker(ctx, snap, qubitCoordsFunc, p, mi, obsCount, detCount);
    }
}

/**
 * @param {!CanvasRenderingContext2D} ctx
 * @param {!StateSnapshot} snap
 * @param {!function(q: !int): ![!number, !number]} qubitCoordsFunc
 * @param {!PropagatedPauliFrames} propagatedMarkers
 * @param {!int} mi
 * @param {!Map} hitCount
 */
function drawSingleMarker(ctx, snap, qubitCoordsFunc, propagatedMarkers, mi, hitCount) {
    let basesQubitMap = propagatedMarkers.atLayer(snap.curLayer + 0.5).bases;

    // Convert qubit indices to draw coordinates.
    let basisCoords = [];
    for (let [q, b] of basesQubitMap.entries()) {
        basisCoords.push([b, qubitCoordsFunc(q)]);
    }

    // Draw a polygon for the marker set.
    if (mi >= 0 && basisCoords.length > 0) {
        if (basisCoords.every(e => e[0] === 'X')) {
            ctx.fillStyle = 'red';
        } else if (basisCoords.every(e => e[0] === 'Y')) {
            ctx.fillStyle = 'green';
        } else if (basisCoords.every(e => e[0] === 'Z')) {
            ctx.fillStyle = 'blue';
        } else {
            ctx.fillStyle = 'black';
        }
        ctx.strokeStyle = ctx.fillStyle;
        let coords = basisCoords.map(e => e[1]);
        let cx = 0;
        let cy = 0;
        for (let [x, y] of coords) {
            cx += x;
            cy += y;
        }
        cx /= coords.length;
        cy /= coords.length;
        coords.sort((a, b) => {
            let [ax, ay] = a;
            let [bx, by] = b;
            let av = Math.atan2(ay - cy, ax - cx);
            let bv = Math.atan2(by - cy, bx - cx);
            if (ax === cx && ay === cy) {
                av = -100;
            }
            if (bx === cx && by === cy) {
                bv = -100;
            }
            return av - bv;
        })
        beginPathPolygon(ctx, coords);
        ctx.globalAlpha *= 0.25;
        ctx.fill();
        ctx.globalAlpha *= 4;
        ctx.lineWidth = 2;
        ctx.stroke();
        ctx.lineWidth = 1;
    }

    // Draw individual qubit markers.
    for (let [b, [x, y]] of basisCoords) {
        let {dx, dy, wx, wy} = marker_placement(mi, `${x}:${y}`, hitCount);
        if (b === 'X') {
            ctx.fillStyle = 'red'
        } else if (b === 'Y') {
            ctx.fillStyle = 'green'
        } else if (b === 'Z') {
            ctx.fillStyle = 'blue'
        } else {
            throw new Error('Not a pauli: ' + b);
        }
        ctx.fillRect(x - dx, y - dy, wx, wy);
    }

    // Show error highlights.
    let errorsQubitSet = propagatedMarkers.atLayer(snap.curLayer).errors;
    for (let q of errorsQubitSet) {
        let [x, y] = qubitCoordsFunc(q);
        let {dx, dy, wx, wy} = marker_placement(mi, `${x}:${y}`, hitCount);
        if (mi < 0) {
            ctx.lineWidth = 2;
        } else {
            ctx.lineWidth = 8;
        }
        ctx.strokeStyle = 'magenta'
        ctx.strokeRect(x - dx, y - dy, wx, wy);
        ctx.lineWidth = 1;
        ctx.fillStyle = 'black'
        ctx.fillRect(x - dx, y - dy, wx, wy);
    }
}

let _defensive_draw_enabled = true;

/**
 * @param {!boolean} val
 */
function setDefensiveDrawEnabled(val) {
    _defensive_draw_enabled = val;
}

/**
 * @param {!CanvasRenderingContext2D} ctx
 * @param {!function} body
 */
function defensiveDraw(ctx, body) {
    ctx.save();
    try {
        if (_defensive_draw_enabled) {
            body();
        } else {
            try {
                body();
            } catch (ex) {
                console.error(ex);
            }
        }
    } finally {
        ctx.restore();
    }
}

/**
 * @param {!CanvasRenderingContext2D} ctx
 * @param {!StateSnapshot} snap
 */
function draw(ctx, snap) {
    let circuit = snap.circuit;

    let numPropagatedLayers = 0;
    let reversedMarkers = new Set();
    for (let layer of circuit.layers) {
        for (let op of layer.markers) {
            let gate = op.gate;
            if (gate.name === "MARKX" || gate.name === "MARKY" || gate.name === "MARKZ") {
                numPropagatedLayers = Math.max(numPropagatedLayers, op.args[0] + 1);
            }
            if (gate.name === "REVERSE") {
                reversedMarkers.add(op.args[0]);
            }
        }
    }

    let c2dCoordTransform = (x, y) => [x*pitch - OFFSET_X, y*pitch - OFFSET_Y];
    let qubitDrawCoords = q => {
        let x = circuit.qubitCoordData[2 * q];
        let y = circuit.qubitCoordData[2 * q + 1];
        return c2dCoordTransform(x, y);
    };
    let propagatedMarkerLayers = /** @type {!Map<!int, !PropagatedPauliFrames>} */ new Map();
    for (let mi = 0; mi < numPropagatedLayers; mi++) {
        propagatedMarkerLayers.set(mi, PropagatedPauliFrames.fromCircuit(circuit, mi, reversedMarkers.has(mi)));
    }
    let {dets: dets, obs: obs} = circuit.collectDetectorsAndObservables(false);
    let batch_input = [];
    for (let mi = 0; mi < dets.length; mi++) {
        batch_input.push(dets[mi].mids);
    }
    for (let mi of obs.keys()) {
        batch_input.push(obs.get(mi));
    }
    let batch_output = PropagatedPauliFrames.batchFromMeasurements(circuit, batch_input);
    let batch_index = 0;
    for (let mi = 0; mi < dets.length; mi++) {
        propagatedMarkerLayers.set(~mi, batch_output[batch_index++]);
    }
    for (let mi of obs.keys()) {
        propagatedMarkerLayers.set(~mi ^ (1 << 30), batch_output[batch_index++]);
    }

    let operatedOnQubits = new Set();
    for (let layer of circuit.layers) {
        for (let t of layer.id_ops.keys()) {
            operatedOnQubits.add(t);
        }
    }
    let usedQubitCoordSet = new Set();
    let operatedOnQubitSet = new Set();
    for (let q of circuit.allQubits()) {
        let qx = circuit.qubitCoordData[q * 2];
        let qy = circuit.qubitCoordData[q * 2 + 1];
        usedQubitCoordSet.add(`${qx},${qy}`);
        if (operatedOnQubits.has(q)) {
            operatedOnQubitSet.add(`${qx},${qy}`);
        }
    }

    defensiveDraw(ctx, () => {
        ctx.fillStyle = 'white';
        ctx.clearRect(0, 0, ctx.canvas.width, ctx.canvas.height);
        let [focusX, focusY] = xyToPos(snap.curMouseX, snap.curMouseY);

        // Draw the background polygons.
        let lastPolygonLayer = snap.curLayer;
        for (let r = 0; r <= snap.curLayer; r++) {
            for (let op of circuit.layers[r].markers) {
                if (op.gate.name === 'POLYGON') {
                    lastPolygonLayer = r;
                    break;
                }
            }
        }
        let polygonMarkers = [...circuit.layers[lastPolygonLayer].markers];
        polygonMarkers.sort((a, b) => b.id_targets.length - a.id_targets.length);
        for (let op of polygonMarkers) {
            if (op.gate.name === 'POLYGON') {
                op.id_draw(qubitDrawCoords, ctx);
            }
        }

        // Draw the grid of qubits.
        defensiveDraw(ctx, () => {
            for (let qx = 0; qx < 100; qx += 0.5) {
                let [x, _] = c2dCoordTransform(qx, 0);
                let s = `${qx}`;
                ctx.fillStyle = 'black';
                ctx.fillText(s, x - ctx.measureText(s).width / 2, 15);
            }
            for (let qy = 0; qy < 100; qy += 0.5) {
                let [_, y] = c2dCoordTransform(0, qy);
                let s = `${qy}`;
                ctx.fillStyle = 'black';
                ctx.fillText(s, 18 - ctx.measureText(s).width, y);
            }

            ctx.strokeStyle = 'black';
            for (let qx = 0; qx < 100; qx += 0.5) {
                let [x, _] = c2dCoordTransform(qx, 0);
                let s = `${qx}`;
                ctx.fillStyle = 'black';
                ctx.fillText(s, x - ctx.measureText(s).width / 2, 15);
                for (let qy = qx % 1; qy < 100; qy += 1) {
                    let [x, y] = c2dCoordTransform(qx, qy);
                    ctx.fillStyle = 'white';
                    let isUnused = !usedQubitCoordSet.has(`${qx},${qy}`);
                    let isVeryUnused = !operatedOnQubitSet.has(`${qx},${qy}`);
                    if (isUnused) {
                        ctx.globalAlpha *= 0.25;
                    }
                    if (isVeryUnused) {
                        ctx.globalAlpha *= 0.25;
                    }
                    ctx.fillRect(x - rad, y - rad, 2*rad, 2*rad);
                    ctx.strokeRect(x - rad, y - rad, 2*rad, 2*rad);
                    if (isUnused) {
                        ctx.globalAlpha *= 4;
                    }
                    if (isVeryUnused) {
                        ctx.globalAlpha *= 4;
                    }
                }
            }
        });

        for (let [mi, p] of propagatedMarkerLayers.entries()) {
            drawCrossMarkers(ctx, snap, qubitDrawCoords, p, mi);
        }

        for (let op of circuit.layers[snap.curLayer].iter_gates_and_markers()) {
            if (op.gate.name !== 'POLYGON') {
                op.id_draw(qubitDrawCoords, ctx);
            }
        }

        defensiveDraw(ctx, () => {
            ctx.globalAlpha *= 0.25
            for (let [qx, qy] of snap.timelineSet.values()) {
                let [x, y] = c2dCoordTransform(qx, qy);
                ctx.fillStyle = 'yellow';
                ctx.fillRect(x - rad * 1.25, y - rad * 1.25, 2.5*rad, 2.5*rad);
            }
        });

        defensiveDraw(ctx, () => {
            ctx.globalAlpha *= 0.5
            for (let [qx, qy] of snap.focusedSet.values()) {
                let [x, y] = c2dCoordTransform(qx, qy);
                ctx.fillStyle = 'blue';
                ctx.fillRect(x - rad * 1.25, y - rad * 1.25, 2.5*rad, 2.5*rad);
            }
        });

        drawMarkers(ctx, snap, qubitDrawCoords, propagatedMarkerLayers);

        if (focusX !== undefined) {
            ctx.save();
            ctx.globalAlpha *= 0.5;
            let [x, y] = c2dCoordTransform(focusX, focusY);
            ctx.fillStyle = 'red';
            ctx.fillRect(x - rad, y - rad, 2*rad, 2*rad);
            ctx.restore();
        }

        defensiveDraw(ctx, () => {
            ctx.globalAlpha *= 0.25;
            ctx.fillStyle = 'blue';
            if (snap.mouseDownX !== undefined && snap.curMouseX !== undefined) {
                let x1 = Math.min(snap.curMouseX, snap.mouseDownX);
                let x2 = Math.max(snap.curMouseX, snap.mouseDownX);
                let y1 = Math.min(snap.curMouseY, snap.mouseDownY);
                let y2 = Math.max(snap.curMouseY, snap.mouseDownY);
                x1 -= 1;
                x2 += 1;
                y1 -= 1;
                y2 += 1;
                x1 -= OFFSET_X;
                x2 -= OFFSET_X;
                y1 -= OFFSET_Y;
                y2 -= OFFSET_Y;
                ctx.fillRect(x1, y1, x2 - x1, y2 - y1);
            }
            for (let [qx, qy] of snap.boxHighlightPreview) {
                let [x, y] = c2dCoordTransform(qx, qy);
                ctx.fillRect(x - rad, y - rad, rad*2, rad*2);
            }
        });
    });

    drawTimeline(ctx, snap, propagatedMarkerLayers, qubitDrawCoords, circuit.layers.length, reversedMarkers);

    // Draw scrubber.
    ctx.save();
    try {
        ctx.strokeStyle = 'black';
        ctx.translate(Math.floor(ctx.canvas.width / 2), 0);
        for (let k = 0; k < circuit.layers.length; k++) {
            let hasPolygons = false;
            let hasXMarker = false;
            let hasYMarker = false;
            let hasZMarker = false;
            let hasResetOperations = circuit.layers[k].hasResetOperations();
            let hasMeasurements = circuit.layers[k].hasMeasurementOperations();
            let hasTwoQubitGate = false;
            let hasMultiQubitGate = false;
            let hasSingleQubitClifford = circuit.layers[k].hasSingleQubitCliffords();
            for (let op of circuit.layers[k].markers) {
                hasPolygons |= op.gate.name === "POLYGON";
                hasXMarker |= op.gate.name === "MARKX";
                hasYMarker |= op.gate.name === "MARKY";
                hasZMarker |= op.gate.name === "MARKZ";
            }
            for (let op of circuit.layers[k].id_ops.values()) {
                hasTwoQubitGate |= op.id_targets.length === 2;
                hasMultiQubitGate |= op.id_targets.length > 2;
            }
            ctx.fillStyle = 'white';
            ctx.fillRect(k * 8, 0, 8, 20);
            if (hasSingleQubitClifford) {
                ctx.fillStyle = '#FF0';
                ctx.fillRect(k * 8, 0, 8, 20);
            } else if (hasPolygons) {
                ctx.fillStyle = '#FBB';
                ctx.fillRect(k * 8, 0, 8, 7);
                ctx.fillStyle = '#BFB';
                ctx.fillRect(k * 8, 7, 8, 7);
                ctx.fillStyle = '#BBF';
                ctx.fillRect(k * 8, 14, 8, 6);
            }
            if (hasMeasurements) {
                ctx.fillStyle = '#DDD';
                ctx.fillRect(k * 8, 0, 8, 20);
            } else if (hasResetOperations) {
                ctx.fillStyle = '#DDD';
                ctx.fillRect(k * 8, 0, 4, 20);
            }
            if (hasXMarker) {
                ctx.fillStyle = 'red';
                ctx.fillRect(k * 8 + 3, 14, 3, 3);
            }
            if (hasYMarker) {
                ctx.fillStyle = 'green';
                ctx.fillRect(k * 8 + 3, 9, 3, 3);
            }
            if (hasZMarker) {
                ctx.fillStyle = 'blue';
                ctx.fillRect(k * 8 + 3, 3, 3, 3);
            }
            if (hasMultiQubitGate) {
                ctx.strokeStyle = 'black';
                ctx.beginPath();
                let x = k * 8 + 0.5;
                for (let dx of [3, 5]) {
                    ctx.moveTo(x + dx, 6);
                    ctx.lineTo(x + dx, 15);
                }
                ctx.stroke();
            }
            if (hasTwoQubitGate) {
                ctx.strokeStyle = 'black';
                ctx.beginPath();
                ctx.moveTo(k * 8 + 0.5 + 4, 6);
                ctx.lineTo(k * 8 + 0.5 + 4, 15);
                ctx.stroke();
            }
        }
        ctx.fillStyle = 'black';
        ctx.beginPath();
        ctx.moveTo(snap.curLayer * 8 + 0.5 + 4, 16);
        ctx.lineTo(snap.curLayer * 8 + 0.5 - 2, 28);
        ctx.lineTo(snap.curLayer * 8 + 0.5 + 10, 28);
        ctx.closePath();
        ctx.fill();

        for (let k = 0; k < circuit.layers.length; k++) {
            let has_errors = ![...propagatedMarkerLayers.values()].every(p => p.atLayer(k).errors.size === 0);
            let hasOps = circuit.layers[k].id_ops.size > 0 || circuit.layers[k].markers.length > 0;
            if (has_errors) {
                ctx.strokeStyle = 'magenta';
                ctx.lineWidth = 4;
                ctx.strokeRect(k*8 + 0.5 - 1, 0.5 - 1, 7 + 2, 20 + 2);
                ctx.lineWidth = 1;
            } else {
                ctx.strokeStyle = '#000';
                ctx.strokeRect(k*8 + 0.5, 0.5, 8, 20);
            }
        }
    } finally {
        ctx.restore();
    }
}

export {xyToPos, draw, setDefensiveDrawEnabled, OFFSET_X, OFFSET_Y}
