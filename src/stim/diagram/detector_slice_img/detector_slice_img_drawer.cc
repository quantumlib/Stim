#include "stim/diagram/detector_slice_img/detector_slice_img_drawer.h"
#include "stim/simulators/error_analyzer.h"
#include "stim/diagram/timeline_ascii/diagram_timeline_ascii_drawer.h"

using namespace stim;
using namespace stim_draw_internal;

struct DetectorSliceSetComputer {
    ErrorAnalyzer analyzer;
    uint64_t num_ticks_left;
    DetectorSliceSetComputer(const Circuit &circuit, uint64_t tick_index);
    bool process_block_rev(const Circuit &block);
    bool process_op_rev(const Circuit &parent, const Operation &op);
};

bool DetectorSliceSetComputer::process_block_rev(const Circuit &block) {
    for (size_t k = block.operations.size(); k--;) {
        if (process_op_rev(block, block.operations[k])) {
            return true;
        }
    }
    return false;
}

bool DetectorSliceSetComputer::process_op_rev(const Circuit &parent, const Operation &op) {
    if (op.gate->id == gate_name_to_id("TICK")) {
        num_ticks_left--;
        return num_ticks_left == 0;
    } else if (op.gate->id == gate_name_to_id("REPEAT")) {
        const auto &loop_body = op_data_block_body(parent, op.target_data);
        uint64_t reps = op_data_rep_count(op.target_data);
        uint64_t num_loop_ticks = loop_body.count_ticks();
        if (num_loop_ticks * reps < num_ticks_left) {
            analyzer.run_loop(loop_body, reps);
            num_ticks_left -= num_loop_ticks * reps;
            return false;
        }

        uint64_t iterations = (num_ticks_left - 1) / num_loop_ticks;
        analyzer.run_loop(loop_body, iterations);
        num_ticks_left -= iterations * num_loop_ticks;
        while (!process_block_rev(loop_body)) {
        }
        return true;
    } else {
        (analyzer.*(op.gate->reverse_error_analyzer_function))(op.target_data);
        return false;
    }
}
DetectorSliceSetComputer::DetectorSliceSetComputer(const Circuit &circuit, uint64_t tick_index) :
      analyzer(
            circuit.count_detectors(),
            circuit.count_qubits(),
            false,
            true,
            true,
            1,
            false,
            false) {
    num_ticks_left = circuit.count_ticks();
    if (num_ticks_left == 0) {
        throw std::invalid_argument("Circuit contains no TICK instructions to slice at.");
    }
    if (tick_index >= num_ticks_left) {
        std::stringstream ss;
        ss << "tick_index=" << tick_index << " >= circuit.num_ticks=" << num_ticks_left;
        throw std::invalid_argument(ss.str());
    }
    num_ticks_left -= tick_index;
    analyzer.accumulate_errors = false;
}

std::string DetectorSliceSet::str() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
}
std::ostream &stim_draw_internal::operator<<(std::ostream &out, const DetectorSliceSet &slice) {
    slice.write_text_diagram_to(out);
    return out;
}

void DetectorSliceSet::write_text_diagram_to(std::ostream &out) const {
    DiagramTimelineAsciiDrawer drawer(num_qubits, false);
    drawer.moment_spacing = 2;

    for (const auto &s : slices) {
        drawer.reserve_drawing_room_for_targets(s.second);
        for (const auto &t : s.second) {
            std::stringstream ss;
            if (t.is_x_target()) {
                ss << "X";
            } else if (t.is_y_target()) {
                ss << "Y";
            } else if (t.is_z_target()) {
                ss << "Z";
            } else {
                ss << "?";
            }
            ss << ":";
            ss << s.first;
            drawer.add_cell(DiagramTimelineAsciiCellContents{
                DiagramTimelineAsciiAlignedPos{
                    drawer.m2x(drawer.cur_moment),
                    drawer.q2y(t.qubit_value()),
                    0,
                    0.5,
                },
                ss.str(),
            });
        }
    }

    // Make sure qubit lines are drawn first, so they are in the background.
    drawer.lines.insert(drawer.lines.begin(), drawer.num_qubits, {{0, 0, 0.0, 0.5}, {0, 0, 1.0, 0.5}});
    for (size_t q = 0; q < drawer.num_qubits; q++) {
        drawer.lines[q] = {
            {0, drawer.q2y(q), 1.0, 0.5},
            {drawer.m2x(drawer.cur_moment) + 1, drawer.q2y(q), 1.0, 0.5},
        };
        std::stringstream ss;
        ss << "q";
        ss << q;
        ss << ":";
        auto p = coordinates.find(q);
        if (p != coordinates.end()) {
            ss << "(" << comma_sep(p->second) << ")";
        }
        ss << " ";
        drawer.add_cell(DiagramTimelineAsciiCellContents{
            {0, drawer.q2y(q), 1.0, 0.5},
            ss.str(),
        });
    }

    drawer.render(out);
}

DetectorSliceSet DetectorSliceSet::from_circuit_tick(const stim::Circuit &circuit, uint64_t tick_index) {
    DetectorSliceSetComputer helper(circuit, tick_index);
    size_t num_qubits = helper.analyzer.xs.size();
    helper.process_block_rev(circuit);

    std::set<DemTarget> xs;
    std::set<DemTarget> ys;
    std::set<DemTarget> zs;
    DetectorSliceSet result;
    result.num_qubits = num_qubits;
    for (size_t q = 0; q < num_qubits; q++) {
        xs.clear();
        ys.clear();
        zs.clear();
        for (auto t : helper.analyzer.xs[q]) {
            xs.insert(t);
        }
        for (auto t : helper.analyzer.zs[q]) {
            if (xs.find(t) == xs.end()) {
                zs.insert(t);
            } else {
                xs.erase(t);
                ys.insert(t);
            }
        }

        for (const auto &t : xs) {
            result.slices[t].push_back(GateTarget::x(q));
        }
        for (const auto &t : ys) {
            result.slices[t].push_back(GateTarget::y(q));
        }
        for (const auto &t : zs) {
            result.slices[t].push_back(GateTarget::z(q));
        }
    }

    result.coordinates = circuit.get_final_qubit_coords();

    return result;
}
