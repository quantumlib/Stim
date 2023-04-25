// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <cassert>
#include <sstream>

#include "stim/circuit/circuit.h"
#include "stim/mem/simd_util.h"
#include "stim/stabilizers/pauli_string.h"
#include "stim/stabilizers/tableau.h"

using namespace stim;

PauliStringRef::PauliStringRef(
    size_t init_num_qubits,
    bit_ref init_sign,
    simd_bits_range_ref<MAX_BITWORD_WIDTH> init_xs,
    simd_bits_range_ref<MAX_BITWORD_WIDTH> init_zs)
    : num_qubits(init_num_qubits), sign(init_sign), xs(init_xs), zs(init_zs) {
    assert(init_xs.num_bits_padded() == init_zs.num_bits_padded());
    assert(init_xs.num_simd_words == (init_num_qubits + MAX_BITWORD_WIDTH - 1) / MAX_BITWORD_WIDTH);
}

std::string PauliStringRef::sparse_str() const {
    std::stringstream out;
    out << "+-"[(bool)sign];
    bool first = true;
    for (size_t k = 0; k < num_qubits; k++) {
        auto x = xs[k];
        auto z = zs[k];
        auto p = x + 2 * z;
        if (p) {
            if (!first) {
                out << '*';
            }
            first = false;
            out << "IXZY"[p] << k;
        }
    }
    if (first) {
        out << 'I';
    }
    return out.str();
}

void PauliStringRef::swap_with(PauliStringRef other) {
    assert(num_qubits == other.num_qubits);
    sign.swap_with(other.sign);
    xs.swap_with(other.xs);
    zs.swap_with(other.zs);
}

PauliStringRef &PauliStringRef::operator=(const PauliStringRef &other) {
    assert(num_qubits == other.num_qubits);
    sign = other.sign;
    assert((bool)sign == (bool)other.sign);
    xs = other.xs;
    zs = other.zs;
    return *this;
}

std::string PauliStringRef::str() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
}

bool PauliStringRef::operator==(const PauliStringRef &other) const {
    return num_qubits == other.num_qubits && sign == other.sign && xs == other.xs && zs == other.zs;
}

bool PauliStringRef::operator!=(const PauliStringRef &other) const {
    return !(*this == other);
}

std::ostream &stim::operator<<(std::ostream &out, const PauliStringRef &ps) {
    out << "+-"[ps.sign];
    for (size_t k = 0; k < ps.num_qubits; k++) {
        out << "_XZY"[ps.xs[k] + 2 * ps.zs[k]];
    }
    return out;
}

PauliStringRef &PauliStringRef::operator*=(const PauliStringRef &rhs) {
    uint8_t log_i = inplace_right_mul_returning_log_i_scalar(rhs);
    assert((log_i & 1) == 0);
    sign ^= log_i & 2;
    return *this;
}

uint8_t PauliStringRef::inplace_right_mul_returning_log_i_scalar(const PauliStringRef &rhs) noexcept {
    assert(num_qubits == rhs.num_qubits);

    // Accumulator registers for counting mod 4 in parallel across each bit position.
    simd_word cnt1{};
    simd_word cnt2{};

    xs.for_each_word(zs, rhs.xs, rhs.zs, [&cnt1, &cnt2](simd_word &x1, simd_word &z1, simd_word &x2, simd_word &z2) {
        // Update the left hand side Paulis.
        auto old_x1 = x1;
        auto old_z1 = z1;
        x1 ^= x2;
        z1 ^= z2;

        // At each bit position: accumulate anti-commutation (+i or -i) counts.
        auto x1z2 = old_x1 & z2;
        auto anti_commutes = (x2 & old_z1) ^ x1z2;
        cnt2 ^= (cnt1 ^ x1 ^ z1 ^ x1z2) & anti_commutes;
        cnt1 ^= anti_commutes;
    });

    // Combine final anti-commutation phase tally (mod 4).
    auto s = (uint8_t)cnt1.popcount();
    s ^= cnt2.popcount() << 1;
    s ^= (uint8_t)rhs.sign << 1;
    return s & 3;
}

bool PauliStringRef::commutes(const PauliStringRef &other) const noexcept {
    if (num_qubits > other.num_qubits) {
        return other.commutes(*this);
    }
    simd_word cnt1{};
    xs.for_each_word(zs, other.xs, other.zs, [&cnt1](simd_word &x1, simd_word &z1, simd_word &x2, simd_word &z2) {
        cnt1 ^= (x1 & z2) ^ (x2 & z1);
    });
    return (cnt1.popcount() & 1) == 0;
}

void PauliStringRef::after_inplace_broadcast(const Tableau &tableau, SpanRef<const size_t> indices, bool inverse) {
    if (tableau.num_qubits == 0 || indices.size() % tableau.num_qubits != 0) {
        throw std::invalid_argument("len(tableau) == 0 or len(indices) % len(tableau) != 0");
    }
    for (auto e : indices) {
        if (e >= num_qubits) {
            throw std::invalid_argument("Attempted to apply a tableau past the end of the pauli string.");
        }
    }
    if (inverse) {
        auto inverse_tableau = tableau.inverse();
        for (size_t k = indices.size(); k > 0;) {
            k -= tableau.num_qubits;
            inverse_tableau.apply_within(*this, {indices.ptr_start + k, indices.ptr_start + k + tableau.num_qubits});
        }
    } else {
        for (size_t k = 0; k < indices.size(); k += tableau.num_qubits) {
            tableau.apply_within(*this, {indices.ptr_start + k, indices.ptr_start + k + tableau.num_qubits});
        }
    }
}

