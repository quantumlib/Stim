#include "stim/simulators/graph_simulator.h"

using namespace stim;

GraphSimulator::GraphSimulator(size_t num_qubits)
    : num_qubits(num_qubits), adj(num_qubits, num_qubits), paulis(num_qubits), x2outs(num_qubits), z2outs(num_qubits) {
    for (size_t k = 0; k < num_qubits; k++) {
        x2outs.zs[k] = 1;
        z2outs.xs[k] = 1;
    }
}

void GraphSimulator::do_1q_gate(GateType gate, size_t qubit) {
    GateTarget t = GateTarget::qubit(qubit);
    x2outs.ref().do_instruction(CircuitInstruction{gate, {}, &t, ""});
    z2outs.ref().do_instruction(CircuitInstruction{gate, {}, &t, ""});
    paulis.xs[qubit] ^= z2outs.sign;
    paulis.zs[qubit] ^= x2outs.sign;
    x2outs.sign = 0;
    z2outs.sign = 0;
}

std::tuple<bool, bool, bool> GraphSimulator::after2inside_basis_transform(size_t qubit, bool x, bool z) {
    bool xx = x2outs.xs[qubit];
    bool xz = x2outs.zs[qubit];
    bool zx = z2outs.xs[qubit];
    bool zz = z2outs.zs[qubit];
    bool out_x = (x & zz) ^ (z & zx);
    bool out_z = (x & xz) ^ (z & xx);
    bool sign = false;
    sign ^= paulis.xs[qubit] & out_z;
    sign ^= paulis.zs[qubit] & out_x;
    sign ^= out_x == out_z && !(xx ^ zz) && !(xx ^ xz ^ zx);
    return {out_x, out_z, sign};
}

void GraphSimulator::inside_do_cz(size_t a, size_t b) {
    adj[a][b] ^= 1;
    adj[b][a] ^= 1;
}

void GraphSimulator::inside_do_cx(size_t c, size_t t) {
    adj[c] ^= adj[t];
    for (size_t k = 0; k < num_qubits; k++) {
        adj[k][c] = adj[c][k];
    }
    paulis.zs[c] ^= adj[c][c];
    adj[c][c] = 0;
}

void GraphSimulator::inside_do_sqrt_z(size_t q) {
    bool x2x = x2outs.xs[q];
    bool x2z = x2outs.zs[q];
    bool z2x = z2outs.xs[q];
    bool z2z = z2outs.zs[q];
    paulis.zs[q] ^= paulis.xs[q];
    paulis.zs[q] ^= !(x2x ^ z2z) && !(x2x ^ x2z ^ z2x);
    x2outs.xs[q] ^= z2x;
    x2outs.zs[q] ^= z2z;
}

void GraphSimulator::inside_do_sqrt_x_dag(size_t q) {
    bool x2x = x2outs.xs[q];
    bool x2z = x2outs.zs[q];
    bool z2x = z2outs.xs[q];
    bool z2z = z2outs.zs[q];
    paulis.xs[q] ^= paulis.zs[q];
    paulis.xs[q] ^= !(x2x ^ z2z) && !(x2x ^ x2z ^ z2x);
    z2outs.xs[q] ^= x2x;
    z2outs.zs[q] ^= x2z;
}

void GraphSimulator::inside_do_cy(size_t c, size_t t) {
    inside_do_cz(c, t);
    inside_do_cx(c, t);
    inside_do_sqrt_z(c);
}

void GraphSimulator::verify_invariants() const {
    // No self-adjacency.
    for (size_t q = 0; q < num_qubits; q++) {
        assert(!adj[q][q]);
    }
    // Undirected adjacency.
    for (size_t q1 = 0; q1 < num_qubits; q1++) {
        for (size_t q2 = q1 + 1; q2 < num_qubits; q2++) {
            assert(adj[q1][q2] == adj[q2][q1]);
        }
    }
    // Single qubits gates are clifford.
    for (size_t q = 0; q < num_qubits; q++) {
        assert(x2outs.xs[q] || x2outs.zs[q]);
        assert(z2outs.xs[q] || z2outs.zs[q]);
        assert((x2outs.xs[q] != z2outs.xs[q]) || (x2outs.zs[q] != z2outs.zs[q]));
    }
}

