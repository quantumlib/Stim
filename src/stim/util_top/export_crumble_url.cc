#include "stim/util_top/export_crumble_url.h"

#include "stim/simulators/matched_error.h"

using namespace stim;

void write_crumble_name_with_args(const CircuitInstruction &instruction, std::ostream &out) {
    if (instruction.gate_type == GateType::DETECTOR) {
        out << "DT";
    } else if (instruction.gate_type == GateType::QUBIT_COORDS) {
        out << "Q";
    } else if (instruction.gate_type == GateType::OBSERVABLE_INCLUDE) {
        out << "OI";
    } else {
        out << GATE_DATA[instruction.gate_type].name;
    }
    if (!instruction.args.empty()) {
        out << '(';
        bool first = true;
        for (auto e : instruction.args) {
            if (first) {
                first = false;
            } else {
                out << ",";
            }
            if (e > (double)INT64_MIN && e < (double)INT64_MAX && (int64_t)e == e) {
                out << (int64_t)e;
            } else {
                out << e;
            }
        }
        out << ')';
    }
}

void write_crumble_url(
    const Circuit &circuit,
    bool skip_detectors,
    const std::vector<std::pair<int, CircuitErrorLocation>> &marks,
    std::ostream &out) {
    ExplainedError err;
    std::vector<std::pair<int, CircuitErrorLocation>> active_marks;
    for (size_t k = 0; k < circuit.operations.size(); k++) {
        active_marks.clear();
        for (const auto &mark : marks) {
            if (!mark.second.stack_frames.empty() && mark.second.stack_frames.back().instruction_offset == k) {
                active_marks.push_back(mark);
                active_marks.back().second.stack_frames.pop_back();
            }
        }

        bool adding_markers = false;
        for (const auto &mark : active_marks) {
            adding_markers |= mark.second.stack_frames.empty();
        }
        if (adding_markers) {
            out << ";TICK";
        }
        for (const auto &mark : active_marks) {
            if (mark.second.stack_frames.empty()) {
                const auto &v1 = mark.second.flipped_pauli_product;
                const auto &v2 = mark.second.flipped_measurement.measured_observable;
                for (const auto &e : v1) {
                    out << ";MARK" << e.gate_target.pauli_type() << "(" << mark.first << ")"
                        << e.gate_target.qubit_value();
                }
                if (!v2.empty()) {
                    auto t = v2[0].gate_target;
                    char c = "XZ"[t.pauli_type() == 'X'];
                    out << ";MARK" << c << "(" << mark.first << ")" << v2[0].gate_target.qubit_value();
                }
            }
        }
        if (adding_markers) {
            out << ";TICK";
        }

        const auto &instruction = circuit.operations[k];
        if (instruction.gate_type == GateType::DETECTOR && skip_detectors) {
            continue;
        }
        if (k > 0 || adding_markers) {
            out << ';';
        }

        if (instruction.gate_type == GateType::REPEAT) {
            if (active_marks.empty()) {
                out << "REPEAT_" << instruction.repeat_block_rep_count() << "_{;";
                write_crumble_url(instruction.repeat_block_body(circuit), skip_detectors, active_marks, out);
                out << ";}";
            } else {
                std::vector<std::pair<int, CircuitErrorLocation>> iter_marks;
                for (size_t k2 = 0; k2 < instruction.repeat_block_rep_count(); k2++) {
                    iter_marks.clear();
                    for (const auto &mark : active_marks) {
                        if (!mark.second.stack_frames.empty() &&
                            mark.second.stack_frames.back().iteration_index == k2) {
                            iter_marks.push_back(mark);
                        }
                    }
                    write_crumble_url(instruction.repeat_block_body(circuit), skip_detectors, iter_marks, out);
                }
            }
            continue;
        }
        write_crumble_name_with_args(instruction, out);

        for (size_t k2 = 0; k2 < instruction.targets.size(); k2++) {
            auto target = instruction.targets[k2];
            if (target.is_combiner()) {
                out << '*';
                k2 += 1;
                target = instruction.targets[k2];
            } else if (k2 > 0 || instruction.args.empty()) {
                out << '_';
            }
            target.write_succinct(out);
        }
    }
}

std::string stim::export_crumble_url(
    const Circuit &circuit, bool skip_detectors, const std::map<int, std::vector<ExplainedError>> &mark) {
    std::vector<std::pair<int, CircuitErrorLocation>> marks;
    for (const auto &[k, vs] : mark) {
        for (const auto &v : vs) {
            if (!v.circuit_error_locations.empty()) {
                marks.push_back({k, v.circuit_error_locations[0]});
            }
        }
    }
    std::stringstream s;
    s << "https://algassert.com/crumble#circuit=";
    write_crumble_url(circuit, skip_detectors, marks, s);
    return s.str();
}
