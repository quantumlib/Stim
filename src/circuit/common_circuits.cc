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

#include "common_circuits.h"

#include <algorithm>
#include <complex>
#include <functional>
#include <sstream>
#include <unordered_set>
#include <vector>

template <typename T1, typename T2>
std::string op_line(const std::string &name, const T1 &targets, const T2 &qubit_func, bool sort = false) {
    if (targets.empty()) {
        return "";
    }
    std::stringstream out;
    std::vector<size_t> sorted_targets;
    for (auto q : targets) {
        sorted_targets.push_back(qubit_func(q));
    }
    if (sort) {
        std::sort(sorted_targets.begin(), sorted_targets.end());
    }
    out << name;
    for (auto q : sorted_targets) {
        out << " " << q;
    }
    out << "\n";
    return out.str();
}

template <typename T>
std::string op_line(const std::string &name, const T &targets, bool sort = false) {
    return op_line(
        name, targets,
        [](size_t q) {
            return q;
        },
        sort);
}

void indented_into(const std::string &data, std::ostream &out) {
    size_t start = 0;
    for (size_t k = 0; k <= data.size(); k++) {
        if (data[k] == '\n') {
            out << "  " << data.substr(start, k - start + 1);
            start = k + 1;
        } else if (data[k] == '\0') {
            if (k > start) {
                out << "  " << data.substr(start, k - start);
            }
            break;
        }
    }
}

std::string unrotated_surface_code_program_text(size_t distance, size_t rounds, double noise_level) {
    if ((distance & 1) == 0) {
        throw std::out_of_range("Unrotated surface code can't have even code distances.");
    }

    std::stringstream result{};
    size_t diam = distance * 2 - 1;
    auto qubit = [&](std::complex<float> c) {
        return (size_t)(c.real() * diam + c.imag());
    };
    auto in_range = [=](std::complex<float> c) {
        return c.real() >= 0 && c.real() < (float)diam && c.imag() >= 0 && c.imag() < (float)diam;
    };
    std::vector<std::complex<float>> c_data{};
    std::vector<std::complex<float>> c_xs{};
    std::vector<std::complex<float>> c_zs{};
    for (size_t x = 0; x < diam; x++) {
        for (size_t y = 0; y < diam; y++) {
            if (x % 2 == 1 && y % 2 == 0) {
                c_xs.emplace_back((float)x, (float)y);
            } else if (x % 2 == 0 && y % 2 == 1) {
                c_zs.emplace_back((float)x, (float)y);
            } else {
                c_data.emplace_back((float)x, (float)y);
            }
        }
    }
    std::vector<size_t> q_xs;
    std::vector<size_t> q_zs;
    std::vector<size_t> q_zxs;
    std::vector<size_t> q_datas;
    for (const auto &e : c_data) {
        q_datas.push_back(qubit(e));
    }
    for (const auto &z : c_zs) {
        q_zxs.push_back(qubit(z));
        q_zs.push_back(qubit(z));
    }
    for (const auto &x : c_xs) {
        q_zxs.push_back(qubit(x));
        q_xs.push_back(qubit(x));
    }
    std::vector<std::complex<float>> dirs{
        {1, 0},
        {0, 1},
        {0, -1},
        {-1, 0},
    };

    auto pair_targets = [&](std::complex<float> d) {
        std::vector<std::complex<float>> result;
        for (const auto &z : c_zs) {
            auto p = z + d;
            if (in_range(p)) {
                result.push_back(p);
                result.push_back(z);
            }
        }
        for (const auto &x : c_xs) {
            auto p = x + d;
            if (in_range(p)) {
                result.push_back(x);
                result.push_back(p);
            }
        }
        return result;
    };
    auto moment_noise = [&](std::vector<std::complex<float>> doubles) {
        if (noise_level == 0) {
            return std::string("");
        }
        std::unordered_set<size_t> singles{};
        for (auto q : q_datas) {
            singles.insert(q);
        }
        for (auto q : q_zxs) {
            singles.insert(q);
        }
        for (auto c : doubles) {
            singles.erase(qubit(c));
        }
        std::stringstream n;
        n << noise_level;
        return op_line("DEPOLARIZE2(" + n.str() + ")", doubles, qubit) +
               op_line("DEPOLARIZE1(" + n.str() + ")", singles, true);
    };

    std::stringstream bulk;
    {
        bulk << moment_noise({});
        bulk << op_line("H", q_xs, true);
        for (const auto &d : dirs) {
            bulk << moment_noise(pair_targets(d));
            bulk << op_line("CNOT", pair_targets(d), qubit);
        }
        bulk << moment_noise({});
        bulk << op_line("H", q_xs, true);
        bulk << moment_noise({});
        bulk << op_line("MR", q_zxs, true);
    }
    std::stringstream x_detectors;
    std::stringstream z_detectors;
    for (auto q : q_xs) {
        x_detectors << "DETECTOR " << q << "@-1 " << q << "@-2\n";
    }
    for (auto q : q_zs) {
        z_detectors << "DETECTOR " << q << "@-1 " << q << "@-2\n";
    }

    result << bulk.str();
    for (auto q : q_zs) {
        result << "DETECTOR " << q << "@-1\n";
    }
    result << "REPEAT " << (rounds - 1) << " {\n";
    indented_into(bulk.str(), result);
    indented_into(x_detectors.str(), result);
    indented_into(z_detectors.str(), result);
    result << "}\n";

    // Final data measurements.
    result << "M";
    for (auto q : q_datas) {
        result << " " << q;
    }
    result << "\n";
    for (auto z : c_zs) {
        result << "DETECTOR " << qubit(z) << "@-1";
        for (const auto &d : dirs) {
            auto p = z + d;
            if (in_range(p)) {
                result << " " << qubit(p) << "@-1";
            }
        }
        result << "\n";
    }

    result << "OBSERVABLE_INCLUDE(0)";
    for (size_t k = 0; k <= distance; k++) {
        result << " " << qubit(k) << "@-1";
    }
    result << "\n";

    return result.str();
}
