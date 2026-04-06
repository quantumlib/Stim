#include "stim/util_top/missing_detectors.h"

#include "stim/util_top/circuit_flow_generators.h"

using namespace stim;

static Circuit missing_detectors_impl(const Circuit &circuit) {
    size_t num_measurements = circuit.count_measurements();
    std::vector<std::pair<simd_bits<64>, bool>> rows;
    std::vector<simd_bits<64>> logical_operators;
    std::set<size_t> ignored_logical_operators;

    // Turn existing detectors and observables into rows in the table.
    uint64_t measurement_offset = 0;
    circuit.for_each_operation([&](CircuitInstruction inst) {
        measurement_offset += inst.count_measurement_results();

        if (inst.gate_type == GateType::DETECTOR || inst.gate_type == GateType::OBSERVABLE_INCLUDE) {
            if (inst.gate_type == GateType::DETECTOR) {
                rows.push_back({simd_bits<64>(num_measurements), false});
            } else {
                while (logical_operators.size() <= (size_t)inst.args[0]) {
                    logical_operators.push_back(simd_bits<64>(num_measurements));
                }
            }
            simd_bits_range_ref<64> row =
                inst.gate_type == GateType::DETECTOR ? rows.back().first : logical_operators[(size_t)inst.args[0]];
            for (auto e : inst.targets) {
                if (e.is_measurement_record_target()) {
                    row[e.rec_offset() + measurement_offset] ^= true;
                } else if (e.is_pauli_target() && inst.gate_type == GateType::OBSERVABLE_INCLUDE) {
                    ignored_logical_operators.insert((size_t)inst.args[0]);
                }
            }
        }
    });
    for (size_t k = 0; k < logical_operators.size(); k++) {
        if (!ignored_logical_operators.contains(k)) {
            rows.push_back({std::move(logical_operators[k]), false});
        }
    }
    std::vector<simd_bits<64>> original_detector_rows_for_cleanup;
    for (const auto &row : rows) {
        original_detector_rows_for_cleanup.push_back(row.first);
    }

    // Turn measurement invariants into rows in the table.
    for (const auto &generator : circuit_flow_generators<64>(circuit)) {
        if (generator.input.ref().has_no_pauli_terms() && generator.output.ref().has_no_pauli_terms() &&
            generator.observables.empty()) {
            rows.push_back({simd_bits<64>(num_measurements), true});
            for (int32_t e : generator.measurements) {
                if (e < 0) {
                    rows.back().first[e + num_measurements] ^= true;
                } else {
                    rows.back().first[e] ^= true;
                }
            }
        }
    }

    // Perform Gaussian elimination on the table.
    size_t num_solved = 0;
    for (size_t k = 0; k < num_measurements; k++) {
        size_t pivot = SIZE_MAX;
        // Try to find a DETECTOR pivot.
        for (size_t r = num_solved; r < rows.size() && pivot == SIZE_MAX; r++) {
            if (rows[r].first[k] && !rows[r].second) {
                pivot = r;
            }
        }
        // Fall back to a flow invariant pivot.
        for (size_t r = num_solved; r < rows.size() && pivot == SIZE_MAX; r++) {
            if (rows[r].first[k]) {
                pivot = r;
            }
        }
        if (pivot == SIZE_MAX) {
            continue;
        }

        for (size_t r = 0; r < rows.size(); r++) {
            if (rows[r].first[k] && r != pivot) {
                rows[r].first ^= rows[pivot].first;
            }
        }
        if (pivot != num_solved) {
            std::swap(rows[pivot], rows[num_solved]);
        }
        num_solved++;
    }

    // Any rows from invariants that the detector rows failed to clear are assumed to be the missing detectors.
    Circuit result;
    for (auto &r : rows) {
        if (r.second && r.first.not_zero()) {
            // Attempt to reduce the weight of the results by at least not overlapping with existing detectors.
            for (const auto &det : original_detector_rows_for_cleanup) {
                if (r.first.is_subset_of_or_equal_to(det)) {
                    r.first ^= det;
                }
            }

            // Convert set bits into a DETECTOR instruction.
            r.first.for_each_set_bit([&](size_t bit_position) {
                result.target_buf.append_tail(GateTarget::rec((int32_t)bit_position - (int32_t)num_measurements));
            });
            result.operations.push_back(
                CircuitInstruction{
                    GateType::DETECTOR,
                    {},
                    result.target_buf.commit_tail(),
                    "",
                });
        }
    }

    return result;
}

Circuit stim::missing_detectors(const Circuit &circuit, bool unknown_input) {
    if (unknown_input) {
        return missing_detectors_impl(circuit);
    } else {
        Circuit with_resets;
        uint32_t num_qubits = (uint32_t)circuit.count_qubits();
        for (uint32_t k = 0; k < num_qubits; k++) {
            with_resets.target_buf.append_tail(GateTarget::qubit(k));
        }
        with_resets.operations.push_back(
            CircuitInstruction{
                GateType::R,
                {},
                with_resets.target_buf.commit_tail(),
                "",
            });
        with_resets += circuit;
        return missing_detectors_impl(with_resets);
    }
}
