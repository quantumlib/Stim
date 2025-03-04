#include "stim/util_top/export_qasm.h"

#include "gtest/gtest.h"

#include "stim/circuit/circuit.test.h"
#include "stim/util_top/transform_without_feedback.h"

using namespace stim;

TEST(export_qasm, export_open_qasm_feedback) {
    Circuit c(R"CIRCUIT(
        H 0
        CX 0 1
        C_XYZ 1
        M 0
        CX rec[-1] 1
        CX sweep[5] 1
        TICK
        H 0
    )CIRCUIT");
    std::stringstream out;
    export_open_qasm(c, out, 3, false);
    ASSERT_EQ(out.str(), R"QASM(OPENQASM 3.0;
include "stdgates.inc";
gate cxyz q0 { U(pi/2, 0, pi/2) q0; }

qreg q[2];
creg rec[1];
creg sweep[6];

h q[0];
cx q[0], q[1];
cxyz q[1];
measure q[0] -> rec[0];
if (ms[0]) {
    X q[1];
}
if (sweep[5]) {
    X q[1];
}
barrier q;

h q[0];
)QASM");
}

TEST(export_qasm, export_open_qasm_inverted_measurements) {
    Circuit c(R"CIRCUIT(
        M !0
        MX !0
        MXX !0 1
        MPP !X0*Z1
    )CIRCUIT");
    std::stringstream out;
    export_open_qasm(c, out, 3, false);
    ASSERT_EQ(out.str(), R"QASM(OPENQASM 3.0;
include "stdgates.inc";
def mx(qubit q0) -> bit { bit b; h q0; measure q0 -> b; h q0; return b; }
def mxx(qubit q0, qubit q1) -> bit { bit b; cx q0, q1; h q0; measure q0 -> b; h q0; cx q0, q1; return b; }

qreg q[2];
creg rec[4];

measure q[0] -> rec[0];rec[0] = rec[0] ^ 1;
rec[1] = mx(q[0]) ^ 1;
rec[2] = mxx(q[0], q[1]) ^ 1;
// --- begin decomposed MPP !X0*Z1
h q[0];
cx q[1], q[0];
measure q[0] -> rec[3];rec[3] = rec[3] ^ 1;
cx q[1], q[0];
h q[0];
// --- end decomposed MPP
)QASM");
}

TEST(export_qasm, export_open_qasm_qec) {
    Circuit c(R"CIRCUIT(
        R 0 1 2
        TICK
        X 0
        TICK
        CX 0 1
        TICK
        CX 2 1
        TICK
        M 1
        DETECTOR rec[-1]
        TICK
        R 1
        TICK
        CX 0 1
        TICK
        CX 2 1
        TICK
        M 0 1 2
        DETECTOR rec[-2] rec[-4]
        DETECTOR rec[-1] rec[-2] rec[-3]
        OBSERVABLE_INCLUDE(0) rec[-1]
    )CIRCUIT");

    std::stringstream out;
    export_open_qasm(c, out, 3, false);
    ASSERT_EQ(out.str(), R"QASM(OPENQASM 3.0;
include "stdgates.inc";

qreg q[3];
creg rec[4];
creg dets[3];
creg obs[1];

reset q[0];
reset q[1];
reset q[2];
barrier q;

x q[0];
barrier q;

cx q[0], q[1];
barrier q;

cx q[2], q[1];
barrier q;

measure q[1] -> rec[0];
dets[0] = rec[0] ^ 1;
barrier q;

reset q[1];
barrier q;

cx q[0], q[1];
barrier q;

cx q[2], q[1];
barrier q;

measure q[0] -> rec[1];
measure q[1] -> rec[2];
measure q[2] -> rec[3];
dets[1] = rec[2] ^ rec[0] ^ 0;
dets[2] = rec[3] ^ rec[2] ^ rec[1] ^ 0;
obs[0] = obs[0] ^ rec[3] ^ 0;
)QASM");

    out.str("");
    export_open_qasm(c, out, 2, true);
    ASSERT_EQ(out.str(), R"QASM(OPENQASM 2.0;
include "qelib1.inc";

qreg q[3];
creg rec[4];

reset q[0];
reset q[1];
reset q[2];
barrier q;

x q[0];
barrier q;

cx q[0], q[1];
barrier q;

cx q[2], q[1];
barrier q;

measure q[1] -> rec[0];
barrier q;

reset q[1];
barrier q;

cx q[0], q[1];
barrier q;

cx q[2], q[1];
barrier q;

measure q[0] -> rec[1];
measure q[1] -> rec[2];
measure q[2] -> rec[3];
)QASM");
}

