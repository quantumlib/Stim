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
