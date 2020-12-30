#include <iostream>
#include <map>
#include "pauli_string.h"
#include "tableau.h"

Tableau Tableau::identity(size_t size) {
    Tableau result;
    result.qubits.reserve(size);
    for (size_t i = 0; i < size; i++) {
        TableauQubit q {
                PauliString::identity(size),
                PauliString::identity(size),
        };
        size_t i0 = i / 256;
        size_t i1 = (i & 255) / 64;
        size_t i2 = i & 63;
        q.x._x[i0].m256i_u64[i1] |= 1ull << i2;
        q.y._y[i0].m256i_u64[i1] |= 1ull << i2;
        result.qubits.emplace_back(q);
    }
    return result;
}

Tableau Tableau::gate1(const char *x, const char *y) {
    return Tableau{{{{PauliString::from_str(x), PauliString::from_str(y)}}}};
}

Tableau Tableau::gate2(const char *x1,
                     const char *y1,
                     const char *x2,
                     const char *y2) {
    return Tableau{{{{PauliString::from_str(x1), PauliString::from_str(y1)},
                     {PauliString::from_str(x2), PauliString::from_str(y2)}}}};
}

std::ostream &operator<<(std::ostream &out, const Tableau &t) {
    out << "Tableau {\n";
    for (size_t i = 0; i < t.qubits.size(); i++) {
        out << "  qubit " << i << "_x: " << t.qubits[i].x << "\n";
        out << "  qubit " << i << "_y: " << t.qubits[i].y << "\n";
    }
    out << "}";
    return out;
}

std::string Tableau::str() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
}

void Tableau::inplace_append(const Tableau &operation, const size_t *target_qubits) {
    for (auto &q : qubits) {
        operation.apply_within(q.x, target_qubits);
        operation.apply_within(q.y, target_qubits);
    }
}

void Tableau::inplace_prepend(const Tableau &operation, const size_t *target_qubits) {
    std::vector<TableauQubit> new_qubits;
    new_qubits.reserve(operation.qubits.size());
    for (const auto &q : operation.qubits) {
        new_qubits.push_back({
            scatter_eval(q.x, target_qubits),
            scatter_eval(q.y, target_qubits),
        });
    }
    for (size_t i = 0; i < operation.qubits.size(); i++) {
        qubits[target_qubits[i]] = std::move(new_qubits[i]);
    }
}


PauliString Tableau::scatter_eval(const PauliString &gathered_input, const size_t *scattered_indices) const {
    auto result = PauliString::identity(qubits.size());
    result._sign = gathered_input._sign;
    for (size_t k_gathered = 0; k_gathered < gathered_input.size; k_gathered++) {
        size_t k_scattered = scattered_indices[k_gathered];
        auto x = gathered_input.get_x_bit(k_gathered);
        auto y = gathered_input.get_y_bit(k_gathered);
        if (x) {
            if (y) {
                // Build Z from i*X*Y.
                uint8_t log_i_1;
                uint8_t log_i_2;
                result.inplace_right_mul_with_scalar_output(qubits[k_scattered].x, &log_i_1);
                result.inplace_right_mul_with_scalar_output(qubits[k_scattered].y, &log_i_2);
                uint8_t log_i = log_i_1 + log_i_2 + 1;
                assert((log_i & 1) == 0);
                result._sign ^= (log_i & 2) != 0;
            } else {
                result *= qubits[k_scattered].x;
            }
        } else if (y) {
            result *= qubits[k_scattered].y;
        }
    }
    return result;
}

void Tableau::apply_within(PauliString &target, const size_t *target_qubits) const {
    auto sub = PauliString::identity(qubits.size());
//    target.gather_into(sub, target_qubits);
//    apply_within()
//    self.call2(sub).scatter_into(targets, target)
    /*
        if self.sign:
            target.sign = not target.sign
        for k_in, k_out in enumerate(indices):
            i0 = k_in // 64
            i1 = np.uint64(k_in & 63)
            j0 = k_out // 64
            j1 = np.uint64(k_out & 63)
            x = (self.x[i0] >> i1) & np.uint64(1)
            z = (self.z[i0] >> i1) & np.uint64(1)
            target.x[j0] &= ~(np.uint64(1) << j1)
            target.z[j0] &= ~(np.uint64(1) << j1)
            target.x[j0] |= x << j1
            target.z[j0] |= z << j1
     */
}

const std::map<std::string, const Tableau> GATE_TABLEAUS {
    {"I", Tableau::gate1("+X", "+Y")},
    // Pauli gates.
    {"X", Tableau::gate1("+X", "-Y")},
    {"Y", Tableau::gate1("-X", "+Y")},
    {"Z", Tableau::gate1("-X", "-Y")},
    // Axis exchange gates.
    {"H", Tableau::gate1("+Z", "-Y")},
    {"H_XY", Tableau::gate1("+Y", "+X")},
    {"H_XZ", Tableau::gate1("+Z", "-Y")},
    {"H_YZ", Tableau::gate1("-X", "+Z")},
    // 90 degree rotation gates.
    {"SQRT_X", Tableau::gate1("+X", "+Z")},
    {"SQRT_X_DAG", Tableau::gate1("+X", "-Z")},
    {"SQRT_Y", Tableau::gate1("-Z", "+Y")},
    {"SQRT_Y_DAG", Tableau::gate1("+Z", "+Y")},
    {"SQRT_Z", Tableau::gate1("+Y", "-X")},
    {"SQRT_Z_DAG", Tableau::gate1("-Y", "+X")},
    {"S", Tableau::gate1("+Y", "-X")},
    {"S_DAG", Tableau::gate1("-Y", "+X")},
    // Two qubit gates.
    {"CNOT", Tableau::gate2("+XX", "+YX", "+IX", "+ZY")},
    {"CZ", Tableau::gate2("+XZ", "+YZ", "+ZX", "+ZY")},
    {"SWAP", Tableau::gate2("+IX", "+IY", "+XI", "+YI")},
};
