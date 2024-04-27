#include "stim/util_bot/error_decomp.h"

#include "gtest/gtest.h"

#include "stim/util_bot/test_util.test.h"

using namespace stim;

TEST(conversions, independent_to_disjoint_xyz_errors) {
    double out_x;
    double out_y;
    double out_z;

    independent_to_disjoint_xyz_errors(0.5, 0.5, 0.5, &out_x, &out_y, &out_z);
    ASSERT_NEAR(out_x, 1 / 4.0, 1e-6);
    ASSERT_NEAR(out_y, 1 / 4.0, 1e-6);
    ASSERT_NEAR(out_z, 1 / 4.0, 1e-6);

    independent_to_disjoint_xyz_errors(0.1, 0, 0, &out_x, &out_y, &out_z);
    ASSERT_NEAR(out_x, 0.1, 1e-6);
    ASSERT_NEAR(out_y, 0, 1e-6);
    ASSERT_NEAR(out_z, 0, 1e-6);

    independent_to_disjoint_xyz_errors(0, 0.2, 0, &out_x, &out_y, &out_z);
    ASSERT_NEAR(out_x, 0, 1e-6);
    ASSERT_NEAR(out_y, 0.2, 1e-6);
    ASSERT_NEAR(out_z, 0, 1e-6);

    independent_to_disjoint_xyz_errors(0, 0, 0.05, &out_x, &out_y, &out_z);
    ASSERT_NEAR(out_x, 0, 1e-6);
    ASSERT_NEAR(out_y, 0, 1e-6);
    ASSERT_NEAR(out_z, 0.05, 1e-6);

    independent_to_disjoint_xyz_errors(0.1, 0.1, 0, &out_x, &out_y, &out_z);
    ASSERT_NEAR(out_x, 0.1 - 0.01, 1e-6);
    ASSERT_NEAR(out_y, 0.1 - 0.01, 1e-6);
    ASSERT_NEAR(out_z, 0.01, 1e-6);
}

TEST(conversions, disjoint_to_independent_xyz_errors_approx) {
    double out_x;
    double out_y;
    double out_z;

    ASSERT_TRUE(try_disjoint_to_independent_xyz_errors_approx(0.4, 0.0, 0.0, &out_x, &out_y, &out_z));
    ASSERT_NEAR(out_x, 0.4, 1e-6);
    ASSERT_NEAR(out_y, 0.0, 1e-6);
    ASSERT_NEAR(out_z, 0.0, 1e-6);

    ASSERT_TRUE(try_disjoint_to_independent_xyz_errors_approx(0.5, 0.0, 0.0, &out_x, &out_y, &out_z));
    ASSERT_NEAR(out_x, 0.5, 1e-6);
    ASSERT_NEAR(out_y, 0.0, 1e-6);
    ASSERT_NEAR(out_z, 0.0, 1e-6);

    ASSERT_TRUE(try_disjoint_to_independent_xyz_errors_approx(0.6, 0.0, 0.0, &out_x, &out_y, &out_z));
    ASSERT_NEAR(out_x, 0.6, 1e-6);
    ASSERT_NEAR(out_y, 0.0, 1e-6);
    ASSERT_NEAR(out_z, 0.0, 1e-6);

    ASSERT_TRUE(try_disjoint_to_independent_xyz_errors_approx(0.25, 0.25, 0.25, &out_x, &out_y, &out_z));
    ASSERT_NEAR(out_x, 0.5, 1e-6);
    ASSERT_NEAR(out_y, 0.5, 1e-6);
    ASSERT_NEAR(out_z, 0.5, 1e-6);

    ASSERT_TRUE(try_disjoint_to_independent_xyz_errors_approx(0.1, 0, 0, &out_x, &out_y, &out_z));
    ASSERT_NEAR(out_x, 0.1, 1e-6);
    ASSERT_NEAR(out_y, 0, 1e-6);
    ASSERT_NEAR(out_z, 0, 1e-6);

    ASSERT_TRUE(try_disjoint_to_independent_xyz_errors_approx(0, 0.2, 0, &out_x, &out_y, &out_z));
    ASSERT_NEAR(out_x, 0, 1e-6);
    ASSERT_NEAR(out_y, 0.2, 1e-6);
    ASSERT_NEAR(out_z, 0, 1e-6);

    ASSERT_TRUE(try_disjoint_to_independent_xyz_errors_approx(0, 0, 0.05, &out_x, &out_y, &out_z));
    ASSERT_NEAR(out_x, 0, 1e-6);
    ASSERT_NEAR(out_y, 0, 1e-6);
    ASSERT_NEAR(out_z, 0.05, 1e-6);

    ASSERT_TRUE(try_disjoint_to_independent_xyz_errors_approx(0.1 - 0.01, 0.1 - 0.01, 0.01, &out_x, &out_y, &out_z));
    ASSERT_NEAR(out_x, 0.1, 1e-6);
    ASSERT_NEAR(out_y, 0.1, 1e-6);
    ASSERT_NEAR(out_z, 0, 1e-6);

    ASSERT_FALSE(try_disjoint_to_independent_xyz_errors_approx(0.2, 0.2, 0, &out_x, &out_y, &out_z));
    independent_to_disjoint_xyz_errors(out_x, out_y, out_z, &out_x, &out_y, &out_z);
    ASSERT_NEAR(out_x, 0.2, 1e-6);
    ASSERT_NEAR(out_y, 0.2, 1e-6);
    ASSERT_LT(out_z, 0.08);

    ASSERT_FALSE(try_disjoint_to_independent_xyz_errors_approx(0.2, 0.1, 0, &out_x, &out_y, &out_z));
    independent_to_disjoint_xyz_errors(out_x, out_y, out_z, &out_x, &out_y, &out_z);
    ASSERT_NEAR(out_x, 0.2, 1e-6);
    ASSERT_NEAR(out_y, 0.1, 1e-6);
    ASSERT_LT(out_z, 0.03);

    ASSERT_TRUE(try_disjoint_to_independent_xyz_errors_approx(0.3 * 0.6, 0.4 * 0.7, 0.3 * 0.4, &out_x, &out_y, &out_z));
    ASSERT_NEAR(out_x, 0.3, 1e-6);
    ASSERT_NEAR(out_y, 0.4, 1e-6);
    ASSERT_NEAR(out_z, 0.0, 1e-6);
    independent_to_disjoint_xyz_errors(out_x, out_y, out_z, &out_x, &out_y, &out_z);
    ASSERT_NEAR(out_x, 0.3 * 0.6, 1e-6);
    ASSERT_NEAR(out_y, 0.4 * 0.7, 1e-6);
    ASSERT_NEAR(out_z, 0.3 * 0.4, 1e-6);
}

