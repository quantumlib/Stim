import {test, assertThat} from "../test/test_util.js"
import {Circuit, processTargetsTextIntoTargets, splitUncombinedTargets} from "./circuit.js"
import {Operation} from "./operation.js";
import {GATE_MAP} from "../gates/gateset.js";
import {make_mpp_gate} from '../gates/gateset_mpp.js';

test("circuit.fromStimCircuit", () => {
    let c1 = Circuit.fromStimCircuit(`
        QUBIT_COORDS(1, 2) 0
        QUBIT_COORDS(3, 4) 1
        QUBIT_COORDS(5, 6) 2
        H 0
        S 1 2
        TICK
        CX 2 0
    `);
    assertThat(c1.qubitCoordData).isEqualTo(new Float64Array([1, 2, 3, 4, 5, 6]));

    let c2 = Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(0, 1) 1
        QUBIT_COORDS(2, 0) 2
        QUBIT_COORDS(2, 4) 3
        QUBIT_COORDS(2, 3) 4
        H 0
        S 1 2
        CX 3 4
    `);
    assertThat(c2.qubitCoordData).isEqualTo(new Float64Array([0, 0, 0, 1, 2, 0, 2, 4, 2, 3]));
    assertThat(c2.layers.length).isEqualTo(1);
    assertThat(c2.layers[0].id_ops).isEqualTo(new Map([
        [0, new Operation(GATE_MAP.get('H'), new Float32Array(), new Uint32Array([0]))],
        [1, new Operation(GATE_MAP.get('S'), new Float32Array(), new Uint32Array([1]))],
        [2, new Operation(GATE_MAP.get('S'), new Float32Array(), new Uint32Array([2]))],
        [3, new Operation(GATE_MAP.get('CX'), new Float32Array(), new Uint32Array([3, 4]))],
        [4, new Operation(GATE_MAP.get('CX'), new Float32Array(), new Uint32Array([3, 4]))],
    ]));
    assertThat(c2.toStimCircuit()).isEqualTo(`
QUBIT_COORDS(0, 0) 0
QUBIT_COORDS(0, 1) 1
QUBIT_COORDS(2, 0) 2
QUBIT_COORDS(2, 3) 3
QUBIT_COORDS(2, 4) 4
CX 4 3
H 0
S 1 2
    `.trim());
});

test("circuit.fromStimCircuit_mpp", () => {
    let c1 = Circuit.fromStimCircuit(`
        QUBIT_COORDS(1, 2) 0
        QUBIT_COORDS(3, 4) 1
        QUBIT_COORDS(5, 6) 2
        MPP X0*Z1*Y2 X2*Y1
    `);
    assertThat(c1.qubitCoordData).isEqualTo(
        new Float64Array([1, 2, 3, 4, 5, 6]));
    assertThat(c1.layers.length).isEqualTo(2);
    let op1 = new Operation(make_mpp_gate("XZY"), new Float32Array([]), new Uint32Array([0, 1, 2]));
    assertThat(c1.layers[0].id_ops).isEqualTo(new Map([
        [0, op1],
        [1, op1],
        [2, op1],
    ]));
    let op2 = new Operation(make_mpp_gate("XY"), new Float32Array([]), new Uint32Array([2, 1]));
    assertThat(c1.layers[1].id_ops).isEqualTo(new Map([
        [1, op2],
        [2, op2],
    ]));
    assertThat(c1.toStimCircuit()).isEqualTo(`
QUBIT_COORDS(1, 2) 0
QUBIT_COORDS(3, 4) 1
QUBIT_COORDS(5, 6) 2
MPP X0*Z1*Y2
TICK
MPP X2*Y1
    `.trim());
});

test('circuit.coordToQubitMap', () => {
    let c = Circuit.fromStimCircuit(`
        QUBIT_COORDS(5, 6) 1
        QUBIT_COORDS(10, 7) 2
        QUBIT_COORDS(20, 17) 0
    `);
    assertThat(c.qubitCoordData).isEqualTo(new Float64Array([20, 17, 5, 6, 10, 7]));
    assertThat(c.coordToQubitMap()).isEqualTo(new Map([
        ['5,6', 1],
        ['10,7', 2],
        ['20,17', 0],
    ]));
});

test('circuit.processTargetsTextIntoTargets', () => {
    assertThat(processTargetsTextIntoTargets('')).isEqualTo([]);
    assertThat(processTargetsTextIntoTargets(' X0*X1')).isEqualTo(['X0', '*', 'X1']);
    assertThat(processTargetsTextIntoTargets(' 0 1 2')).isEqualTo(['0', '1', '2']);
    assertThat(processTargetsTextIntoTargets(' X0*X1*Z3 Y1 Y2*Y4')).isEqualTo(['X0', '*', 'X1', '*', 'Z3', 'Y1', 'Y2', '*', 'Y4']);
});

test('circuit.splitUncombinedTargets', () => {
    assertThat(splitUncombinedTargets([])).isEqualTo([]);
    assertThat(splitUncombinedTargets(['X1'])).isEqualTo([['X1']]);
    assertThat(splitUncombinedTargets(['X1', 'X2'])).isEqualTo([['X1'], ['X2']]);
    assertThat(splitUncombinedTargets(['X1', '*', 'X2'])).isEqualTo([['X1', 'X2']]);
    assertThat(splitUncombinedTargets(['X1', '*', 'X2', 'Y1', '*', 'Y2', '*', 'Y3', 'Z1', 'Z2', '*', 'Z3'])).isEqualTo([['X1', 'X2'], ['Y1', 'Y2', 'Y3'], ['Z1'], ['Z2', 'Z3']]);
    assertThat(() => splitUncombinedTargets(['X1', '*', '*', 'X2'])).throwsAnExceptionThat().matches(/Adjacent combiners/);
    assertThat(() => splitUncombinedTargets(['X1', '*'])).throwsAnExceptionThat().matches(/Dangling combiner/);
    assertThat(() => splitUncombinedTargets(['*', 'X1'])).throwsAnExceptionThat().matches(/Leading combiner/);
});

test('circuit.withCoordsIncluded', () => {
    let c = Circuit.fromStimCircuit(`
        QUBIT_COORDS(10, 7) 1
        QUBIT_COORDS(5, 6) 0
    `);
    assertThat(c.qubitCoordData).isEqualTo(new Float64Array([5, 6, 10, 7]));
    let c2 = c.withCoordsIncluded([[5, 6], [2, 4], [2, 4]])
    assertThat(c2.qubitCoordData).isEqualTo(new Float64Array([5, 6, 10, 7, 2, 4]));
});

test("circuit.isEqualTo", () => {
    assertThat(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 0) 0
        H 0
    `)).isEqualTo(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 0) 0
        H 0
    `));

    assertThat(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 0) 0
        H 0
    `)).isNotEqualTo(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 0) 0
        X 0
    `));

    assertThat(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 0) 0
        H 0
    `)).isNotEqualTo(Circuit.fromStimCircuit(`
        QUBIT_COORDS(1, 0) 0
        H 0
    `));
});

