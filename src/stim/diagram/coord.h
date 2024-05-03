#ifndef _STIM_DIAGRAM_COORD_H
#define _STIM_DIAGRAM_COORD_H

#include <array>
#include <cmath>
#include <iostream>

#include "stim/mem/span_ref.h"

namespace stim_draw_internal {

/// Coordinate data. Used for individual vertex positions and also UV texture coordinates.
template <size_t DIM>
struct Coord {
    std::array<float, DIM> xyz;

    Coord<DIM> &operator+=(const Coord<DIM> &other) {
        for (size_t k = 0; k < DIM; k++) {
            xyz[k] += other.xyz[k];
        }
        return *this;
    }
    Coord<DIM> &operator-=(const Coord<DIM> &other) {
        for (size_t k = 0; k < DIM; k++) {
            xyz[k] -= other.xyz[k];
        }
        return *this;
    }
    Coord<DIM> &operator/=(float f) {
        for (size_t k = 0; k < DIM; k++) {
            xyz[k] /= f;
        }
        return *this;
    }
    Coord<DIM> &operator*=(float f) {
        for (size_t k = 0; k < DIM; k++) {
            xyz[k] *= f;
        }
        return *this;
    }
    Coord<DIM> operator+(const Coord<DIM> &other) const {
        Coord<DIM> result = *this;
        result += other;
        return result;
    }
    Coord<DIM> operator-(const Coord<DIM> &other) const {
        Coord<DIM> result = *this;
        result -= other;
        return result;
    }
    Coord<DIM> operator/(float other) const {
        Coord<DIM> result = *this;
        result /= other;
        return result;
    }
    Coord<DIM> operator*(float other) const {
        Coord<DIM> result = *this;
        result *= other;
        return result;
    }
    float dot(const Coord<DIM> &other) const {
        float t = 0;
        for (size_t k = 0; k < DIM; k++) {
            t += xyz[k] * other.xyz[k];
        }
        return t;
    }
    float norm2() const {
        return dot(*this);
    }
    float norm() const {
        return sqrtf(norm2());
    }

    bool operator<(Coord<DIM> other) const {
        for (size_t k = 0; k < DIM; k++) {
            if (xyz[k] != other.xyz[k]) {
                return xyz[k] < other.xyz[k];
            }
        }
        return false;
    }

    bool operator==(Coord<DIM> other) const {
        return xyz == other.xyz;
    }

    static std::pair<Coord<DIM>, Coord<DIM>> min_max(stim::SpanRef<const Coord<DIM>> coords) {
        if (coords.empty()) {
            return {{}, {}};
        }
        Coord<DIM> v_min;
        Coord<DIM> v_max;
        for (size_t k = 0; k < DIM; k++) {
            v_min.xyz[k] = INFINITY;
            v_max.xyz[k] = -INFINITY;
        }
        for (const auto &v : coords) {
            for (size_t k = 0; k < DIM; k++) {
                v_min.xyz[k] = std::min(v_min.xyz[k], v.xyz[k]);
                v_max.xyz[k] = std::max(v_max.xyz[k], v.xyz[k]);
            }
        }
        return {v_min, v_max};
    }
};

template <size_t DIM>
std::ostream &operator<<(std::ostream &out, const Coord<DIM> &coord) {
    for (size_t k = 0; k < DIM; k++) {
        if (k) {
            out << ',';
        }
        out << coord.xyz[k];
    }
    return out;
}

}  // namespace stim_draw_internal

#endif