TEST(export_qasm, export_open_qasm_mpad) {
    Circuit c(R"CIRCUIT(
        H 0
        MPAD 0 1 0
        M 0
    )CIRCUIT");

    std::stringstream out;
    export_open_qasm(c, out, 3, false);
    ASSERT_EQ(out.str(), R"QASM(OPENQASM 3.0;
include "stdgates.inc";

qreg q[1];
creg rec[4];

h q[0];
rec[0] = 0;
rec[1] = 1;
rec[2] = 0;
measure q[0] -> rec[3];
)QASM");

    out.str("");
    ASSERT_THROW({ export_open_qasm(c, out, 2, true); }, std::invalid_argument);
    c = Circuit(R"CIRCUIT(
        H 0
        MPAD 0 0 0
        M 0
    )CIRCUIT");

    out.str("");
    export_open_qasm(c, out, 2, true);
    ASSERT_EQ(out.str(), R"QASM(OPENQASM 2.0;
include "qelib1.inc";

qreg q[1];
creg rec[4];

h q[0];
measure q[0] -> rec[3];
)QASM");
}

TEST(export_qasm, export_qasm_decomposed_operations) {
    Circuit c(R"CIRCUIT(
        R 3
        RX 0 1
        MX 2
        TICK

        MXX 0 1
        DETECTOR rec[-1]
        TICK

        M 2
        MR 3
        MRX 4
    )CIRCUIT");

    std::stringstream out;
    export_open_qasm(c, out, 3, false);
    ASSERT_EQ(out.str(), R"QASM(OPENQASM 3.0;
include "stdgates.inc";
def mr(qubit q0) -> bit { bit b; measure q0 -> b; reset q0; return b; }
def mrx(qubit q0) -> bit { bit b; h q0; measure q0 -> b; reset q0; h q0; return b; }
def mx(qubit q0) -> bit { bit b; h q0; measure q0 -> b; h q0; return b; }
def mxx(qubit q0, qubit q1) -> bit { bit b; cx q0, q1; h q0; measure q0 -> b; h q0; cx q0, q1; return b; }
def rx(qubit q0) { reset q0; h q0; }

qreg q[5];
creg rec[5];
creg dets[1];

reset q[3];
rx(q[0]);
rx(q[1]);
rec[0] = mx(q[2]);
barrier q;

rec[1] = mxx(q[0], q[1]);
dets[0] = rec[1] ^ 0;
barrier q;

measure q[2] -> rec[2];
rec[3] = mr(q[3]);
rec[4] = mrx(q[4]);
)QASM");

    out.str("");
    export_open_qasm(c, out, 2, true);
    ASSERT_EQ(out.str(), R"QASM(OPENQASM 2.0;
include "qelib1.inc";

qreg q[5];
creg rec[5];

reset q[3];
reset q[0]; h q[0]; // decomposed RX
reset q[1]; h q[1]; // decomposed RX
h q[2]; measure q[2] -> rec[0]; h q[2]; // decomposed MX
barrier q;

cx q[0], q[1]; h q[0]; measure q[0] -> rec[1]; h q[0]; cx q[0], q[1]; // decomposed MXX
barrier q;

measure q[2] -> rec[2];
measure q[3] -> rec[3]; reset q[3]; // decomposed MR
h q[4]; measure q[4] -> rec[4]; reset q[4]; h q[4]; // decomposed MRX
)QASM");
}

