import {GATE_MAP, GATE_ALIAS_MAP} from "./gateset.js"
import {test, assertThat, skipRestOfTestIfHeadless} from "../test/test_util.js";
import {Operation} from '../circuit/operation.js';
import {KNOWN_GATE_NAMES_FROM_STIM} from '../test/generated_gate_name_list.test.js';

test("gateset.expected_gates", () => {
    let expectedGates = new Set();
    for (let e of KNOWN_GATE_NAMES_FROM_STIM.split("\n")) {
        if (e.length > 0) {
            expectedGates.add(e);
        }
    }
    for (let e of GATE_ALIAS_MAP.keys()) {
        expectedGates.delete(e);
    }

    // Special cased.
    expectedGates.delete("REPEAT");
    expectedGates.delete("QUBIT_COORDS");
    expectedGates.delete("SHIFT_COORDS");
    expectedGates.delete("TICK");

    // Custom crumble gates and markers.
    expectedGates.add("POLYGON");
    expectedGates.add("MARKX");
    expectedGates.add("MARKY");
    expectedGates.add("MARKZ");
    expectedGates.add("MARK");

    // Special handling.
    expectedGates.delete("MPP");
    expectedGates.delete("SPP");
    expectedGates.delete("SPP_DAG");

    assertThat(new Set([...GATE_MAP.keys()])).isEqualTo(expectedGates);
});

test("gateset.allDrawCallsRun", () => {
    skipRestOfTestIfHeadless();

    let ctx = document.createElement('canvas').getContext('2d');
    for (let gate of GATE_MAP.values()) {
        let op = new Operation(gate, new Float32Array([1e-3]), new Uint32Array([5, 6]));
        assertThat(() => {
            gate.drawer(op, q => [2 * q, q + 1], ctx);
        }).withInfo(gate.name).runsWithoutThrowingAnException();
    }
});
