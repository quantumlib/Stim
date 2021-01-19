#include "gtest/gtest.h"
#include "sim_bulk_pauli_frame.h"
#include "program_frame.h"
#include "sim_tableau.h"

TEST(SimBulkPauliFrames, get_set_frame) {
    SimBulkPauliFrames sim(6, 4, 999);
    ASSERT_EQ(sim.get_frame(0), PauliStringVal::from_str("______"));
    ASSERT_EQ(sim.get_frame(1), PauliStringVal::from_str("______"));
    ASSERT_EQ(sim.get_frame(2), PauliStringVal::from_str("______"));
    ASSERT_EQ(sim.get_frame(3), PauliStringVal::from_str("______"));
    sim.set_frame(0, PauliStringVal::from_str("_XYZ__"));
    ASSERT_EQ(sim.get_frame(0), PauliStringVal::from_str("_XYZ__"));
    ASSERT_EQ(sim.get_frame(1), PauliStringVal::from_str("______"));
    ASSERT_EQ(sim.get_frame(2), PauliStringVal::from_str("______"));
    ASSERT_EQ(sim.get_frame(3), PauliStringVal::from_str("______"));
    sim.set_frame(3, PauliStringVal::from_str("ZZZZZZ"));
    ASSERT_EQ(sim.get_frame(0), PauliStringVal::from_str("_XYZ__"));
    ASSERT_EQ(sim.get_frame(1), PauliStringVal::from_str("______"));
    ASSERT_EQ(sim.get_frame(2), PauliStringVal::from_str("______"));
    ASSERT_EQ(sim.get_frame(3), PauliStringVal::from_str("ZZZZZZ"));

    SimBulkPauliFrames big_sim(501, 1001, 999);
    big_sim.set_frame(258, PauliStringVal::from_pattern(false, 501, [](size_t k) { return "_X"[k == 303]; }));
    ASSERT_EQ(big_sim.get_frame(258).ptr().sparse().str(), "+X303");
    ASSERT_EQ(big_sim.get_frame(257).ptr().sparse().str(), "+I");
}

TEST(SimBulkPauliFrames, MUL_INTO_FRAME) {
    SimBulkPauliFrames big_sim(501, 1001, 999);
    __m256i mask[4];
    mask[0] = _mm256_set1_epi8(4);
    mask[1] = _mm256_set1_epi8(-1);
    mask[2] = _mm256_set1_epi8(0);
    mask[3] = _mm256_set1_epi8(0);
    auto ps1 = PauliStringVal::from_pattern(false, 501, [](size_t k) { return "_X"[(k | 2) == 303]; }).ptr().sparse();
    auto ps2 = PauliStringVal::from_pattern(false, 501, [](size_t k) { return "_Z"[(k | 1) == 303]; }).ptr().sparse();
    big_sim.MUL_INTO_FRAME(ps1, mask);
    ASSERT_EQ(big_sim.get_frame(0).ptr().sparse().str(), "+I");
    ASSERT_EQ(big_sim.get_frame(1).ptr().sparse().str(), "+I");
    ASSERT_EQ(big_sim.get_frame(2).ptr().sparse().str(), "+X301*X303");
    ASSERT_EQ(big_sim.get_frame(3).ptr().sparse().str(), "+I");
    ASSERT_EQ(big_sim.get_frame(255).ptr().sparse().str(), "+I");
    ASSERT_EQ(big_sim.get_frame(256).ptr().sparse().str(), "+X301*X303");
    ASSERT_EQ(big_sim.get_frame(257).ptr().sparse().str(), "+X301*X303");
    ASSERT_EQ(big_sim.get_frame(511).ptr().sparse().str(), "+X301*X303");
    ASSERT_EQ(big_sim.get_frame(512).ptr().sparse().str(), "+I");

    mask[1] = _mm256_set1_epi8(0);
    big_sim.MUL_INTO_FRAME(ps2, mask);
    ASSERT_EQ(big_sim.get_frame(0).ptr().sparse().str(), "+I");
    ASSERT_EQ(big_sim.get_frame(1).ptr().sparse().str(), "+I");
    ASSERT_EQ(big_sim.get_frame(2).ptr().sparse().str(), "+X301*Z302*Y303");
    ASSERT_EQ(big_sim.get_frame(255).ptr().sparse().str(), "+I");
    ASSERT_EQ(big_sim.get_frame(256).ptr().sparse().str(), "+X301*X303");
    ASSERT_EQ(big_sim.get_frame(257).ptr().sparse().str(), "+X301*X303");
    ASSERT_EQ(big_sim.get_frame(511).ptr().sparse().str(), "+X301*X303");
    ASSERT_EQ(big_sim.get_frame(512).ptr().sparse().str(), "+I");
}