test("circuit.copy", () => {
    let c = Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(2, 3) 1
        H 0
        CNOT 0 1
        S 1
    `);
    let c2 = c.copy();
    assertThat(c !== c2).isEqualTo(true);
    assertThat(c).isEqualTo(c2);
});

test("circuit.afterRectification", () => {
    assertThat(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(0, 1) 1
        X 0 1
    `).afterRectification()).isEqualTo(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(0, 1) 1
        X 0 1
    `));

    assertThat(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(0, 2) 1
        X 0 1
    `).afterRectification()).isEqualTo(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(0, 1) 1
        X 0 1
    `));

    assertThat(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(0, 0.5) 1
        X 0 1
    `).afterRectification()).isEqualTo(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(0, 1) 1
        X 0 1
    `));

    assertThat(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(1, 0) 1
        X 0 1
    `).afterRectification()).isEqualTo(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(1, 0) 1
        X 0 1
    `));

    assertThat(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(0.5, 0) 1
        X 0 1
    `).afterRectification()).isEqualTo(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(1, 0) 1
        X 0 1
    `));

    assertThat(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(2, 0) 1
        X 0 1
    `).afterRectification()).isEqualTo(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(1, 0) 1
        X 0 1
    `));

    assertThat(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(0.5, 0.5) 1
        X 0 1
    `).afterRectification()).isEqualTo(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(0.5, 0.5) 1
        X 0 1
    `));

    assertThat(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0.5, 0) 0
        QUBIT_COORDS(1, 0.5) 1
        X 0 1
    `).afterRectification()).isEqualTo(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(0.5, 0.5) 1
        X 0 1
    `));

    assertThat(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 0.5) 0
        QUBIT_COORDS(0.5, 0) 1
        X 0 1
    `).afterRectification()).isEqualTo(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0.5, 0.5) 0
        QUBIT_COORDS(1, 0) 1
        X 0 1
    `));

    assertThat(Circuit.fromStimCircuit(`
        QUBIT_COORDS(1, 2) 0
        QUBIT_COORDS(2, 1) 1
        X 0 1
    `).afterRectification()).isEqualTo(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0.5, 0.5) 0
        QUBIT_COORDS(1, 0) 1
        X 0 1
    `));

    assertThat(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0.5, 0.5) 0
        QUBIT_COORDS(1, 0) 1
        X 0 1
    `).afterRectification()).isEqualTo(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0.5, 0.5) 0
        QUBIT_COORDS(1, 0) 1
        X 0 1
    `));

    assertThat(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(0.25, 0.25) 1
        X 0 1
    `).afterRectification()).isEqualTo(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(0.5, 0.5) 1
        X 0 1
    `));

    assertThat(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(1, 1) 1
        X 0 1
    `).afterRectification()).isEqualTo(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(0.5, 0.5) 1
        X 0 1
    `));

    assertThat(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 1) 0
        QUBIT_COORDS(1, 0) 1
        X 0 1
    `).afterRectification()).isEqualTo(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0.5, 0.5) 0
        QUBIT_COORDS(1, 0) 1
        X 0 1
    `));

    assertThat(Circuit.fromStimCircuit(`
        QUBIT_COORDS(10, 20) 0
        QUBIT_COORDS(12, 20) 1
        QUBIT_COORDS(12, 21) 2
        QUBIT_COORDS(10, 21) 3
        H 0
        X 1
        Y 2
        Z 3
    `).afterRectification()).isEqualTo(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(2, 0) 1
        QUBIT_COORDS(2, 1) 2
        QUBIT_COORDS(0, 1) 3
        H 0
        X 1
        Y 2
        Z 3
    `));

    assertThat(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(-2, 0) 1
        QUBIT_COORDS(-2, -1) 2
        QUBIT_COORDS(0, -1) 3
        H 0
        X 1
        Y 2
        Z 3
    `).afterRectification()).isEqualTo(Circuit.fromStimCircuit(`
        QUBIT_COORDS(2, 1) 0
        QUBIT_COORDS(0, 1) 1
        QUBIT_COORDS(0, 0) 2
        QUBIT_COORDS(2, 0) 3
        H 0
        X 1
        Y 2
        Z 3
    `));

    assertThat(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 4) 0
        QUBIT_COORDS(2, 0) 1
        QUBIT_COORDS(0, 0) 2
        QUBIT_COORDS(4, 0) 3
        H 0
        X 1
        Y 2
        Z 3
    `).afterRectification()).isEqualTo(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 2) 0
        QUBIT_COORDS(1, 0) 1
        QUBIT_COORDS(0, 0) 2
        QUBIT_COORDS(2, 0) 3
        H 0
        X 1
        Y 2
        Z 3
    `));

    assertThat(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 0.5) 0
        QUBIT_COORDS(0.25, 0) 1
        QUBIT_COORDS(0, 0) 2
        QUBIT_COORDS(0.5, 0) 3
        H 0
        X 1
        Y 2
        Z 3
    `).afterRectification()).isEqualTo(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 2) 0
        QUBIT_COORDS(1, 0) 1
        QUBIT_COORDS(0, 0) 2
        QUBIT_COORDS(2, 0) 3
        H 0
        X 1
        Y 2
        Z 3
    `));

    assertThat(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(0.5, 0.5) 1
        QUBIT_COORDS(1, 1) 2
        QUBIT_COORDS(0.5, 1.5) 3
        H 0
        X 1
        Y 2
        Z 3
    `).afterRectification()).isEqualTo(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(0.5, 0.5) 1
        QUBIT_COORDS(1, 1) 2
        QUBIT_COORDS(0.5, 1.5) 3
        H 0
        X 1
        Y 2
        Z 3
    `));
});

