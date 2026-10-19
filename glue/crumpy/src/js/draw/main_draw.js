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

import {pitch, rad, showAnnotationRegions, OFFSET_X, OFFSET_Y} from "./config.js"
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
    });
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
  for (let layer of circuit.layers) {
    for (let op of layer.markers) {
      let gate = op.gate;
      if (gate.name === "MARKX" || gate.name === "MARKY" || gate.name === "MARKZ") {
        numPropagatedLayers = Math.max(numPropagatedLayers, op.args[0] + 1);
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
      propagatedMarkerLayers.set(mi, PropagatedPauliFrames.fromCircuit(circuit, mi));
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

  if (showAnnotationRegions) {
    for (let mi = 0; mi < dets.length; mi++) {
      propagatedMarkerLayers.set(~mi, batch_output[batch_index++]);
    }
    for (let mi of obs.keys()) {
      propagatedMarkerLayers.set(~mi ^ (1 << 30), batch_output[batch_index++]);
    }
  }

  drawTimeline(
    ctx,
    snap,
    propagatedMarkerLayers,
    qubitDrawCoords,
    circuit.layers.length,
  );

  ctx.save();
}

export {xyToPos, draw, setDefensiveDrawEnabled, OFFSET_X, OFFSET_Y}