TEST(conversions, fuzz_depolarize1_consistency) {
    auto rng = INDEPENDENT_TEST_RNG();
    std::uniform_real_distribution<double> dis(0, 1);
    for (size_t k = 0; k < 10; k++) {
        double p = dis(rng) * 0.75;
        double p2 = depolarize1_probability_to_independent_per_channel_probability(p);
        double x2, y2, z2;
        ASSERT_TRUE(try_disjoint_to_independent_xyz_errors_approx(p / 3, p / 3, p / 3, &x2, &y2, &z2));
        ASSERT_NEAR(x2, p2, 1e-6) << "p=" << p;
        ASSERT_NEAR(y2, p2, 1e-6) << "p=" << p;
        ASSERT_NEAR(z2, p2, 1e-6) << "p=" << p;

        double p3 = independent_per_channel_probability_to_depolarize1_probability(p2);
        double x3, y3, z3;
        independent_to_disjoint_xyz_errors(x2, y2, z2, &x3, &y3, &z3);
        ASSERT_NEAR(x3, p / 3, 1e-6) << "p=" << p;
        ASSERT_NEAR(y3, p / 3, 1e-6) << "p=" << p;
        ASSERT_NEAR(z3, p / 3, 1e-6) << "p=" << p;
        ASSERT_NEAR(p3, p, 1e-6) << "p=" << p;
    }
}

TEST(conversions, fuzz_depolarize2_consistency) {
    auto rng = INDEPENDENT_TEST_RNG();
    std::uniform_real_distribution<double> dis(0, 1);
    for (size_t k = 0; k < 10; k++) {
        double p = dis(rng) * 0.75;
        double p2 = depolarize2_probability_to_independent_per_channel_probability(p);
        double p3 = independent_per_channel_probability_to_depolarize2_probability(p2);
        ASSERT_NEAR(p3, p, 1e-6) << "p=" << p;
    }
}

TEST(conversions, independent_vs_disjoint_xyz_errors_round_trip_fuzz) {
    auto rng = INDEPENDENT_TEST_RNG();
    std::uniform_real_distribution<double> dis(0, 1);
    for (size_t k = 0; k < 25; k++) {
        double x;
        double y;
        double z;
        do {
            x = dis(rng);
            y = dis(rng);
            z = dis(rng);
        } while (x + y + z >= 0.999);
        double x2, y2, z2;
        independent_to_disjoint_xyz_errors(x, y, z, &x2, &y2, &z2);
        double x3, y3, z3;
        ASSERT_TRUE(try_disjoint_to_independent_xyz_errors_approx(x2, y2, z2, &x3, &y3, &z3));
        ASSERT_NEAR(x, x3, 1e-6) << "x=" << x << ",y=" << y << ",z=" << z;
        ASSERT_NEAR(y, y3, 1e-6) << "x=" << x << ",y=" << y << ",z=" << z;
        ASSERT_NEAR(z, z3, 1e-6) << "x=" << x << ",y=" << y << ",z=" << z;
    }
}
