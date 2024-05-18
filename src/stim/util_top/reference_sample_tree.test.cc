#include "stim/util_top/reference_sample_tree.h"

#include "gtest/gtest.h"

#include "stim/gen/gen_surface_code.h"

using namespace stim;

TEST(ReferenceSampleTree, equality) {
    ReferenceSampleTree empty1{
        .prefix_bits={},
        .suffix_children={},
        .repetitions=0,
    };
    ReferenceSampleTree empty2;
    ASSERT_EQ(empty1, empty2);

    ASSERT_FALSE(empty1 != empty2);
    ASSERT_NE(empty1, (ReferenceSampleTree{.prefix_bits={},.suffix_children{},.repetitions=1}));
    ASSERT_NE(empty1, (ReferenceSampleTree{.prefix_bits={0},.suffix_children{},.repetitions=0}));
    ASSERT_NE(empty1, (ReferenceSampleTree{.prefix_bits={},.suffix_children{{}},.repetitions=0}));
}

TEST(ReferenceSampleTree, str) {
    ASSERT_EQ((ReferenceSampleTree{
        .prefix_bits={},
        .suffix_children={},
        .repetitions=0,
    }.str()), "0*('')");

    ASSERT_EQ((ReferenceSampleTree{
        .prefix_bits={1, 1, 0, 1},
        .suffix_children={},
        .repetitions=0,
    }.str()), "0*('1101')");

    ASSERT_EQ((ReferenceSampleTree{
        .prefix_bits={1, 1, 0, 1},
        .suffix_children={},
        .repetitions=2,
    }.str()), "2*('1101')");

    ASSERT_EQ((ReferenceSampleTree{
        .prefix_bits={1, 1, 0, 1},
        .suffix_children={
            ReferenceSampleTree{
                .prefix_bits={1},
                .suffix_children={},
                .repetitions=5,
            }
        },
        .repetitions=2,
    }.str()), "2*('1101'+5*('1'))");
}

TEST(ReferenceSampleTree, simplified) {
    ReferenceSampleTree raw{
        .prefix_bits={},
        .suffix_children={
            ReferenceSampleTree{
                .prefix_bits={},
                .suffix_children={},
                .repetitions=1,
            },
            ReferenceSampleTree{
                .prefix_bits={1, 0, 1},
                .suffix_children={{}},
                .repetitions=0,
            },
            ReferenceSampleTree{
                .prefix_bits={1, 1, 1},
                .suffix_children={},
                .repetitions=2,
            },
        },
        .repetitions=3,
    };
    ASSERT_EQ(raw.simplified().str(), "6*('111')");
}

TEST(ReferenceSampleTree, decompress_into) {
    std::vector<bool> result;
    ReferenceSampleTree{
        .prefix_bits={1, 1, 0, 1},
        .suffix_children={
            ReferenceSampleTree{
                .prefix_bits={1},
                .suffix_children={},
                .repetitions=5,
            }
        },
        .repetitions=2,
    }.decompress_into(result);
    ASSERT_EQ(result, (std::vector<bool>{1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1}));

    result.clear();
    ReferenceSampleTree{
        .prefix_bits={1, 1, 0, 1},
        .suffix_children={
            ReferenceSampleTree{
                .prefix_bits={1, 0, 1},
                .suffix_children={},
                .repetitions=8,
            },
            ReferenceSampleTree{
                .prefix_bits={0, 0},
                .suffix_children={},
                .repetitions=1,
            },
        },
        .repetitions=1,
    }.decompress_into(result);
    ASSERT_EQ(result, (std::vector<bool>{1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0}));
}

TEST(ReferenceSampleTree, simple_circuit) {
    Circuit circuit(R"CIRCUIT(
        M 0
        X 0
        M 0
    )CIRCUIT");
    auto ref = ReferenceSampleTree::from_circuit_reference_sample(circuit);
    ASSERT_EQ(ref.str(), "1*('01')");
}

TEST(ReferenceSampleTree, simple_loop) {
    Circuit circuit(R"CIRCUIT(
        REPEAT 50 {
            M 0
            X 0
            M 0
        }
    )CIRCUIT");
    auto ref = ReferenceSampleTree::from_circuit_reference_sample(circuit);
    ASSERT_EQ(ref.str(), "1*('01'+24*('1001')+1*('10'))");
}

