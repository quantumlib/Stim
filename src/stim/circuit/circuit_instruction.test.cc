#include "stim/circuit/circuit_instruction.h"

#include "gtest/gtest.h"

#include "stim/circuit/circuit.h"
#include "stim/circuit/circuit.test.h"

using namespace stim;

TEST(circuit_instruction, for_combined_targets) {
    Circuit circuit(R"CIRCUIT(
        X
        CX
        S 1
        H 0 2
        TICK
        CX 0 1 2 3
        CY 3 5
        SPP
        MPP X0*X1*Z2 Z7 X5*X9
        SPP Z5
    )CIRCUIT");
    auto get_k = [&](size_t k) {
        std::vector<std::vector<GateTarget>> results;
        circuit.operations[k].for_combined_target_groups([&](std::span<const GateTarget> group) {
            std::vector<GateTarget> items;
            for (auto g : group) {
                items.push_back(g);
            }
            results.push_back(items);
        });
        return results;
    };
    ASSERT_EQ(get_k(0), (std::vector<std::vector<GateTarget>>{}));
    ASSERT_EQ(get_k(1), (std::vector<std::vector<GateTarget>>{}));
    ASSERT_EQ(
        get_k(2),
        (std::vector<std::vector<GateTarget>>{
            {GateTarget::qubit(1)},
        }));
    ASSERT_EQ(
        get_k(3),
        (std::vector<std::vector<GateTarget>>{
            {GateTarget::qubit(0)},
            {GateTarget::qubit(2)},
        }));
    ASSERT_EQ(get_k(4), (std::vector<std::vector<GateTarget>>{}));
    ASSERT_EQ(
        get_k(5),
        (std::vector<std::vector<GateTarget>>{
            {GateTarget::qubit(0), GateTarget::qubit(1)},
            {GateTarget::qubit(2), GateTarget::qubit(3)},
        }));
    ASSERT_EQ(
        get_k(6),
        (std::vector<std::vector<GateTarget>>{
            {GateTarget::qubit(3), GateTarget::qubit(5)},
        }));
    ASSERT_EQ(get_k(7), (std::vector<std::vector<GateTarget>>{}));
    ASSERT_EQ(
        get_k(8),
        (std::vector<std::vector<GateTarget>>{
            {GateTarget::x(0), GateTarget::combiner(), GateTarget::x(1), GateTarget::combiner(), GateTarget::z(2)},
            {GateTarget::z(7)},
            {GateTarget::x(5), GateTarget::combiner(), GateTarget::x(9)},
        }));
    ASSERT_EQ(
        get_k(9),
        (std::vector<std::vector<GateTarget>>{
            {GateTarget::z(5)},
        }));
}

TEST(circuit_instruction, for_combined_targets_works_on_all) {
    Circuit c = generate_test_circuit_with_all_operations();
    size_t count = 0;
    for (const auto &e : c.operations) {
        if (e.gate_type == GateType::REPEAT) {
            continue;
        }
        e.for_combined_target_groups([&](std::span<const GateTarget> group) {
            count += group.size();
        });
    }
    ASSERT_TRUE(count > 0);
}
