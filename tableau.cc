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
        size_t i0 = i >> 6;
        size_t i1 = i & 63;
        q.x._x[i0] |= 1ull << i1;
        q.y._y[i0] |= 1ull << i1;
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

void Tableau::inplace_scatter_append(const Tableau &operation, const std::vector<size_t> &target_qubits) {
    assert(operation.qubits.size() == target_qubits.size());
    for (auto &q : qubits) {
        operation.apply_within(q.x, target_qubits);
        operation.apply_within(q.y, target_qubits);
    }
}

bool Tableau::operator==(const Tableau &other) const {
    if (qubits.size() != other.qubits.size()) {
        return false;
    }
    for (size_t k = 0; k < qubits.size(); k++) {
        if (qubits[k].x != other.qubits[k].x || qubits[k].y != other.qubits[k].y) {
            return false;
        }
    }
    return true;
}

bool Tableau::operator!=(const Tableau &other) const {
    return !(*this == other);
}

void Tableau::inplace_scatter_prepend(const Tableau &operation, const std::vector<size_t> &target_qubits) {
    assert(operation.qubits.size() == target_qubits.size());
    std::vector<TableauQubit> new_qubits;
    new_qubits.reserve(operation.qubits.size());
    for (const auto &q : operation.qubits) {
        new_qubits.push_back({
            scatter_eval(q.x, target_qubits),
            scatter_eval(q.y, target_qubits),
        });
    }
    for (size_t i = 0; i < operation.qubits.size(); i++) {
        auto &t = qubits[target_qubits[i]];
        auto &q = new_qubits[i];
        t.x._sign = q.x._sign;
        t.y._sign = q.y._sign;
        t.x._x = q.x._x;
        t.x._y = q.x._y;
        t.y._x = q.y._x;
        t.y._y = q.y._y;
        q.x._x = nullptr;
        q.x._y = nullptr;
        q.y._x = nullptr;
        q.y._y = nullptr;
//        qubits[target_qubits[i]] = std::move(new_qubits[i]);
    }
}


PauliString Tableau::scatter_eval(const PauliString &gathered_input, const std::vector<size_t> &scattered_indices) const {
    assert(gathered_input.size == scattered_indices.size());
    auto result = PauliString::identity(qubits.size());
    result._sign = gathered_input._sign;
    for (size_t k_gathered = 0; k_gathered < gathered_input.size; k_gathered++) {
        size_t k_scattered = scattered_indices[k_gathered];
        auto x = gathered_input.get_x_bit(k_gathered);
        auto y = gathered_input.get_y_bit(k_gathered);
        if (x) {
            if (y) {
                // Multiply by Z using Z = -i*X*Y.
                uint8_t log_i = 3;
                log_i += result.inplace_right_mul_with_scalar_output(qubits[k_scattered].x);
                log_i += result.inplace_right_mul_with_scalar_output(qubits[k_scattered].y);
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

PauliString Tableau::operator()(const PauliString &p) const {
    assert(p.size == qubits.size());
    std::vector<size_t> indices;
    for (size_t k = 0; k < p.size; k++) {
        indices.push_back(k);
    }
    return scatter_eval(p, indices);
}

void Tableau::apply_within(PauliString &target, const std::vector<size_t> &target_qubits) const {
    assert(qubits.size() == target_qubits.size());
    auto inp = PauliString::identity(qubits.size());
    target.gather_into(inp, target_qubits);
    auto out = (*this)(inp);
    out.scatter_into(target, target_qubits);
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
    {"CX", Tableau::gate2("+XX", "+YX", "+IX", "+ZY")},
    {"CY", Tableau::gate2("+XY", "+YY", "+ZX", "+IY")},
    {"CZ", Tableau::gate2("+XZ", "+YZ", "+ZX", "+ZY")},
    {"SWAP", Tableau::gate2("+IX", "+IY", "+XI", "+YI")},
};
