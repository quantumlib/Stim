#include <gtest/gtest.h>

#include "common_circuits.h"

TEST(common_circuits, unrotated_surface_code_program_text) {
    ASSERT_EQ(unrotated_surface_code_program_text(2),
            "REPEAT 2 {\n"
            "  H 3 5\n"
            "  CNOT 4 1 3 6 5 8\n"
            "  CNOT 2 1 8 7 3 4\n"
            "  CNOT 0 1 6 7 5 4\n"
            "  CNOT 4 7 3 0 5 2\n"
            "  H 3 5\n"
            "  M 1 7 3 5\n"
            "}\n"
            "M 0 2 4 6 8\n");
}
