#include "gtest/gtest.h"

#include "fixed_cap_vector.h"

using namespace stim_internal;

TEST(FixedCapVector, basic_usage) {
    auto v = FixedCapVector<std::string, 5>();
    const auto &c = v;

    ASSERT_EQ(v.empty(), true);
    ASSERT_EQ(v.size(), 0);
    ASSERT_EQ(v.begin(), v.end());
    ASSERT_EQ(v.find("x"), v.end());
    ASSERT_THROW({
        v.front();
    }, std::out_of_range);
    ASSERT_THROW({
        v.back();
    }, std::out_of_range);
    ASSERT_THROW({
        c.front();
    }, std::out_of_range);
    ASSERT_THROW({
        c.back();
    }, std::out_of_range);

    v.push_back("test");
    ASSERT_EQ(v.empty(), false);
    ASSERT_EQ(v.size(), 1);
    ASSERT_EQ(*v.begin(), "test");
    ASSERT_EQ(v.front(), "test");
    ASSERT_EQ(v.back(), "test");
    ASSERT_EQ(v.begin() + 1, v.end());
    ASSERT_EQ(v.find("test"), v.begin());
    ASSERT_EQ(v.find("other"), v.end());
    ASSERT_EQ(v[0], "test");

    ASSERT_EQ(c.empty(), false);
    ASSERT_EQ(c.size(), 1);
    ASSERT_EQ(*c.begin(), "test");
    ASSERT_EQ(c.front(), "test");
    ASSERT_EQ(c.back(), "test");
    ASSERT_EQ(c.begin() + 1, c.end());
    ASSERT_EQ(c.find("test"), c.begin());
    ASSERT_EQ(c.find("other"), c.end());
    ASSERT_EQ(c[0], "test");

    v.push_back("1");
    v.push_back("2");
    v.push_back("3");
    v.push_back("4");
    ASSERT_THROW({
        v.push_back("5");
    }, std::out_of_range);

    ASSERT_EQ(v.find("2"), v.begin() + 2);

    v[0] = "new";
    ASSERT_EQ(c[0], "new");
}

TEST(FixedCapVector, push_pop) {
    auto v = FixedCapVector<std::vector<int>, 5>();
    std::vector<int> v2{1, 2, 3};

    v.push_back(v2);
    ASSERT_EQ(v2, (std::vector<int>{1, 2, 3}));
    ASSERT_EQ(v, (FixedCapVector<std::vector<int>, 5>{std::vector<int>{1, 2, 3}}));

    v2.push_back(4);
    v.push_back(std::move(v2));
    ASSERT_EQ(v2, (std::vector<int>{}));
    ASSERT_EQ(v, (FixedCapVector<std::vector<int>, 5>{
        std::vector<int>{1, 2, 3},
        std::vector<int>{1, 2, 3, 4},
    }));

    v.pop_back();
    ASSERT_EQ(v, (FixedCapVector<std::vector<int>, 5>{
        std::vector<int>{1, 2, 3},
    }));

    v.pop_back();
    ASSERT_EQ(v, (FixedCapVector<std::vector<int>, 5>{}));

    ASSERT_THROW({
        v.pop_back();
    }, std::out_of_range);
}

TEST(FixedCapVector, ordering) {
    auto v123 = FixedCapVector<int, 3>{1, 2, 3};
    auto v12 = FixedCapVector<int, 3>{1, 2};
    auto w12 = FixedCapVector<int, 3>{1, 2};
    auto v423 = FixedCapVector<int, 3>{4, 2, 3};
    ASSERT_LT(v123, v423);
    ASSERT_LT(v12, v123);
    ASSERT_TRUE(v12 == w12);
    ASSERT_TRUE(!(v12 != w12));
    ASSERT_TRUE(v12 != v123);
    ASSERT_TRUE(!(v12 == v123));
    ASSERT_TRUE(v423 != v123);
    ASSERT_TRUE(!(v423 == v123));
}
