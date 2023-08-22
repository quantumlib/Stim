#include "stim/diagram/coord.h"

#include "gtest/gtest.h"

#include "stim/diagram/ascii_diagram.h"

using namespace stim_draw_internal;

TEST(coord, arithmetic) {
    ASSERT_EQ((Coord<2>{2, 3} + Coord<2>{5, 7}), (Coord<2>{7, 10}));
    ASSERT_EQ((Coord<2>{2, 3} - Coord<2>{5, 7}), (Coord<2>{-3, -4}));
    ASSERT_EQ((Coord<2>{2, 3} * 5), (Coord<2>{10, 15}));
    ASSERT_EQ((Coord<2>{2, 3} / 8), (Coord<2>{0.25, 0.375}));
    ASSERT_EQ((Coord<2>{2, 3}.dot(Coord<2>{5, 7})), 2 * 5 + 3 * 7);
    ASSERT_EQ((Coord<2>{2, 3}.norm2()), 13);
    ASSERT_EQ((Coord<2>{3, 4}.norm()), 5);
    ASSERT_TRUE((Coord<2>{3, 4} == Coord<2>{3, 4}));
    ASSERT_FALSE((Coord<2>{3, 4} == Coord<2>{2, 4}));
    ASSERT_FALSE((Coord<2>{3, 4} == Coord<2>{3, 2}));

    ASSERT_FALSE((Coord<2>{3, 4} < Coord<2>{3, 4}));
    ASSERT_FALSE((Coord<2>{3, 4} < Coord<2>{2, 4}));
    ASSERT_TRUE((Coord<2>{3, 4} < Coord<2>{4, 4}));

    ASSERT_TRUE((Coord<2>{3, 4} < Coord<2>{3, 6}));
    ASSERT_FALSE((Coord<2>{3, 4} < Coord<2>{3, 2}));

    ASSERT_TRUE((Coord<2>{3, 4} < Coord<2>{10, 0}));
    ASSERT_FALSE((Coord<2>{3, 4} < Coord<2>{0, 10}));
}

TEST(coord, min_max) {
    std::vector<Coord<2>> a{
        {1, 10},
        {10, 30},
        {50, 5},
    };
    ASSERT_EQ((Coord<2>::min_max(a)), (std::pair<Coord<2>, Coord<2>>{{1, 5}, {50, 30}}));
}
