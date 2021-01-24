#include <gtest/gtest.h>
#include "frame_simulator.h"
#include "tableau_simulator.h"
#include "gate_data.h"
#include "../test_util.test.h"

TEST(FrameSimulator, get_set_frame) {
    FrameSimulator sim(6, 4, 999, SHARED_TEST_RNG());
    ASSERT_EQ(sim.get_frame(0), PauliString::from_str("______"));
    ASSERT_EQ(sim.get_frame(1), PauliString::from_str("______"));
    ASSERT_EQ(sim.get_frame(2), PauliString::from_str("______"));
    ASSERT_EQ(sim.get_frame(3), PauliString::from_str("______"));
    sim.set_frame(0, PauliString::from_str("_XYZ__"));
    ASSERT_EQ(sim.get_frame(0), PauliString::from_str("_XYZ__"));
    ASSERT_EQ(sim.get_frame(1), PauliString::from_str("______"));
    ASSERT_EQ(sim.get_frame(2), PauliString::from_str("______"));
    ASSERT_EQ(sim.get_frame(3), PauliString::from_str("______"));
    sim.set_frame(3, PauliString::from_str("ZZZZZZ"));
    ASSERT_EQ(sim.get_frame(0), PauliString::from_str("_XYZ__"));
    ASSERT_EQ(sim.get_frame(1), PauliString::from_str("______"));
    ASSERT_EQ(sim.get_frame(2), PauliString::from_str("______"));
    ASSERT_EQ(sim.get_frame(3), PauliString::from_str("ZZZZZZ"));

    FrameSimulator big_sim(501, 1001, 999, SHARED_TEST_RNG());
    big_sim.set_frame(258, PauliString::from_func(false, 501, [](size_t k) { return "_X"[k == 303]; }));
    ASSERT_EQ(big_sim.get_frame(258).ref().sparse_str(), "+X303");
    ASSERT_EQ(big_sim.get_frame(257).ref().sparse_str(), "+I");
}

TEST(FrameSimulator, recorded_bit_address) {
    FrameSimulator sim(1, 750, 1250, SHARED_TEST_RNG());

    ASSERT_EQ(sim.recorded_bit_address(0, 0), 0);
    ASSERT_EQ(sim.recorded_bit_address(1, 0), 1);
    ASSERT_EQ(sim.recorded_bit_address(0, 1), 1 << 8);
    ASSERT_EQ(sim.recorded_bit_address(256, 0), 1 << 16);
    ASSERT_EQ(sim.recorded_bit_address(0, 256), 3 << 16);

    sim.do_transpose();
    ASSERT_EQ(sim.recorded_bit_address(0, 0), 0);
    ASSERT_EQ(sim.recorded_bit_address(1, 0), 1 << 8);
    ASSERT_EQ(sim.recorded_bit_address(0, 1), 1);
    ASSERT_EQ(sim.recorded_bit_address(256, 0), 1 << 16);
    ASSERT_EQ(sim.recorded_bit_address(0, 256), 3 << 16);
}

TEST(FrameSimulator, measure_deterministic) {
    FrameSimulator sim(1, 750, 1250, SHARED_TEST_RNG());
    sim.clear();
    sim.m_table.data.randomize(sim.m_table.data.num_bits_padded(), SHARED_TEST_RNG());
    sim.measure({{0, 0}, {false, true}, 0});
    for (size_t s = 0; s < 750; s++) {
        ASSERT_FALSE(sim.m_table.data[sim.recorded_bit_address(s, 0)]);
        ASSERT_FALSE(sim.m_table.data[sim.recorded_bit_address(s, 1)]);
    }
}

bool is_bulk_frame_operation_consistent_with_tableau(const std::string &op_name) {
    const auto &tableau = GATE_TABLEAUS.at(op_name);
    const auto &bulk_func = SIM_BULK_PAULI_FRAMES_GATE_DATA.at(op_name);

    size_t num_qubits = 500;
    size_t num_samples = 1000;
    size_t num_measurements = 10;
    FrameSimulator sim(num_qubits, num_samples, num_measurements, SHARED_TEST_RNG());
    size_t num_targets = tableau.num_qubits;
    assert(num_targets == 1 || num_targets == 2);
    std::vector<size_t> targets {101, 403, 202, 100};
    while (targets.size() > num_targets) {
        targets.pop_back();
    }
    for (size_t k = 7; k < num_samples; k += 101) {
        PauliString test_value = PauliString::random(num_qubits, SHARED_TEST_RNG());
        PauliStringRef test_value_ref(test_value);
        sim.set_frame(k, test_value);
        bulk_func(sim, targets);
        for (size_t k = 0; k < targets.size(); k += num_targets) {
            if (num_targets == 1) {
                tableau.apply_within(test_value_ref, {targets[k]});
            } else {
                tableau.apply_within(test_value_ref, {targets[k], targets[k + 1]});
            }
        }
        test_value.sign = false;
        if (test_value != sim.get_frame(k)) {
            return false;
        }
    }
    return true;
}

