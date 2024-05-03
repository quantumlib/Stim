#include "stim/diagram/base64.h"

#include "gtest/gtest.h"

using namespace stim_draw_internal;

TEST(base64, write_base64) {
    auto f = [](std::string_view c) {
        std::stringstream ss;
        write_data_as_base64_to(c, ss);
        return ss.str();
    };

    EXPECT_EQ(f("light work."), "bGlnaHQgd29yay4=");
    EXPECT_EQ(f("light work"), "bGlnaHQgd29yaw==");
    EXPECT_EQ(f("light wor"), "bGlnaHQgd29y");
    EXPECT_EQ(f("light wo"), "bGlnaHQgd28=");
    EXPECT_EQ(f("light w"), "bGlnaHQgdw==");
    EXPECT_EQ(f(""), "");
    EXPECT_EQ(f("f"), "Zg==");
    EXPECT_EQ(f("fo"), "Zm8=");
    EXPECT_EQ(f("foo"), "Zm9v");
    EXPECT_EQ(f("foob"), "Zm9vYg==");
    EXPECT_EQ(f("fooba"), "Zm9vYmE=");
    EXPECT_EQ(f("foobar"), "Zm9vYmFy");
}
