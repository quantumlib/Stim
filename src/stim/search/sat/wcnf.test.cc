#include "stim/search/sat/wcnf.h"

#include "gtest/gtest.h"

using namespace stim;

const std::string unsatisfiable_wdimacs = "p wcnf 1 2 3\n3 -1 0\n3 1 0\n";

TEST(shortest_error_sat_problem, no_error) {
    // No errors. Should get an unsatisfiable formula.
    ASSERT_EQ(stim::shortest_error_sat_problem(DetectorErrorModel()), "p wcnf 1 2 3\n3 -1 0\n3 1 0\n");
}

TEST(shortest_error_sat_problem, single_obs_no_dets) {
    std::string wcnf = stim::shortest_error_sat_problem(DetectorErrorModel(R"DEM(
        error(0.1) L0
    )DEM"));
    EXPECT_EQ(
        wcnf,
        R"WDIMACS(p wcnf 1 2 3
1 -1 0
3 1 0
)WDIMACS");
}

TEST(shortest_error_sat_problem, single_dets_no_obs) {
    std::string wcnf = stim::shortest_error_sat_problem(DetectorErrorModel(R"DEM(
        error(0.1) D0
    )DEM"));
    EXPECT_EQ(wcnf, unsatisfiable_wdimacs);
}

TEST(shortest_error_sat_problem, no_dets_no_obs) {
    std::string wcnf = stim::shortest_error_sat_problem(DetectorErrorModel(R"DEM(
        error(0.1)
    )DEM"));
    EXPECT_EQ(wcnf, unsatisfiable_wdimacs);
}

TEST(shortest_error_sat_problem, no_errors) {
    std::string wcnf = stim::shortest_error_sat_problem(DetectorErrorModel(R"DEM()DEM"));
    EXPECT_EQ(wcnf, unsatisfiable_wdimacs);
}

TEST(shortest_error_sat_problem, single_detector_single_observable) {
    // To test that it ignores weights entirely, we try several combinations of weights.
    for (std::string dem_str : {
             R"DEM(
            error(0.1) D0 L0
            error(0.1) D0
        )DEM",
             R"DEM(
            error(1.0) D0 L0
            error(0) D0
        )DEM",
             R"DEM(
            error(0.5) D0 L0
            error(0.999) D0
        )DEM",
             R"DEM(
            error(0.001) D0 L0
            error(0.999) D0
        )DEM",
             R"DEM(
            error(0) D0 L0
            error(0) D0
        )DEM",
             R"DEM(
            error(0.5) D0 L0
            error(0.5) D0
        )DEM"}) {
        std::string wcnf = stim::shortest_error_sat_problem(DetectorErrorModel(dem_str.c_str()));
        // x_0 -- error 0 occurred
        // x_1 -- error 1 occurred
        // x_2 -- XOR of x_0 and x_1
        // There should be 2 soft clauses:
        // soft clause NOT(x_0) with weight 1
        // soft clause NOT(x_0) with weight 1
        // There should be 4 hard clauses to forbid the strings such that x_2 != XOR(x_0, x_1):
        // hard clause x != (0, 0, 1)
        // hard clause x != (0, 1, 0)
        // hard clause x != (1, 0, 0)
        // hard clause x != (1, 1, 1)
        // Plus 1 hard clause to ensure detector is not flipped
        // hard clause -x_2
        // Plus 1 hard clause to ensure an observable is flipped:
        // hard clause x_0
        // This gives a total of 8 clauses
        // The top value should be at least 1 + 1 + 1 = 3. In our implementation ends up being 9.
        std::stringstream expected;
        // WDIMACS header format: p wcnf nbvar nbclauses top
        expected << "p wcnf 3 8 9\n";
        // Soft clause
        expected << "1 -1 0\n";
        // Hard clauses
        expected << "9 1 2 -3 0\n";
        expected << "9 1 -2 3 0\n";
        expected << "9 -1 2 3 0\n";
        expected << "9 -1 -2 -3 0\n";
        // Soft clause
        expected << "1 -2 0\n";
        // Hard clause for the detector not to be flipped
        expected << "9 -3 0\n";
        // Hard clause for the observable flipped
        expected << "9 1 0\n";
        ASSERT_EQ(wcnf, expected.str());
        // The optimal value of this wcnf file should be 2, but we don't have
        // a maxSAT solver to be able to test it here.
    }
}

TEST(likeliest_error_sat_problem, no_error) {
    // No errors. Should get an unsatisfiable formula.
    ASSERT_EQ(stim::likeliest_error_sat_problem(DetectorErrorModel()), unsatisfiable_wdimacs);
}