test("circuit.rotated45", () => {
    let circuit = Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(2, 0) 1
        QUBIT_COORDS(2, 1) 2
        QUBIT_COORDS(0, 1) 3
        H 0
        X 1
        Y 2
        Z 3
    `);
    let c45 = circuit.rotated45().afterRectification();
    let c90 = c45.rotated45().afterRectification();
    let c180 = c90.rotated45().rotated45().afterRectification();

    assertThat(circuit.rotated45().rotated45().rotated45().rotated45()).isEqualTo(
        circuit.afterCoordTransform((x, y) => [-4*x, -4*y]));
    assertThat(c180).isEqualTo(
        circuit.afterCoordTransform((x, y) => [2-x, 1-y]));

    assertThat(c90).isEqualTo(Circuit.fromStimCircuit(`
        QUBIT_COORDS(1, 0) 0
        QUBIT_COORDS(1, 2) 1
        QUBIT_COORDS(0, 2) 2
        QUBIT_COORDS(0, 0) 3
        H 0
        X 1
        Y 2
        Z 3
    `));

    assertThat(c45).isEqualTo(Circuit.fromStimCircuit(`
        QUBIT_COORDS(1, 0) 0
        QUBIT_COORDS(2, 1) 1
        QUBIT_COORDS(1.5, 1.5) 2
        QUBIT_COORDS(0.5, 0.5) 3
        H 0
        X 1
        Y 2
        Z 3
    `));
});

test("circuit.fromStimCircuit.automaticTicks", () => {
    assertThat(Circuit.fromStimCircuit(`
        H 0
        CX 0 1
    `)).isEqualTo(Circuit.fromStimCircuit(`
        H 0
        TICK
        CX 0 1
    `));
});

test("circuit.inferAndConvertCoordinates", () => {
    assertThat(Circuit.fromStimCircuit(`
        H 0 1 2 3
    `)).isEqualTo(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(1, 0) 1
        QUBIT_COORDS(2, 0) 2
        QUBIT_COORDS(3, 0) 3
        H 0 1 2 3
    `));

    assertThat(Circuit.fromStimCircuit(`
        H 0 3 2 1
    `)).isEqualTo(Circuit.fromStimCircuit(`
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(1, 0) 1
        QUBIT_COORDS(2, 0) 2
        QUBIT_COORDS(3, 0) 3
        H 0 3 2 1
    `));

    assertThat(Circuit.fromStimCircuit(`
        QUBIT_COORDS(2) 0
        QUBIT_COORDS(3) 1
        QUBIT_COORDS(5) 2
        QUBIT_COORDS(7) 3
        H 0 3 2 1
    `)).isEqualTo(Circuit.fromStimCircuit(`
        QUBIT_COORDS(2, 0) 0
        QUBIT_COORDS(3, 0) 1
        QUBIT_COORDS(5, 0) 2
        QUBIT_COORDS(7, 0) 3
        H 0 3 2 1
    `));

    assertThat(Circuit.fromStimCircuit(`
        QUBIT_COORDS(2, 1, 9, 9, 9) 0
        QUBIT_COORDS(3, 1) 1
        QUBIT_COORDS(5, 1, 9) 2
        QUBIT_COORDS(7, 1, 9, 9) 3
        H 0 3 2 1
    `)).isEqualTo(Circuit.fromStimCircuit(`
        QUBIT_COORDS(2, 1) 0
        QUBIT_COORDS(3, 1) 1
        QUBIT_COORDS(5, 1) 2
        QUBIT_COORDS(7, 1) 3
        H 0 3 2 1
    `));
});

