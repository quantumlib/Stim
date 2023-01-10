import {rad} from "./config.js";
import {stroke_connector_to} from "../gates/gate_draw_util.js"

let TIMELINE_PITCH = 32;

/**
 * @param {!CanvasRenderingContext2D} ctx
 * @param {!StateSnapshot} ds
 * @param {!function(!int, !number): ![!number, !number]} qubitTimeCoordFunc
 * @param {!PropagatedPauliFrames} propagatedMarkers
 * @param {!int} mi
 * @param {!int} min_t
 * @param {!int} max_t
 * @param {!number} x_pitch
 */
function drawTimelineMarkers(ctx, ds, qubitTimeCoordFunc, propagatedMarkers, mi, min_t, max_t, x_pitch) {
    let dx, dy, wx, wy;
    dx = 0;
    wx = x_pitch;
    wy = 5;
    if (mi === 0) {
        dy = 10;
    } else if (mi === 1) {
        dy = 5;
    } else if (mi === 2) {
        dy = 0;
    } else if (mi === 3) {
        dy = -5;
    }
    for (let t = min_t; t <= max_t; t++) {
        let p = propagatedMarkers.atLayer(t);
        for (let [q, b] of p.bases.entries()) {
            let [x, y] = qubitTimeCoordFunc(q, t);
            if (x === undefined || y === undefined) {
                continue;
            }
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
        for (let q of p.errors) {
            let [x, y] = qubitTimeCoordFunc(q, t - 0.5);
            if (x === undefined || y === undefined) {
                continue;
            }
            ctx.fillStyle = 'magenta'
            ctx.fillRect(x - dx - 8, y - dy - 8, wx + 16, wy + 16);
            ctx.fillStyle = 'black'
            ctx.fillRect(x - dx, y - dy, wx, wy);
        }
        for (let {q1, q2, color} of p.crossings) {
            let [x1, y1] = qubitTimeCoordFunc(q1, t);
            let [x2, y2] = qubitTimeCoordFunc(q2, t);
            if (color === 'X') {
                ctx.strokeStyle = 'red';
            } else if (color === 'Y') {
                ctx.strokeStyle = 'green';
            } else if (color === 'Z') {
                ctx.strokeStyle = 'blue';
            } else {
                ctx.strokeStyle = 'magenta'
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
 * @param {!Array<!PropagatedPauliFrames>} propagatedMarkerLayers
 * @param {!function(!int): ![!number, !number]} timesliceQubitCoordsFunc
 */
function drawTimeline(ctx, snap, propagatedMarkerLayers, timesliceQubitCoordsFunc) {
    let w = ctx.canvas.width / 2;

    let qubits = snap.timelineQubits();
    qubits.sort((a, b) => {
        let [x1, y1] = timesliceQubitCoordsFunc(a);
        let [x2, y2] = timesliceQubitCoordsFunc(b);
        if (y1 !== y2) {
            return y1 - y2;
        }
        return x1 - x2;
    });

    let base_y2xy = new Map();
    let prev_y = undefined;
    let cur_x = 0;
    let cur_y = 0;
    let max_run = 0;
    let cur_run = 0;
    for (let q of qubits) {
        let [x, y] = timesliceQubitCoordsFunc(q);
        cur_y += TIMELINE_PITCH;
        if (prev_y !== y) {
            prev_y = y;
            cur_x = w * 1.5;
            max_run = Math.max(max_run, cur_run);
            cur_run = 0;
            cur_y += TIMELINE_PITCH * 0.25;
        } else {
            cur_x += rad * 0.25;
            cur_run++;
        }
        base_y2xy.set(`${x},${y}`, [cur_x, cur_y]);
    }

    let x_pitch = TIMELINE_PITCH + rad*max_run*0.25;
    let coordTransform_t = ([x, y, t]) => {
        let key = `${x},${y}`;
        if (!base_y2xy.has(key)) {
            return [undefined, undefined];
        }
        let [xb, yb] = base_y2xy.get(key);
        return [xb + (t - snap.curLayer)*x_pitch, yb];
    };
    let qubitTimeCoords = (q, t) => {
        let [x, y] = timesliceQubitCoordsFunc(q);
        return coordTransform_t([x, y, t]);
    }
    let num_cols_half = Math.floor(ctx.canvas.width / 4 / x_pitch);
    let min_t = Math.max(0, snap.curLayer - num_cols_half + 1);
    let max_t = snap.curLayer + num_cols_half + 2;

    ctx.save();
    try {
        ctx.clearRect(w, 0, w, ctx.canvas.height);
        for (let mi = 0; mi < propagatedMarkerLayers.length; mi++) {
            if (mi < 4) {
                drawTimelineMarkers(ctx, snap, qubitTimeCoords, propagatedMarkerLayers[mi], mi, min_t, max_t, x_pitch);
            }
        }
        ctx.globalAlpha *= 0.5;
        ctx.fillStyle = 'black';
        ctx.fillRect(w*1.5 - rad*1.3, 0, x_pitch, ctx.canvas.height);
        ctx.globalAlpha *= 2;

        ctx.strokeStyle = 'black';
        ctx.fillStyle = 'black';

        // Draw wire lines.
        for (let q of qubits) {
            let [x0, y0] = qubitTimeCoords(q, min_t - 1);
            let [x1, y1] = qubitTimeCoords(q, max_t + 1);
            ctx.beginPath();
            ctx.moveTo(x0, y0);
            ctx.lineTo(x1, y1);
            ctx.stroke();
        }

        // Draw labels.
        ctx.textAlign = 'right';
        ctx.textBaseline = 'middle';
        for (let q of qubits) {
            let [x, y] = qubitTimeCoords(q, min_t - 1);
            let qx = snap.circuit.qubitCoordData[q * 2];
            let qy = snap.circuit.qubitCoordData[q * 2 + 1];
            ctx.fillText(`${qx},${qy}:`, x, y);
        }

        for (let time = min_t; time <= max_t; time++) {
            let qubitsCoordsFuncForLayer = q => qubitTimeCoords(q, time);
            let layer = snap.circuit.layers[time];
            if (layer === undefined) {
                continue;
            }
            for (let op of layer.iter_gates_and_markers()) {
                op.id_draw(qubitsCoordsFuncForLayer, ctx);
            }
        }

        // Draw links to timeslice viewer.
        ctx.globalAlpha = 0.25;
        ctx.setLineDash([1, 1]);
        let max_x = 0;
        for (let q of qubits) {
            max_x = Math.max(max_x, timesliceQubitCoordsFunc(q)[0]);
        }
        for (let q of qubits) {
            let [x0, y0] = qubitTimeCoords(q, min_t - 1);
            let [_, y1] = timesliceQubitCoordsFunc(q);
            ctx.beginPath();
            ctx.moveTo(x0, y0);
            ctx.lineTo(max_x + 0.25, y1);
            ctx.stroke();
        }
    } finally {
        ctx.restore();
    }
}

export {drawTimeline}