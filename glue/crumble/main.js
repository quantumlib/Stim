import {Circuit} from "./circuit/circuit.js"
import {minXY} from "./circuit/layer.js"
import {pitch} from "./draw/config.js"
import {GATE_MAP} from "./gates/gateset.js"
import {EditorState} from "./editor/editor_state.js";
import {initUrlCircuitSync} from "./editor/sync_url_to_state.js";
import {draw} from "./draw/main_draw.js";
import {drawToolbox} from "./keyboard/toolbox.js";
import {Operation} from "./circuit/operation.js";

const OFFSET_X = -pitch + Math.floor(pitch / 4) + 0.5;
const OFFSET_Y = -pitch + Math.floor(pitch / 4) + 0.5;

const btnUndo = /** @type{!HTMLButtonElement} */ document.getElementById('btnUndo');
const btnRedo = /** @type{!HTMLButtonElement} */ document.getElementById('btnRedo');
const btnClearMarkers = /** @type{!HTMLButtonElement} */ document.getElementById('btnClearMarkers');
const btnImportExport = /** @type{!HTMLButtonElement} */ document.getElementById('btnShowHideImportExport');
const btnNextLayer = /** @type{!HTMLButtonElement} */ document.getElementById('btnNextLayer');
const btnPrevLayer = /** @type{!HTMLButtonElement} */ document.getElementById('btnPrevLayer');
const btnRotate45 = /** @type{!HTMLButtonElement} */ document.getElementById('btnRotate45');
const btnRotate45Counter = /** @type{!HTMLButtonElement} */ document.getElementById('btnRotate45Counter');
const btnExport = /** @type {!HTMLButtonElement} */ document.getElementById('btnExport');
const btnImport = /** @type {!HTMLButtonElement} */ document.getElementById('btnImport');
const btnClear = /** @type {!HTMLButtonElement} */ document.getElementById('clear');
const txtStimCircuit = /** @type {!HTMLTextAreaElement} */ document.getElementById('txtStimCircuit');

// Prevent typing in the import/export text editor from causing changes in the main circuit editor.
txtStimCircuit.addEventListener('keyup', ev => ev.stopPropagation());
txtStimCircuit.addEventListener('keydown', ev => ev.stopPropagation());

let editorState = /** @type {!EditorState} */ new EditorState(document.getElementById('cvn'));

btnExport.addEventListener('click', _ev => {
    exportCurrentState();
});
btnImport.addEventListener('click', _ev => {
    let text = txtStimCircuit.value;
    let circuit = Circuit.fromStimCircuit(text.replaceAll('\n#!pragma ', '\n'));
    editorState.commit(circuit);
});

btnImportExport.addEventListener('click', _ev => {
    let div = /** @type{!HTMLDivElement} */ document.getElementById('divImportExport');
    if (div.style.display === 'none') {
        div.style.display = 'block';
        btnImportExport.textContent = "Hide Import/Export";
        exportCurrentState();
    } else {
        div.style.display = 'none';
        btnImportExport.textContent = "Show Import/Export";
        txtStimCircuit.value = '';
    }
    setTimeout(() => {
        window.scrollTo(0, 0);
    }, 0);
});

btnClear.addEventListener('click', _ev => {
    editorState.clearCircuit();
});

btnUndo.addEventListener('click', _ev => {
    editorState.undo();
});
btnRedo.addEventListener('click', _ev => {
    editorState.redo();
});

btnClearMarkers.addEventListener('click', _ev => {
    editorState.clearMarkers();
});

btnRotate45.addEventListener('click', _ev => {
    editorState.rotate45(+1, false);
});
btnRotate45Counter.addEventListener('click', _ev => {
    editorState.rotate45(-1, false);
});

btnNextLayer.addEventListener('click', _ev => {
    editorState.changeCurLayerTo(editorState.curLayer + 1);
});
btnPrevLayer.addEventListener('click', _ev => {
    editorState.changeCurLayerTo(editorState.curLayer - 1);
});

window.addEventListener('resize', _ev => {
    editorState.canvas.width = editorState.canvas.scrollWidth;
    editorState.canvas.height = editorState.canvas.scrollHeight;
    editorState.force_redraw();
});

function exportCurrentState() {
    let validStimCircuit = editorState.copyOfCurCircuit().toStimCircuit().
        replaceAll('\nPOLYGON', '\n#!pragma POLYGON').
        replaceAll('\nMARK', '\n#!pragma MARK');
    let txt = txtStimCircuit;
    txt.value = validStimCircuit + '\n';
    txt.focus();
    txt.select();
}

