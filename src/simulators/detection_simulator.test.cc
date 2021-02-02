#include "detection_simulator.h"

#include <gtest/gtest.h>

#include "../test_util.test.h"

TEST(DetectionSimulator, detector_samples) {
    auto circuit = Circuit::from_text(
        "X_ERROR(1) 0\n"
        "M 0\n"
        "DETECTOR 0@-1\n");
    auto samples = detector_samples(circuit, 5, false, false, SHARED_TEST_RNG());
    ASSERT_EQ(
        samples.str(5),
        "11111\n"
        ".....\n"
        ".....\n"
        ".....\n"
        ".....");
}
