#include "stim/dem/dem_instruction.h"

#include "gtest/gtest.h"

#include "stim/dem/detector_error_model.h"

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

TEST(dem_instruction, for_separated_targets) {
    DetectorErrorModel dem("error(0.1) D0 ^ D2 L0 ^ D1 D2 D3");
    std::vector<std::vector<DemTarget>> results;
    dem.instructions[0].for_separated_targets([&](std::span<const DemTarget> group) {
        std::vector<DemTarget> items;
        for (auto g : group) {
            items.push_back(g);
        }
        results.push_back(items);
    });
    ASSERT_EQ(
        results,
        (std::vector<std::vector<DemTarget>>{
            {DemTarget::relative_detector_id(0)},
            {DemTarget::relative_detector_id(2), DemTarget::observable_id(0)},
            {DemTarget::relative_detector_id(1),
             DemTarget::relative_detector_id(2),
             DemTarget::relative_detector_id(3)},
        }));

    dem = DetectorErrorModel("error(0.1) D0");
    results.clear();
    dem.instructions[0].for_separated_targets([&](std::span<const DemTarget> group) {
        std::vector<DemTarget> items;
        for (auto g : group) {
            items.push_back(g);
        }
        results.push_back(items);
    });
    ASSERT_EQ(
        results,
        (std::vector<std::vector<DemTarget>>{
            {DemTarget::relative_detector_id(0)},
        }));

    dem = DetectorErrorModel("error(0.1)");
    results.clear();
    dem.instructions[0].for_separated_targets([&](std::span<const DemTarget> group) {
        std::vector<DemTarget> items;
        for (auto g : group) {
            items.push_back(g);
        }
        results.push_back(items);
    });
    ASSERT_EQ(
        results,
        (std::vector<std::vector<DemTarget>>{
            {},
        }));
}
