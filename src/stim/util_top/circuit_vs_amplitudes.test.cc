#include "stim/util_top/circuit_vs_amplitudes.h"

#include "gtest/gtest.h"

#include "stim/simulators/tableau_simulator.h"
#include "stim/util_bot/test_util.test.h"

using namespace stim;

TEST(conversions, stabilizer_state_vector_to_circuit_basic) {
    ASSERT_THROW(stabilizer_state_vector_to_circuit({}, false), std::invalid_argument);

    ASSERT_THROW(
        stabilizer_state_vector_to_circuit(
            {
                {0},
            },
            false),
        std::invalid_argument);

    ASSERT_EQ(
        stabilizer_state_vector_to_circuit(
            {
                {1},
            },
            false),
        stim::Circuit(R"CIRCUIT(
    )CIRCUIT"));

    ASSERT_EQ(
        stabilizer_state_vector_to_circuit(
            {
                {-1},
            },
            false),
        stim::Circuit(R"CIRCUIT(
    )CIRCUIT"));

    ASSERT_EQ(
        stabilizer_state_vector_to_circuit(
            {
                {0, 1},
                {0},
            },
            false),
        stim::Circuit(R"CIRCUIT(
        I 0
    )CIRCUIT"));

    ASSERT_EQ(
        stabilizer_state_vector_to_circuit(
            {
                {0},
                {1},
            },
            false),
        stim::Circuit(R"CIRCUIT(
        X 0
    )CIRCUIT"));

    ASSERT_EQ(
        stabilizer_state_vector_to_circuit(
            {
                {sqrtf(0.5)},
                {sqrtf(0.5)},
            },
            false),
        stim::Circuit(R"CIRCUIT(
        H 0
    )CIRCUIT"));

    ASSERT_EQ(
        stabilizer_state_vector_to_circuit(
            {
                {0},
                {1},
                {0},
                {0},
            },
            false),
        stim::Circuit(R"CIRCUIT(
        X 1
    )CIRCUIT"));

    ASSERT_EQ(
        stabilizer_state_vector_to_circuit(
            {
                {0},
                {0},
                {1},
                {0},
            },
            false),
        stim::Circuit(R"CIRCUIT(
        X 0
        I 1
    )CIRCUIT"));

    ASSERT_EQ(
        stabilizer_state_vector_to_circuit(
            {
                {0},
                {1},
                {0},
                {0},
            },
            true),
        stim::Circuit(R"CIRCUIT(
        X 0
        I 1
    )CIRCUIT"));

    ASSERT_EQ(
        stabilizer_state_vector_to_circuit(
            {
                {0},
                {0},
                {1},
                {0},
            },
            true),
        stim::Circuit(R"CIRCUIT(
        X 1
    )CIRCUIT"));

    ASSERT_EQ(
        stabilizer_state_vector_to_circuit(
            {
                {sqrtf(0.5)},
                {0, sqrtf(0.5)},
            },
            true),
        stim::Circuit(R"CIRCUIT(
        H 0
        S 0
    )CIRCUIT"));

    ASSERT_EQ(
        stabilizer_state_vector_to_circuit(
            {
                {sqrtf(0.5)},
                {0, -sqrtf(0.5)},
            },
            true),
        stim::Circuit(R"CIRCUIT(
        H 0
        S_DAG 0
    )CIRCUIT"));

    ASSERT_EQ(
        stabilizer_state_vector_to_circuit(
            {
                {sqrtf(0.5)},
                {-sqrtf(0.5)},
            },
            true),
        stim::Circuit(R"CIRCUIT(
        H 0
        Z 0
    )CIRCUIT"));
}

TEST(conversions, stabilizer_state_vector_to_circuit_fuzz_round_trip) {
    auto rng = INDEPENDENT_TEST_RNG();
    for (const auto &little_endian : std::vector<bool>{false, true}) {
        for (size_t n = 0; n < 5; n++) {
            // Pick a random stabilizer state.
            TableauSimulator<64> sim(INDEPENDENT_TEST_RNG(), n);
            sim.inv_state = Tableau<64>::random(n, rng);
            auto desired_vec = sim.to_state_vector(little_endian);

            // Round trip through a circuit.
            auto circuit = stabilizer_state_vector_to_circuit(desired_vec, little_endian);
            auto actual_vec = circuit_to_output_state_vector(circuit, little_endian);
            ASSERT_EQ(actual_vec, desired_vec) << "little_endian=" << little_endian << ", n=" << n;
        }
    }
}

TEST(conversions, stabilizer_state_vector_to_circuit_unnormalized_fuzz_round_trip) {
    auto rng = INDEPENDENT_TEST_RNG();
    auto little_endian = true;

    for (size_t i = 0; i < 100; i++) {
        // Pick a random stabilizer state.
        size_t n = i % 5;
        TableauSimulator<64> sim(INDEPENDENT_TEST_RNG(), n);
        sim.inv_state = Tableau<64>::random(n, rng);
        auto desired_vec = sim.to_state_vector(little_endian);

        // Unnormalize by multiplying by a random non-zero factor.
        auto scaled_vec = desired_vec;
        std::uniform_real_distribution<float> dist(-1000.0, +1000.0);
        std::complex<float> scale = {dist(rng), dist(rng)};
        while (std::norm(scale) < 0.01) {
            scale = {dist(rng), dist(rng)};
        }
        for (auto &c : scaled_vec) {
            c *= scale;
        }

        // Round trip through a circuit.
        auto circuit = stabilizer_state_vector_to_circuit(scaled_vec, little_endian);
        auto actual_vec = circuit_to_output_state_vector(circuit, little_endian);
        ASSERT_EQ(actual_vec, desired_vec) << " scale=" << scale;
    }
}

TEST(conversions, circuit_to_output_state_vector) {
    ASSERT_EQ(circuit_to_output_state_vector(Circuit(""), false), (std::vector<std::complex<float>>{{1}}));
    ASSERT_EQ(
        circuit_to_output_state_vector(Circuit("H 0 1"), false),
        (std::vector<std::complex<float>>{{0.5}, {0.5}, {0.5}, {0.5}}));
    ASSERT_EQ(
        circuit_to_output_state_vector(Circuit("X 1"), false), (std::vector<std::complex<float>>{{0}, {1}, {0}, {0}}));
    ASSERT_EQ(
        circuit_to_output_state_vector(Circuit("X 1"), true), (std::vector<std::complex<float>>{{0}, {0}, {1}, {0}}));
}
