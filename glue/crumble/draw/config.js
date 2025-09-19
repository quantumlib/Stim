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

const pitch = 50;
const rad = 10;
const OFFSET_X = -pitch + Math.floor(pitch / 4) + 0.5;
const OFFSET_Y = -pitch + Math.floor(pitch / 4) + 0.5;
let indentCircuitLines = true;
let curveConnectors = true;
let showAnnotationRegions = true;

const setIndentCircuitLines = (newBool) => {
  if (typeof newBool !== "boolean") {
    throw new TypeError(`Expected a boolean, but got ${typeof newBool}`);
  }
  indentCircuitLines = newBool;
};

const setCurveConnectors = (newBool) => {
  if (typeof newBool !== "boolean") {
    throw new TypeError(`Expected a boolean, but got ${typeof newBool}`);
  }
  curveConnectors = newBool;
};

const setShowAnnotationRegions = (newBool) => {
  if (typeof newBool !== "boolean") {
    throw new TypeError(`Expected a boolean, but got ${typeof newBool}`);
  }
  showAnnotationRegions = newBool;
};

export {
  pitch,
  rad,
  OFFSET_X,
  OFFSET_Y,
  indentCircuitLines,
  curveConnectors,
  showAnnotationRegions,
  setIndentCircuitLines,
  setCurveConnectors,
  setShowAnnotationRegions,
};
