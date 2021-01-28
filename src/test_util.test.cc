#include "probability_util.h"
#include "test_util.test.h"

static bool shared_test_rng_initialized;
static std::mt19937_64 shared_test_rng;

std::mt19937_64 &SHARED_TEST_RNG() {
    if (!shared_test_rng_initialized) {
        shared_test_rng = externally_seeded_rng();
        shared_test_rng_initialized = true;
    }
    return shared_test_rng;
}
