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

import { Circuit } from "./circuit/circuit.js";
import { draw } from "./draw/main_draw.js";
import { EditorState } from "./editor/editor_state.js";
import {
  setIndentCircuitLines,
  setCurveConnectors,
  setShowAnnotationRegions,
} from "./draw/config.js";

const CANVAS_W = 600;
const CANVAS_H = 300;

function initCanvas(el) {
  let scrollWrap = document.createElement("div");
  scrollWrap.setAttribute("style", "overflow-x: auto; overflow-y: hidden;");

  let canvas = document.createElement("canvas");
  canvas.id = "cvn";
  canvas.setAttribute("style", `margin: 0; padding: 0;`);
  canvas.tabIndex = 0;
  canvas.width = CANVAS_W;
  canvas.height = CANVAS_H;

  scrollWrap.appendChild(canvas);
  el.appendChild(scrollWrap);

  return canvas;
}

function render({ model, el }) {
  const traitlets = {
    getStim: () => model.get("stim"),
    getIndentCircuitLines: () => model.get("indentCircuitLines"),
    getCurveConnectors: () => model.get("curveConnectors"),
    getShowAnnotationRegions: () => model.get("showAnnotationRegions"),
  };

  const canvas = initCanvas(el);

  let editorState = /** @type {!EditorState} */ new EditorState(canvas);

  const exportCurrentState = () =>
    editorState
      .copyOfCurCircuit()
      .toStimCircuit()
      .replaceAll("\nPOLYGON", "\n#!pragma POLYGON")
      .replaceAll("\nERR", "\n#!pragma ERR")
      .replaceAll("\nMARK", "\n#!pragma MARK");
  function commitStimCircuit(stim_str) {
    let circuit = Circuit.fromStimCircuit(stim_str);
    editorState.commit(circuit);
  }

  // Changes to circuit on the Python/notebook side update the JS
  model.on("change:stim", () => commitStimCircuit(traitlets.getStim()));
  model.on("change:indentCircuitLines", () => {
    setIndentCircuitLines(traitlets.getIndentCircuitLines());
    editorState.force_redraw();
  });
  model.on("change:curveConnectors", () => {
    setCurveConnectors(traitlets.getCurveConnectors());
    editorState.force_redraw();
  });
  model.on("change:showAnnotationRegions", () => {
    setShowAnnotationRegions(traitlets.getShowAnnotationRegions());
    editorState.force_redraw();
  });

  // Listeners on editor state that trigger redraws
  editorState.rev.changes().subscribe(() => {
    editorState.obs_val_draw_state.set(editorState.toSnapshot(undefined));
  });
  editorState.obs_val_draw_state.observable().subscribe((ds) =>
    requestAnimationFrame(() => {
      draw(editorState.canvas.getContext("2d"), ds);
    }),
  );

  // Configure initial settings and stim
  setIndentCircuitLines(traitlets.getIndentCircuitLines());
  setCurveConnectors(traitlets.getCurveConnectors());
  setShowAnnotationRegions(traitlets.getShowAnnotationRegions());
  commitStimCircuit(traitlets.getStim());
}
export default { render };
