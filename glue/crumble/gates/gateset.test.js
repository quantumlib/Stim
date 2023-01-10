import {GATE_MAP} from "./gateset.js"
import {test, assertThat, skipRestOfTestIfHeadless} from "../test/test_util.js";
import {Operation} from '../circuit/operation.js';

test("gateset.expected_gates", () => {
    let stimGates = new Set([
        "I",
        "X",
        "Y",
        "Z",
        "C_XYZ",
        "C_ZYX",
        "H",
        "H_XY",
        "H_XZ",
        "H_YZ",
        "S",
        "SQRT_X",
        "SQRT_X_DAG",
        "SQRT_Y",
        "SQRT_Y_DAG",
        "SQRT_Z",
        "SQRT_Z_DAG",
        "S_DAG",
        "CNOT",
        "CX",
        "CY",
        "CZ",
        "ISWAP",
        "ISWAP_DAG",
        "SQRT_XX",
        "SQRT_XX_DAG",
        "SQRT_YY",
        "SQRT_YY_DAG",
        "SQRT_ZZ",
        "SQRT_ZZ_DAG",
        "SWAP",
        "XCX",
        "XCY",
        "XCZ",
        "YCX",
        "YCY",
        "YCZ",
        "ZCX",
        "ZCY",
        "ZCZ",
        "CORRELATED_ERROR",
        "DEPOLARIZE1",
        "DEPOLARIZE2",
        "E",
        "ELSE_CORRELATED_ERROR",
        "PAULI_CHANNEL_1",
        "PAULI_CHANNEL_2",
        "X_ERROR",
        "Y_ERROR",
        "Z_ERROR",
        "M",
        "MPP",
        "MR",
        "MRX",
        "MRY",
        "MRZ",
        "MX",
        "MY",
        "MZ",
        "R",
        "RX",
        "RY",
        "RZ",
        "REPEAT",
        "DETECTOR",
        "OBSERVABLE_INCLUDE",
        "QUBIT_COORDS",
        "SHIFT_COORDS",
        "TICK",
    ]);

    let expectedGates = new Set([...stimGates]);

    // Aliases.
    expectedGates.delete("CNOT");
    expectedGates.delete("MZ");
    expectedGates.delete("MRZ");
    expectedGates.delete("RZ");
    expectedGates.delete("H_XZ");
    expectedGates.delete("SQRT_Z");
    expectedGates.delete("SQRT_Z_DAG");
    expectedGates.delete("XCZ");
    expectedGates.delete("ZCX");
    expectedGates.delete("ZCY");
    expectedGates.delete("ZCZ");
    expectedGates.delete("YCX");
    expectedGates.delete("YCZ");

    // Noise.
    expectedGates.delete("CORRELATED_ERROR");
    expectedGates.delete("DEPOLARIZE1");
    expectedGates.delete("DEPOLARIZE2");
    expectedGates.delete("E");
    expectedGates.delete("ELSE_CORRELATED_ERROR");
    expectedGates.delete("PAULI_CHANNEL_1");
    expectedGates.delete("PAULI_CHANNEL_2");
    expectedGates.delete("X_ERROR");
    expectedGates.delete("Y_ERROR");
    expectedGates.delete("Z_ERROR");

    // Annotations.
    expectedGates.delete("REPEAT");
    expectedGates.delete("DETECTOR");
    expectedGates.delete("OBSERVABLE_INCLUDE");
    expectedGates.delete("QUBIT_COORDS");
    expectedGates.delete("SHIFT_COORDS");
    expectedGates.delete("TICK");

    // Custom crumble gates and markers.
    expectedGates.add("MXX");
    expectedGates.add("MYY");
    expectedGates.add("MZZ");
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
