#include "stim/util_top/reference_sample_tree.h"

#include "gtest/gtest.h"

#include "stim/gen/gen_surface_code.h"

using namespace stim;

void expect_tree_matches_normal_reference_sample_of(const ReferenceSampleTree &tree, const Circuit &circuit) {
    std::vector<bool> decompressed;
    tree.decompress_into(decompressed);
    simd_bits<MAX_BITWORD_WIDTH> actual(decompressed.size());
    for (size_t k = 0; k < decompressed.size(); k++) {
        actual[k] = decompressed[k];
    }
    auto expected = TableauSimulator<MAX_BITWORD_WIDTH>::reference_sample_circuit(circuit);
    EXPECT_EQ(actual, expected);
}

TEST(ReferenceSampleTree, equality) {
    ReferenceSampleTree empty1{
        .prefix_bits = {},
        .suffix_children = {},
        .repetitions = 0,
    };
    ReferenceSampleTree empty2;
    ASSERT_EQ(empty1, empty2);

    ASSERT_FALSE(empty1 != empty2);
    ASSERT_NE(empty1, (ReferenceSampleTree{.prefix_bits = {}, .suffix_children{}, .repetitions = 1}));
    ASSERT_NE(empty1, (ReferenceSampleTree{.prefix_bits = {0}, .suffix_children{}, .repetitions = 0}));
    ASSERT_NE(empty1, (ReferenceSampleTree{.prefix_bits = {}, .suffix_children{{}}, .repetitions = 0}));
}

TEST(ReferenceSampleTree, str) {
    ASSERT_EQ(
        (ReferenceSampleTree{
            .prefix_bits = {},
            .suffix_children = {},
            .repetitions = 0,
        }
             .str()),
        "0*('')");

    ASSERT_EQ(
        (ReferenceSampleTree{
            .prefix_bits = {1, 1, 0, 1},
            .suffix_children = {},
            .repetitions = 0,
        }
             .str()),
        "0*('1101')");

    ASSERT_EQ(
        (ReferenceSampleTree{
            .prefix_bits = {1, 1, 0, 1},
            .suffix_children = {},
            .repetitions = 2,
        }
             .str()),
        "2*('1101')");

    ASSERT_EQ(
        (ReferenceSampleTree{
            .prefix_bits = {1, 1, 0, 1},
            .suffix_children = {ReferenceSampleTree{
                .prefix_bits = {1},
                .suffix_children = {},
                .repetitions = 5,
            }},
            .repetitions = 2,
        }
             .str()),
        "2*('1101'+5*('1'))");
}

TEST(ReferenceSampleTree, simplified) {
    ReferenceSampleTree raw{
        .prefix_bits = {},
        .suffix_children =
            {
                ReferenceSampleTree{
                    .prefix_bits = {},
                    .suffix_children = {},
                    .repetitions = 1,
                },
                ReferenceSampleTree{
                    .prefix_bits = {1, 0, 1},
                    .suffix_children = {{}},
                    .repetitions = 0,
                },
                ReferenceSampleTree{
                    .prefix_bits = {1, 1, 1},
                    .suffix_children = {},
                    .repetitions = 2,
                },
            },
        .repetitions = 3,
    };
    ASSERT_EQ(raw.simplified().str(), "6*('111')");
}

TEST(ReferenceSampleTree, decompress_into) {
    std::vector<bool> result;
    ReferenceSampleTree{
        .prefix_bits = {1, 1, 0, 1},
        .suffix_children = {ReferenceSampleTree{
            .prefix_bits = {1},
            .suffix_children = {},
            .repetitions = 5,
        }},
        .repetitions = 2,
    }
        .decompress_into(result);
    ASSERT_EQ(result, (std::vector<bool>{1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1}));

    result.clear();
    ReferenceSampleTree{
        .prefix_bits = {1, 1, 0, 1},
        .suffix_children =
            {
                ReferenceSampleTree{
                    .prefix_bits = {1, 0, 1},
                    .suffix_children = {},
                    .repetitions = 8,
                },
                ReferenceSampleTree{
                    .prefix_bits = {0, 0},
                    .suffix_children = {},
                    .repetitions = 1,
                },
            },
        .repetitions = 1,
    }
        .decompress_into(result);
    ASSERT_EQ(result, (std::vector<bool>{1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0,
                                         1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0}));
}