editorState.canvas.addEventListener('mousemove', ev => {
    editorState.curMouseX = ev.offsetX + OFFSET_X;
    editorState.curMouseY = ev.offsetY + OFFSET_Y;

    // Scrubber.
    if (editorState.mouseDownX - OFFSET_X < 10 && editorState.curMouseX - OFFSET_X < 10 && ev.buttons === 1) {
        editorState.changeCurLayerTo(Math.floor(ev.offsetY / 5));
        ev.preventDefault();
        return;
    }

    editorState.force_redraw();
});

editorState.canvas.addEventListener('mousedown', ev => {
    editorState.curMouseX = ev.offsetX + OFFSET_X;
    editorState.curMouseY = ev.offsetY + OFFSET_Y;
    editorState.mouseDownX = ev.offsetX + OFFSET_X;
    editorState.mouseDownY = ev.offsetY + OFFSET_Y;
    if (editorState.mouseDownX - OFFSET_X < 10 && ev.buttons === 1) {
        editorState.changeCurLayerTo(Math.floor(ev.offsetY / 5));
        return;
    }
    editorState.force_redraw();
});

editorState.canvas.addEventListener('mouseup', ev => {
    let highlightedArea = editorState.currentPositionsBoxesByMouseDrag(ev.altKey);
    editorState.mouseDownX = undefined;
    editorState.mouseDownY = undefined;
    editorState.curMouseX = ev.offsetX + OFFSET_X;
    editorState.curMouseY = ev.offsetY + OFFSET_Y;
    editorState.changeFocus(highlightedArea, ev.shiftKey, ev.ctrlKey);
});

/**
 * @return {!Map<!string, !function(preview: !boolean) : void>}
 */
