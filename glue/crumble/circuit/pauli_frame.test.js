import {test, assertThat} from "../test/test_util.js"
import {PauliFrame} from "./pauli_frame.js"
import {GATE_MAP} from "../gates/gateset.js"
import {make_mpp_gate, make_spp_gate} from "../gates/gateset_mpp.js"
import {Operation} from "./operation.js";

test("pauli_frame.to_from", () => {
    let strings = [
        "_XYZ",
        "XXXX",
        "!&$%",
    ];
    let frame = PauliFrame.from_strings(strings);
    assertThat(frame.num_frames).isEqualTo(3);
    assertThat(frame.num_qubits).isEqualTo(4);
    assertThat(frame.toString()).isEqualTo(strings.join('\n'));
});

test("pauli_frame.to_from_dicts", () => {
    let frame = PauliFrame.from_strings([
        "_XYZ",
        "XXX!",
    ]);
    let qubit_keys = ["A", "B", "C", "D"];
    let dicts = frame.to_dicts(qubit_keys);
    assertThat(dicts).isEqualTo([
        new Map([["B", "X"], ["C", "Y"], ["D", "Z"]]),
        new Map([["A", "X"], ["B", "X"], ["C", "X"], ["D", "ERR:flag"]]),
    ]);
    assertThat(PauliFrame.from_dicts(dicts, qubit_keys)).isEqualTo(frame);
});

test("pauli_frame.do_gate_vs_old_frame_updates", () => {
    let gates = [...GATE_MAP.values(), make_mpp_gate("XYY"), make_spp_gate("XYY")];
    for (let g of gates) {
        if (g.name === 'DETECTOR' || g.name === 'OBSERVABLE_INCLUDE') {
            continue;
        }
        if (g.name.startsWith('REVMARK')) {
            // skipping REVMARKs since they are not supported by legacy implementation
            continue;
        }
        let before, after, returned;
        if (g.num_qubits === 1) {
            before = new PauliFrame(4, g.num_qubits);
            before.xs[0] = 0b0011;
            before.zs[0] = 0b0101;
            after = before.copy();
            after.do_gate(g, [0]);
            returned = after.copy();
            returned.undo_gate(g, [0]);
        } else {
            before = new PauliFrame(16, g.num_qubits);
            before.xs[0] = 0b0000000011111111;
            before.zs[0] = 0b0000111100001111;
            before.xs[1] = 0b0011001100110011;
            before.zs[1] = 0b0101010101010101;
            let targets = [0, 1];
            for (let k = 2; k < g.num_qubits; k++) {
                targets.push(k);
            }
            after = before.copy();
            after.do_gate(g, targets);
            returned = after.copy();
            returned.undo_gate(g, targets);
        }
        if (!returned.flags[0]) {
            assertThat(returned).withInfo({'gate': g.name}).isEqualTo(before);
        }

        let before_strings = before.to_strings();
        let actual_after_strings = after.to_strings();

        // Broadcast failure.
        for (let k = 0; k < actual_after_strings.length; k++) {
            let a = actual_after_strings[k];
            if (a.indexOf('&') !== -1 || a.indexOf('!') !== -1 || a.indexOf('%') !== -1 || a.indexOf('$') !== -1) {
                a = a.replaceAll('_', '!')
                a = a.replaceAll('X', '%')
                a = a.replaceAll('Y', '&')
                a = a.replaceAll('Z', '$')
                actual_after_strings[k] = a;
            }
        }

        let op = new Operation(g, '', new Float32Array(0), new Uint32Array([0, 1]));
        let expected_after_strings = [];
        for (let f = 0; f < before_strings.length; f++) {
            let t = op.pauliFrameAfter(before_strings[f].replaceAll('_', 'I'));
            t = t.replaceAll('I', '_');
            if (t.startsWith('ERR:')) {
                t = t.substring(4);
                t = t.replaceAll('_', '!')
                t = t.replaceAll('X', '%')
                t = t.replaceAll('Y', '&')
                t = t.replaceAll('Z', '$')
            }
            expected_after_strings.push(t);
        }

        assertThat(actual_after_strings).withInfo(`gate = ${g.name}`).isEqualTo(expected_after_strings);
    }
});
