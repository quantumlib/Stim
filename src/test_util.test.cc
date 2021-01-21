#include "test_util.test.h"

static std::mt19937_64 *shared_test_rng;

std::mt19937_64 &SHARED_TEST_RNG() {
    if (shared_test_rng == nullptr) {
        shared_test_rng = new std::mt19937_64((std::random_device {})());
    }
    return *shared_test_rng;
}
