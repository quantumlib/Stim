#include "stim/util_top/circuit_to_dem.h"

#include "gtest/gtest.h"

using namespace stim;

TEST(circuit_to_dem, heralded_noise_basis) {
    ASSERT_EQ(
        circuit_to_dem(Circuit(R"CIRCUIT(
            MXX 0 1
            MZZ 0 1
            HERALDED_PAULI_CHANNEL_1(0.25, 0, 0, 0) 0
            MXX 0 1
            MZZ 0 1
            DETECTOR(2) rec[-3]
            DETECTOR(3) rec[-2] rec[-5]
            DETECTOR(5) rec[-1] rec[-4]
        )CIRCUIT")),
        DetectorErrorModel(R"DEM(
            error(0.25) D0
            detector(2) D0
            detector(3) D1
            detector(5) D2
        )DEM"));

    ASSERT_EQ(
        circuit_to_dem(Circuit(R"CIRCUIT(
            MXX 0 1
            MZZ 0 1
            HERALDED_PAULI_CHANNEL_1(0, 0.25, 0, 0) 0
            MXX 0 1
            MZZ 0 1
            DETECTOR(2) rec[-3]
            DETECTOR(3) rec[-2] rec[-5]
            DETECTOR(5) rec[-1] rec[-4]
        )CIRCUIT")),
        DetectorErrorModel(R"DEM(
            error(0.25) D0 D2
            detector(2) D0
            detector(3) D1
            detector(5) D2
        )DEM"));

    ASSERT_EQ(
        circuit_to_dem(Circuit(R"CIRCUIT(
            MXX 0 1
            MZZ 0 1
            HERALDED_PAULI_CHANNEL_1(0, 0, 0.25, 0) 0
            MXX 0 1
            MZZ 0 1
            DETECTOR(2) rec[-3]
            DETECTOR(3) rec[-2] rec[-5]
            DETECTOR(5) rec[-1] rec[-4]
        )CIRCUIT")),
        DetectorErrorModel(R"DEM(
            error(0.25) D0 D1 D2
            detector(2) D0
            detector(3) D1
            detector(5) D2
        )DEM"));

    ASSERT_EQ(
        circuit_to_dem(Circuit(R"CIRCUIT(
            MXX 0 1
            MZZ 0 1
            HERALDED_PAULI_CHANNEL_1(0, 0, 0, 0.25) 0
            MXX 0 1
            MZZ 0 1
            DETECTOR(2) rec[-3]
            DETECTOR(3) rec[-2] rec[-5]
            DETECTOR(5) rec[-1] rec[-4]
        )CIRCUIT")),
        DetectorErrorModel(R"DEM(
            error(0.25) D0 D1
            detector(2) D0
            detector(3) D1
            detector(5) D2
        )DEM"));

    ASSERT_EQ(
        circuit_to_dem(
            Circuit(R"CIRCUIT(
            MXX 0 1
            MZZ 0 1
            HERALDED_PAULI_CHANNEL_1(0.125, 0, 0.25, 0) 0
            MXX 0 1
            MZZ 0 1
            DETECTOR(2) rec[-3]
            DETECTOR(3) rec[-2] rec[-5]
            DETECTOR(5) rec[-1] rec[-4]
        )CIRCUIT"),
            {.approximate_disjoint_errors_threshold = 1}),
        DetectorErrorModel(R"DEM(
            error(0.125) D0
            error(0.25) D0 D1 D2
            detector(2) D0
            detector(3) D1
            detector(5) D2
        )DEM"));

    ASSERT_THROW(
        {
            circuit_to_dem(Circuit(R"CIRCUIT(
            MXX 0 1
            MZZ 0 1
            HERALDED_PAULI_CHANNEL_1(0.125, 0, 0.25, 0) 0
            MXX 0 1
            MZZ 0 1
            DETECTOR(2) rec[-3]
            DETECTOR(3) rec[-2] rec[-5]
            DETECTOR(5) rec[-1] rec[-4]
        )CIRCUIT"));
        },
        std::invalid_argument);
}
