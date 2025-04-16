import pytest
import stim


def test_to_qasm_exact_strings():
    c = stim.Circuit(
        """
        RX 0 1
        TICK
        H 1
        CX 0 1
        TICK
        M 0 1
        DETECTOR rec[-1] rec[-2]
        C_XYZ 0
    """
    )

    assert (
        c.to_qasm(open_qasm_version=3).strip()
        == """
OPENQASM 3.0;
include "stdgates.inc";
gate cxyz q0 { U(pi/2, 0, pi/2) q0; }
def rx(qubit q0) { reset q0; h q0; }

qreg q[2];
creg rec[2];
creg dets[1];

rx(q[0]);
rx(q[1]);
barrier q;

h q[1];
cx q[0], q[1];
barrier q;

measure q[0] -> rec[0];
measure q[1] -> rec[1];
dets[0] = rec[1] ^ rec[0] ^ 0;
cxyz q[0];
    """.strip()
    )

    assert (
        c.to_qasm(open_qasm_version=2, skip_dets_and_obs=True).strip()
        == """
OPENQASM 2.0;
include "qelib1.inc";
gate cxyz q0 { U(pi/2, 0, pi/2) q0; }

qreg q[2];
creg rec[2];

reset q[0]; h q[0]; // decomposed RX
reset q[1]; h q[1]; // decomposed RX
barrier q;

h q[1];
cx q[0], q[1];
barrier q;

measure q[0] -> rec[0];
measure q[1] -> rec[1];
cxyz q[0];
    """.strip()
    )


def test_to_qasm2_runs_in_qiskit():
    pytest.importorskip("qiskit")
    pytest.importorskip("qiskit_aer")
    import qiskit
    import qiskit_aer

    stim_circuit = stim.Circuit(
        """
        R 0 1
        MZZ !0 1
        MPAD 0 0
    """
    )
    qasm = stim_circuit.to_qasm(open_qasm_version=2)

    qiskit_circuit = qiskit.QuantumCircuit.from_qasm_str(qasm)
    counts = (
        qiskit_aer.AerSimulator()
        .run(qiskit_circuit, shots=8)
        .result()
        .get_counts(qiskit_circuit)
    )
    assert counts["001"] == 8


def test_to_qasm3_parses_in_qiskit():
    pytest.importorskip("qiskit")
    pytest.importorskip("qiskit_qasm3_import")
    import qiskit.qasm3

    # Note: can't really exercise this because currently `qiskit.qasm3.loads`
    # fails on subroutines, classical assignments, and all the other non-qasm-2
    # stuff that's actually worth testing.
    stim_circuit = stim.Circuit(
        """
        R 0 1
        SQRT_XX 0 1
        M 0 1
    """
    )

    qiskit_circuit = qiskit.qasm3.loads(stim_circuit.to_qasm(open_qasm_version=3))
    assert qiskit_circuit is not None
