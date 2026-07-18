/**
 * Copyright 2023 Craig Gidney
 * Copyright 2025 Riverlane
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Modifications:
 * - Refactored for CrumPy
 */

import { indentCircuitLines, drawLinksToTimelineViewer, OFFSET_Y, rad } from "./config.js";
import { stroke_connector_to } from "../gates/gate_draw_util.js";
import { marker_placement } from "../gates/gateset_markers.js";

let TIMELINE_PITCH = 32;
let PADDING_VERTICAL = rad;
let MAX_CANVAS_WIDTH = 4096;

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
 * @param {!Map<!int, !PropagatedPauliFrames>} propagatedMarkerLayers
 * @param {!function(!int): ![!number, !number]} timesliceQubitCoordsFunc
 * @param {!int} numLayers
 */
function drawTimeline(ctx, snap, propagatedMarkerLayers, timesliceQubitCoordsFunc, numLayers) {
  let w = MAX_CANVAS_WIDTH;

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
  let cur_y = PADDING_VERTICAL + rad;
  let max_run = 0;
  let cur_run = 0;
  for (let q of qubits) {
    let [x, y] = timesliceQubitCoordsFunc(q);
    if (prev_y !== y) {
      cur_x = w / 2;
      max_run = Math.max(max_run, cur_run);
      cur_run = 0;
      if (prev_y !== undefined) {
        // first qubit's y is at initial cur_y value
        cur_y += TIMELINE_PITCH * 0.25;
      }
      prev_y = y;
    } else {
      if (indentCircuitLines) {
        cur_x += rad * 0.25; // slight x offset between qubits in a row
      }
      cur_run++;
    }
    base_y2xy.set(`${x},${y}`, [Math.round(cur_x) + 0.5, Math.round(cur_y) + 0.5]);
    cur_y += TIMELINE_PITCH;
  }

  let x_pitch = TIMELINE_PITCH + Math.ceil(rad*max_run*0.25);
  let num_cols_half = Math.floor(w / 2 / x_pitch);
  let min_t_free = snap.curLayer - num_cols_half + 1;
  let min_t_clamp = Math.max(0, Math.min(min_t_free, numLayers - num_cols_half*2 + 1));
  let max_t = Math.min(min_t_clamp + num_cols_half*2 + 2, numLayers);
  let t2t = t => {
    let dt = t - snap.curLayer;
    dt -= min_t_clamp - min_t_free;
    return dt*x_pitch;
  }
  let coordTransform_t = ([x, y, t]) => {
    let key = `${x},${y}`;
    if (!base_y2xy.has(key)) {
      return [undefined, undefined];
    }
    let [xb, yb] = base_y2xy.get(key);
    return [xb + t2t(t), yb];
  };
  let qubitTimeCoords = (q, t) => {
    let [x, y] = timesliceQubitCoordsFunc(q);
    return coordTransform_t([x, y, t]);
  };

  ctx.save();

  // Using coords function, see if any qubit labels would get cut off
  let maxLabelWidth = 0;
  let topLeftX = qubitTimeCoords(qubits[0], min_t_clamp - 1)[0];
  for (let q of qubits) {
    let [x, y] = qubitTimeCoords(q, min_t_clamp - 1);
    let qx = snap.circuit.qubitCoordData[q * 2];
    let qy = snap.circuit.qubitCoordData[q * 2 + 1];
    let label = `${qx},${qy}:`;
    let labelWidth = ctx.measureText(label).width;
    let labelWidthFromTop = labelWidth - (x - topLeftX);
    maxLabelWidth = Math.max(maxLabelWidth, labelWidthFromTop);
  }
  let textOverflowLen = Math.max(0, maxLabelWidth - topLeftX);

  // Adjust coords function to ensure all qubit labels fit on canvas (+ small pad)
  let labelShiftedQTC = (q, t) => {
    let [x, y] = qubitTimeCoords(q, t);
    return [x + Math.ceil(textOverflowLen) + 3, y];
  };

  // Resize canvas to fit circuit
  let timelineHeight =
    labelShiftedQTC(qubits.at(-1), max_t + 1)[1] + rad + PADDING_VERTICAL; // y of lowest qubit line + padding
  let timelineWidth = Math.max(
    ...qubits.map((q) => labelShiftedQTC(q, max_t + 1)[0]),
  ); // max x of any qubit line's endpoint
  ctx.canvas.width = Math.floor(timelineWidth);
  ctx.canvas.height = Math.floor(timelineHeight);

  try {
    ctx.fillStyle = 'white';
    ctx.fillRect(0, 0, ctx.canvas.width, ctx.canvas.height);

    // Draw colored indicators showing Pauli propagation.
    let hitCounts = new Map();
    for (let [mi, p] of propagatedMarkerLayers.entries()) {
      drawTimelineMarkers(ctx, snap, labelShiftedQTC, p, mi, min_t_clamp, max_t, x_pitch, hitCounts);
    }

    // Draw wire lines.
    ctx.strokeStyle = 'black';
    ctx.fillStyle = 'black';
    for (let q of qubits) {
      let [x0, y0] = labelShiftedQTC(q, min_t_clamp - 1);
      let [x1, y1] = labelShiftedQTC(q, max_t + 1);
      ctx.beginPath();
      ctx.moveTo(x0, y0);
      ctx.lineTo(x1, y1);
      ctx.stroke();
    }

    // Draw wire labels.
    ctx.textAlign = 'right';
    ctx.textBaseline = 'middle';
    for (let q of qubits) {
      let [x, y] = labelShiftedQTC(q, min_t_clamp - 1);
      let qx = snap.circuit.qubitCoordData[q * 2];
      let qy = snap.circuit.qubitCoordData[q * 2 + 1];
      let label = `${qx},${qy}:`;
      ctx.fillText(label, x, y);
    }

    // Draw layers of gates.
    for (let time = min_t_clamp; time <= max_t; time++) {
      let qubitsCoordsFuncForLayer = (q) => labelShiftedQTC(q, time);
      let layer = snap.circuit.layers[time];
      if (layer === undefined) {
        continue;
      }
      for (let op of layer.iter_gates_and_markers()) {
        op.id_draw(qubitsCoordsFuncForLayer, ctx);
      }
    }
    if (drawLinksToTimelineViewer) {
      // Draw links to timeslice viewer.
      ctx.globalAlpha = 0.5;
      for (let q of qubits) {
          let [x0, y0] = qubitTimeCoords(q, min_t_clamp - 1);
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
    }
  } finally {
    ctx.restore();
  }
}

export { drawTimeline };
