#include "stim/util_top/reference_sample_tree.h"

using namespace stim;

bool ReferenceSampleTree::empty() const {
    if (repetitions == 0) {
        return true;
    }
    if (!prefix_bits.empty()) {
        return false;
    }
    for (const auto &child : suffix_children) {
        if (!child.empty()) {
            return false;
        }
    }
    return true;
}

void ReferenceSampleTree::flatten_and_simplify_into(std::vector<ReferenceSampleTree> &out) const {
    if (repetitions == 0) {
        return;
    }

    // Flatten children.
    std::vector<ReferenceSampleTree> flattened;
    if (!prefix_bits.empty()) {
        flattened.push_back(
            ReferenceSampleTree{
                .prefix_bits = prefix_bits,
                .suffix_children = {},
                .repetitions = 1,
            });
    }
    for (const auto &child : suffix_children) {
        child.flatten_and_simplify_into(flattened);
    }

    // Fuse children.
    std::vector<ReferenceSampleTree> fused;
    if (!flattened.empty()) {
        fused.push_back(std::move(flattened[0]));
    }
    for (size_t k = 1; k < flattened.size(); k++) {
        auto &dst = fused.back();
        auto &src = flattened[k];

        // Combine children with identical contents by adding their rep counts.
        if (dst.prefix_bits == src.prefix_bits && dst.suffix_children == src.suffix_children) {
            dst.repetitions += src.repetitions;

            // Fuse children with unrepeated contents if they can be fused.
        } else if (src.repetitions == 1 && dst.repetitions == 1 && dst.suffix_children.empty()) {
            dst.suffix_children = std::move(src.suffix_children);
            dst.prefix_bits.insert(dst.prefix_bits.end(), src.prefix_bits.begin(), src.prefix_bits.end());

        } else {
            fused.push_back(std::move(src));
        }
    }

    if (repetitions == 1) {
        // Un-nest all the children.
        for (auto &e : fused) {
            out.push_back(e);
        }
    } else if (fused.size() == 1) {
        // Merge with single child.
        fused[0].repetitions *= repetitions;
        out.push_back(std::move(fused[0]));
    } else if (fused.empty()) {
        // Nothing to report.
    } else if (fused[0].suffix_children.empty() && fused[0].repetitions == 1) {
        // Take payload from first child.
        ReferenceSampleTree result = std::move(fused[0]);
        fused.erase(fused.begin());
        result.repetitions = repetitions;
        result.suffix_children = std::move(fused);
        out.push_back(std::move(result));
    } else {
        out.push_back(
            ReferenceSampleTree{
                .prefix_bits = {},
                .suffix_children = std::move(fused),
                .repetitions = repetitions,
            });
    }
}

/// Finds how far back feedback operations ever look, within the loop.
uint64_t stim::max_feedback_lookback_in_loop(const Circuit &loop) {
    uint64_t furthest_lookback = 0;
    for (const auto &inst : loop.operations) {
        if (inst.gate_type == GateType::REPEAT) {
            furthest_lookback =
                std::max(furthest_lookback, max_feedback_lookback_in_loop(inst.repeat_block_body(loop)));
        } else {
            auto f = GATE_DATA[inst.gate_type].flags;
            if ((f & GateFlags::GATE_CAN_TARGET_BITS) && (f & GateFlags::GATE_TARGETS_PAIRS)) {
                // Feedback-capable operation. Check for any measurement record targets.
                for (auto t : inst.targets) {
                    if (t.is_measurement_record_target()) {
                        furthest_lookback = std::max(furthest_lookback, (uint64_t)-t.rec_offset());
                    }
                }
            }
        }
    }
    return furthest_lookback;
}

void ReferenceSampleTree::try_factorize(size_t period_factor) {
    if (prefix_bits.size() != 0 || suffix_children.size() % period_factor != 0) {
        return;
    }

    // Check if contents are periodic with the factor.
    size_t h = suffix_children.size() / period_factor;
    for (size_t k = h; k < suffix_children.size(); k++) {
        if (suffix_children[k - h] != suffix_children[k]) {
            return;
        }
    }

    // Factorize.
    suffix_children.resize(h);
    repetitions *= period_factor;
}

ReferenceSampleTree ReferenceSampleTree::simplified() const {
    std::vector<ReferenceSampleTree> flat;
    flatten_and_simplify_into(flat);
    if (flat.empty()) {
        return ReferenceSampleTree();
    } else if (flat.size() == 1) {
        return std::move(flat[0]);
    }

    ReferenceSampleTree result;
    result.repetitions = 1;

    // Take payload from first child.
    if (flat[0].repetitions == 1 && flat[0].suffix_children.empty()) {
        result = std::move(flat[0]);
        flat.erase(flat.begin());
    }

    result.suffix_children = std::move(flat);
    return result;
}

size_t ReferenceSampleTree::size() const {
    size_t result = prefix_bits.size();
    for (const auto &child : suffix_children) {
        result += child.size();
    }
    return result * repetitions;
}

void ReferenceSampleTree::decompress_into(std::vector<bool> &output) const {
    for (uint64_t k = 0; k < repetitions; k++) {
        output.insert(output.end(), prefix_bits.begin(), prefix_bits.end());
        for (const auto &child : suffix_children) {
            child.decompress_into(output);
        }
    }
}

ReferenceSampleTree ReferenceSampleTree::from_circuit_reference_sample(const Circuit &circuit) {
    auto stats = circuit.compute_stats();
    std::mt19937_64 irrelevant_rng{0};
    CompressedReferenceSampleHelper<MAX_BITWORD_WIDTH> helper(
        TableauSimulator<MAX_BITWORD_WIDTH>(
            std::move(irrelevant_rng), stats.num_qubits, +1, MeasureRecord(stats.max_lookback)));
    return helper.do_loop_with_tortoise_hare_folding(circuit, 1).simplified();
}

std::string ReferenceSampleTree::str() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
}

bool ReferenceSampleTree::operator==(const ReferenceSampleTree &other) const {
    return repetitions == other.repetitions && prefix_bits == other.prefix_bits &&
           suffix_children == other.suffix_children;
}
bool ReferenceSampleTree::operator!=(const ReferenceSampleTree &other) const {
    return !(*this == other);
}

std::ostream &stim::operator<<(std::ostream &out, const ReferenceSampleTree &v) {
    out << v.repetitions << "*";
    out << "(";
    out << "'";
    for (auto b : v.prefix_bits) {
        out << "01"[b];
    }
    out << "'";
    for (const auto &child : v.suffix_children) {
        out << "+";
        out << child;
    }
    out << ")";
    return out;
}