TEST(likeliest_error_sat_problem, single_detector_single_observable) {
    std::string wcnf = stim::likeliest_error_sat_problem(DetectorErrorModel(R"DEM(
      error(0.1) D0 L0
      error(0.1) D0
    )DEM"));
    // There should be 3 variables: x = (x_0, x_1, x_2)
    // x_0 -- error 0 occurred
    // x_1 -- error 1 occurred
    // x_2 -- XOR of x_0 and x_1
    // There should be 2 soft clauses:
    // soft clause NOT(x_0) with weight 1
    // soft clause NOT(x_0) with weight 1
    // There should be 4 hard clauses to forbid the strings such that x_2 != XOR(x_0, x_1):
    // hard clause x != (0, 0, 1)
    // hard clause x != (0, 1, 0)
    // hard clause x != (1, 0, 0)
    // hard clause x != (1, 1, 1)
    // Plus 1 hard clause to ensure detector is not flipped
    // hard clause -x_2
    // Plus 1 hard clause to ensure an observable is flipped:
    // hard clause x_0
    // This gives a total of 8 clauses
    // The top value should be at least 1 + 1 + 1 = 3. In our implementation ends up being 9.
    std::stringstream expected;
    // WDIMACS header format: p wcnf nbvar nbclauses top
    expected << "p wcnf 3 8 81\n";
    // Soft clause
    expected << "10 -1 0\n";
    // Hard clauses
    expected << "81 1 2 -3 0\n";
    expected << "81 1 -2 3 0\n";
    expected << "81 -1 2 3 0\n";
    expected << "81 -1 -2 -3 0\n";
    // Soft clause
    expected << "10 -2 0\n";
    // Hard clause for the detector not to be flipped
    expected << "81 -3 0\n";
    // Hard clause for the observable flipped
    expected << "81 1 0\n";
    ASSERT_EQ(wcnf, expected.str());
    // The optimal value of this wcnf file should be 20, but we don't have
    // a maxSAT solver to be able to test it here.
}

TEST(likeliest_error_sat_problem, single_detector_single_observable_large_probability) {
    std::string wcnf = stim::likeliest_error_sat_problem(DetectorErrorModel(R"DEM(
      error(0.1) D0 L0
      error(0.9) D0
    )DEM"));
    // There should be 3 variables: x = (x_0, x_1, x_2)
    // x_0 -- error 0 occurred
    // x_1 -- error 1 occurred
    // x_2 -- XOR of x_0 and x_1
    // There should be 2 soft clauses:
    // soft clause NOT(x_0) with weight 1
    // soft clause NOT(x_0) with weight 1
    // There should be 4 hard clauses to forbid the strings such that x_2 != XOR(x_0, x_1):
    // hard clause x != (0, 0, 1)
    // hard clause x != (0, 1, 0)
    // hard clause x != (1, 0, 0)
    // hard clause x != (1, 1, 1)
    // Plus 1 hard clause to ensure detector is not flipped
    // hard clause -x_2
    // Plus 1 hard clause to ensure an observable is flipped:
    // hard clause x_0
    // This gives a total of 8 clauses
    // The top value should be at least 1 + 1 + 1 = 3. In our implementation ends up being 9.
    std::stringstream expected;
    // WDIMACS header format: p wcnf nbvar nbclauses top
    expected << "p wcnf 3 8 81\n";
    // Soft clause
    expected << "10 -1 0\n";
    // Hard clauses
    expected << "81 1 2 -3 0\n";
    expected << "81 1 -2 3 0\n";
    expected << "81 -1 2 3 0\n";
    expected << "81 -1 -2 -3 0\n";
    // Soft clause for the 2nd error to flip, with weight 10.
    // This is because the probability of that error is 0.9.
    expected << "10 2 0\n";
    // Hard clause for the detector not to be flipped
    expected << "81 -3 0\n";
    // Hard clause for the observable flipped
    expected << "81 1 0\n";
    ASSERT_EQ(wcnf, expected.str());
    // The optimal value of this wcnf file should be 20, but we don't have
    // a maxSAT solver to be able to test it here.
}

TEST(likeliest_error_sat_problem, single_detector_single_observable_half_probability) {
    std::string wcnf = stim::likeliest_error_sat_problem(DetectorErrorModel(R"DEM(
      error(0.1) D0 L0
      error(0.5) D0
    )DEM"));
    // There should be 3 variables: x = (x_0, x_1, x_2)
    // x_0 -- error 0 occurred
    // x_1 -- error 1 occurred
    // x_2 -- XOR of x_0 and x_1
    // There should be 2 soft clauses:
    // soft clause NOT(x_0) with weight 1
    // soft clause NOT(x_0) with weight 1
    // There should be 4 hard clauses to forbid the strings such that x_2 != XOR(x_0, x_1):
    // hard clause x != (0, 0, 1)
    // hard clause x != (0, 1, 0)
    // hard clause x != (1, 0, 0)
    // hard clause x != (1, 1, 1)
    // Plus 1 hard clause to ensure detector is not flipped
    // hard clause -x_2
    // Plus 1 hard clause to ensure an observable is flipped:
    // hard clause x_0
    // This gives a total of 8 clauses
    // The top value should be at least 1 + 1 + 1 = 3. In our implementation ends up being 9.
    std::stringstream expected;
    // WDIMACS header format: p wcnf nbvar nbclauses top
    expected << "p wcnf 3 7 71\n";
    // Soft clause
    expected << "10 -1 0\n";
    // Hard clauses
    expected << "71 1 2 -3 0\n";
    expected << "71 1 -2 3 0\n";
    expected << "71 -1 2 3 0\n";
    expected << "71 -1 -2 -3 0\n";
    // // Soft clause for the 2nd error to flip, with weight 10.
    // // This is because the probability of that error is 0.9.
    // expected << "10 2 0\n";
    // Hard clause for the detector not to be flipped
    expected << "71 -3 0\n";
    // Hard clause for the observable flipped
    expected << "71 1 0\n";
    ASSERT_EQ(wcnf, expected.str());
    // The optimal value of this wcnf file should be 20, but we don't have
    // a maxSAT solver to be able to test it here.
}
