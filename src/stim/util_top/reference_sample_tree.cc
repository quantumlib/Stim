#include "stim/util_top/reference_sample_tree.h"

using namespace stim;

bool ReferenceSampleTree::empty() const {
    if (repetitions == 0) {
        return true;
    }
    if (!prefix_bits.empty()) {
        return false;
    }
    for (const auto &child: suffix_children) {
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

    std::vector<ReferenceSampleTree> flattened;
    if (!prefix_bits.empty()) {
        flattened.push_back(ReferenceSampleTree{
            .prefix_bits = prefix_bits,
            .suffix_children = {},
            .repetitions = 1,
        });
    }
    for (const auto &child : suffix_children) {
        child.flatten_and_simplify_into(flattened);
    }

    // Fuse children.
    auto &f = flattened;
    for (size_t k = 0; k < f.size(); k++) {
        // Combine children with identical contents by adding their rep counts.
        while (k + 1 < f.size() && f[k].prefix_bits == f[k + 1].prefix_bits && f[k].suffix_children == f[k + 1].suffix_children) {
            f[k].repetitions += f[k + 1].repetitions;
            f.erase(f.begin() + k + 1);
        }

        // Fuse children with unrepeated contents if they can be fused.
        while (k + 1 < f.size() && f[k].repetitions == 1 && f[k].suffix_children.empty() && f[k + 1].repetitions == 1) {
            f[k].suffix_children = std::move(f[k + 1].suffix_children);
            f[k].prefix_bits.insert(f[k].prefix_bits.end(), f[k + 1].prefix_bits.begin(), f[k + 1].prefix_bits.end());
            f.erase(f.begin() + k + 1);
        }
    }

    if (repetitions == 1) {
        // Un-nest all the children.
        for (auto &e : flattened) {
            out.push_back(e);
        }
    } else if (flattened.size() == 1) {
        // Merge with single child.
        flattened[0].repetitions *= repetitions;
        out.push_back(std::move(flattened[0]));
    } else if (flattened.empty()) {
        // Nothing to report.
    } else if (flattened[0].suffix_children.empty() && flattened[0].repetitions == 1) {
        // Take payload from first child.
        auto result = std::move(flattened[0]);
        flattened.erase(flattened.begin());
        result.repetitions = repetitions;
        result.suffix_children = std::move(flattened);
        out.push_back(std::move(result));
    } else {
        out.push_back(ReferenceSampleTree{
            .prefix_bits={},
            .suffix_children=std::move(flattened),
            .repetitions=repetitions,
        });
    }
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
        return flat[0];
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
    for (const auto &child: suffix_children) {
        result += child.size();
    }
    return result * repetitions;
}

void ReferenceSampleTree::decompress_into(std::vector<bool> &output) const {
    for (uint64_t k = 0; k < repetitions; k++) {
        output.insert(output.end(), prefix_bits.begin(), prefix_bits.end());
        for (const auto &child: suffix_children) {
            child.decompress_into(output);
        }
    }
}

ReferenceSampleTree ReferenceSampleTree::from_circuit_reference_sample(const Circuit &circuit) {
    auto stats = circuit.compute_stats();
    std::mt19937_64 irrelevant_rng{0};
    CompressedReferenceSampleHelper<MAX_BITWORD_WIDTH> helper(
        TableauSimulator<MAX_BITWORD_WIDTH>(
            std::move(irrelevant_rng),
            stats.num_qubits,
            +1,
            MeasureRecord(stats.max_lookback)));
    return helper.do_loop_with_tortoise_hare_folding(circuit, 1).simplified();
}

std::string ReferenceSampleTree::str() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
}

bool ReferenceSampleTree::operator==(const ReferenceSampleTree &other) const {
    return repetitions == other.repetitions &&
           prefix_bits == other.prefix_bits &&
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
