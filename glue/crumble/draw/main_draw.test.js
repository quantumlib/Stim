import {draw, setDefensiveDrawEnabled} from './main_draw.js';
import {StateSnapshot} from './state_snapshot.js';
import {test, assertThat, skipRestOfTestIfHeadless} from "../test/test_util.js";
import {Circuit} from "../circuit/circuit.js";

test("main_draw.drawRuns", () => {
    skipRestOfTestIfHeadless();
    setDefensiveDrawEnabled(false);

    let ctx = document.createElement('canvas').getContext('2d');
    let circuit = Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(1, 0) 1
        QUBIT_COORDS(0, 1) 2
        QUBIT_COORDS(1, 1) 3
        MARKX(0) 1
        CX 1 2
        TICK
        H 0
        CY 2 3
        TICK
        S 0 1 2 3
    `);
    let curLayer = 1;
    let focusedSet = new Set(['0,0', '1,0', '2,0']);
    let timelineSet = new Set(['0,1', '1,0']);
    let curMouseX = 0;
    let curMouseY = 0;
    let mouseDownX = 0;
    let mouseDownY = 0;
    let boxHighlightPreview = [[1, 1], [1, 2]];
    let snap = new StateSnapshot(
        circuit,
        curLayer,
        focusedSet,
        timelineSet,
        curMouseX,
        curMouseY,
        mouseDownX,
        mouseDownY,
        boxHighlightPreview,
    );
    draw(ctx, snap);
    assertThat(() => draw(ctx, snap)).runsWithoutThrowingAnException();
});