test("circuit.parse_mpp", () => {
    assertThat(Circuit.fromStimCircuit(`
        MPP Z0*Z1*Z2 Z3*Z4*Z6
    `).toString()).isEqualTo(`
QUBIT_COORDS(0, 0) 0
QUBIT_COORDS(1, 0) 1
QUBIT_COORDS(2, 0) 2
QUBIT_COORDS(3, 0) 3
QUBIT_COORDS(4, 0) 4
QUBIT_COORDS(6, 0) 5
MPP Z0*Z1*Z2 Z3*Z4*Z5
    `.trim())

    assertThat(Circuit.fromStimCircuit(`
        MPP Z0*Z1*Z2 Z3*Z4*Z5*X6
    `).toString()).isEqualTo(`
QUBIT_COORDS(0, 0) 0
QUBIT_COORDS(1, 0) 1
QUBIT_COORDS(2, 0) 2
QUBIT_COORDS(3, 0) 3
QUBIT_COORDS(4, 0) 4
QUBIT_COORDS(5, 0) 5
QUBIT_COORDS(6, 0) 6
MPP Z0*Z1*Z2 Z3*Z4*Z5*X6
    `.trim())
});

test("circuit.fromStimCircuit_manygates", () => {
    let c = Circuit.fromStimCircuit(`
        QUBIT_COORDS(1, 2, 3) 0

        # Pauli gates
        I 0
        X 1
        Y 2
        Z 3
        TICK

        # Single Qubit Clifford Gates
        C_XYZ 0
        C_ZYX 1
        H_XY 2
        H_XZ 3
        H_YZ 4
        SQRT_X 0
        SQRT_X_DAG 1
        SQRT_Y 2
        SQRT_Y_DAG 3
        SQRT_Z 4
        SQRT_Z_DAG 5
        TICK

        # Two Qubit Clifford Gates
        CXSWAP 0 1
        ISWAP 2 3
        ISWAP_DAG 4 5
        SWAP 6 7
        SWAPCX 8 9
        CZSWAP 10 11
        SQRT_XX 0 1
        SQRT_XX_DAG 2 3
        SQRT_YY 4 5
        SQRT_YY_DAG 6 7
        SQRT_ZZ 8 9
        SQRT_ZZ_DAG 10 11
        XCX 0 1
        XCY 2 3
        XCZ 4 5
        YCX 6 7
        YCY 8 9
        YCZ 10 11
        ZCX 12 13
        ZCY 14 15
        ZCZ 16 17
        TICK

        # Noise Channels
        CORRELATED_ERROR(0.01) X1 Y2 Z3
        ELSE_CORRELATED_ERROR(0.02) X4 Y7 Z6
        DEPOLARIZE1(0.02) 0
        DEPOLARIZE2(0.03) 1 2
        PAULI_CHANNEL_1(0.01, 0.02, 0.03) 3
        PAULI_CHANNEL_2(0.001, 0.002, 0.003, 0.004, 0.005, 0.006, 0.007, 0.008, 0.009, 0.010, 0.011, 0.012, 0.013, 0.014, 0.015) 4 5
        X_ERROR(0.01) 0
        Y_ERROR(0.02) 1
        Z_ERROR(0.03) 2
        HERALDED_ERASE(0.04) 3
        HERALDED_PAULI_CHANNEL_1(0.01, 0.02, 0.03, 0.04) 6
        TICK

        # Collapsing Gates
        MPP X0*Y1*Z2 Z0*Z1
        MRX 0
        MRY 1
        MRZ 2
        MX 3
        MY 4
        MZ 5 6
        RX 7
        RY 8
        RZ 9
        TICK

        # Pair Measurement Gates
        MXX 0 1 2 3
        MYY 4 5
        MZZ 6 7
        TICK

        # Control Flow
        REPEAT 3 {
            H 0
            CX 0 1
            S 1
            TICK
        }
        TICK

        # Annotations
        MR 0
        X_ERROR(0.1) 0
        MR(0.01) 0
        SHIFT_COORDS(1, 2, 3)
        DETECTOR(1, 2, 3) rec[-1]
        OBSERVABLE_INCLUDE(0) rec[-1]
        MPAD 0 1 0
        TICK

        # Inverted measurements.
        MRX !0
        MY !1
        MZZ !2 3
        MYY !4 !5
        MPP X6*!Y7*Z8
        TICK

        # Feedback
        CX rec[-1] 0
        CY sweep[0] 1
        CZ 2 rec[-1]
    `);
    assertThat(c).isNotEqualTo(undefined);
})
