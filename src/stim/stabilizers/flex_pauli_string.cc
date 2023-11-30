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

FlexPauliString FlexPauliString::from_text(std::string_view text) {
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
