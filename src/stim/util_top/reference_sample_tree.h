#ifndef _STIM_UTIL_TOP_REFERENCE_SAMPLE_TREE_H
#define _STIM_UTIL_TOP_REFERENCE_SAMPLE_TREE_H

#include "stim/simulators/tableau_simulator.h"

namespace stim {

/// A compressed tree representation of a reference sample.
struct ReferenceSampleTree {
    /// Bits to repeatedly output before outputting bits for the children.
    std::vector<bool> prefix_bits;
    /// Compressed representations of additional bits to output after the prefix.
    std::vector<ReferenceSampleTree> suffix_children;
    /// The number of times to repeatedly output the prefix and suffix bits.
    size_t repetitions = 0;

    /// Initializes a reference sample tree containing a reference sample for the given circuit.
    static ReferenceSampleTree from_circuit_reference_sample(const Circuit &circuit);

    /// Returns a tree with the same compressed contents, but a simpler tree structure.
    ReferenceSampleTree simplified() const;

    /// Determines whether the tree contains any bits at all.
    bool empty() const;

    bool operator==(const ReferenceSampleTree &other) const;
    bool operator!=(const ReferenceSampleTree &other) const;
    std::string str() const;

    /// Computes the total size of the uncompressed bits represented by the tree.
    size_t size() const;

    /// Writes the contents of the tree into the given output vector.
    void decompress_into(std::vector<bool> &output) const;

   private:
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
    ReferenceSampleTree do_loop_with_no_folding(
        const Circuit &loop,
        uint64_t reps);

    /// Runs tortoise-and-hare analysis of the loop while simulating its
    /// reference sample, in order to attempt to return a compressed
    /// representation.
    ReferenceSampleTree do_loop_with_tortoise_hare_folding(
        const Circuit &loop,
        uint64_t reps);
};

}  // namespace stim

#include "stim/util_top/reference_sample_tree.inl"

#endif