TEST(ReferenceSampleTree, period4_loop) {
    Circuit circuit(R"CIRCUIT(
        M 0
        X 0
        M 0
        REPEAT 50 {
            CX 0 1 1 2 2 3
            M 0 1 2 3
        }
        X 0
        M 0
        X 2
        M 2 2 2 2 2
    )CIRCUIT");
    auto ref = ReferenceSampleTree::from_circuit_reference_sample(circuit);
    ASSERT_EQ(ref.size(), circuit.count_measurements());
    ASSERT_EQ(ref.str(), "1*('01111110101100'+11*('1000111110101100')+1*('100011111010000000'))");
}

TEST(ReferenceSampleTree, nested_loops) {
    Circuit circuit(R"CIRCUIT(
        REPEAT 100 {
            REPEAT 100 {
                M 0
                X 0
                M 0
            }
            REPEAT 200 {
                M 0
            }
            X 1
            CX 1 0
        }
    )CIRCUIT");
    auto ref = ReferenceSampleTree::from_circuit_reference_sample(circuit);
    ASSERT_EQ(ref.str(), "1*('01'+49*('1001')+1*('10')+200*('0')+1*('10')+49*('0110')+1*('01')+200*('1')+1*('10')+49*('0110')+1*('01')+200*('1')+24*('01'+49*('1001')+1*('10')+200*('0')+1*('01')+49*('1001')+1*('10')+200*('0')+1*('10')+49*('0110')+1*('01')+200*('1')+1*('10')+49*('0110')+1*('01')+200*('1'))+1*('01')+49*('1001')+1*('10')+200*('0'))");

    std::vector<bool> ref_uncompressed;
    ref.decompress_into(ref_uncompressed);
    simd_bits<MAX_BITWORD_WIDTH> ref_flat(ref_uncompressed.size());
    for (size_t k = 0; k < ref_uncompressed.size(); k++) {
        ref_flat[k] = ref_uncompressed[k];
    }
    auto ref2 = TableauSimulator<MAX_BITWORD_WIDTH>::reference_sample_circuit(circuit);
    ASSERT_EQ(ref_flat, ref2);
}

TEST(ReferenceSampleTree, surface_code) {
    CircuitGenParameters params(10000, 5, "rotated_memory_x");
    auto circuit = generate_surface_code_circuit(params).circuit;
    auto ref = ReferenceSampleTree::from_circuit_reference_sample(circuit);
    ASSERT_EQ(ref.str(), "1*('000000000000000000000000000000000000000000000000'+4999*('000000000000000000000000000000000000000000000000')+1*('0000000000000000000000000'))");
}

TEST(ReferenceSampleTree, surface_code_with_pauli) {
    CircuitGenParameters params(10000, 3, "rotated_memory_x");
    auto circuit = generate_surface_code_circuit(params).circuit;
    circuit.blocks[0].append_from_text("X 10 11 12 13");
    auto ref = ReferenceSampleTree::from_circuit_reference_sample(circuit);
    ASSERT_EQ(ref.str(), "1*('0000000000000000'+4999*('0110000000110000')+1*('000000000'))");
}

TEST(ReferenceSampleTree, surface_code_with_pauli_vs_normal_reference_sample) {
    CircuitGenParameters params(20, 3, "rotated_memory_x");
    auto circuit = generate_surface_code_circuit(params).circuit;
    circuit.blocks[0].append_from_text("X 10 11 12 13");
    auto ref = ReferenceSampleTree::from_circuit_reference_sample(circuit);
    ASSERT_EQ(ref.size(), circuit.count_measurements());

    std::vector<bool> ref_uncompressed;
    ref.decompress_into(ref_uncompressed);
    simd_bits<MAX_BITWORD_WIDTH> ref_flat(ref_uncompressed.size());
    for (size_t k = 0; k < ref_uncompressed.size(); k++) {
        ref_flat[k] = ref_uncompressed[k];
    }

    auto ref2 = TableauSimulator<MAX_BITWORD_WIDTH>::reference_sample_circuit(circuit);
    ASSERT_EQ(ref_flat, ref2);
}