function makeChordHandlers() {
    let res = /** @type {!Map<!string, !function(preview: !boolean) : void>} */ new Map();

    res.set('shift+t', preview => editorState.rotate45(-1, preview));
    res.set('t', preview => editorState.rotate45(+1, preview));
    res.set('escape', () => editorState.clearFocus);
    res.set('delete', preview => editorState.deleteAtFocus(preview));
    res.set('backspace', preview => editorState.deleteAtFocus(preview));
    res.set('ctrl+delete', preview => editorState.deleteCurLayer(preview));
    res.set('ctrl+insert', preview => editorState.insertLayer(preview));
    res.set('ctrl+backspace', () => editorState.deleteCurLayer);
    res.set('ctrl+z', preview => { if (!preview) editorState.undo() });
    res.set('ctrl+y', preview => { if (!preview) editorState.redo() });
    res.set('ctrl+shift+z', preview => { if (!preview) editorState.redo() });
    res.set('ctrl+c', async preview => { if (!preview) await copyToClipboard(); });
    res.set('ctrl+v', pasteFromClipboard);
    res.set('ctrl+x', async preview => {
        await copyToClipboard();
        editorState.deleteAtFocus(preview);
    });
    res.set('l', preview => {
        if (!preview) {
            editorState.timelineSet = new Map(editorState.focusedSet.entries());
            editorState.force_redraw();
        }
    });
    res.set(' ', preview => editorState.unmarkFocusInferBasis(preview));
    res.set('q', preview => { if (!preview) editorState.changeCurLayerTo(editorState.curLayer - 1); });
    res.set('e', preview => { if (!preview) editorState.changeCurLayerTo(editorState.curLayer + 1); });

    for (let [key, val] of [
        ['1', 0],
        ['2', 1],
        ['3', 2],
        ['4', 3],
        ['5', 4],
        ['6', 5],
        ['7', 6],
        ['8', 7],
        ['9', 8],
        ['0', 9],
        ['-', 10],
        ['=', 11],
        ['\\', 12],
        ['`', 13],
    ]) {
        res.set(`${key}`, preview => editorState.markFocusInferBasis(preview, val));
        res.set(`${key}+x`, preview => editorState.writeGateToFocus(preview, GATE_MAP.get('MARKX').withDefaultArgument(val)));
        res.set(`${key}+y`, preview => editorState.writeGateToFocus(preview, GATE_MAP.get('MARKY').withDefaultArgument(val)));
        res.set(`${key}+z`, preview => editorState.writeGateToFocus(preview, GATE_MAP.get('MARKZ').withDefaultArgument(val)));
    }

    res.set('p', preview => editorState.writeGateToFocus(preview, GATE_MAP.get("POLYGON"), [1, 0, 0, 0.5]));
    res.set('alt+p', preview => editorState.writeGateToFocus(preview, GATE_MAP.get("POLYGON"), [0, 1, 0, 0.5]));
    res.set('shift+p', preview => editorState.writeGateToFocus(preview, GATE_MAP.get("POLYGON"), [0, 0, 1, 0.5]));
    res.set('p+x', preview => editorState.writeGateToFocus(preview, GATE_MAP.get("POLYGON"), [1, 0, 0, 0.5]));
    res.set('p+y', preview => editorState.writeGateToFocus(preview, GATE_MAP.get("POLYGON"), [0, 1, 0, 0.5]));
    res.set('p+z', preview => editorState.writeGateToFocus(preview, GATE_MAP.get("POLYGON"), [0, 0, 1, 0.5]));
    res.set('p+x+y', preview => editorState.writeGateToFocus(preview, GATE_MAP.get("POLYGON"), [1, 1, 0, 0.5]));
    res.set('p+x+z', preview => editorState.writeGateToFocus(preview, GATE_MAP.get("POLYGON"), [1, 0, 1, 0.5]));
    res.set('p+y+z', preview => editorState.writeGateToFocus(preview, GATE_MAP.get("POLYGON"), [0, 1, 1, 0.5]));
    res.set('p+x+y+z', preview => editorState.writeGateToFocus(preview, GATE_MAP.get("POLYGON"), [1, 1, 1, 0.5]));

    /**
     * @param {!Array<!string>} chords
     * @param {!string} name
     * @param {undefined|!string=}inverse_name
     */
    function addGateChords(chords, name, inverse_name=undefined) {
        for (let chord of chords) {
            if (res.has(chord)) {
                throw new Error("Chord collision: " + chord);
            }
            res.set(chord, preview => editorState.writeGateToFocus(preview, GATE_MAP.get(name)));
        }
        if (inverse_name !== undefined) {
            addGateChords(chords.map(e => 'shift+' + e), inverse_name);
        }
    }

    addGateChords(['h', 'h+y', 'h+x+z'], "H", "H");
    addGateChords(['h+z', 'h+x+y'], "H_XY", "H_XY");
    addGateChords(['h+x', 'h+y+z'], "H_YZ", "H_YZ");
    addGateChords(['s+x', 's+y+z'], "SQRT_X", "SQRT_X_DAG");
    addGateChords(['s+y', 's+x+z'], "SQRT_Y", "SQRT_Y_DAG");
    addGateChords(['s', 's+z', 's+x+y'], "S", "S_DAG");
    addGateChords(['r+x', 'r+y+z'], "RX");
    addGateChords(['r+y', 'r+x+z'], "RY");
    addGateChords(['r', 'r+z', 'r+x+y'], "R");
    addGateChords(['m+x', 'm+y+z'], "MX");
    addGateChords(['m+y', 'm+x+z'], "MY");
    addGateChords(['m', 'm+z', 'm+x+y'], "M");
    addGateChords(['m+r+x', 'm+r+y+z'], "MRX");
    addGateChords(['m+r+y', 'm+r+x+z'], "MRY");
    addGateChords(['m+r', 'm+r+z', 'm+r+x+y'], "MR");
    addGateChords(['c+x'], "CX", "CX");
    addGateChords(['c+y'], "CY", "CY");
    addGateChords(['c+z'], "CZ", "CZ");
    addGateChords(['c+x+y'], "XCY", "XCY");
    addGateChords(['alt+c+x'], "XCX", "XCX");
    addGateChords(['alt+c+y'], "YCY", "YCY");

    addGateChords(['w'], "SWAP", "SWAP");
    addGateChords(['w+x'], "CXSWAP", undefined);
    addGateChords(['c+w+x'], "CXSWAP", undefined);
    addGateChords(['i+w'], "ISWAP", "ISWAP_DAG");

    addGateChords(['f'], "C_XYZ", "C_ZYX");
    addGateChords(['c+s+x'], "SQRT_XX", "SQRT_XX_DAG");
    addGateChords(['c+s+y'], "SQRT_YY", "SQRT_YY_DAG");
    addGateChords(['c+s+z'], "SQRT_ZZ", "SQRT_ZZ_DAG");

    addGateChords(['c+m+x'], "MXX", "MXX");
    addGateChords(['c+m+y'], "MYY", "MYY");
    addGateChords(['c+m+z'], "MZZ", "MZZ");

    return res;
}

