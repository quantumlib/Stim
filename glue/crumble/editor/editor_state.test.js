import {test, assertThat} from "../test/test_util.js";
import {EditorState} from "./editor_state.js";
import {GATE_MAP} from "../gates/gateset.js";
import {Circuit} from "../circuit/circuit.js";
import {pitch} from "../draw/config.js";

test("editor_state.changeFocus", () => {
    let state = new EditorState(undefined);
    assertThat(state.focusedSet).isEqualTo(new Map());

    state.changeFocus([[0, 0], [0, 1]], false, false);
    assertThat(state.focusedSet).isEqualTo(new Map([
        ['0,0', [0, 0]],
        ['0,1', [0, 1]],
    ]));

    state.changeFocus([[0, 0], [2, 1]], false, false);
    assertThat(state.focusedSet).isEqualTo(new Map([
        ['0,0', [0, 0]],
        ['2,1', [2, 1]],
    ]));

    state.changeFocus([[0, 0], [1, 0]], false, true);
    assertThat(state.focusedSet).isEqualTo(new Map([
        ['2,1', [2, 1]],
        ['1,0', [1, 0]],
    ]));

    state.changeFocus([[0, 0], [1, 0]], true, false);
    assertThat(state.focusedSet).isEqualTo(new Map([
        ['0,0', [0, 0]],
        ['2,1', [2, 1]],
        ['1,0', [1, 0]],
    ]));
});

test("editor_state.markFocusInferBasis", () => {
    let state = new EditorState(undefined);
    state.commit(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(0, 1) 1
        QUBIT_COORDS(0, 2) 2
        QUBIT_COORDS(1, 0) 3
        QUBIT_COORDS(1, 1) 4
        QUBIT_COORDS(2, 2) 5
        H 0 1
        RX 2
        RY 3
        RZ 4
        RX 5
    `));

    state.changeFocus([[0, 0], [0, 1], [0, 2], [1, 0], [1, 1], [1, 2]], false, false);
    state.markFocusInferBasis(false, 1);
    assertThat(state.copyOfCurCircuit()).isEqualTo(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(0, 1) 1
        QUBIT_COORDS(1, 1) 2
        QUBIT_COORDS(0, 2) 3
        QUBIT_COORDS(2, 2) 4
        QUBIT_COORDS(1, 0) 5
        QUBIT_COORDS(1, 2) 6
        H 0 1
        R 2
        RX 3 4
        RY 5
        MARKX(1) 3
        MARKY(1) 5
        MARKZ(1) 0 1 2 6
    `));

    state.changeFocus([[0, 0], [0, 1], [0, 2], [1, 2]], false, false);
    state.markFocusInferBasis(false, 1);
    assertThat(state.copyOfCurCircuit()).isEqualTo(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(0, 1) 1
        QUBIT_COORDS(1, 1) 2
        QUBIT_COORDS(0, 2) 3
        QUBIT_COORDS(2, 2) 4
        QUBIT_COORDS(1, 0) 5
        QUBIT_COORDS(1, 2) 6
        H 0 1
        R 2
        RX 3 4
        RY 5
        MARKX(1) 0 1 3 6
        MARKY(1) 5
        MARKZ(1) 2
    `));
});

test("editor_state.writeGateToFocus", () => {
    let state = new EditorState(undefined);
    state.changeFocus([[0, 0], [0, 1]], false, false);
    state.writeGateToFocus(false, GATE_MAP.get('H'), undefined);
    assertThat(state.copyOfCurCircuit()).isEqualTo(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(0, 1) 1
        H 0 1
    `));
    state.changeFocus([[2, 0], [0, 1]], false, false);
    state.writeGateToFocus(false, GATE_MAP.get('S'), undefined);
    assertThat(state.copyOfCurCircuit()).isEqualTo(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(2, 0) 1
        QUBIT_COORDS(0, 1) 2
        H 0
        S 1 2
    `));

    state.curMouseX = 5 * pitch;
    state.curMouseY = 6 * pitch;
    state.writeGateToFocus(false, GATE_MAP.get('CX'), undefined);
    assertThat(state.copyOfCurCircuit()).isEqualTo(Circuit.fromStimCircuit(`
        QUBIT_COORDS(2, 0) 0
        QUBIT_COORDS(7, 5) 1
        QUBIT_COORDS(0, 1) 2
        QUBIT_COORDS(5, 6) 3
        QUBIT_COORDS(0, 0) 4
        CX 0 1 2 3
        H 4
    `));

    state.clearCircuit();
    assertThat(state.copyOfCurCircuit()).isEqualTo(Circuit.fromStimCircuit(''));
    state.writeGateToFocus(false, GATE_MAP.get("POLYGON"), [1, 1, 0, 0.5]);
    assertThat(state.copyOfCurCircuit()).isEqualTo(Circuit.fromStimCircuit(`
        QUBIT_COORDS(2, 0) 0
        QUBIT_COORDS(0, 1) 1
        POLYGON(1,1,0,0.5) 0 1
    `));
});

test('editor_state.edit_measurement_near_observables', () => {
    let state = new EditorState(undefined);
    state.commit(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(1, 0) 1
        QUBIT_COORDS(2, 0) 2
        RX 1
        TICK
        MX 2
        TICK
        M 0
        OBSERVABLE_INCLUDE(0) rec[-1] rec[-2]
    `));
    state.changeFocus([[1, 0]]);
    state.changeCurLayerTo(1);
    state.writeGateToFocus(false, GATE_MAP.get('M'));
    assertThat(state.copyOfCurCircuit()).isEqualTo(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(1, 0) 1
        QUBIT_COORDS(2, 0) 2
        RX 1
        TICK
        M 1
        MX 2
        TICK
        M 0
        OBSERVABLE_INCLUDE(0) rec[-1] rec[-2]
    `));
});