TEST(export_qasm, export_qasm_all_operations_v3) {
    Circuit c = generate_test_circuit_with_all_operations();
    c = c.without_noise();

    std::stringstream out;
    export_open_qasm(c, out, 3, false);
    ASSERT_EQ(out.str(), R"QASM(OPENQASM 3.0;
include "stdgates.inc";
gate cxyz q0 { U(pi/2, 0, pi/2) q0; }
gate czyx q0 { U(pi/2, pi/2, pi/2) q0; }
gate cnxyz q0 { U(pi/2, pi/2, pi/2) q0; }
gate cxnyz q0 { U(pi/2, 0, -pi/2) q0; }
gate cxynz q0 { U(pi/2, pi/2, -pi/2) q0; }
gate cnzyx q0 { U(pi/2, -pi/2, 0) q0; }
gate cznyx q0 { U(pi/2, -pi/2, pi/2) q0; }
gate czynx q0 { U(pi/2, pi/2, 0) q0; }
gate hxy q0 { U(pi/2, 0, pi/2) q0; }
gate hyz q0 { U(pi/2, pi/2, pi/2) q0; }
gate hnxy q0 { U(pi/2, 0, -pi/2) q0; }
gate hnxz q0 { U(pi/2, pi/2, 0) q0; }
gate hnyz q0 { U(pi/2, -pi/2, -pi/2) q0; }
gate sy q0 { U(pi/2, 0, 0) q0; }
gate sydg q0 { U(pi/2, pi/2, pi/2) q0; }
gate cxswap q0, q1 { cx q1, q0; cx q0, q1; }
gate czswap q0, q1 { h q0; cx q0, q1; cx q1, q0; h q1; }
gate iswap q0, q1 { h q0; cx q0, q1; cx q1, q0; h q1; s q1; s q0; }
gate iswapdg q0, q1 { s q0; s q0; s q0; s q1; s q1; s q1; h q1; cx q1, q0; cx q0, q1; h q0; }
gate sxx q0, q1 { h q0; cx q0, q1; h q1; s q0; s q1; h q0; h q1; }
gate sxxdg q0, q1 { h q0; cx q0, q1; h q1; s q0; s q0; s q0; s q1; s q1; s q1; h q0; h q1; }
gate syy q0, q1 { s q0; s q0; s q0; s q1; s q1; s q1; h q0; cx q0, q1; h q1; s q0; s q1; h q0; h q1; s q0; s q1; }
gate syydg q0, q1 { s q0; s q0; s q0; s q1; h q0; cx q0, q1; h q1; s q0; s q1; h q0; h q1; s q0; s q1; s q1; s q1; }
gate szz q0, q1 { h q1; cx q0, q1; h q1; s q0; s q1; }
gate szzdg q0, q1 { h q1; cx q0, q1; h q1; s q0; s q0; s q0; s q1; s q1; s q1; }
gate swapcx q0, q1 { cx q0, q1; cx q1, q0; }
gate xcx q0, q1 { h q0; cx q0, q1; h q0; }
gate xcy q0, q1 { h q0; s q1; s q1; s q1; cx q0, q1; h q0; s q1; }
gate xcz q0, q1 { cx q1, q0; }
gate ycx q0, q1 { s q0; s q0; s q0; h q1; cx q1, q0; s q0; h q1; }
gate ycy q0, q1 { s q0; s q0; s q0; s q1; s q1; s q1; h q0; cx q0, q1; h q0; s q0; s q1; }
gate ycz q0, q1 { s q0; s q0; s q0; cx q1, q0; s q0; }
def mr(qubit q0) -> bit { bit b; measure q0 -> b; reset q0; return b; }
def mrx(qubit q0) -> bit { bit b; h q0; measure q0 -> b; reset q0; h q0; return b; }
def mry(qubit q0) -> bit { bit b; s q0; s q0; s q0; h q0; measure q0 -> b; reset q0; h q0; s q0; return b; }
def mx(qubit q0) -> bit { bit b; h q0; measure q0 -> b; h q0; return b; }
def mxx(qubit q0, qubit q1) -> bit { bit b; cx q0, q1; h q0; measure q0 -> b; h q0; cx q0, q1; return b; }
def my(qubit q0) -> bit { bit b; s q0; s q0; s q0; h q0; measure q0 -> b; h q0; s q0; return b; }
def myy(qubit q0, qubit q1) -> bit { bit b; s q0; s q1; cx q0, q1; h q0; measure q0 -> b; s q1; s q1; h q0; cx q0, q1; s q0; s q1; return b; }
def mzz(qubit q0, qubit q1) -> bit { bit b; cx q0, q1; measure q1 -> b; cx q0, q1; return b; }
def rx(qubit q0) { reset q0; h q0; }
def ry(qubit q0) { reset q0; h q0; s q0; }

qreg q[18];
creg rec[25];
creg dets[1];
creg obs[2];
creg sweep[1];

id q[0];
x q[1];
y q[2];
z q[3];
barrier q;

cxyz q[0];
cnxyz q[1];
cxnyz q[2];
cxynz q[3];
czyx q[4];
cnzyx q[5];
cznyx q[6];
czynx q[7];
hxy q[0];
h q[1];
hyz q[2];
hnxy q[3];
hnxz q[4];
hnyz q[5];
sx q[0];
sxdg q[1];
sy q[2];
sydg q[3];
s q[4];
sdg q[5];
barrier q;

cxswap q[0], q[1];
iswap q[2], q[3];
iswapdg q[4], q[5];
swap q[6], q[7];
swapcx q[8], q[9];
czswap q[10], q[11];
sxx q[0], q[1];
sxxdg q[2], q[3];
syy q[4], q[5];
syydg q[6], q[7];
szz q[8], q[9];
szzdg q[10], q[11];
xcx q[0], q[1];
xcy q[2], q[3];
xcz q[4], q[5];
ycx q[6], q[7];
ycy q[8], q[9];
ycz q[10], q[11];
cx q[12], q[13];
cy q[14], q[15];
cz q[16], q[17];
barrier q;

rec[0] = 0;
rec[1] = 0;
barrier q;

// --- begin decomposed MPP X0*Y1*Z2 Z0*Z1
h q[0];
hyz q[1];
cx q[1], q[0];
cx q[2], q[0];
measure q[0] -> rec[2];
cx q[1], q[0];
cx q[2], q[0];
hyz q[1];
h q[0];
cx q[1], q[0];
measure q[0] -> rec[3];
cx q[1], q[0];
// --- end decomposed MPP
// --- begin decomposed SPP X0*Y1*Z2 X3
h q[0];
hyz q[1];
cx q[1], q[0];
cx q[2], q[0];
s q[0];
cx q[1], q[0];
cx q[2], q[0];
hyz q[1];
h q[0];
h q[3];
s q[3];
h q[3];
// --- end decomposed SPP
// --- begin decomposed SPP_DAG X0*Y1*Z2 X2
h q[0];
hyz q[1];
cx q[1], q[0];
cx q[2], q[0];
sdg q[0];
cx q[1], q[0];
cx q[2], q[0];
hyz q[1];
h q[0];
h q[2];
sdg q[2];
h q[2];
// --- end decomposed SPP
barrier q;

rec[4] = mrx(q[0]);
rec[5] = mry(q[1]);
rec[6] = mr(q[2]);
rec[7] = mx(q[3]);
rec[8] = my(q[4]);
measure q[5] -> rec[9];
measure q[6] -> rec[10];
rx(q[7]);
ry(q[8]);
reset q[9];
barrier q;

rec[11] = mxx(q[0], q[1]);
rec[12] = mxx(q[2], q[3]);
rec[13] = myy(q[4], q[5]);
rec[14] = mzz(q[6], q[7]);
barrier q;

h q[0];
cx q[0], q[1];
s q[1];
barrier q;

h q[0];
cx q[0], q[1];
s q[1];
barrier q;

h q[0];
cx q[0], q[1];
s q[1];
barrier q;

barrier q;

rec[15] = mr(q[0]);
rec[16] = mr(q[0]);
dets[0] = rec[16] ^ 0;
obs[0] = obs[0] ^ rec[16] ^ 0;
rec[17] = 0;
rec[18] = 1;
rec[19] = 0;
obs[1] = obs[1] ^ 0;
// Warning: ignored pauli terms in OBSERVABLE_INCLUDE(1) Z2 Z3
barrier q;

rec[20] = mrx(q[0]) ^ 1;
rec[21] = my(q[1]) ^ 1;
rec[22] = mzz(q[2], q[3]) ^ 1;
obs[1] = obs[1] ^ rec[22] ^ 1;
rec[23] = myy(q[4], q[5]);
// --- begin decomposed MPP X6*!Y7*Z8
h q[6];
hyz q[7];
cx q[7], q[6];
cx q[8], q[6];
measure q[6] -> rec[24];rec[24] = rec[24] ^ 1;
cx q[7], q[6];
cx q[8], q[6];
hyz q[7];
h q[6];
// --- end decomposed MPP
barrier q;

if (ms[24]) {
    X q[0];
}
if (sweep[0]) {
    Y q[1];
}
if (ms[24]) {
    Z q[2];
}
)QASM");
}

