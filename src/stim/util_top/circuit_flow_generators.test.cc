#include "stim/util_top/circuit_flow_generators.h"

#include "gtest/gtest.h"

#include "stim/circuit/circuit.test.h"
#include "stim/gen/gen_surface_code.h"
#include "stim/mem/simd_word.test.h"
#include "stim/util_top/circuit_inverse_qec.h"
#include "stim/util_top/has_flow.h"

using namespace stim;

TEST_EACH_WORD_SIZE_W(circuit_flow_generators, various, {
    EXPECT_EQ(
        circuit_flow_generators<W>(Circuit(R"CIRCUIT(
        )CIRCUIT")),
        (std::vector<Flow<W>>{}));

    EXPECT_EQ(
        circuit_flow_generators<W>(Circuit(R"CIRCUIT(
            X 0
        )CIRCUIT")),
        (std::vector<Flow<W>>{
            Flow<W>::from_str("X -> X"),
            Flow<W>::from_str("Z -> -Z"),
        }));

    EXPECT_EQ(
        circuit_flow_generators<W>(Circuit(R"CIRCUIT(
            H 0
        )CIRCUIT")),
        (std::vector<Flow<W>>{
            Flow<W>::from_str("X -> Z"),
            Flow<W>::from_str("Z -> X"),
        }));

    EXPECT_EQ(
        circuit_flow_generators<W>(Circuit(R"CIRCUIT(
            M 0
        )CIRCUIT")),
        (std::vector<Flow<W>>{
            Flow<W>::from_str("1 -> Z xor rec[0]"),
            Flow<W>::from_str("Z -> rec[0]"),
        }));

    EXPECT_EQ(
        circuit_flow_generators<W>(Circuit(R"CIRCUIT(
            M 0 0
        )CIRCUIT")),
        (std::vector<Flow<W>>{
            Flow<W>::from_str("1 -> rec[0] xor rec[1]"),
            Flow<W>::from_str("1 -> Z xor rec[1]"),
            Flow<W>::from_str("Z -> rec[1]"),
        }));

    EXPECT_EQ(
        circuit_flow_generators<W>(Circuit(R"CIRCUIT(
            MXX 2 0
        )CIRCUIT")),
        (std::vector<Flow<W>>{
            Flow<W>::from_str("1 -> X_X xor rec[0]"),
            Flow<W>::from_str("__X -> __X"),
            Flow<W>::from_str("_X_ -> _X_"),
            Flow<W>::from_str("_Z_ -> _Z_"),
            Flow<W>::from_str("X__ -> __X xor rec[0]"),
            Flow<W>::from_str("Z_Z -> Z_Z"),
        }));

    EXPECT_EQ(
        circuit_flow_generators<W>(Circuit(R"CIRCUIT(
            MYY 3 1 2 3
        )CIRCUIT")),
        (std::vector<Flow<W>>{
            Flow<W>::from_str("1 -> __YY xor rec[1]"),
            Flow<W>::from_str("1 -> _Y_Y xor rec[0]"),
            Flow<W>::from_str("___Y -> ___Y"),
            Flow<W>::from_str("__Y_ -> ___Y xor rec[1]"),
            Flow<W>::from_str("_XZZ -> _ZZX xor rec[0]"),
            Flow<W>::from_str("_ZZZ -> _ZZZ"),
            Flow<W>::from_str("X___ -> X___"),
            Flow<W>::from_str("Z___ -> Z___"),
        }));

    EXPECT_EQ(
        circuit_flow_generators<W>(Circuit(R"CIRCUIT(
            MZZ 3 1 2 3
        )CIRCUIT")),
        (std::vector<Flow<W>>{
            Flow<W>::from_str("1 -> __ZZ xor rec[1]"),
            Flow<W>::from_str("1 -> _Z_Z xor rec[0]"),
            Flow<W>::from_str("___Z -> ___Z"),
            Flow<W>::from_str("__Z_ -> ___Z xor rec[1]"),
            Flow<W>::from_str("_XXX -> _XXX"),
            Flow<W>::from_str("_Z__ -> ___Z xor rec[0]"),
            Flow<W>::from_str("X___ -> X___"),
            Flow<W>::from_str("Z___ -> Z___"),
        }));

    EXPECT_EQ(
        circuit_flow_generators<W>(Circuit(R"CIRCUIT(
            ISWAP 3 1 2 3
        )CIRCUIT")),
        (std::vector<Flow<W>>{
            Flow<W>::from_str("___X -> _YZ_"),
            Flow<W>::from_str("___Z -> _Z__"),
            Flow<W>::from_str("__X_ -> __ZY"),
            Flow<W>::from_str("__Z_ -> ___Z"),
            Flow<W>::from_str("_X__ -> -_ZXZ"),
            Flow<W>::from_str("_Z__ -> __Z_"),
            Flow<W>::from_str("X___ -> X___"),
            Flow<W>::from_str("Z___ -> Z___"),
        }));

    EXPECT_EQ(
        circuit_flow_generators<W>(Circuit(R"CIRCUIT(
            S 0
        )CIRCUIT")),
        (std::vector<Flow<W>>{
            Flow<W>::from_str("X -> Y"),
            Flow<W>::from_str("Z -> Z"),
        }));

    EXPECT_EQ(
        circuit_flow_generators<W>(Circuit(R"CIRCUIT(
            S_DAG 0
        )CIRCUIT")),
        (std::vector<Flow<W>>{
            Flow<W>::from_str("X -> -Y"),
            Flow<W>::from_str("Z -> Z"),
        }));

    EXPECT_EQ(
        circuit_flow_generators<W>(Circuit(R"CIRCUIT(
            SPP Z0
        )CIRCUIT")),
        (std::vector<Flow<W>>{
            Flow<W>::from_str("X -> Y"),
            Flow<W>::from_str("Z -> Z"),
        }));

    EXPECT_EQ(
        circuit_flow_generators<W>(Circuit(R"CIRCUIT(
            SQRT_X 0
            S 0
        )CIRCUIT")),
        (std::vector<Flow<W>>{
            Flow<W>::from_str("X -> Y"),
            Flow<W>::from_str("Z -> X"),
        }));
    EXPECT_EQ(
        circuit_flow_generators<W>(Circuit(R"CIRCUIT(
            SPP X0 Z0
        )CIRCUIT")),
        (std::vector<Flow<W>>{
            Flow<W>::from_str("X -> Y"),
            Flow<W>::from_str("Z -> X"),
        }));

    EXPECT_EQ(
        circuit_flow_generators<W>(Circuit(R"CIRCUIT(
            SPP X0*X1
        )CIRCUIT")),
        (std::vector<Flow<W>>{
            Flow<W>::from_str("_X -> _X"),
            Flow<W>::from_str("_Z -> -XY"),
            Flow<W>::from_str("X_ -> X_"),
            Flow<W>::from_str("Z_ -> -YX"),
        }));

    EXPECT_EQ(
        circuit_flow_generators<W>(Circuit(R"CIRCUIT(
            SPP_DAG Z0
        )CIRCUIT")),
        (std::vector<Flow<W>>{
            Flow<W>::from_str("X -> -Y"),
            Flow<W>::from_str("Z -> Z"),
        }));

    EXPECT_EQ(
        circuit_flow_generators<W>(Circuit(R"CIRCUIT(
            M 0
            CX rec[-1] 0
        )CIRCUIT")),
        (std::vector<Flow<W>>{
            Flow<W>::from_str("1 -> Z"),
            Flow<W>::from_str("Z -> rec[0]"),
        }));

    EXPECT_EQ(
        circuit_flow_generators<W>(Circuit(R"CIRCUIT(
            R 0
        )CIRCUIT")),
        (std::vector<Flow<W>>{
            Flow<W>::from_str("1 -> Z"),
        }));

    EXPECT_EQ(
        circuit_flow_generators<W>(Circuit(R"CIRCUIT(
            MR 0
        )CIRCUIT")),
        (std::vector<Flow<W>>{
            Flow<W>::from_str("1 -> Z"),
            Flow<W>::from_str("Z -> rec[0]"),
        }));

    EXPECT_EQ(
        circuit_flow_generators<W>(Circuit(R"CIRCUIT(
            M 0
            XCZ 0 rec[-1]
        )CIRCUIT")),
        (std::vector<Flow<W>>{
            Flow<W>::from_str("1 -> Z"),
            Flow<W>::from_str("Z -> rec[0]"),
        }));

    EXPECT_EQ(
        circuit_flow_generators<W>(Circuit(R"CIRCUIT(
            MPAD 0 1 1 0
        )CIRCUIT")),
        (std::vector<Flow<W>>{
            Flow<W>::from_str("1 -> rec[0]"),
            Flow<W>::from_str("1 -> rec[3]"),
            Flow<W>::from_str("1 -> -rec[1]"),
            Flow<W>::from_str("1 -> -rec[2]"),
        }));

    EXPECT_EQ(
        circuit_flow_generators<W>(Circuit(R"CIRCUIT(
            M 0
            CY rec[-1] 1
        )CIRCUIT")),
        (std::vector<Flow<W>>{
            Flow<W>::from_str("1 -> Z_ xor rec[0]"),
            Flow<W>::from_str("_X -> _X xor rec[0]"),
            Flow<W>::from_str("_Z -> _Z xor rec[0]"),
            Flow<W>::from_str("Z_ -> rec[0]"),
        }));

    EXPECT_EQ(
        circuit_flow_generators<W>(Circuit(R"CIRCUIT(
            HERALDED_ERASE(0.04) 1
            HERALDED_PAULI_CHANNEL_1(0.01, 0.02, 0.03, 0.04) 1
            TICK
            MPP X0*Y1*Z2 Z0*Z1
        )CIRCUIT")),
        (std::vector<Flow<W>>{
            Flow<W>::from_str("1 -> rec[0]"),
            Flow<W>::from_str("1 -> rec[1]"),
            Flow<W>::from_str("1 -> XYZ xor rec[2]"),
            Flow<W>::from_str("1 -> ZZ_ xor rec[3]"),
            Flow<W>::from_str("__Z -> __Z"),
            Flow<W>::from_str("_ZX -> _ZX"),
            Flow<W>::from_str("XXX -> _ZY xor rec[2]"),
            Flow<W>::from_str("Z_X -> _ZX xor rec[3]"),
        }));
})

TEST_EACH_WORD_SIZE_W(circuit_flow_generators, all_operations, {
    auto circuit = generate_test_circuit_with_all_operations();
    auto generators = circuit_flow_generators<W>(circuit);
    auto rng = externally_seeded_rng();
    auto passes = sample_if_circuit_has_stabilizer_flows<W>(256, rng, circuit, generators);
    for (size_t k = 0; k < passes.size(); k++) {
        EXPECT_TRUE(passes[k]) << k << ": " << generators[k];
    }
})
