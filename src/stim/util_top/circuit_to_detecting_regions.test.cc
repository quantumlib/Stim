#include "stim/util_top/circuit_to_detecting_regions.h"

#include "gtest/gtest.h"

using namespace stim;

TEST(circuit_to_detecting_regions, simple) {
    Circuit circuit(R"CIRCUIT(
        H 0
        TICK
        CX 0 1
        TICK
        MXX 0 1
        DETECTOR rec[-1]
    )CIRCUIT");
    auto actual = circuit_to_detecting_regions(circuit, {DemTarget::relative_detector_id(0)}, {0, 1}, false);
    std::map<DemTarget, std::map<uint64_t, FlexPauliString>> expected{
        {DemTarget::relative_detector_id(0),
         {
             {0, FlexPauliString::from_text("X_")},
             {1, FlexPauliString::from_text("XX")},
         }},
    };
}
