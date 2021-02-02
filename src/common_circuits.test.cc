#include "common_circuits.h"

#include <gtest/gtest.h>

#include "circuit.h"

TEST(common_circuits, unrotated_surface_code_program_text) {
    auto circuit = Circuit::from_text(unrotated_surface_code_program_text(5, 4, 0.001));
    ASSERT_EQ(circuit.detectors.size(), 5 * 4 * 2 * 4);
}
