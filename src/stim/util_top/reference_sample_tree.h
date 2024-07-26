#ifndef _STIM_UTIL_TOP_REFERENCE_SAMPLE_TREE_H
#define _STIM_UTIL_TOP_REFERENCE_SAMPLE_TREE_H

#include "stim/simulators/tableau_simulator.h"

namespace stim {

/// A compressed tree representation of a reference sample.
struct ReferenceSampleTree {
    /// Raw bits to output before bits from the children.
    std::vector<bool> prefix_bits;
    /// Compressed representations of additional bits to output after the prefix.
    std::vector<ReferenceSampleTree> suffix_children;
    /// The number of times to repeatedly output the prefix+suffix bits.
    size_t repetitions = 0;

    /// Initializes a reference sample tree containing a reference sample for the given circuit.
    static ReferenceSampleTree from_circuit_reference_sample(const Circuit &circuit);

    /// Returns a tree with the same compressed contents, but a simpler tree structure.
    ReferenceSampleTree simplified() const;

    /// Checks if two trees are exactly the same, including structure (not just uncompressed contents).
    bool operator==(const ReferenceSampleTree &other) const;
    /// Checks if two trees are not exactly the same, including structure (not just uncompressed contents).
    bool operator!=(const ReferenceSampleTree &other) const;
    /// Returns a simple description of the tree's structure, like "5*('101'+6*('11'))".
    std::string str() const;

    /// Determines whether the tree contains any bits at all.
    bool empty() const;
    /// Computes the total size of the uncompressed bits represented by the tree.
    size_t size() const;

    /// Writes the contents of the tree into the given output vector.
    void decompress_into(std::vector<bool> &output) const;

    /// Folds redundant children into the repetition count, if they repeat this many times.
    ///
    /// For example, if the tree's children are [A, B, C, A, B, C] and the tree has no
    /// prefix, then `try_factorize(2)` will reduce the children to [A, B, C] and double
    /// the repetition count.
    void try_factorize(size_t period_factor);

   private:
    /// Helper method for `simplified`.
    void flatten_and_simplify_into(std::vector<ReferenceSampleTree> &out) const;
};
std::ostream &operator<<(std::ostream &out, const ReferenceSampleTree &v);

/// Helper class for computing compressed reference samples.
template <size_t W>
struct CompressedReferenceSampleHelper {
    TableauSimulator<W> sim;

    CompressedReferenceSampleHelper(TableauSimulator<MAX_BITWORD_WIDTH> sim) : sim(sim) {
    }

    /// Processes a loop with no top-level folding.
    ///
    /// Loops containing within the body of this loop (or circuit body) may
    /// still be compressed. Only the top-level loop is not folded.
    ReferenceSampleTree do_loop_with_no_folding(const Circuit &loop, uint64_t reps);

    /// Runs tortoise-and-hare analysis of the loop while simulating its
    /// reference sample, in order to attempt to return a compressed
    /// representation.
    ReferenceSampleTree do_loop_with_tortoise_hare_folding(const Circuit &loop, uint64_t reps);

    bool in_same_recent_state_as(
        const CompressedReferenceSampleHelper<W> &other, uint64_t max_record_lookback, bool allow_false_negative) const;
};

uint64_t max_feedback_lookback_in_loop(const Circuit &loop);

}  // namespace stim

#include "stim/util_top/reference_sample_tree.inl"

#endif
