#include "stim/gen/gen_color_code.h"

#include <algorithm>
#include <array>
#include <map>
#include <set>
#include <vector>

using namespace stim;

struct color_coord {
    float x;
    float y;
    color_coord operator+(color_coord other) const {
        return {x + other.x, y + other.y};
    }
    color_coord operator-(color_coord other) const {
        return {x - other.x, y - other.y};
    }
    bool operator==(color_coord other) const {
        return x == other.x && y == other.y;
    }
    bool operator<(color_coord other) const {
        if (x != other.x) {
            return x < other.x;
        }
        return y < other.y;
    }
};

GeneratedCircuit stim::generate_color_code_circuit(const CircuitGenParameters &params) {
    if (params.task != "memory_xyz") {
        throw std::invalid_argument(
            "Unrecognized task '" + params.task +
            "'. Known color_code tasks:\n"
            "    'memory_xyz': Initialize logical |0>, protect by cycling X then Y then Z stabilizer measurements, "
            "measure logical Z.\n");
    }
    if (params.rounds < 2) {
        throw std::invalid_argument("Need rounds >= 2.");
    }
    if (params.distance < 2 || params.distance % 2 == 0) {
        throw std::invalid_argument("Need an odd distance >= 3.");
    }

    uint32_t d = params.distance;
    uint32_t w = d + (d - 1) / 2;

    // Lay out and index qubits.
    std::set<color_coord> data_coords;
    std::set<color_coord> measure_coords;
    std::map<color_coord, uint32_t> p2q;
    std::vector<uint32_t> data_qubits;
    std::vector<uint32_t> measurement_qubits;
    for (size_t y = 0; y < w; y++) {
        for (size_t x = 0; x < w - y; x++) {
            color_coord q{x + y / 2.0f, (float)y};
            auto i = (uint32_t)p2q.size();
            p2q[q] = i;
            if ((x + 2 * y) % 3 == 2) {
                measure_coords.insert(q);
                measurement_qubits.push_back(p2q[q]);
            } else {
                data_coords.insert(q);
                data_qubits.push_back(p2q[q]);
            }
        }
    }

    // Keep targets sorted to make the output look a bit cleaner.
    std::vector<uint32_t> all_qubits;
    all_qubits.insert(all_qubits.end(), data_qubits.begin(), data_qubits.end());
    all_qubits.insert(all_qubits.end(), measurement_qubits.begin(), measurement_qubits.end());
    std::sort(all_qubits.begin(), all_qubits.end());
    std::sort(data_qubits.begin(), data_qubits.end());
    std::sort(measurement_qubits.begin(), measurement_qubits.end());

    // Reverse indices.
    std::map<color_coord, uint32_t> data_coord_to_order;
    std::map<color_coord, uint32_t> measure_coord_to_order;
    std::map<uint32_t, color_coord> q2p;
    for (const auto &kv : p2q) {
        q2p[kv.second] = kv.first;
    }
    for (auto q : data_qubits) {
        auto i = (uint32_t)data_coord_to_order.size();
        data_coord_to_order[q2p[q]] = i;
    }
    for (auto q : measurement_qubits) {
        auto i = (uint32_t)measure_coord_to_order.size();
        measure_coord_to_order[q2p[q]] = i;
    }

    // Precompute targets for each tick of CNOT gates.
    std::array<std::vector<uint32_t>, 6> cnot_targets;
    std::vector<color_coord> deltas{
        {1, 0},
        {0.5, 1},
        {0.5, -1},
        {-1, 0},
        {-0.5, 1},
        {-0.5, -1},
    };
    for (size_t k = 0; k < deltas.size(); k++) {
        for (auto measure : measure_coords) {
            auto data = measure + deltas[k];
            if (p2q.find(data) != p2q.end()) {
                cnot_targets[k].push_back(p2q[data]);
                cnot_targets[k].push_back(p2q[measure]);
            }
        }
    }

    // Build the repeated actions that make up the color code cycle.
    Circuit cycle_actions;
    params.append_begin_round_tick(cycle_actions, data_qubits);
    params.append_unitary_1(cycle_actions, "C_XYZ", data_qubits);
    for (const auto &targets : cnot_targets) {
        cycle_actions.append_op("TICK", {});
        params.append_unitary_2(cycle_actions, "CNOT", targets);
    }
    cycle_actions.append_op("TICK", {});
    params.append_measure_reset(cycle_actions, measurement_qubits);

    // Build the start of the circuit, getting a state that's ready to cycle.
    // In particular, the first cycle has different detectors and so has to be handled special.
    auto m = (uint32_t)measurement_qubits.size();
    Circuit head;
    for (auto q : all_qubits) {
        color_coord c = q2p[q];
        head.append_op("QUBIT_COORDS", {q}, {c.x, c.y});
    }
    params.append_reset(head, all_qubits);
    head += cycle_actions * 2;
    for (uint32_t k = m; k-- > 0;) {
        color_coord c = q2p[measurement_qubits[m - k - 1]];
        head.append_op("DETECTOR", {(k + 1) | TARGET_RECORD_BIT, (k + 1 + m) | TARGET_RECORD_BIT}, {c.x, c.y, 0});
    }

    // Build the repeated body of the circuit, including the detectors comparing to previous cycles.
    Circuit body = cycle_actions;
    body.append_op("SHIFT_COORDS", {}, {0, 0, 1});
    for (uint32_t k = m; k-- > 0;) {
        color_coord c = q2p[measurement_qubits[m - k - 1]];
        body.append_op(
            "DETECTOR",
            {(k + 1) | TARGET_RECORD_BIT, (k + 1 + m) | TARGET_RECORD_BIT, (k + 1 + 2 * m) | TARGET_RECORD_BIT},
            {c.x, c.y, 0});
    }

    // Build the end of the circuit, getting out of the cycle state and terminating.
    // In particular, the data measurements create detectors that have to be handled special.
    // Also, the tail is responsible for identifying the logical observable.
    Circuit tail;
    params.append_measure(tail, data_qubits, "ZXY"[params.rounds % 3]);
    for (auto m_q : measurement_qubits) {
        auto measure = q2p[m_q];
        std::vector<uint32_t> detectors;
        for (auto delta : deltas) {
            auto data = measure + delta;
            if (p2q.find(data) != p2q.end()) {
                detectors.push_back((uint32_t)(data_qubits.size() - data_coord_to_order[data]) | TARGET_RECORD_BIT);
            }
        }
        uint32_t p =
            (data_qubits.size() + measurement_qubits.size() - measure_coord_to_order[measure]) | TARGET_RECORD_BIT;
        // Depending on if the last two rounds were XY, YZ, or ZX, different combinations are equal the data
        // measurements.
        if (params.rounds % 3 == 0) {
            detectors.push_back(p);
        } else if (params.rounds % 3 == 1) {
            detectors.push_back(p + m);
        } else {
            detectors.push_back(p);
            detectors.push_back(p + m);
        }
        std::sort(detectors.begin(), detectors.end());
        tail.append_op("DETECTOR", detectors, {measure.x, measure.y, 1});
    }
    // Logical observable.
    std::vector<uint32_t> obs_inc;
    for (auto q : data_coords) {
        if (q.y == 0) {
            obs_inc.push_back((data_qubits.size() - data_coord_to_order[q]) | TARGET_RECORD_BIT);
        }
    }
    std::sort(obs_inc.begin(), obs_inc.end());
    tail.append_op("OBSERVABLE_INCLUDE", obs_inc, 0);

    // Put it all together.
    auto full_circuit = head + body * (params.rounds - 2) + tail;

    // Make 2d layout.
    std::map<std::pair<uint32_t, uint32_t>, std::pair<std::string, uint32_t>> layout;
    for (auto q : data_coords) {
        layout[{(uint32_t)(q.x * 2), (uint32_t)q.y}] = {q.y == 0 ? "L" : "d", p2q[q]};
    }
    std::array<const char *, 3> rgb{"R", "G", "B"};
    for (auto q : measure_coords) {
        auto x = (uint32_t)(q.x * 2);
        auto y = (uint32_t)q.y;
        layout[{x, y}] = {rgb[(x + y) % 3], p2q[q]};
    }

    return {
        full_circuit,
        layout,
        "# Legend:\n"
        "#     d# = data qubit\n"
        "#     L# = data qubit with logical observable crossing\n"
        "#     R# = measurement qubit (red hex)\n"
        "#     G# = measurement qubit (green hex)\n"
        "#     B# = measurement qubit (blue hex)\n"};
}
