#include "surface_code.h"

#include <algorithm>
#include <map>
#include <set>

using namespace stim_internal;

struct coord {
    float x;
    float y;
    coord operator+(coord other) const {
        return {x + other.x, y + other.y};
    }
    coord operator-(coord other) const {
        return {x - other.x, y - other.y};
    }
    bool operator==(coord other) const {
        return x == other.x && y == other.y;
    }
    bool operator<(coord other) const {
        if (x != other.x) {
            return x < other.x;
        }
        return y < other.y;
    }
};

GeneratedCircuit _finish_surface_code_circuit(
        std::function<uint32_t(coord)> coord_to_index,
        const std::set<coord> &data_coords,
        const std::set<coord> &x_measure_coords,
        const std::set<coord> &z_measure_coords,
        const CircuitGenParameters &params,
        const std::vector<coord> &x_order,
        const std::vector<coord> &z_order,
        const std::vector<coord> x_observable,
        const std::vector<coord> z_observable) {
    if (params.basis != "X" && params.basis != "Z") {
        throw std::out_of_range("Surface code supports basis=X and basis=Z, not basis='" + params.basis + "'.");
    }
    const auto &chosen_basis_observable = params.basis == "X" ? x_observable : z_observable;
    const auto &chosen_basis_measure_coords = params.basis == "X" ? x_measure_coords : z_measure_coords;

    std::map<coord, uint32_t> p2q;
    std::vector<uint32_t> all_qubits;
    std::vector<uint32_t> data_qubits;
    std::vector<uint32_t> measurement_qubits;
    std::vector<uint32_t> x_measurement_qubits;
    std::array<std::vector<uint32_t>, 4> cnot_targets;
    for (auto q : data_coords) {
        size_t count = 0;
        for (auto delta : x_order) {
            count += x_measure_coords.find(q + delta) != x_measure_coords.end();
            count += z_measure_coords.find(q + delta) != z_measure_coords.end();
        }
        if (count > 1) {
            p2q[q] = coord_to_index(q);
            data_qubits.push_back(p2q[q]);
        }
    }
    for (auto q : x_measure_coords) {
        p2q[q] = coord_to_index(q);
        measurement_qubits.push_back(p2q[q]);
        x_measurement_qubits.push_back(p2q[q]);
    }
    for (auto q : z_measure_coords) {
        p2q[q] = coord_to_index(q);
        measurement_qubits.push_back(p2q[q]);
    }
    std::map<uint32_t, coord> q2p;
    for (const auto &kv : p2q) {
        q2p[kv.second] = kv.first;
    }
    all_qubits.insert(all_qubits.end(), data_qubits.begin(), data_qubits.end());
    all_qubits.insert(all_qubits.end(), measurement_qubits.begin(), measurement_qubits.end());

    std::sort(all_qubits.begin(), all_qubits.end());
    std::sort(data_qubits.begin(), data_qubits.end());
    std::sort(measurement_qubits.begin(), measurement_qubits.end());
    std::sort(x_measurement_qubits.begin(), x_measurement_qubits.end());
    std::map<coord, uint32_t> data_coord_to_order;
    std::map<coord, uint32_t> measure_coord_to_order;
    for (auto q : data_qubits) {
        data_coord_to_order[q2p[q]] = data_coord_to_order.size();
    }
    for (auto q : measurement_qubits) {
        measure_coord_to_order[q2p[q]] = measure_coord_to_order.size();
    }

    for (size_t k = 0; k < 4; k++) {
        for (auto measure : x_measure_coords) {
            auto data = measure + x_order[k];
            if (p2q.find(data) != p2q.end()) {
                cnot_targets[k].push_back(p2q[measure]);
                cnot_targets[k].push_back(p2q[data]);
            }
        }
        for (auto measure : z_measure_coords) {
            auto data = measure + z_order[k];
            if (p2q.find(data) != p2q.end()) {
                cnot_targets[k].push_back(p2q[data]);
                cnot_targets[k].push_back(p2q[measure]);
            }
        }
    }

    Circuit body;
    params.append_round_transition(body, data_qubits);
    params.append_unitary_1(body, "H", x_measurement_qubits);
    body.append_op("TICK", {});
    for (const auto &targets : cnot_targets) {
        params.append_unitary_2(body, "CNOT", targets);
        body.append_op("TICK", {});
    }
    params.append_unitary_1(body, "H", x_measurement_qubits);
    body.append_op("TICK", {});
    params.append_measure_reset(body, measurement_qubits);
    uint32_t m = measurement_qubits.size();

    Circuit head;
    params.append_reset(head, all_qubits);
    if (params.basis == "X") {
        params.append_unitary_1(head, "H", data_qubits);
    }
    head += body;
    for (auto measure : chosen_basis_measure_coords) {
        head.append_op("DETECTOR", {(uint32_t)(measurement_qubits.size() - measure_coord_to_order[measure]) | TARGET_RECORD_BIT});
    }
    for (uint32_t k = 0; k < m; k++) {
        body.append_op("DETECTOR", {(k + 1) | TARGET_RECORD_BIT, (k + 1 + m) | TARGET_RECORD_BIT});
    }

    Circuit tail;
    if (params.basis == "X") {
        params.append_unitary_1(tail, "H", data_qubits);
    }
    params.append_measure(tail, data_qubits);
    for (auto measure : chosen_basis_measure_coords) {
        std::vector<uint32_t> detectors;
        for (auto delta : z_order) {
            auto data = measure + delta;
            if (p2q.find(data) != p2q.end()) {
                detectors.push_back((data_qubits.size() - data_coord_to_order[data]) | TARGET_RECORD_BIT);
            }
        }
        detectors.push_back((data_qubits.size() + measurement_qubits.size() - measure_coord_to_order[measure]) | TARGET_RECORD_BIT);
        std::sort(detectors.begin(), detectors.end());
        tail.append_op("DETECTOR", detectors);
    }

    std::vector<uint32_t> obs_inc;
    for (auto q : chosen_basis_observable) {
        obs_inc.push_back((data_qubits.size() - data_coord_to_order[q]) | TARGET_RECORD_BIT);
    }
    std::sort(obs_inc.begin(), obs_inc.end());
    tail.append_op("OBSERVABLE_INCLUDE", obs_inc, 0);


    Circuit result = head + body * (params.rounds - 1) + tail;
    std::map<std::pair<uint32_t, uint32_t>, std::pair<std::string, uint32_t>> layout;
    float scale = x_order[0].x == 0.5 ? 2 : 1;
    for (auto q : data_coords) {
        layout[{(uint32_t)(q.x * scale), (uint32_t)(q.y * scale)}] = {"d", p2q[q]};
    }
    for (auto q : x_measure_coords) {
        layout[{(uint32_t)(q.x * scale), (uint32_t)(q.y * scale)}] = {"X", p2q[q]};
    }
    for (auto q : z_measure_coords) {
        layout[{(uint32_t)(q.x * scale), (uint32_t)(q.y * scale)}] = {"Z", p2q[q]};
    }
    for (auto q : chosen_basis_observable) {
        layout[{(uint32_t)(q.x * scale), (uint32_t)(q.y * scale)}].first = "L";
    }
    return {result, layout};
}