TEST(export_qasm, export_qasm_all_operations_v2) {
    Circuit c = generate_test_circuit_with_all_operations();
    c = c.without_noise();

    std::stringstream out;
    c = circuit_with_inlined_feedback(c);
    for (size_t k = 0; k < c.operations.size(); k++) {
        bool drop = false;
        for (auto t : c.operations[k].targets) {
            drop |= t.is_sweep_bit_target();
            drop |= c.operations[k].gate_type == GateType::MPAD && t.qubit_value() > 0;
        }
        if (drop) {
            c.operations.erase(c.operations.begin() + k);
            k--;
        }
    }
    export_open_qasm(c, out, 2, true);
    ASSERT_EQ(out.str(), R"QASM(OPENQASM 2.0;
include "qelib1.inc";
gate cxyz q0 { U(pi/2, 0, pi/2) q0; }
gate czyx q0 { U(pi/2, pi/2, pi/2) q0; }
gate cnxyz q0 { U(pi/2, pi/2, pi/2) q0; }
gate cxnyz q0 { U(pi/2, 0, -pi/2) q0; }
gate cxynz q0 { U(pi/2, pi/2, -pi/2) q0; }
gate cnzyx q0 { U(pi/2, -pi/2, 0) q0; }
gate cznyx q0 { U(pi/2, -pi/2, pi/2) q0; }
gate czynx q0 { U(pi/2, pi/2, 0) q0; }
gate hxy q0 { U(pi/2, 0, pi/2) q0; }
gate hyz q0 { U(pi/2, pi/2, pi/2) q0; }
gate hnxy q0 { U(pi/2, 0, -pi/2) q0; }
gate hnxz q0 { U(pi/2, pi/2, 0) q0; }
gate hnyz q0 { U(pi/2, -pi/2, -pi/2) q0; }
gate sy q0 { U(pi/2, 0, 0) q0; }
gate sydg q0 { U(pi/2, pi/2, pi/2) q0; }
gate cxswap q0, q1 { cx q1, q0; cx q0, q1; }
gate czswap q0, q1 { h q0; cx q0, q1; cx q1, q0; h q1; }
gate iswap q0, q1 { h q0; cx q0, q1; cx q1, q0; h q1; s q1; s q0; }
gate iswapdg q0, q1 { s q0; s q0; s q0; s q1; s q1; s q1; h q1; cx q1, q0; cx q0, q1; h q0; }
gate sxx q0, q1 { h q0; cx q0, q1; h q1; s q0; s q1; h q0; h q1; }
gate sxxdg q0, q1 { h q0; cx q0, q1; h q1; s q0; s q0; s q0; s q1; s q1; s q1; h q0; h q1; }
gate syy q0, q1 { s q0; s q0; s q0; s q1; s q1; s q1; h q0; cx q0, q1; h q1; s q0; s q1; h q0; h q1; s q0; s q1; }
gate syydg q0, q1 { s q0; s q0; s q0; s q1; h q0; cx q0, q1; h q1; s q0; s q1; h q0; h q1; s q0; s q1; s q1; s q1; }
gate szz q0, q1 { h q1; cx q0, q1; h q1; s q0; s q1; }
gate szzdg q0, q1 { h q1; cx q0, q1; h q1; s q0; s q0; s q0; s q1; s q1; s q1; }
gate swapcx q0, q1 { cx q0, q1; cx q1, q0; }
gate xcx q0, q1 { h q0; cx q0, q1; h q0; }
gate xcy q0, q1 { h q0; s q1; s q1; s q1; cx q0, q1; h q0; s q1; }
gate xcz q0, q1 { cx q1, q0; }
gate ycx q0, q1 { s q0; s q0; s q0; h q1; cx q1, q0; s q0; h q1; }
gate ycy q0, q1 { s q0; s q0; s q0; s q1; s q1; s q1; h q0; cx q0, q1; h q0; s q0; s q1; }
gate ycz q0, q1 { s q0; s q0; s q0; cx q1, q0; s q0; }

qreg q[18];
creg rec[22];

id q[0];
x q[1];
y q[2];
z q[3];
barrier q;

cxyz q[0];
cnxyz q[1];
cxnyz q[2];
cxynz q[3];
czyx q[4];
cnzyx q[5];
cznyx q[6];
czynx q[7];
hxy q[0];
h q[1];
hyz q[2];
hnxy q[3];
hnxz q[4];
hnyz q[5];
sx q[0];
sxdg q[1];
sy q[2];
sydg q[3];
s q[4];
sdg q[5];
barrier q;

cxswap q[0], q[1];
iswap q[2], q[3];
iswapdg q[4], q[5];
swap q[6], q[7];
swapcx q[8], q[9];
czswap q[10], q[11];
sxx q[0], q[1];
sxxdg q[2], q[3];
syy q[4], q[5];
syydg q[6], q[7];
szz q[8], q[9];
szzdg q[10], q[11];
xcx q[0], q[1];
xcy q[2], q[3];
xcz q[4], q[5];
ycx q[6], q[7];
ycy q[8], q[9];
ycz q[10], q[11];
cx q[12], q[13];
cy q[14], q[15];
cz q[16], q[17];
barrier q;

barrier q;

// --- begin decomposed MPP X0*Y1*Z2 Z0*Z1
h q[0];
hyz q[1];
cx q[1], q[0];
cx q[2], q[0];
measure q[0] -> rec[2];
cx q[1], q[0];
cx q[2], q[0];
hyz q[1];
h q[0];
cx q[1], q[0];
measure q[0] -> rec[3];
cx q[1], q[0];
// --- end decomposed MPP
// --- begin decomposed SPP X0*Y1*Z2 X3
h q[0];
hyz q[1];
cx q[1], q[0];
cx q[2], q[0];
s q[0];
cx q[1], q[0];
cx q[2], q[0];
hyz q[1];
h q[0];
h q[3];
s q[3];
h q[3];
// --- end decomposed SPP
// --- begin decomposed SPP_DAG X0*Y1*Z2 X2
h q[0];
hyz q[1];
cx q[1], q[0];
cx q[2], q[0];
sdg q[0];
cx q[1], q[0];
cx q[2], q[0];
hyz q[1];
h q[0];
h q[2];
sdg q[2];
h q[2];
// --- end decomposed SPP
barrier q;

h q[0]; measure q[0] -> rec[4]; reset q[0]; h q[0]; // decomposed MRX
s q[1]; s q[1]; s q[1]; h q[1]; measure q[1] -> rec[5]; reset q[1]; h q[1]; s q[1]; // decomposed MRY
measure q[2] -> rec[6]; reset q[2]; // decomposed MR
h q[3]; measure q[3] -> rec[7]; h q[3]; // decomposed MX
s q[4]; s q[4]; s q[4]; h q[4]; measure q[4] -> rec[8]; h q[4]; s q[4]; // decomposed MY
measure q[5] -> rec[9];
measure q[6] -> rec[10];
reset q[7]; h q[7]; // decomposed RX
reset q[8]; h q[8]; s q[8]; // decomposed RY
reset q[9];
barrier q;

cx q[0], q[1]; h q[0]; measure q[0] -> rec[11]; h q[0]; cx q[0], q[1]; // decomposed MXX
cx q[2], q[3]; h q[2]; measure q[2] -> rec[12]; h q[2]; cx q[2], q[3]; // decomposed MXX
s q[4]; s q[5]; cx q[4], q[5]; h q[4]; measure q[4] -> rec[13]; s q[5]; s q[5]; h q[4]; cx q[4], q[5]; s q[4]; s q[5]; // decomposed MYY
cx q[6], q[7]; measure q[7] -> rec[14]; cx q[6], q[7]; // decomposed MZZ
barrier q;

h q[0];
cx q[0], q[1];
s q[1];
barrier q;

h q[0];
cx q[0], q[1];
s q[1];
barrier q;

h q[0];
cx q[0], q[1];
s q[1];
barrier q;

barrier q;

measure q[0] -> rec[15]; reset q[0]; // decomposed MR
measure q[0] -> rec[16]; reset q[0]; // decomposed MR
barrier q;

h q[0]; x q[0];measure q[0] -> rec[17];x q[0]; reset q[0]; h q[0]; // decomposed MRX
s q[1]; s q[1]; s q[1]; h q[1]; x q[1];measure q[1] -> rec[18];x q[1]; h q[1]; s q[1]; // decomposed MY
cx q[2], q[3]; x q[3];measure q[3] -> rec[19];x q[3]; cx q[2], q[3]; // decomposed MZZ
s q[4]; s q[5]; cx q[4], q[5]; h q[4]; measure q[4] -> rec[20]; s q[5]; s q[5]; h q[4]; cx q[4], q[5]; s q[4]; s q[5]; // decomposed MYY
// --- begin decomposed MPP X6*!Y7*Z8
h q[6];
hyz q[7];
cx q[7], q[6];
cx q[8], q[6];
x q[6];measure q[6] -> rec[21];x q[6];
cx q[7], q[6];
cx q[8], q[6];
hyz q[7];
h q[6];
// --- end decomposed MPP
barrier q;

)QASM");
}