TEST(SimBulkPauliFrames, recorded_bit_address) {
    SimBulkPauliFrames sim(1, 750, 1250);

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

TEST(SimBulkPauliFrames, unpack_measurements) {
    SimBulkPauliFrames sim(1, 500, 1000);
    std::vector<aligned_bits256> expected;
    for (size_t k = 0; k < sim.num_samples_raw; k++) {
        expected.push_back(aligned_bits256::random(sim.num_measurements_raw));
    }

    sim.clear();
    for (size_t s = 0; s < sim.num_samples_raw; s++) {
        for (size_t m = 0; m < sim.num_measurements_raw; m++) {
            sim.recorded_results.set_bit(sim.recorded_bit_address(s, m), expected[s].get_bit(m));
        }
    }
    sim.do_transpose();
    ASSERT_EQ(sim.unpack_measurements(), expected);

    for (size_t s = 0; s < sim.num_samples_raw; s++) {
        for (size_t m = 0; m < sim.num_measurements_raw; m++) {
            sim.recorded_results.set_bit(sim.recorded_bit_address(s, m), expected[s].get_bit(m));
        }
    }
    ASSERT_EQ(sim.unpack_measurements(), expected);
}

TEST(SimBulkPauliFrames, measure_deterministic) {
    SimBulkPauliFrames sim(1, 750, 1250);
    sim.clear();
    sim.recorded_results = aligned_bits256::random(sim.recorded_results.num_bits);
    sim.measure_deterministic({
        {0, false},
        {0, true},
    });
    for (size_t s = 0; s < 750; s++) {
        ASSERT_FALSE(sim.recorded_results.get_bit(sim.recorded_bit_address(s, 0)));
        ASSERT_TRUE(sim.recorded_results.get_bit(sim.recorded_bit_address(s, 1)));
    }
}

bool is_bulk_frame_operation_consistent_with_tableau(const std::string &op_name) {
    const auto &tableau = GATE_TABLEAUS.at(op_name);
    const auto &bulk_func = SIM_BULK_PAULI_FRAMES_GATE_DATA.at(op_name);

    size_t num_qubits = 500;
    size_t num_samples = 1000;
    size_t num_measurements = 10;
    SimBulkPauliFrames sim(num_qubits, num_samples, num_measurements);
    size_t num_targets = tableau.num_qubits;
    assert(num_targets == 1 || num_targets == 2);
    std::vector<size_t> targets {101, 403, 202, 100};
    while (targets.size() > num_targets) {
        targets.pop_back();
    }
    for (size_t k = 7; k < num_samples; k += 101) {
        PauliStringVal test_value = PauliStringVal::random(num_qubits);
        auto test_value_ptr = test_value.ptr();
        sim.set_frame(k, test_value.ptr());
        bulk_func(sim, targets);
        for (size_t k = 0; k < targets.size(); k += num_targets) {
            if (num_targets == 1) {
                tableau.apply_within(test_value_ptr, {targets[k]});
            } else {
                tableau.apply_within(test_value_ptr, {targets[k], targets[k + 1]});
            }
        }
        test_value.val_sign = false;
        if (test_value != sim.get_frame(k)) {
            return false;
        }
    }
    return true;
}

TEST(SimBulkPauliFrames, operations_consistent_with_tableau_data) {
    assert(GATE_TABLEAUS.size() == SIM_BULK_PAULI_FRAMES_GATE_DATA.size());
    for (const auto &kv : GATE_TABLEAUS) {
        const auto &name = kv.first;
        EXPECT_TRUE(is_bulk_frame_operation_consistent_with_tableau(name)) << name;
    }
}

#define EXPECT_SAMPLES_POSSIBLE(program) EXPECT_TRUE(is_sim_frame_consistent_with_sim_tableau(program)) << PauliFrameProgram::from_stabilizer_circuit(Circuit::from_text(program).operations)

bool is_output_possible_promising_no_bare_resets(const Circuit &circuit, const aligned_bits256 &output) {
    auto tableau_sim = SimTableau(circuit.num_qubits);
    size_t out_p = 0;
    for (const auto &op : circuit.operations) {
        if (op.name == "TICK") {
            continue;
        }
        if (op.name == "M") {
            for (auto q : op.targets) {
                bool b = output.get_bit(out_p);
                if (tableau_sim.measure(q, b) != b) {
                    return false;
                }
                out_p++;
            }
        } else if (op.name == "R") {
            tableau_sim.reset_many(op.targets);
        } else {
            tableau_sim.func_op(op.name, op.targets);
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
    auto data = aligned_bits256(2);
    data.u64[0] = 0b00;
    ASSERT_EQ(false, is_output_possible_promising_no_bare_resets(circuit, data));
    data.u64[0] = 0b01;
    ASSERT_EQ(true, is_output_possible_promising_no_bare_resets(circuit, data));
    data.u64[0] = 0b10;
    ASSERT_EQ(true, is_output_possible_promising_no_bare_resets(circuit, data));
    data.u64[0] = 0b11;
    ASSERT_EQ(false, is_output_possible_promising_no_bare_resets(circuit, data));
}

bool is_sim_frame_consistent_with_sim_tableau(const std::string &program) {
    auto circuit = Circuit::from_text(program);
    auto frame_program = PauliFrameProgram::from_stabilizer_circuit(circuit.operations);

    for (const auto &sample : frame_program.sample(10)) {
        if (!is_output_possible_promising_no_bare_resets(circuit, sample)) {
            std::cerr << "Impossible output: ";
            for (size_t k = 0; k < frame_program.num_measurements; k++) {
                std::cerr << "01"[sample.get_bit(k)];
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
    auto program = PauliFrameProgram::from_stabilizer_circuit(Circuit::from_text(
            "X 0\n"
            "M 1\n"
            "M 0\n"
            "M 2\n"
            "M 3\n"
            ).operations);
    auto r = program.sample(10);
    ASSERT_EQ(r.size(), 10);
    for (const auto &e : r) {
        ASSERT_EQ(e.u64[0], 0b0010);
    }

    FILE *tmp = tmpfile();
    program.sample_out(5, tmp, SAMPLE_FORMAT_ASCII);
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
    program.sample_out(5, tmp, SAMPLE_FORMAT_BINLE8);
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
    auto program = PauliFrameProgram::from_stabilizer_circuit(ops);
    auto r = program.sample(750);
    ASSERT_EQ(r.size(), 750);
    for (const auto &e : r) {
        for (size_t k = 0; k < 1250; k++) {
            ASSERT_EQ(e.get_bit(k), k % 3 == 0);
        }
    }

    FILE *tmp = tmpfile();
    program.sample_out(750, tmp, SAMPLE_FORMAT_ASCII);
    rewind(tmp);
    for (size_t s = 0; s < 750; s++) {
        for (size_t k = 0; k < 1250; k++) {
            ASSERT_EQ(getc(tmp), "01"[k % 3 == 0]);
        }
        ASSERT_EQ(getc(tmp), '\n');
    }
    ASSERT_EQ(getc(tmp), EOF);

    tmp = tmpfile();
    program.sample_out(750, tmp, SAMPLE_FORMAT_BINLE8);
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
    auto program = PauliFrameProgram::from_stabilizer_circuit(ops);
    auto r = program.sample(1000);
    ASSERT_EQ(r.size(), 1000);
    for (size_t k = 0; k < r.size(); k++) {
        ASSERT_TRUE(any_non_zero(r[k].u256, ceil256(r[k].num_bits) >> 8)) << k;
    }
}