void PauliStringRef::after_inplace(const Circuit &circuit) {
    for (const auto &op : circuit.operations) {
        if (op.gate_type == GateType::REPEAT) {
            const auto &body = op.repeat_block_body(circuit);
            auto reps = op.repeat_block_rep_count();
            for (size_t k = 0; k < reps; k++) {
                after_inplace(body);
            }
        } else {
            after_inplace(op, false);
        }
    }
}

void PauliStringRef::after_inplace(const CircuitInstruction &operation, bool inverse) {
    const auto &gate_data = GATE_DATA.items[operation.gate_type];
    if (gate_data.flags & GATE_IS_UNITARY) {
        // TODO: use hand-optimized methods instead of the generic tableau method.
        std::vector<size_t> indices;
        for (auto t : operation.targets) {
            if (!t.is_qubit_target()) {
                throw std::invalid_argument("Operation isn't unitary: " + operation.str());
            }
            if (t.qubit_value() >= num_qubits) {
                throw std::invalid_argument("Operation reaches past end of pauli string: " + operation.str());
            }
            indices.push_back(t.qubit_value());
        }
        after_inplace_broadcast(gate_data.tableau(), indices, inverse);
    } else if (gate_data.flags & GATE_HAS_NO_EFFECT_ON_QUBITS) {
        // Gate can be ignored.
    } else if (gate_data.flags & GATE_IS_RESET) {
        // Only fail if the pauli string actually touches the reset.
        for (const auto &t : operation.targets) {
            assert(t.is_qubit_target());
            auto q = t.qubit_value();
            if (q < num_qubits && (xs[q] || zs[q])) {
                std::stringstream ss;
                ss << "The pauli observable '" << *this << "' doesn't have a well specified value after '" << operation
                   << "' because the reset discards information.";
                throw std::invalid_argument(ss.str());
            }
        }
    } else if (operation.gate_type == GateType::MPP) {
        size_t start = 0;
        const auto &targets = operation.targets;
        while (start < targets.size()) {
            size_t end = start + 1;
            bool anticommutes = false;
            while (true) {
                auto t = targets[end - 1];
                auto q = t.qubit_value();
                if (q < num_qubits) {
                    anticommutes ^= zs[q] && (t.data & TARGET_PAULI_X_BIT);
                    anticommutes ^= xs[q] && (t.data & TARGET_PAULI_Z_BIT);
                }
                if (end >= targets.size() || !targets[end].is_combiner()) {
                    break;
                }
                end += 2;
            }
            if (anticommutes) {
                std::stringstream ss;
                ss << "The pauli observable '" << *this << "' doesn't have a well specified value after '" << operation
                   << "' because it anticommutes with the measurement.";
                throw std::invalid_argument(ss.str());
            }
            start = end;
        }
    } else if (gate_data.flags & GATE_PRODUCES_NOISY_RESULTS) {
        bool x_dep, z_dep;
        if (operation.gate_type == GateType::M) {
            x_dep = true;
            z_dep = false;
        } else if (operation.gate_type == GateType::MX) {
            x_dep = false;
            z_dep = true;
        } else if (operation.gate_type == GateType::MY) {
            x_dep = true;
            z_dep = true;
        } else {
            throw std::invalid_argument("Unrecognized measurement type: " + operation.str());
        }
        for (const auto &t : operation.targets) {
            assert(t.is_qubit_target());
            auto q = t.qubit_value();
            if (q < num_qubits && ((xs[q] & x_dep) ^ (zs[q] & z_dep))) {
                std::stringstream ss;
                ss << "The pauli observable '" << *this << "' doesn't have a well specified value after '" << operation
                   << "' because it anticommutes with the measurement.";
                throw std::invalid_argument(ss.str());
            }
        }
    } else {
        std::stringstream ss;
        ss << "The pauli string '" << *this << "' doesn't have a well defined deterministic value after '" << operation
           << "'.";
        throw std::invalid_argument(ss.str());
    }
}

PauliString PauliStringRef::after(const Circuit &circuit) const {
    PauliString result = *this;
    result.ref().after_inplace(circuit);
    return result;
}

PauliString PauliStringRef::after(const Tableau &tableau, SpanRef<const size_t> indices) const {
    PauliString result = *this;
    result.ref().after_inplace_broadcast(tableau, indices, false);
    return result;
}
PauliString PauliStringRef::after(const CircuitInstruction &operation) const {
    PauliString result = *this;
    result.ref().after_inplace(operation, false);
    return result;
}

PauliString PauliStringRef::before(const Circuit &circuit) const {
    // TODO: use hand-optimized methods instead of the generic circuit inverse method.
    return after(circuit.inverse(true));
}
PauliString PauliStringRef::before(const CircuitInstruction &operation) const {
    PauliString result = *this;
    result.ref().after_inplace(operation, true);
    return result;
}
PauliString PauliStringRef::before(const Tableau &tableau, SpanRef<const size_t> indices) const {
    PauliString result = *this;
    result.ref().after_inplace_broadcast(tableau, indices, true);
    return result;
}

void PauliStringRef::gather_into(PauliStringRef out, SpanRef<const size_t> in_indices) const {
    assert(in_indices.size() == out.num_qubits);
    for (size_t k_out = 0; k_out < out.num_qubits; k_out++) {
        size_t k_in = in_indices[k_out];
        out.xs[k_out] = xs[k_in];
        out.zs[k_out] = zs[k_in];
    }
}

void PauliStringRef::scatter_into(PauliStringRef out, SpanRef<const size_t> out_indices) const {
    assert(num_qubits == out_indices.size());
    for (size_t k_in = 0; k_in < num_qubits; k_in++) {
        size_t k_out = out_indices[k_in];
        out.xs[k_out] = xs[k_in];
        out.zs[k_out] = zs[k_in];
    }
    out.sign ^= sign;
}
