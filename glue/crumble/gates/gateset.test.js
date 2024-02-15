import {GATE_MAP, GATE_ALIAS_MAP} from "./gateset.js"
import {test, assertThat, skipRestOfTestIfHeadless} from "../test/test_util.js";
import {Operation} from '../circuit/operation.js';

test("gateset.expected_gates", async () => {
    let gateNamesFile = await fetch("generated_gate_name_list.txt");
    assertThat(gateNamesFile.ok).withInfo(gateNamesFile.url).isEqualTo(true);
    let gateNamesText = await gateNamesFile.text();

    let expectedGates = new Set();
    for (let e of gateNamesText.split("\n")) {
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

    // Not supported yet.
    expectedGates.delete("MPP");

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
