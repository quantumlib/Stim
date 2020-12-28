#include "gtest/gtest.h"
#include "tableau.h"

TEST(tableau, identity) {
    auto t = Tableau::identity(4);
    ASSERT_EQ(t.str(), ""
                       "Tableau {\n"
                       "  qubit 0_x: +X___\n"
                       "  qubit 0_y: +Y___\n"
                       "  qubit 1_x: +_X__\n"
                       "  qubit 1_y: +_Y__\n"
                       "  qubit 2_x: +__X_\n"
                       "  qubit 2_y: +__Y_\n"
                       "  qubit 3_x: +___X\n"
                       "  qubit 3_y: +___Y\n"
                       "}");
}

TEST(tableau, gate_data) {
    ASSERT_EQ(GATE_TABLEAUS.at("H").str(),
              "Tableau {\n"
              "  qubit 0_x: +Z\n"
              "  qubit 0_y: -Y\n"
              "}");
}
