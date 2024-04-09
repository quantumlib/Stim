#include "stim/util_top/dem_to_matrix.h"

#include "gtest/gtest.h"

using namespace stim;

TEST(dem_to_matrix, xor_sort) {
    std::vector<uint8_t> vals;
    size_t v;

    vals = {};
    v = xor_sort<uint8_t>(vals);
    ASSERT_EQ(v, 0);
    ASSERT_EQ(vals, (std::vector<uint8_t>{}));

    vals = {5};
    v = xor_sort<uint8_t>(vals);
    ASSERT_EQ(v, 1);
    ASSERT_EQ(vals, (std::vector<uint8_t>{5}));

    vals = {5, 6};
    v = xor_sort<uint8_t>(vals);
    ASSERT_EQ(v, 2);
    ASSERT_EQ(vals, (std::vector<uint8_t>{5, 6}));

    vals = {6, 5};
    v = xor_sort<uint8_t>(vals);
    ASSERT_EQ(v, 2);
    ASSERT_EQ(vals, (std::vector<uint8_t>{5, 6}));

    vals = {5, 5};
    v = xor_sort<uint8_t>(vals);
    ASSERT_EQ(v, 0);

    vals = {5, 5, 5};
    v = xor_sort<uint8_t>(vals);
    ASSERT_EQ(v, 1);
    while (vals.size() > v) {
        vals.pop_back();
    }
    ASSERT_EQ(vals, (std::vector<uint8_t>{5}));

    vals = {5, 6, 5, 6, 5};
    v = xor_sort<uint8_t>(vals);
    ASSERT_EQ(v, 1);
    while (vals.size() > v) {
        vals.pop_back();
    }
    ASSERT_EQ(vals, (std::vector<uint8_t>{5}));

    vals = {5, 6, 5, 6, 5, 2, 3, 5};
    v = xor_sort<uint8_t>(vals);
    ASSERT_EQ(v, 2);
    while (vals.size() > v) {
        vals.pop_back();
    }
    ASSERT_EQ(vals, (std::vector<uint8_t>{2, 3}));
}
