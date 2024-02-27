#include "stim/search/sat/wcnf.h"
#include "gtest/gtest.h"

using namespace stim;

TEST(shortest_undetectable_logical_error_wcnf, no_error) {
    // No error.
    ASSERT_THROW(
        { stim::shortest_undetectable_logical_error_wcnf(DetectorErrorModel()); },
        std::invalid_argument);
}

TEST(shortest_undetectable_logical_error_wcnf, single_detector_single_observable) {
  std::string wcnf = stim::shortest_undetectable_logical_error_wcnf(DetectorErrorModel(R"DEM(
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
  // Plus 1 hard clause to ensure an observable is flipped:
  // hard clause x_0
  // This gives a total of 7 clauses
  // The top value should be at least 1 + 1 + 1 = 3. In our implementation ends up being 8.
  std::stringstream expected;
  // WDIMACS header format: p wcnf nbvar nbclauses top
  expected << "p wcnf 3 7 8\n";
  // Soft clause
  expected << "1 -0\n";
  // Hard clauses
  expected << "8 0 1 -2\n";
  expected << "8 0 -1 2\n";
  expected << "8 -0 1 2\n";
  expected << "8 -0 -1 -2\n";
  // Soft clause
  expected << "1 -1\n";
  // Hard clause for the observable flipped
  expected << "8 0\n";
  ASSERT_EQ(wcnf, expected.str());
}
