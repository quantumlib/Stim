import {GATE_MAP} from "../gates/gateset.js"

let toolboxCanvas = /** @type {!HTMLCanvasElement} */ document.getElementById('toolbox');

let DIAM = 28;
let PITCH = DIAM + 4;
let PAD = 10.5;
let GRACE_DELAY = 200;

let COLUMNS = ['H', 'S', 'R', 'M', 'C', '1', '2', '3', '4'];
let DEF_ROW = [1, 2, 2, 2, undefined, -1, -1, -1, -1];

/**
 * @param {!ChordEvent} ev
 * @returns {undefined|!{row: !int, strength: !number}}
 */
function getFocusedRow(ev) {
    if (ev.ctrlKey) {
        return undefined;
    }
    let hasX = +ev.chord.has('x');
    let hasY = +ev.chord.has('y');
    let hasZ = +ev.chord.has('z');
    if ((hasX && !hasY && !hasZ) || (!hasX && hasY && hasZ)) {
        return {row: 0, strength: Math.max(hasX, Math.min(hasY, hasZ))};
    }
    if ((!hasX && hasY && !hasZ) || (hasX && !hasY && hasZ)) {
        return {row: 1, strength: Math.max(hasY, Math.min(hasX, hasZ))};
    }
    if ((!hasX && !hasY && hasZ) || (hasX && hasY && !hasZ)) {
        return {row: 2, strength: Math.max(hasZ, Math.min(hasX, hasY))};
    }
    return undefined;
}
/**
 * @param {!ChordEvent} ev
 * @returns {undefined|!{col: !int, strength: !number}}
 */
function getFocusedCol(ev) {
    if (ev.ctrlKey) {
        return undefined;
    }
    let best = undefined;
    for (let k = 0; k < COLUMNS.length; k++) {
        let s = ev.chord.has(COLUMNS[k].toLowerCase());
        if (s > 0) {
            if (best === undefined || s > best.strength) {
                best = {col: k, strength: s};
            }
        }
    }
    return best;
}

let POS_TO_GATE_DICT = new Map([
    ['0,0', GATE_MAP.get("H_YZ")],
    ['0,1', GATE_MAP.get("H")],
    ['0,2', GATE_MAP.get("H_XY")],
    ['1,0', GATE_MAP.get("SQRT_X")],
    ['1,1', GATE_MAP.get("SQRT_Y")],
    ['1,2', GATE_MAP.get("S")],
    ['2,0', GATE_MAP.get("RX")],
    ['2,1', GATE_MAP.get("RY")],
    ['2,2', GATE_MAP.get("R")],
    ['3,0', GATE_MAP.get("MX")],
    ['3,1', GATE_MAP.get("MY")],
    ['3,2', GATE_MAP.get("M")],
    ['4,0', GATE_MAP.get("CX")],
    ['4,1', GATE_MAP.get("CY")],
    ['4,2', GATE_MAP.get("CZ")],
    ['5,0', GATE_MAP.get("MARKX").withDefaultArgument(0)],
    ['5,1', GATE_MAP.get("MARKY").withDefaultArgument(0)],
    ['5,2', GATE_MAP.get("MARKZ").withDefaultArgument(0)],
    ['6,0', GATE_MAP.get("MARKX").withDefaultArgument(1)],
    ['6,1', GATE_MAP.get("MARKY").withDefaultArgument(1)],
    ['6,2', GATE_MAP.get("MARKZ").withDefaultArgument(1)],
    ['7,0', GATE_MAP.get("MARKX").withDefaultArgument(2)],
    ['7,1', GATE_MAP.get("MARKY").withDefaultArgument(2)],
    ['7,2', GATE_MAP.get("MARKZ").withDefaultArgument(2)],
    ['8,0', GATE_MAP.get("MARKX").withDefaultArgument(3)],
    ['8,1', GATE_MAP.get("MARKY").withDefaultArgument(3)],
    ['8,2', GATE_MAP.get("MARKZ").withDefaultArgument(3)],
    ['5,-1', GATE_MAP.get("MARK").withDefaultArgument(0)],
    ['6,-1', GATE_MAP.get("MARK").withDefaultArgument(1)],
    ['7,-1', GATE_MAP.get("MARK").withDefaultArgument(2)],
    ['8,-1', GATE_MAP.get("MARK").withDefaultArgument(3)],
])

/**
 * @param {!ChordEvent} ev
 * @returns {{focusedRow: (!{row: !int, strength: !number}|undefined), partialFocusedRow: (!{row: !int, strength: !number}|undefined), focusedCol: (!{col: !int, strength: !number}|undefined), chosenGate: undefined|!Gate}}
 */
function getToolboxFocusedData(ev) {
    let partialFocusedRow = getFocusedRow(ev);
    let focusedCol = getFocusedCol(ev);

    let focusedRow = partialFocusedRow;
    if (focusedCol !== undefined && partialFocusedRow === undefined) {
        let row = DEF_ROW[focusedCol.col];
        if (row === undefined) {
            focusedRow = undefined;
        } else {
            focusedRow = {strength: 0, row: row};
        }
    }
    let chosenGate = undefined;
    if (focusedRow !== undefined && focusedCol !== undefined) {
        let key = `${focusedCol.col},${focusedRow.row}`;
        if (POS_TO_GATE_DICT.has(key)) {
            chosenGate = POS_TO_GATE_DICT.get(key);
        }
    }

    return {partialFocusedRow, focusedRow, focusedCol, chosenGate};
}