void GraphSimulator::do_complementation(size_t q) {
    buffer.clear();
    for (size_t neighbor = 0; neighbor < num_qubits; neighbor++) {
        if (adj[q][neighbor]) {
            buffer.push_back(neighbor);
            inside_do_sqrt_z(neighbor);
        }
    }
    for (size_t k1 = 0; k1 < buffer.size(); k1++) {
        for (size_t k2 = k1 + 1; k2 < buffer.size(); k2++) {
            inside_do_cz(buffer[k1], buffer[k2]);
        }
    }
    inside_do_sqrt_x_dag(q);
}

void GraphSimulator::inside_do_ycx(size_t q1, size_t q2) {
    if (adj[q1][q2]) {
        // Y:X -> SQRT_X(Y):SQRT_Z(X) = Z:Y
        do_complementation(q1);
        inside_do_cy(q1, q2);
        paulis.zs[q1] ^= 1;
    } else {
        // Y:X -> SQRT_X(Y):Y = Z:X
        do_complementation(q1);
        inside_do_cx(q1, q2);
    }
}

void GraphSimulator::inside_do_ycy(size_t q1, size_t q2) {
    if (adj[q1][q2]) {
        // Y:Y -> SQRT_X(Y):SQRT_Z(Y) = Z:X
        do_complementation(q1);
        inside_do_cx(q1, q2);
    } else {
        // Y:Y -> SQRT_X(Y):Y = Z:Y
        do_complementation(q1);
        inside_do_cy(q1, q2);
    }
}

void GraphSimulator::inside_do_xcx(size_t q1, size_t q2) {
    if (adj[q1][q2]) {
        // X:X -> S(X):SQRT_X_DAG(X) = (-Y):X
        do_complementation(q2);
        // (-Y):X -> SQRT_X_DAG(-Y):S(X) = (-Z):(-Y)
        do_complementation(q1);
        inside_do_cy(q1, q2);
        paulis.zs[q1] ^= 1;
        paulis.xs[q2] ^= 1;
        paulis.zs[q2] ^= 1;
    } else {
        // Need an S gate.
        // Get it by finding a neighbor to do local complementation on.
        for (size_t q3 = 0; q3 < num_qubits; q3++) {
            if (adj[q1][q3]) {
                do_complementation(q3);
                if (adj[q2][q3]) {
                    // X:X -> S(X):S(X) = (-Y):(-Y)
                    paulis.xs[q1] ^= 1;
                    paulis.zs[q1] ^= 1;
                    paulis.xs[q2] ^= 1;
                    paulis.zs[q2] ^= 1;
                    inside_do_ycy(q1, q2);
                } else {
                    // X:X -> S(X):X = (-Y):X
                    paulis.xs[q2] ^= 1;
                    inside_do_ycx(q1, q2);
                }
                return;
            }
        }

        // q1 has no CZ gates applied to it.
        // Therefore, inside the single qubit gates, q1 is in the |+> state.
        // Therefore the XCX gate's control isn't satisfied, and it does nothing.
        // <-- look at all the doing-nothing right here. -->
    }
}

void GraphSimulator::inside_do_pauli_interaction(bool x1, bool z1, bool x2, bool z2, size_t q1, size_t q2) {
    int p1 = x1 + z1 * 2 - 1;
    int p2 = x2 + z2 * 2 - 1;
    switch (p1 + p2 * 3) {
        case 0:  // X:X
            inside_do_xcx(q1, q2);
            break;
        case 1:  // Z:X
            inside_do_cx(q1, q2);
            break;
        case 2:  // Y:X
            inside_do_ycx(q1, q2);
            break;
        case 3:  // X:Z
            inside_do_cx(q2, q1);
            break;
        case 4:  // Z:Z
            inside_do_cz(q1, q2);
            break;
        case 5:  // Y:Z
            inside_do_cy(q2, q1);
            break;
        case 6:  // X:Y
            inside_do_ycx(q2, q1);
            break;
        case 7:  // Z:Y
            inside_do_cy(q1, q2);
            break;
        case 8:  // Y:Y
            inside_do_ycy(q1, q2);
            break;
        default:
            throw std::invalid_argument("Unknown pauli interaction.");
    }
}