TEST(ReferenceSampleTree, simple_circuit) {
    Circuit circuit(R"CIRCUIT(
        M 0
        X 0
        M 0
    )CIRCUIT");
    auto ref = ReferenceSampleTree::from_circuit_reference_sample(circuit);
    expect_tree_matches_normal_reference_sample_of(ref, circuit);
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
    expect_tree_matches_normal_reference_sample_of(ref, circuit);
    ASSERT_EQ(ref.str(), "25*('0110')");
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
        MPAD 1 0 1 0 1 1
    )CIRCUIT");
    auto ref = ReferenceSampleTree::from_circuit_reference_sample(circuit);
    ASSERT_EQ(ref.size(), circuit.count_measurements());
    expect_tree_matches_normal_reference_sample_of(ref, circuit);
    ASSERT_EQ(ref.str(), "1*('01111110101100100011111010'+11*('1100100011111010')+1*('000000101011'))");
}

TEST(ReferenceSampleTree, feedback) {
    Circuit circuit(R"CIRCUIT(
        MPAD 0 0 1 0
        REPEAT 200 {
            CX rec[-4] 1
            M 1
        }
    )CIRCUIT");
    auto ref = ReferenceSampleTree::from_circuit_reference_sample(circuit);
    ASSERT_EQ(ref.size(), circuit.count_measurements());
    expect_tree_matches_normal_reference_sample_of(ref, circuit);
    ASSERT_EQ(ref.str(), "1*('0010'+2*('0')+4*('1')+1*('01011001000111')+12*('101011001000111'))");
}

TEST(max_feedback_lookback_in_loop, simple) {
    ASSERT_EQ(max_feedback_lookback_in_loop(Circuit()), 0);

    ASSERT_EQ(
        max_feedback_lookback_in_loop(Circuit(R"CIRCUIT(
        REPEAT 100 {
            REPEAT 100 {
                M 0
                X 0
                M 0
            }
            REPEAT 200 {
                M 0
                DETECTOR rec[-1]
            }
            X 1
            CX 1 0
        }
    )CIRCUIT")),
        0);

    ASSERT_EQ(
        max_feedback_lookback_in_loop(Circuit(R"CIRCUIT(
        CX rec[-1] 0
    )CIRCUIT")),
        1);

    ASSERT_EQ(
        max_feedback_lookback_in_loop(Circuit(R"CIRCUIT(
        CZ 0 rec[-2]
    )CIRCUIT")),
        2);

    ASSERT_EQ(
        max_feedback_lookback_in_loop(Circuit(R"CIRCUIT(
        CZ 0 rec[-2]
        CY 0 rec[-3]
    )CIRCUIT")),
        3);

    ASSERT_EQ(
        max_feedback_lookback_in_loop(Circuit(R"CIRCUIT(
        CZ 0 rec[-2]
        REPEAT 100 {
            CX rec[-5] 0
        }
    )CIRCUIT")),
        5);
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
    expect_tree_matches_normal_reference_sample_of(ref, circuit);
    ASSERT_EQ(
        ref.str(),
        "1*(''+50*('0110')+200*('0')+50*('1001')+200*('1')+50*('1001')+200*('1')+50*('0110')+200*('0')+24*(''+50*('"
        "0110')+200*('0')+50*('1001')+200*('1')+50*('1001')+200*('1')+50*('0110')+200*('0')))");
}

TEST(ReferenceSampleTree, surface_code) {
    CircuitGenParameters params(10000, 5, "rotated_memory_x");
    auto circuit = generate_surface_code_circuit(params).circuit;
    auto ref = ReferenceSampleTree::from_circuit_reference_sample(circuit);
    ASSERT_EQ(ref.str(), "1*(''+10000*('000000000000000000000000')+1*('0000000000000000000000000'))");
}

TEST(ReferenceSampleTree, surface_code_with_pauli) {
    CircuitGenParameters params(10000, 3, "rotated_memory_x");
    auto circuit = generate_surface_code_circuit(params).circuit;
    circuit.blocks[0].append_from_text("X 10 11 12 13");
    auto ref = ReferenceSampleTree::from_circuit_reference_sample(circuit);
    ASSERT_EQ(ref.str(), "1*(''+2*('00000000')+4999*('0110000000110000')+1*('000000000'))");
}

TEST(ReferenceSampleTree, surface_code_with_pauli_vs_normal_reference_sample) {
    CircuitGenParameters params(20, 3, "rotated_memory_x");
    auto circuit = generate_surface_code_circuit(params).circuit;
    circuit.blocks[0].append_from_text("X 10 11 12 13");
    auto ref = ReferenceSampleTree::from_circuit_reference_sample(circuit);
    ASSERT_EQ(ref.size(), circuit.count_measurements());
    expect_tree_matches_normal_reference_sample_of(ref, circuit);
}