/**
 * @param {!ChordEvent} ev
 */
function drawToolbox(ev) {
    toolboxCanvas.width = toolboxCanvas.scrollWidth;
    toolboxCanvas.height = toolboxCanvas.scrollHeight;
    let ctx = toolboxCanvas.getContext('2d');
    ctx.clearRect(0, 0, toolboxCanvas.width, toolboxCanvas.height);
    ctx.textAlign = 'right';
    ctx.textBaseline = 'middle';
    ctx.fillText('X', PAD - 3, PAD + DIAM / 2);
    ctx.fillText('Y', PAD - 3, PAD + DIAM / 2 + PITCH);
    ctx.fillText('Z', PAD - 3, PAD + DIAM / 2 + PITCH * 2);
    ctx.textAlign = 'center';
    ctx.textBaseline = 'bottom';
    for (let k = 0; k < COLUMNS.length; k++) {
        ctx.fillText(COLUMNS[k], PAD + DIAM / 2 + PITCH * k, PAD);
    }

    ctx.fillStyle = 'white';
    ctx.strokeStyle = 'black';
    let xGates = ['H_YZ', 'S_X', 'R_X', 'M_X', 'C_X', 'X1', 'X2', 'X3', 'X4'];
    let yGates = ['H',    'S_Y', 'R_Y', 'M_Y', 'C_Y', 'Y1', 'Y2', 'Y3', 'Y4'];
    let zGates = ['H_XY', 'S',   'R',   'M',   'C_Z', 'Z1', 'Z2', 'Z3', 'Z4'];
    let gates = [xGates, yGates, zGates];
    for (let k = 0; k < COLUMNS.length; k++) {
        for (let p = 0; p < 3; p++) {
            ctx.fillRect(PAD + PITCH * k, PAD + PITCH * p, DIAM, DIAM);
            ctx.strokeRect(PAD + PITCH * k, PAD + PITCH * p, DIAM, DIAM);
        }
    }
    ctx.fillStyle = 'black';
    for (let k = 0; k < COLUMNS.length; k++) {
        for (let p = 0; p < 3; p++) {
            let text = gates[p][k];
            let cx = PAD + PITCH * k + DIAM / 2;
            let cy = PAD + PITCH * p + DIAM / 2;
            if (text.indexOf('_') !== -1) {
                let [main, sub] = text.split('_');
                ctx.font = '16pt monospace';
                ctx.textAlign = 'right';
                ctx.textBaseline = 'middle';
                ctx.fillText(main, cx, cy);
                ctx.font = sub.length === 1 ? '12pt monospace' : '8pt monospace';
                ctx.textAlign = 'left';
                ctx.textBaseline = 'top';
                ctx.fillText(sub, cx, cy);
            } else {
                ctx.font = '16pt monospace';
                ctx.textAlign = 'center';
                ctx.textBaseline = 'middle';
                ctx.fillText(text, cx, cy);
            }
        }
    }

    let curTime = performance.now();
    let focus = getToolboxFocusedData(ev);

    if (focus.partialFocusedRow !== undefined) {
        ctx.fillStyle = 'rgba(255, 255, 0, ' + (0.5 * focus.partialFocusedRow.strength) + ')';
        ctx.fillRect(0, PAD + PITCH * focus.partialFocusedRow.row - (PITCH - DIAM) / 2, PAD + PITCH * COLUMNS.length, PITCH);
    }
    if (focus.focusedCol !== undefined) {
        ctx.fillStyle = 'rgba(255, 255, 0, ' + (0.5 * focus.focusedCol.strength) + ')';
        ctx.fillRect( PAD + PITCH * focus.focusedCol.col - (PITCH - DIAM) / 2, 0, PITCH, PAD + PITCH * 3);
    }
    if (focus.focusedRow !== undefined && focus.focusedCol !== undefined) {
        ctx.fillStyle = 'rgba(255, 0, 0, 0.5)';
        ctx.fillRect( PAD + PITCH * focus.focusedCol.col - (PITCH - DIAM) / 2, PAD + PITCH * focus.focusedRow.row - (PITCH - DIAM) / 2, PITCH, PITCH);
    }

    ctx.textAlign = 'left';
    ctx.textBaseline = 'middle';
    ctx.fillStyle = 'black';
    ctx.fillText("W=SWAP", PAD + PITCH * COLUMNS.length, PAD);
    ctx.fillText("F=C_XYZ", PAD + PITCH * COLUMNS.length, PAD + 20);
    ctx.fillText("P=POLYGON", PAD + PITCH * COLUMNS.length, PAD + 40);
    ctx.fillText("space=unmark", PAD + PITCH * COLUMNS.length, PAD + 60);
    ctx.fillText("I=ISWAP", PAD + PITCH * COLUMNS.length, PAD + 80);
    ctx.fillText("L=TIMELINEFOCUS", PAD + PITCH * COLUMNS.length, PAD + 100);
}

export {getToolboxFocusedData, drawToolbox};