TEST(FrameSimulator, bulk_operations_consistent_with_tableau_data) {
    for (const auto &kv : GATE_TABLEAUS) {
        const auto &name = kv.first;
        EXPECT_TRUE(is_bulk_frame_operation_consistent_with_tableau(name)) << name;
    }
}

#define EXPECT_SAMPLES_POSSIBLE(program) EXPECT_TRUE(is_sim_frame_consistent_with_sim_tableau(program)) << program

bool is_output_possible_promising_no_bare_resets(const Circuit &circuit, const simd_bits_range_ref output) {
    auto tableau_sim = TableauSimulator(circuit.num_qubits, SHARED_TEST_RNG());
    size_t out_p = 0;
    for (const auto &op : circuit.operations) {
        if (op.name == "M") {
            for (size_t k = 0; k < op.target_data.targets.size(); k++) {
                tableau_sim.sign_bias = output[out_p] ? -1 : +1;
                tableau_sim.measure(
                        OperationData({op.target_data.targets[k]}, {op.target_data.flags[k]}, 0));
                if (output[out_p] != tableau_sim.recorded_measurement_results.front()) {
                    return false;
                }
                tableau_sim.recorded_measurement_results.pop();
                out_p++;
            }
        } else {
            tableau_sim.apply(op.name, op.target_data);
        }
    }

    return true;
}

TEST(PauliFrameSimulation, test_util_is_output_possible) {
    auto circuit = Circuit::from_text(
            "H 0\n"
            "CNOT 0 1\n"
            "X 0\n"
            "M 0\n"
            "M 1\n");
    auto data = simd_bits(2);
    data.u64[0] = 0b00;
    ASSERT_EQ(false, is_output_possible_promising_no_bare_resets(circuit, data));
    data.u64[0] = 0b01;
    ASSERT_EQ(true, is_output_possible_promising_no_bare_resets(circuit, data));
    data.u64[0] = 0b10;
    ASSERT_EQ(true, is_output_possible_promising_no_bare_resets(circuit, data));
    data.u64[0] = 0b11;
    ASSERT_EQ(false, is_output_possible_promising_no_bare_resets(circuit, data));
}

bool is_sim_frame_consistent_with_sim_tableau(const std::string &program_text) {
    auto circuit = Circuit::from_text(program_text);
    auto reference_sample = TableauSimulator::reference_sample_circuit(circuit);
    auto samples = FrameSimulator::sample(circuit, reference_sample, 10, SHARED_TEST_RNG());

    for (size_t k = 0; k < 10; k++) {
        simd_bits_range_ref sample = samples[k];
        if (!is_output_possible_promising_no_bare_resets(circuit, sample)) {
            std::cerr << "Impossible output: ";
            for (size_t k = 0; k < circuit.num_measurements; k++) {
                std::cerr << '0' + sample[k];
            }
            std::cerr << "\n";
            return false;
        }
    }
    return true;
}

TEST(PauliFrameSimulation, consistency) {
    EXPECT_SAMPLES_POSSIBLE(
            "H 0\n"
            "CNOT 0 1\n"
            "M 0\n"
            "M 1\n"
    );

    EXPECT_SAMPLES_POSSIBLE(
            "H 0\n"
            "H 1\n"
            "CNOT 0 2\n"
            "CNOT 1 3\n"
            "X 1\n"
            "M 0\n"
            "M 2\n"
            "M 3\n"
            "M 1\n"
    );

    EXPECT_SAMPLES_POSSIBLE(
            "H 0\n"
            "CNOT 0 1\n"
            "X 0\n"
            "M 0\n"
            "M 1\n"
    );

    EXPECT_SAMPLES_POSSIBLE(
            "H 0\n"
            "CNOT 0 1\n"
            "Z 0\n"
            "M 0\n"
            "M 1\n"
    );

    EXPECT_SAMPLES_POSSIBLE(
            "H 0\n"
            "M 0\n"
            "H 0\n"
            "M 0\n"
            "R 0\n"
            "H 0\n"
            "M 0\n"
    );

    EXPECT_SAMPLES_POSSIBLE(
            "H 0\n"
            "CNOT 0 1\n"
            "M 0\n"
            "R 0\n"
            "M 0\n"
            "M 1\n"
    );

    // Distance 2 surface code.
    EXPECT_SAMPLES_POSSIBLE(
            "R 0\n"
            "R 1\n"
            "R 5\n"
            "R 6\n"
            "tick\n"
            "tick\n"
            "R 2\n"
            "R 3\n"
            "R 4\n"
            "tick\n"
            "H 2\n"
            "H 3\n"
            "H 4\n"
            "tick\n"
            "CZ 3 0\n"
            "CNOT 4 1\n"
            "tick\n"
            "CZ 3 1\n"
            "CNOT 4 6\n"
            "tick\n"
            "CNOT 2 0\n"
            "CZ 3 5\n"
            "tick\n"
            "CNOT 2 5\n"
            "CZ 3 6\n"
            "tick\n"
            "H 2\n"
            "H 3\n"
            "H 4\n"
            "tick\n"
            "M 2\n"
            "M 3\n"
            "M 4\n"
            "tick\n"
            "R 2\n"
            "R 3\n"
            "R 4\n"
            "tick\n"
            "H 2\n"
            "H 3\n"
            "H 4\n"
            "tick\n"
            "CZ 3 0\n"
            "CNOT 4 1\n"
            "tick\n"
            "CZ 3 1\n"
            "CNOT 4 6\n"
            "tick\n"
            "CNOT 2 0\n"
            "CZ 3 5\n"
            "tick\n"
            "CNOT 2 5\n"
            "CZ 3 6\n"
            "tick\n"
            "H 2\n"
            "H 3\n"
            "H 4\n"
            "tick\n"
            "M 2\n"
            "M 3\n"
            "M 4\n"
            "tick\n"
            "tick\n"
            "M 0\n"
            "M 1\n"
            "M 5\n"
            "M 6");
}

