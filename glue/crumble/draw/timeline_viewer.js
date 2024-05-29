import {rad} from "./config.js";
import {stroke_connector_to} from "../gates/gate_draw_util.js"
import {OFFSET_Y} from './main_draw.js';
import {marker_placement} from '../gates/gateset_markers.js';

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
 * @param {!Map} hitCounts
 */
function drawTimelineMarkers(ctx, ds, qubitTimeCoordFunc, propagatedMarkers, mi, min_t, max_t, x_pitch, hitCounts) {
    for (let t = min_t - 1; t <= max_t; t++) {
        if (!hitCounts.has(t)) {
            hitCounts.set(t, new Map());
        }
        let hitCount = hitCounts.get(t);
        let p1 = propagatedMarkers.atLayer(t + 0.5);
        let p0 = propagatedMarkers.atLayer(t);
        for (let [q, b] of p1.bases.entries()) {
            let {dx, dy, wx, wy} = marker_placement(mi, q, hitCount);
            if (mi >= 0 && mi < 4) {
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
            } else {
                dx -= x_pitch / 2;
            }
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
        for (let q of p0.errors) {
            let {dx, dy, wx, wy} = marker_placement(mi, q, hitCount);
            dx -= x_pitch / 2;

            let [x, y] = qubitTimeCoordFunc(q, t - 0.5);
            if (x === undefined || y === undefined) {
                continue;
            }
            ctx.strokeStyle = 'magenta';
            ctx.lineWidth = 8;
            ctx.strokeRect(x - dx, y - dy, wx, wy);
            ctx.lineWidth = 1;
            ctx.fillStyle = 'black';
            ctx.fillRect(x - dx, y - dy, wx, wy);
        }
        for (let {q1, q2, color} of p0.crossings) {
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
 * @param {!Map<!int, !PropagatedPauliFrames>} propagatedMarkerLayers
 * @param {!function(!int): ![!number, !number]} timesliceQubitCoordsFunc
 */
function drawTimeline(ctx, snap, propagatedMarkerLayers, timesliceQubitCoordsFunc) {
    let w = Math.floor(ctx.canvas.width / 2);

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
        base_y2xy.set(`${x},${y}`, [Math.round(cur_x) + 0.5, Math.round(cur_y) + 0.5]);
    }

    let x_pitch = TIMELINE_PITCH + Math.ceil(rad*max_run*0.25);
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
        let hitCounts = new Map();
        for (let [mi, p] of propagatedMarkerLayers.entries()) {
            drawTimelineMarkers(ctx, snap, qubitTimeCoords, p, mi, min_t, max_t, x_pitch, hitCounts);
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
        ctx.globalAlpha = 0.5;
        for (let q of qubits) {
            let [x0, y0] = qubitTimeCoords(q, min_t - 1);
            let [x1, y1] = timesliceQubitCoordsFunc(q);
            if (snap.curMouseX > ctx.canvas.width / 2 && snap.curMouseY >= y0 + OFFSET_Y - TIMELINE_PITCH * 0.55 && snap.curMouseY <= y0 + TIMELINE_PITCH * 0.55 + OFFSET_Y) {
                ctx.beginPath();
                ctx.moveTo(x0, y0);
                ctx.lineTo(x1, y1);
                ctx.stroke();
                ctx.fillStyle = 'black';
                ctx.fillRect(x1 - 20, y1 - 20, 40, 40);
                ctx.fillRect(ctx.canvas.width / 2, y0 - TIMELINE_PITCH / 3, ctx.canvas.width / 2, TIMELINE_PITCH * 2 / 3);
            }
        }
    } finally {
        ctx.restore();
    }
}

export {drawTimeline}