GeneratedCircuit stim_internal::generate_rotated_surface_code_circuit(const CircuitGenParameters &params) {
    uint32_t d = params.distance;
    assert(params.rounds > 0);

    std::set<coord> data_coords;
    std::set<coord> x_measure_coords;
    std::set<coord> z_measure_coords;
    std::vector<coord> x_observable;
    std::vector<coord> z_observable;
    for (float x = 0.5; x <= d; x++) {
        for (float y = 0.5; y <= d; y++) {
            coord q{x, y};
            data_coords.insert(q);
            if (y == 0.5) {
                z_observable.push_back(q);
            }
            if (x == 0.5) {
                x_observable.push_back(q);
            }
        }
    }
    for (size_t x = 0; x <= d; x++) {
        for (size_t y = 0; y <= d; y++) {
            coord q{(float)x, (float)y};
            bool on_boundary_1 = x == 0 || x == d;
            bool on_boundary_2 = y == 0 || y == d;
            bool parity = x % 2 != y % 2;
            if (on_boundary_1 && parity) {
                continue;
            }
            if (on_boundary_2 && !parity) {
                continue;
            }
            if (parity) {
                x_measure_coords.insert(q);
            } else {
                z_measure_coords.insert(q);
            }
        }
    }

    std::vector<coord> x_order{
        {0.5, 0.5},
        {0.5, -0.5},
        {-0.5, 0.5},
        {-0.5, -0.5},
    };
    std::vector<coord> z_order{
        {0.5, 0.5},
        {-0.5, 0.5},
        {0.5, -0.5},
        {-0.5, -0.5},
    };

    return _finish_surface_code_circuit(
        [&](coord q) {
            q = q - coord{0, fmodf(q.x, 1)};
            return (uint32_t)(q.x * 2 + q.y * (2 * d + 1));
        },
        data_coords,
        x_measure_coords,
        z_measure_coords,
        params,
        x_order,
        z_order,
        x_observable,
        z_observable);
}

GeneratedCircuit stim_internal::generate_unrotated_surface_code_circuit(const CircuitGenParameters &params) {
    uint32_t d = params.distance;
    assert(params.rounds > 0);

    std::set<coord> data_coords;
    std::set<coord> x_measure_coords;
    std::set<coord> z_measure_coords;
    std::vector<coord> x_observable;
    std::vector<coord> z_observable;
    for (size_t x = 0; x < 2 * d - 1; x++) {
        for (size_t y = 0; y < 2 * d - 1; y++) {
            coord q{(float)x, (float)y};
            bool parity = x % 2 != y % 2;
            if (parity) {
                if (x % 2 == 0) {
                    z_measure_coords.insert(q);
                } else {
                    x_measure_coords.insert(q);
                }
            } else {
                data_coords.insert(q);
                if (x == 0) {
                    x_observable.push_back(q);
                }
                if (y == 0) {
                    z_observable.push_back(q);
                }
            }
        }
    }

    std::vector<coord> order{
        {1, 0},
        {0, 1},
        {0, -1},
        {-1, 0},
    };

    return _finish_surface_code_circuit(
        [&](coord q) {
            return (uint32_t)(q.x + q.y * (2 * d - 1));
        },
        data_coords,
        x_measure_coords,
        z_measure_coords,
        params,
        order,
        order,
        x_observable,
        z_observable);
}
