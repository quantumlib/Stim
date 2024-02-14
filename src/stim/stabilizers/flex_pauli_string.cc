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

#include "stim/stabilizers/flex_pauli_string.h"

using namespace stim;

FlexPauliString::FlexPauliString(size_t num_qubits) : value(num_qubits), imag(false) {
}

FlexPauliString::FlexPauliString(const PauliStringRef<MAX_BITWORD_WIDTH> val, bool imag) : value(val), imag(imag) {
}

FlexPauliString::FlexPauliString(PauliString<MAX_BITWORD_WIDTH> &&val, bool imag) : value(std::move(val)), imag(imag) {
}

FlexPauliString FlexPauliString::operator+(const FlexPauliString &rhs) const {
    FlexPauliString copy = *this;
    copy += rhs;
    return copy;
}

FlexPauliString &FlexPauliString::operator+=(const FlexPauliString &rhs) {
    if (&rhs == this) {
        *this *= 2;
        return *this;
    }

    size_t n = value.num_qubits;
    value.ensure_num_qubits(value.num_qubits + rhs.value.num_qubits, 1.1);
    for (size_t k = 0; k < rhs.value.num_qubits; k++) {
        value.xs[k + n] = rhs.value.xs[k];
        value.zs[k + n] = rhs.value.zs[k];
    }
    *this *= rhs.get_phase();
    return *this;
}

FlexPauliString FlexPauliString::operator*(std::complex<float> scale) const {
    FlexPauliString copy = *this;
    copy *= scale;
    return copy;
}

FlexPauliString FlexPauliString::operator/(const std::complex<float> &scale) const {
    FlexPauliString copy = *this;
    copy /= scale;
    return copy;
}

FlexPauliString FlexPauliString::operator*(size_t power) const {
    FlexPauliString copy = *this;
    copy *= power;
    return copy;
}

FlexPauliString &FlexPauliString::operator*=(size_t power) {
    switch (power & 3) {
        case 0:
            imag = false;
            value.sign = false;
            break;
        case 1:
            break;
        case 2:
            value.sign = imag;
            imag = false;
            break;
        case 3:
            value.sign ^= imag;
            break;
    }

    value = PauliString<MAX_BITWORD_WIDTH>::from_func(value.sign, value.num_qubits * power, [&](size_t k) {
        return "_XZY"[value.xs[k % value.num_qubits] + 2 * value.zs[k % value.num_qubits]];
    });
    return *this;
}

FlexPauliString &FlexPauliString::operator/=(const std::complex<float> &rhs) {
    if (rhs == std::complex<float>{+1, 0}) {
        return *this;
    } else if (rhs == std::complex<float>{-1, 0}) {
        return *this *= std::complex<float>{-1, 0};
    } else if (rhs == std::complex<float>{0, 1}) {
        return *this *= std::complex<float>{0, -1};
    } else if (rhs == std::complex<float>{0, -1}) {
        return *this *= std::complex<float>{0, +1};
    }
    throw std::invalid_argument("divisor not in (1, -1, 1j, -1j)");
}

FlexPauliString &FlexPauliString::operator*=(std::complex<float> scale) {
    if (scale == std::complex<float>(-1)) {
        value.sign ^= true;
    } else if (scale == std::complex<float>(0, 1)) {
        value.sign ^= imag;
        imag ^= true;
    } else if (scale == std::complex<float>(0, -1)) {
        imag ^= true;
        value.sign ^= imag;
    } else if (scale != std::complex<float>(1)) {
        throw std::invalid_argument("phase factor not in [1, -1, 1, 1j]");
    }
    return *this;
}

bool FlexPauliString::operator==(const FlexPauliString &other) const {
    return value == other.value && imag == other.imag;
}

bool FlexPauliString::operator!=(const FlexPauliString &other) const {
    return !(*this == other);
}

std::complex<float> FlexPauliString::get_phase() const {
    std::complex<float> result{value.sign ? -1.0f : +1.0f};
    if (imag) {
        result *= std::complex<float>{0, 1};
    }
    return result;
}

FlexPauliString FlexPauliString::operator*(const FlexPauliString &rhs) const {
    FlexPauliString copy = *this;
    copy *= rhs;
    return copy;
}