std::ostream &stim::operator<<(std::ostream &out, const GraphSimulator &sim) {
    out << "stim::GraphSimulator{\n";
    out << "    .num_qubits=" << sim.num_qubits << ",\n";
    out << "    .paulis=" << sim.paulis << ",\n";
    out << "    .x2outs=" << sim.x2outs << ",\n";
    out << "    .z2outs=" << sim.z2outs << ",\n";
    out << "    .adj=stim::simd_bit_table<64>::from_text(R\"TAB(\n" << sim.adj.str(sim.num_qubits) << "\n)TAB\"),\n";
    out << "}";
    return out;
}

std::string GraphSimulator::str() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
}

GraphSimulator GraphSimulator::random_state(size_t n, std::mt19937_64 &rng) {
    GraphSimulator sim(n);
    sim.adj = simd_bit_table<64>::random(n, n, rng);
    for (size_t q1 = 0; q1 < n; q1++) {
        sim.adj[q1][q1] = 0;
        for (size_t q2 = q1 + 1; q2 < n; q2++) {
            sim.adj[q1][q2] = sim.adj[q2][q1];
        }
    }
    sim.paulis = PauliString<64>::random(n, rng);
    sim.paulis.sign = 0;
    std::array<uint8_t, 6> gate_xz_data{
        0b1001,  // I
        0b0110,  // H
        0b1011,  // S
        0b1101,  // SQRT_X_DAG
        0b0111,  // C_XYZ
        0b1110,  // C_ZYX
    };
    for (size_t q = 0; q < n; q++) {
        auto r = gate_xz_data[rng() % 6];
        sim.x2outs.xs[q] = r & 1;
        sim.x2outs.zs[q] = r & 2;
        sim.z2outs.xs[q] = r & 4;
        sim.z2outs.zs[q] = r & 8;
    }
    return sim;
}

void GraphSimulator::do_pauli_interaction(bool x1, bool z1, bool x2, bool z2, size_t qubit1, size_t qubit2) {
    // Propagate the interaction across the single qubit gate layer.
    auto [x1_in, z1_in, sign1] = after2inside_basis_transform(qubit1, x1, z1);
    auto [x2_in, z2_in, sign2] = after2inside_basis_transform(qubit2, x2, z2);

    // Kickback minus signs into Pauli gates on the other side.
    if (sign1) {
        paulis.xs[qubit2] ^= x2_in;
        paulis.zs[qubit2] ^= z2_in;
    }
    if (sign2) {
        paulis.xs[qubit1] ^= x1_in;
        paulis.zs[qubit1] ^= z1_in;
    }

    // Do the positive-sign interaction between the single-qubit layer and the CZ layer.
    inside_do_pauli_interaction(x1_in, z1_in, x2_in, z2_in, qubit1, qubit2);
}

