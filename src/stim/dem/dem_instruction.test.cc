#include "stim/dem/dem_instruction.h"

#include "gtest/gtest.h"

using namespace stim;

TEST(dem_instruction, from_str) {
    ASSERT_EQ(DemTarget::from_text("D5"), DemTarget::relative_detector_id(5));
    ASSERT_EQ(DemTarget::from_text("D0"), DemTarget::relative_detector_id(0));
    ASSERT_EQ(DemTarget::from_text("D4611686018427387903"), DemTarget::relative_detector_id(4611686018427387903));

    ASSERT_EQ(DemTarget::from_text("L5"), DemTarget::observable_id(5));
    ASSERT_EQ(DemTarget::from_text("L0"), DemTarget::observable_id(0));
    ASSERT_EQ(DemTarget::from_text("L4294967295"), DemTarget::observable_id(4294967295));

    ASSERT_THROW({ DemTarget::from_text("D4611686018427387904"); }, std::invalid_argument);
    ASSERT_THROW({ DemTarget::from_text("L4294967296"); }, std::invalid_argument);
    ASSERT_THROW({ DemTarget::from_text("L-1"); }, std::invalid_argument);
    ASSERT_THROW({ DemTarget::from_text("L-1"); }, std::invalid_argument);
    ASSERT_THROW({ DemTarget::from_text("D-1"); }, std::invalid_argument);
    ASSERT_THROW({ DemTarget::from_text("Da"); }, std::invalid_argument);
    ASSERT_THROW({ DemTarget::from_text("Da "); }, std::invalid_argument);
    ASSERT_THROW({ DemTarget::from_text(" Da"); }, std::invalid_argument);
    ASSERT_THROW({ DemTarget::from_text("X"); }, std::invalid_argument);
    ASSERT_THROW({ DemTarget::from_text(""); }, std::invalid_argument);
    ASSERT_THROW({ DemTarget::from_text("1"); }, std::invalid_argument);
    ASSERT_THROW({ DemTarget::from_text("-1"); }, std::invalid_argument);
    ASSERT_THROW({ DemTarget::from_text("0"); }, std::invalid_argument);
    ASSERT_THROW({ DemTarget::from_text("'"); }, std::invalid_argument);
    ASSERT_THROW({ DemTarget::from_text(" "); }, std::invalid_argument);
}
