#include "stim/util_top/mbqc_decomposition.h"

#include <stim/util_bot/test_util.test.h>

#include "gtest/gtest.h"

#include "stim/circuit/circuit.h"
#include "stim/util_top/has_flow.h"

using namespace stim;

TEST(mbqc_decomposition, all_gates) {
    auto rng = INDEPENDENT_TEST_RNG();

    for (const auto &g : GATE_DATA.items) {
        const char *decomp = mbqc_decomposition(g.id);
        if (decomp == nullptr) {
            EXPECT_TRUE(g.flow_data.empty()) << g.name;
            continue;
        } else {
            EXPECT_FALSE(g.flow_data.empty()) << g.name;
        }

        Circuit circuit(decomp);
        std::vector<Flow<64>> verified_flows;
        for (auto &flow : g.flows<64>()) {
            if (flow.input.ref().weight() && flow.output.ref().weight()) {
                verified_flows.push_back(std::move(flow));
            }
        }
        std::vector<bool> result = sample_if_circuit_has_stabilizer_flows<64>(256, rng, circuit, verified_flows);
        bool correct = true;
        for (auto b : result) {
            correct &= b;
        }
        EXPECT_TRUE(correct) << g.name;
    }
}