void GraphSimulator::do_gate_by_decomposition(const CircuitInstruction &inst) {
    const Gate &d = GATE_DATA[inst.gate_type];
    bool is_all_qubits = true;
    for (auto t : inst.targets) {
        is_all_qubits &= t.is_qubit_target();
    }
    if (!is_all_qubits || d.h_s_cx_m_r_decomposition == nullptr || !(d.flags & GATE_TARGETS_PAIRS)) {
        throw std::invalid_argument("Not supported: " + inst.str());
    }

    Circuit circuit(d.h_s_cx_m_r_decomposition);
    for (size_t k = 0; k < inst.targets.size(); k += 2) {
        auto a = inst.targets[k];
        auto b = inst.targets[k + 1];
        for (const auto &inst2 : circuit.operations) {
            assert(
                inst2.gate_type == GateType::CX || inst2.gate_type == GateType::H || inst2.gate_type == GateType::S ||
                inst2.gate_type == GateType::R || inst2.gate_type == GateType::M);
            if (inst2.gate_type == GateType::CX) {
                for (size_t k2 = 0; k2 < inst2.targets.size(); k2 += 2) {
                    auto a2 = inst2.targets[k2];
                    auto b2 = inst2.targets[k2 + 1];
                    assert(a2.is_qubit_target());
                    assert(b2.is_qubit_target());
                    assert(a2.qubit_value() == 0 || a2.qubit_value() == 1);
                    assert(b2.qubit_value() == 0 || b2.qubit_value() == 1);
                    auto a3 = a2.qubit_value() == 0 ? a : b;
                    auto b3 = b2.qubit_value() == 0 ? a : b;
                    do_pauli_interaction(false, true, true, false, a3.qubit_value(), b3.qubit_value());
                }
            } else {
                for (GateTarget a2 : inst2.targets) {
                    assert(a2.is_qubit_target());
                    assert(a2.qubit_value() == 0 || a2.qubit_value() == 1);
                    auto a3 = a2.qubit_value() == 0 ? a : b;
                    do_1q_gate(inst2.gate_type, a3.qubit_value());
                }
            }
        }
    }
}

void GraphSimulator::do_2q_unitary_instruction(const CircuitInstruction &inst) {
    uint8_t p1;
    uint8_t p2;
    constexpr uint8_t X = 0b01;
    constexpr uint8_t Y = 0b11;
    constexpr uint8_t Z = 0b10;
    switch (inst.gate_type) {
        case GateType::XCX:
            p1 = X;
            p2 = X;
            break;
        case GateType::XCY:
            p1 = X;
            p2 = Y;
            break;
        case GateType::XCZ:
            p1 = X;
            p2 = Z;
            break;
        case GateType::YCX:
            p1 = Y;
            p2 = X;
            break;
        case GateType::YCY:
            p1 = Y;
            p2 = Y;
            break;
        case GateType::YCZ:
            p1 = Y;
            p2 = Z;
            break;
        case GateType::CX:
            p1 = Z;
            p2 = X;
            break;
        case GateType::CY:
            p1 = Z;
            p2 = Y;
            break;
        case GateType::CZ:
            p1 = Z;
            p2 = Z;
            break;
        default:
            do_gate_by_decomposition(inst);
            return;
    }
    bool x1 = p1 & 1;
    bool z1 = p1 & 2;
    bool x2 = p2 & 1;
    bool z2 = p2 & 2;

    for (size_t k = 0; k < inst.targets.size(); k += 2) {
        auto t1 = inst.targets[k];
        auto t2 = inst.targets[k + 1];
        if (!t1.is_qubit_target() || !t2.is_qubit_target()) {
            throw std::invalid_argument("Unsupported operation: " + inst.str());
        }
        do_pauli_interaction(x1, z1, x2, z2, t1.qubit_value(), t2.qubit_value());
    }
}

void GraphSimulator::output_pauli_layer(Circuit &out, bool to_hs_xyz) const {
    std::array<std::vector<GateTarget>, 4> groups;

    for (size_t q = 0; q < paulis.num_qubits; q++) {
        bool x = paulis.xs[q];
        bool z = paulis.zs[q];
        if (to_hs_xyz) {
            bool xx = x2outs.xs[q];
            bool xz = x2outs.zs[q];
            bool zx = z2outs.xs[q];
            bool zz = z2outs.zs[q];
            z ^= xx == 1 && xz == 1 && zx == 1 && zz == 0;
        }
        groups[x + 2 * z].push_back(GateTarget::qubit(q));
    }

    auto f = [&](GateType g, int k) {
        if (!groups[k].empty()) {
            out.safe_append(CircuitInstruction(g, {}, groups[k], ""));
        }
    };
    f(GateType::X, 0b01);
    f(GateType::Y, 0b11);
    f(GateType::Z, 0b10);
}