let emulatedClipboard = undefined;
async function copyToClipboard() {
    let c = editorState.copyOfCurCircuit();
    c.layers = [c.layers[editorState.curLayer]]
    if (editorState.focusedSet.size > 0) {
        c.layers[0] = c.layers[0].id_filteredByQubit(q => {
            let x = c.qubitCoordData[q * 2];
            let y = c.qubitCoordData[q * 2 + 1];
            return editorState.focusedSet.has(`${x},${y}`);
        });
        let [x, y] = minXY(editorState.focusedSet.values());
        c = c.shifted(-x, -y);
    }

    let content = c.toStimCircuit()
    try {
        await navigator.clipboard.writeText(content);
        emulatedClipboard = undefined;
    } catch (ex) {
        emulatedClipboard = content;
        console.error(ex);
    }
}

/**
 * @param {!boolean} preview
 */
async function pasteFromClipboard(preview) {
    let text;
    try {
        text = await navigator.clipboard.readText();
    } catch (ex) {
        text = emulatedClipboard;
        console.error(ex);
    }
    if (text === undefined) {
        return;
    }

    let pastedCircuit = Circuit.fromStimCircuit(text);
    if (pastedCircuit.layers.length !== 1) {
        throw new Error(text);
    }
    let newCircuit = editorState.copyOfCurCircuit();
    if (editorState.focusedSet.size > 0) {
        let [x, y] = minXY(editorState.focusedSet.values());
        pastedCircuit = pastedCircuit.shifted(x, y);
    }

    // Include new coordinates.
    let usedCoords = [];
    for (let q = 0; q < pastedCircuit.qubitCoordData.length; q += 2) {
        usedCoords.push([pastedCircuit.qubitCoordData[q], pastedCircuit.qubitCoordData[q + 1]]);
    }
    newCircuit = newCircuit.withCoordsIncluded(usedCoords);
    let c2q = newCircuit.coordToQubitMap();

    // Remove existing content at paste location.
    for (let key of editorState.focusedSet.keys()) {
        let q = c2q.get(key);
        if (q !== undefined) {
            newCircuit.layers[editorState.curLayer].id_pop_at(q);
        }
    }

    // Add content to paste location.
    for (let op of pastedCircuit.layers[0].iter_gates_and_markers()) {
        let newTargets = [];
        for (let q of op.id_targets) {
            let x = pastedCircuit.qubitCoordData[2*q];
            let y = pastedCircuit.qubitCoordData[2*q+1];
            newTargets.push(c2q.get(`${x},${y}`));
        }
        newCircuit.layers[editorState.curLayer].put(new Operation(
            op.gate,
            op.args,
            new Uint32Array(newTargets),
        ));
    }

    editorState.commit_or_preview(newCircuit, preview);
}

const CHORD_HANDLERS = makeChordHandlers();
/**
 * @param {!KeyboardEvent} ev
 */
function handleKeyboardEvent(ev) {
    editorState.chorder.handleKeyEvent(ev);
    let evs = editorState.chorder.queuedEvents;
    if (evs.length === 0) {
        return;
    }
    let chord_ev = evs[evs.length - 1];
    while (evs.length > 0) {
        evs.pop();
    }

    let pressed = [...chord_ev.chord];
    if (pressed.length === 0) {
        return;
    }
    pressed.sort();
    let key = '';
    if (chord_ev.altKey) {
        key += 'alt+';
    }
    if (chord_ev.ctrlKey) {
        key += 'ctrl+';
    }
    if (chord_ev.metaKey) {
        key += 'meta+';
    }
    if (chord_ev.shiftKey) {
        key += 'shift+';
    }
    for (let e of pressed) {
        key += `${e}+`;
    }
    key = key.substring(0, key.length - 1);

    let handler = CHORD_HANDLERS.get(key);
    if (handler !== undefined) {
        handler(chord_ev.inProgress);
        ev.preventDefault();
    } else {
        editorState.preview(editorState.copyOfCurCircuit());
    }
}

document.addEventListener('keydown', handleKeyboardEvent);
document.addEventListener('keyup', handleKeyboardEvent);

editorState.canvas.width = editorState.canvas.scrollWidth;
editorState.canvas.height = editorState.canvas.scrollHeight;
editorState.rev.changes().subscribe(() => {
    editorState.obs_val_draw_state.set(editorState.toSnapshot(undefined));
    drawToolbox(editorState.chorder.toEvent(false));
});
initUrlCircuitSync(editorState.rev);
editorState.obs_val_draw_state.observable().subscribe(ds => requestAnimationFrame(() => draw(editorState.canvas.getContext('2d'), ds)));
window.addEventListener('focus', () => {
    editorState.chorder.handleFocusChanged();
});
window.addEventListener('blur', () => {
    editorState.chorder.handleFocusChanged();
});
