import {test, assertThat} from "../test/test_util.js"
import {PauliFrame} from "./pauli_frame.js"
import {GATE_MAP} from "../gates/gateset.js"
import {Operation} from "./operation.js";
import {Layer} from "./layer.js";
import {Circuit} from "./circuit.js";

test("layer.put_get", () => {
    let layer = new Layer();
    assertThat(layer.id_ops).isEqualTo(new Map());
    assertThat(layer.markers).isEqualTo([]);

    let op = new Operation(GATE_MAP.get("CX"), new Float32Array(), new Uint32Array([2, 3]));
    layer.put(op);
    assertThat(layer.id_ops).isEqualTo(new Map([
        [2, op],
        [3, op],
    ]));
    assertThat(layer.markers).isEqualTo([]);

    let marker1 = new Operation(GATE_MAP.get("MARKX"), new Float32Array([0]), new Uint32Array([4]));
    let marker2 = new Operation(GATE_MAP.get("MARKZ"), new Float32Array([1]), new Uint32Array([5]));
    layer.put(marker1);
    layer.put(marker2);
    assertThat(layer.id_ops).isEqualTo(new Map([
        [2, op],
        [3, op],
    ]));
    assertThat(layer.markers).isEqualTo([marker1, marker2]);
});

test("layer.filtered", () => {
    let circuit = Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(1, 1) 1
        QUBIT_COORDS(2, 2) 2
        QUBIT_COORDS(3, 3) 3
        H 0 1 2
        S 3
        TICK
        CNOT 0 1
        CZ 2 3
        TICK
    `);
    let c0 = Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(1, 1) 1
        QUBIT_COORDS(2, 2) 2
        QUBIT_COORDS(3, 3) 3
        H 0
        TICK
        CNOT 0 1
        TICK
    `);
    let c01 = Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(1, 1) 1
        QUBIT_COORDS(2, 2) 2
        QUBIT_COORDS(3, 3) 3
        H 0 1
        TICK
        CNOT 0 1
        TICK
    `);
    assertThat(circuit.layers[0].id_filteredByQubit(q => q === 0)).isEqualTo(c0.layers[0]);
    assertThat(circuit.layers[1].id_filteredByQubit(q => q === 0)).isEqualTo(c0.layers[1]);
    assertThat(circuit.layers[0].id_filteredByQubit(q => q === 0 || q === 1)).isEqualTo(c01.layers[0]);
    assertThat(circuit.layers[1].id_filteredByQubit(q => q === 0 || q === 1)).isEqualTo(c01.layers[1]);
});