void GraphSimulator::output_clifford_layer(Circuit &out, bool to_hs_xyz) const {
    output_pauli_layer(out, to_hs_xyz);

    std::array<std::vector<GateTarget>, 16> groups;
    for (size_t q = 0; q < x2outs.num_qubits; q++) {
        bool xx = x2outs.xs[q];
        bool xz = x2outs.zs[q];
        bool zx = z2outs.xs[q];
        bool zz = z2outs.zs[q];
        groups[xx + 2 * xz + 4 * zx + 8 * zz].push_back(GateTarget::qubit(q));
    }

    std::array<std::vector<GateTarget>, 3> shs;
    auto f = [&](GateType g, int k, std::array<bool, 3> use_shs) {
        if (to_hs_xyz) {
            for (size_t j = 0; j < 3; j++) {
                if (use_shs[j]) {
                    shs[j].insert(shs[j].end(), groups[k].begin(), groups[k].end());
                }
            }
        } else {
            if (!groups[k].empty()) {
                out.safe_append(CircuitInstruction(g, {}, groups[k], ""));
            }
        }
    };
    f(GateType::C_XYZ, 0b0111, {1, 1, 0});
    f(GateType::C_ZYX, 0b1110, {0, 1, 1});
    f(GateType::H, 0b0110, {0, 1, 0});
    f(GateType::S, 0b1011, {1, 0, 0});
    f(GateType::SQRT_X_DAG, 0b1101, {1, 1, 1});
    for (size_t k = 0; k < 3; k++) {
        if (!shs[k].empty()) {
            std::sort(shs[k].begin(), shs[k].end());
            out.safe_append(CircuitInstruction(k == 1 ? GateType::H : GateType::S, {}, shs[k], ""));
        }
    }
}

Circuit GraphSimulator::to_circuit(bool to_hs_xyz) const {
    std::vector<GateTarget> targets;
    targets.reserve(2 * num_qubits);
    Circuit out;

    for (size_t q = 0; q < num_qubits; q++) {
        targets.push_back(GateTarget::qubit(q));
    }
    if (!targets.empty()) {
        out.safe_append(CircuitInstruction(GateType::RX, {}, targets, ""));
    }
    out.safe_append(CircuitInstruction(GateType::TICK, {}, {}, ""));

    bool has_cz = false;
    for (size_t q = 0; q < num_qubits; q++) {
        targets.clear();
        for (size_t q2 = q + 1; q2 < num_qubits; q2++) {
            if (adj[q][q2]) {
                targets.push_back(GateTarget::qubit(q));
                targets.push_back(GateTarget::qubit(q2));
            }
        }
        if (!targets.empty()) {
            out.safe_append(CircuitInstruction(GateType::CZ, {}, targets, ""));
        }
        has_cz |= !targets.empty();
    }
    if (has_cz) {
        out.safe_append(CircuitInstruction(GateType::TICK, {}, {}, ""));
    }

    output_clifford_layer(out, to_hs_xyz);

    return out;
}

void GraphSimulator::do_instruction(const CircuitInstruction &instruction) {
    auto f = GATE_DATA[instruction.gate_type].flags;

    if (f & GATE_IS_UNITARY) {
        if (f & GATE_IS_SINGLE_QUBIT_GATE) {
            for (auto t : instruction.targets) {
                do_1q_gate(instruction.gate_type, t.qubit_value());
            }
            return;
        }
        if (f & GATE_TARGETS_PAIRS) {
            do_2q_unitary_instruction(instruction);
            return;
        }
    }

    switch (instruction.gate_type) {
        case GateType::TICK:
        case GateType::QUBIT_COORDS:
        case GateType::SHIFT_COORDS:
            return;  // No effect.
        default:
            throw std::invalid_argument("Unsupported operation: " + instruction.str());
    }
}

void GraphSimulator::do_circuit(const Circuit &circuit) {
    circuit.for_each_operation([&](const CircuitInstruction &inst) {
        do_instruction(inst);
    });
}