FlexPauliString &FlexPauliString::operator*=(const FlexPauliString &rhs) {
    value.ensure_num_qubits(rhs.value.num_qubits, 1.1);
    if (rhs.value.num_qubits < value.num_qubits) {
        FlexPauliString copy = rhs;
        copy.value.ensure_num_qubits(value.num_qubits, 1.0);
        *this *= copy;
        return *this;
    }

    uint8_t log_i = value.ref().inplace_right_mul_returning_log_i_scalar(rhs.value.ref());
    if (log_i & 2) {
        value.sign ^= true;
    }
    if (log_i & 1) {
        *this *= std::complex<float>{0, 1};
    }
    if (rhs.imag) {
        *this *= std::complex<float>{0, 1};
    }
    return *this;
}

std::ostream &stim::operator<<(std::ostream &out, const FlexPauliString &v) {
    out << "+-"[v.value.sign];
    if (v.imag) {
        out << 'i';
    }
    for (size_t k = 0; k < v.value.num_qubits; k++) {
        out << "_XZY"[v.value.xs[k] + 2 * v.value.zs[k]];
    }
    return out;
}

std::string FlexPauliString::str() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
}

static size_t parse_size_of_pauli_string_shorthand_if_sparse(std::string_view text) {
    size_t cur_index = 0;
    bool has_cur_index = false;
    size_t max_size = 0;
    for (char c : text) {
        switch (c) {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                has_cur_index = true;
                cur_index *= 10;
                cur_index += c - '0';
                break;
            default:
                // do nothing.
                if (has_cur_index) {
                    max_size = std::max(max_size, cur_index + 1);
                    cur_index = 0;
                    has_cur_index = false;
                }
                break;
        }
    }
    return max_size;
}

template <size_t W>
static size_t parse_sparse_pauli_string(std::string_view text, PauliString<W> *out) {
    size_t cur_index = 0;
    bool has_cur_index = false;
    uint8_t cur_pauli = 0;

    auto flush = [&]() {
        if (cur_pauli == '\0' || !has_cur_index) {
            throw std::invalid_argument("");
        }
        out->ref().->ad
        out->xs[cur_index] = cur_pauli == 'X' || cur_pauli == 'Y';
        out->zs[cur_index] = cur_pauli == 'Z' || cur_pauli == 'Y';
    };

    for (char c : text) {
        switch (c) {
            case '*':
                flush();
                break;
            case 'x':
            case 'X':
            case 'y':
            case 'Y':
            case 'z':
            case 'Z':
                if (cur_pauli != '\0') {
                    throw std::invalid_argument("");
                }
                cur_pauli = toupper(c);
                break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                if (cur_pauli == '\0') {
                    throw std::invalid_argument("");
                }
                has_cur_index = true;
                cur_index *= 10;
                cur_index += c - '0';
                break;
            default:
                throw std::invalid_argument("");
        }
    }
    return max_size;
}

FlexPauliString FlexPauliString::from_text(std::string_view text) {
    size_t n = text.size();
    if (n == 0) {
        return FlexPauliString(0);
    }

    size_t k = 0;
    bool negated = false;
    bool imaginary = false;
    if (k < n) {
        if (text[k] == '-') {
            negated = true;
            k++;
        } else if (text[k] == '+') {
            k++;
        }
    }
    if (k < n && text[k] == 'i') {
        imaginary = true;
        k++;
    }

    size_t sparse_size = compute_size_of_pauli_string_shorthand_if_sparse(text);
    if (sparse_size == 0) {

    }
    bool is_sparse = false;
    size_t v = 0;
    size_t max_v = 0;
    for (size_t k2 = 0; k2 < n; k2++) {
        switch (text[k]) {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                is_sparse = true;
                v *= 10;
                v += text[k] - '0';
                break;
            default:
                // do nothing.
                max_v = std::max(max_v, v);
                v = 0;
                break;
        }
    }

    if (is_sparse) {

    }
    while (k < n) {
        switch (text[k]) {
            case 'x':
            case 'X':

            case 'z':
            case 'Z':
            case 'y':
            case 'Y':
        }
        k++;
    }

    std::complex<float> factor{1, 0};
    int offset = 0;
    if (text.starts_with('i')) {
        factor = {0, 1};
        offset = 1;
    } else if (text.starts_with("-i")) {
        factor = {0, -1};
        offset = 2;
    } else if (text.starts_with("+i")) {
        factor = {0, 1};
        offset = 2;
    }


    FlexPauliString value{PauliString<MAX_BITWORD_WIDTH>::from_str(text.substr(offset)), false};
    value *= factor;
    return value;
}