TEST(PauliFrameSimulation, unpack_write_measurements_ascii) {
    auto circuit = Circuit::from_text(
            "X 0\n"
            "M 1\n"
            "M 0\n"
            "M 2\n"
            "M 3\n");
    auto ref = TableauSimulator::reference_sample_circuit(circuit);
    auto r = FrameSimulator::sample(circuit, ref, 10, SHARED_TEST_RNG());
    for (size_t k = 0; k < 10; k++) {
        ASSERT_EQ(r[k].u64[0], 0b0010);
    }

    FILE *tmp = tmpfile();
    FrameSimulator::sample_out(circuit, ref, 5, tmp, SAMPLE_FORMAT_ASCII, SHARED_TEST_RNG());
    rewind(tmp);
    std::stringstream ss;
    while (true) {
        auto i = getc(tmp);
        if (i == EOF) {
            break;
        }
        ss << (char)i;
    }
    ASSERT_EQ(ss.str(), "0100\n0100\n0100\n0100\n0100\n");

    tmp = tmpfile();
    FrameSimulator::sample_out(circuit, ref, 5, tmp, SAMPLE_FORMAT_BINLE8, SHARED_TEST_RNG());
    rewind(tmp);
    for (size_t k = 0; k < 5; k++) {
        ASSERT_EQ(getc(tmp), 0b0010);
    }
    ASSERT_EQ(getc(tmp), EOF);
}

TEST(PauliFrameSimulation, big_circuit_measurements) {
    std::vector<Operation> ops;
    for (size_t k = 0; k < 1250; k += 3) {
        ops.push_back({"X", {k}});
    }
    for (size_t k = 0; k < 1250; k++) {
        ops.push_back({"M", {k}});
    }
    auto circuit = Circuit(ops);
    auto ref = TableauSimulator::reference_sample_circuit(circuit);
    auto r = FrameSimulator::sample(circuit, ref, 750, SHARED_TEST_RNG());
    for (size_t i = 0; i < 750; i++) {
        for (size_t k = 0; k < 1250; k++) {
            ASSERT_EQ(r[i][k], k % 3 == 0) << k;
        }
    }

    FILE *tmp = tmpfile();
    FrameSimulator::sample_out(circuit, ref, 750, tmp, SAMPLE_FORMAT_ASCII, SHARED_TEST_RNG());
    rewind(tmp);
    for (size_t s = 0; s < 750; s++) {
        for (size_t k = 0; k < 1250; k++) {
            ASSERT_EQ(getc(tmp), "01"[k % 3 == 0]);
        }
        ASSERT_EQ(getc(tmp), '\n');
    }
    ASSERT_EQ(getc(tmp), EOF);

    tmp = tmpfile();
    FrameSimulator::sample_out(circuit, ref, 750, tmp, SAMPLE_FORMAT_BINLE8, SHARED_TEST_RNG());
    rewind(tmp);
    for (size_t s = 0; s < 750; s++) {
        for (size_t k = 0; k < 1250; k += 8) {
            char c = getc(tmp);
            for (size_t k2 = 0; k + k2 < 1250 && k2 < 8; k2++) {
                ASSERT_EQ((c >> k2) & 1, (k + k2) % 3 == 0);
            }
        }
    }
    ASSERT_EQ(getc(tmp), EOF);
}

TEST(PauliFrameSimulation, big_circuit_random_measurements) {
    std::vector<Operation> ops;
    for (size_t k = 0; k < 270; k++) {
        ops.push_back({"H", {k}});
    }
    for (size_t k = 0; k < 270; k++) {
        ops.push_back({"M", {k}});
    }
    auto program = Circuit(ops);
    auto ref = TableauSimulator::reference_sample_circuit(program);
    auto r = FrameSimulator::sample(program, ref, 1000, SHARED_TEST_RNG());
    for (size_t k = 0; k < 1000; k++) {
        ASSERT_TRUE(r[k].not_zero()) << k;
    }
}
