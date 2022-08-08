// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "stim/simulators/frame_simulator.h"

#include "gtest/gtest.h"

#include "stim/circuit/circuit.test.h"
#include "stim/simulators/tableau_simulator.h"
#include "stim/test_util.test.h"

using namespace stim;

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
    big_sim.set_frame(258, PauliString::from_func(false, 501, [](size_t k) {
                          return "_X"[k == 303];
                      }));
    ASSERT_EQ(big_sim.get_frame(258).ref().sparse_str(), "+X303");
    ASSERT_EQ(big_sim.get_frame(257).ref().sparse_str(), "+I");
}

bool is_bulk_frame_operation_consistent_with_tableau(const Gate &gate) {
    Tableau tableau = gate.tableau();
    const auto &bulk_func = gate.frame_simulator_function;

    size_t num_qubits = 500;
    size_t num_samples = 1000;
    size_t max_lookback = 10;
    FrameSimulator sim(num_qubits, num_samples, max_lookback, SHARED_TEST_RNG());
    size_t num_targets = tableau.num_qubits;
    assert(num_targets == 1 || num_targets == 2);
    std::vector<GateTarget> targets{{101}, {403}, {202}, {100}};
    while (targets.size() > num_targets) {
        targets.pop_back();
    }
    OperationData op_data{{}, targets};
    for (size_t k = 7; k < num_samples; k += 101) {
        PauliString test_value = PauliString::random(num_qubits, SHARED_TEST_RNG());
        PauliStringRef test_value_ref(test_value);
        sim.set_frame(k, test_value);
        (sim.*bulk_func)(op_data);
        for (size_t k2 = 0; k2 < targets.size(); k2 += num_targets) {
            if (num_targets == 1) {
                tableau.apply_within(test_value_ref, {targets[k2].data});
            } else {
                tableau.apply_within(test_value_ref, {targets[k2].data, targets[k2 + 1].data});
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
    for (const auto &gate : GATE_DATA.gates()) {
        if (gate.flags & GATE_IS_UNITARY) {
            EXPECT_TRUE(is_bulk_frame_operation_consistent_with_tableau(gate)) << gate.name;
        }
    }
}

bool is_output_possible_promising_no_bare_resets(const Circuit &circuit, const simd_bits_range_ref<MAX_BITWORD_WIDTH> output) {
    auto tableau_sim = TableauSimulator(SHARED_TEST_RNG(), circuit.count_qubits());
    size_t out_p = 0;
    bool pass = true;
    circuit.for_each_operation([&](const Operation &op) {
        if (op.gate->name == std::string("M")) {
            for (auto qf : op.target_data.targets) {
                tableau_sim.sign_bias = output[out_p] ? -1 : +1;
                tableau_sim.measure_z(OpDat(qf.data));
                if (output[out_p] != tableau_sim.measurement_record.storage.back()) {
                    pass = false;
                }
                out_p++;
            }
        } else {
            (tableau_sim.*op.gate->tableau_simulator_function)(op.target_data);
        }
    });
    return pass;
}

TEST(FrameSimulator, test_util_is_output_possible) {
    auto circuit = Circuit(
        "H 0\n"
        "CNOT 0 1\n"
        "X 0\n"
        "M 0\n"
        "M 1\n");
    auto data = simd_bits<MAX_BITWORD_WIDTH>(2);
    data.u64[0] = 0;
    ASSERT_EQ(false, is_output_possible_promising_no_bare_resets(circuit, data));
    data.u64[0] = 1;
    ASSERT_EQ(true, is_output_possible_promising_no_bare_resets(circuit, data));
    data.u64[0] = 2;
    ASSERT_EQ(true, is_output_possible_promising_no_bare_resets(circuit, data));
    data.u64[0] = 3;
    ASSERT_EQ(false, is_output_possible_promising_no_bare_resets(circuit, data));
}

bool is_sim_frame_consistent_with_sim_tableau(const char *program_text) {
    auto circuit = Circuit(program_text);
    auto reference_sample = TableauSimulator::reference_sample_circuit(circuit);
    auto samples = FrameSimulator::sample(circuit, reference_sample, 10, SHARED_TEST_RNG());

    for (size_t k = 0; k < 10; k++) {
        simd_bits_range_ref<MAX_BITWORD_WIDTH> sample = samples[k];
        if (!is_output_possible_promising_no_bare_resets(circuit, sample)) {
            std::cerr << "Impossible output: ";
            for (size_t k2 = 0; k2 < circuit.count_measurements(); k2++) {
                std::cerr << '0' + sample[k2];
            }
            std::cerr << "\n";
            return false;
        }
    }
    return true;
}

#define EXPECT_SAMPLES_POSSIBLE(program) EXPECT_TRUE(is_sim_frame_consistent_with_sim_tableau(program)) << program

TEST(FrameSimulator, consistency) {
    EXPECT_SAMPLES_POSSIBLE(
        "H 0\n"
        "CNOT 0 1\n"
        "M 0\n"
        "M 1\n");

    EXPECT_SAMPLES_POSSIBLE(
        "H 0\n"
        "H 1\n"
        "CNOT 0 2\n"
        "CNOT 1 3\n"
        "X 1\n"
        "M 0\n"
        "M 2\n"
        "M 3\n"
        "M 1\n");

    EXPECT_SAMPLES_POSSIBLE(
        "H 0\n"
        "CNOT 0 1\n"
        "X 0\n"
        "M 0\n"
        "M 1\n");

    EXPECT_SAMPLES_POSSIBLE(
        "H 0\n"
        "CNOT 0 1\n"
        "Z 0\n"
        "M 0\n"
        "M 1\n");

    EXPECT_SAMPLES_POSSIBLE(
        "H 0\n"
        "M 0\n"
        "H 0\n"
        "M 0\n"
        "R 0\n"
        "H 0\n"
        "M 0\n");

    EXPECT_SAMPLES_POSSIBLE(
        "H 0\n"
        "CNOT 0 1\n"
        "M 0\n"
        "R 0\n"
        "M 0\n"
        "M 1\n");

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

TEST(FrameSimulator, sample_out) {
    auto circuit = Circuit(
        "X 0\n"
        "M 1\n"
        "M 0\n"
        "M 2\n"
        "M 3\n");
    auto ref = TableauSimulator::reference_sample_circuit(circuit);
    auto r = FrameSimulator::sample(circuit, ref, 10, SHARED_TEST_RNG());
    for (size_t k = 0; k < 10; k++) {
        ASSERT_EQ(r[k].u64[0], 2);
    }

    FILE *tmp = tmpfile();
    FrameSimulator::sample_out(circuit, ref, 5, tmp, SAMPLE_FORMAT_01, SHARED_TEST_RNG());
    ASSERT_EQ(rewind_read_close(tmp), "0100\n0100\n0100\n0100\n0100\n");

    tmp = tmpfile();
    FrameSimulator::sample_out(circuit, ref, 5, tmp, SAMPLE_FORMAT_B8, SHARED_TEST_RNG());
    rewind(tmp);
    for (size_t k = 0; k < 5; k++) {
        ASSERT_EQ(getc(tmp), 2);
    }
    ASSERT_EQ(getc(tmp), EOF);

    tmp = tmpfile();
    FrameSimulator::sample_out(circuit, ref, 64, tmp, SAMPLE_FORMAT_PTB64, SHARED_TEST_RNG());
    rewind(tmp);
    for (size_t k = 0; k < 8; k++) {
        ASSERT_EQ(getc(tmp), 0);
    }
    for (size_t k = 0; k < 8; k++) {
        ASSERT_EQ(getc(tmp), 0xFF);
    }
    for (size_t k = 0; k < 8; k++) {
        ASSERT_EQ(getc(tmp), 0);
        ASSERT_EQ(getc(tmp), 0);
    }
    ASSERT_EQ(getc(tmp), EOF);
}

TEST(FrameSimulator, big_circuit_measurements) {
    Circuit circuit;
    for (uint32_t k = 0; k < 1250; k += 3) {
        circuit.append_op("X", {k});
    }
    for (uint32_t k = 0; k < 1250; k++) {
        circuit.append_op("M", {k});
    }
    auto ref = TableauSimulator::reference_sample_circuit(circuit);
    auto r = FrameSimulator::sample(circuit, ref, 750, SHARED_TEST_RNG());
    for (size_t i = 0; i < 750; i++) {
        for (size_t k = 0; k < 1250; k++) {
            ASSERT_EQ(r[i][k], k % 3 == 0) << k;
        }
    }

    FILE *tmp = tmpfile();
    FrameSimulator::sample_out(circuit, ref, 750, tmp, SAMPLE_FORMAT_01, SHARED_TEST_RNG());
    rewind(tmp);
    for (size_t s = 0; s < 750; s++) {
        for (size_t k = 0; k < 1250; k++) {
            ASSERT_EQ(getc(tmp), "01"[k % 3 == 0]);
        }
        ASSERT_EQ(getc(tmp), '\n');
    }
    ASSERT_EQ(getc(tmp), EOF);

    tmp = tmpfile();
    FrameSimulator::sample_out(circuit, ref, 750, tmp, SAMPLE_FORMAT_B8, SHARED_TEST_RNG());
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

TEST(FrameSimulator, run_length_measurement_formats) {
    Circuit circuit;
    circuit.append_op("X", {100, 500, 501, 551, 1200});
    for (uint32_t k = 0; k < 1250; k++) {
        circuit.append_op("M", {k});
    }
    auto ref = TableauSimulator::reference_sample_circuit(circuit);

    FILE *tmp = tmpfile();
    FrameSimulator::sample_out(circuit, ref, 3, tmp, SAMPLE_FORMAT_HITS, SHARED_TEST_RNG());
    ASSERT_EQ(rewind_read_close(tmp), "100,500,501,551,1200\n100,500,501,551,1200\n100,500,501,551,1200\n");

    tmp = tmpfile();
    FrameSimulator::sample_out(circuit, ref, 3, tmp, SAMPLE_FORMAT_DETS, SHARED_TEST_RNG());
    ASSERT_EQ(
        rewind_read_close(tmp),
        "shot M100 M500 M501 M551 M1200\nshot M100 M500 M501 M551 M1200\nshot M100 M500 M501 M551 M1200\n");

    tmp = tmpfile();
    FrameSimulator::sample_out(circuit, ref, 3, tmp, SAMPLE_FORMAT_R8, SHARED_TEST_RNG());
    rewind(tmp);
    for (size_t k = 0; k < 3; k++) {
        ASSERT_EQ(getc(tmp), 100);
        ASSERT_EQ(getc(tmp), 255);
        ASSERT_EQ(getc(tmp), 400 - 255 - 1);
        ASSERT_EQ(getc(tmp), 0);
        ASSERT_EQ(getc(tmp), 49);
        ASSERT_EQ(getc(tmp), 255);
        ASSERT_EQ(getc(tmp), 255);
        ASSERT_EQ(getc(tmp), 1200 - 551 - 255 * 2 - 1);
        ASSERT_EQ(getc(tmp), 1250 - 1200 - 1);
    }
    ASSERT_EQ(getc(tmp), EOF);
}

TEST(FrameSimulator, big_circuit_random_measurements) {
    Circuit circuit;
    for (uint32_t k = 0; k < 270; k++) {
        circuit.append_op("H_XZ", {k});
    }
    for (uint32_t k = 0; k < 270; k++) {
        circuit.append_op("M", {k});
    }
    auto ref = TableauSimulator::reference_sample_circuit(circuit);
    auto r = FrameSimulator::sample(circuit, ref, 1000, SHARED_TEST_RNG());
    for (size_t k = 0; k < 1000; k++) {
        ASSERT_TRUE(r[k].not_zero()) << k;
    }
}

TEST(FrameSimulator, correlated_error) {
    simd_bits<MAX_BITWORD_WIDTH> ref(5);
    simd_bits<MAX_BITWORD_WIDTH> expected(5);

    expected.clear();
    ASSERT_EQ(
        FrameSimulator::sample(
            Circuit(R"circuit(
        CORRELATED_ERROR(0) X0 X1
        ELSE_CORRELATED_ERROR(0) X1 X2
        ELSE_CORRELATED_ERROR(0) X2 X3
        M 0 1 2 3
    )circuit"),
            ref,
            1,
            SHARED_TEST_RNG())[0],
        expected);

    expected.clear();
    expected[0] = true;
    expected[1] = true;
    ASSERT_EQ(
        FrameSimulator::sample(
            Circuit(R"circuit(
        CORRELATED_ERROR(1) X0 X1
        ELSE_CORRELATED_ERROR(0) X1 X2
        ELSE_CORRELATED_ERROR(0) X2 X3
        M 0 1 2 3
    )circuit"),
            ref,
            1,
            SHARED_TEST_RNG())[0],
        expected);

    expected.clear();
    expected[1] = true;
    expected[2] = true;
    ASSERT_EQ(
        FrameSimulator::sample(
            Circuit(R"circuit(
        CORRELATED_ERROR(0) X0 X1
        ELSE_CORRELATED_ERROR(1) X1 X2
        ELSE_CORRELATED_ERROR(0) X2 X3
        M 0 1 2 3
    )circuit"),
            ref,
            1,
            SHARED_TEST_RNG())[0],
        expected);

    expected.clear();
    expected[2] = true;
    expected[3] = true;
    ASSERT_EQ(
        FrameSimulator::sample(
            Circuit(R"circuit(
        CORRELATED_ERROR(0) X0 X1
        ELSE_CORRELATED_ERROR(0) X1 X2
        ELSE_CORRELATED_ERROR(1) X2 X3
        M 0 1 2 3
    )circuit"),
            ref,
            1,
            SHARED_TEST_RNG())[0],
        expected);

    expected.clear();
    expected[0] = true;
    expected[1] = true;
    ASSERT_EQ(
        FrameSimulator::sample(
            Circuit(R"circuit(
        CORRELATED_ERROR(1) X0 X1
        ELSE_CORRELATED_ERROR(1) X1 X2
        ELSE_CORRELATED_ERROR(0) X2 X3
        M 0 1 2 3
    )circuit"),
            ref,
            1,
            SHARED_TEST_RNG())[0],
        expected);

    expected.clear();
    expected[0] = true;
    expected[1] = true;
    ASSERT_EQ(
        FrameSimulator::sample(
            Circuit(R"circuit(
        CORRELATED_ERROR(1) X0 X1
        ELSE_CORRELATED_ERROR(1) X1 X2
        ELSE_CORRELATED_ERROR(1) X2 X3
        M 0 1 2 3
    )circuit"),
            ref,
            1,
            SHARED_TEST_RNG())[0],
        expected);

    expected.clear();
    expected[0] = true;
    expected[1] = true;
    expected[3] = true;
    expected[4] = true;
    ASSERT_EQ(
        FrameSimulator::sample(
            Circuit(R"circuit(
        CORRELATED_ERROR(1) X0 X1
        ELSE_CORRELATED_ERROR(1) X1 X2
        ELSE_CORRELATED_ERROR(1) X2 X3
        CORRELATED_ERROR(1) X3 X4
        M 0 1 2 3 4
    )circuit"),
            ref,
            1,
            SHARED_TEST_RNG())[0],
        expected);

    int hits[3]{};
    std::mt19937_64 rng(0);
    size_t n = 10000;
    auto samples = FrameSimulator::sample(
        Circuit(R"circuit(
        CORRELATED_ERROR(0.5) X0
        ELSE_CORRELATED_ERROR(0.25) X1
        ELSE_CORRELATED_ERROR(0.75) X2
        M 0 1 2
    )circuit"),
        ref,
        n,
        rng);
    for (size_t k = 0; k < n; k++) {
        hits[0] += samples[k][0];
        hits[1] += samples[k][1];
        hits[2] += samples[k][2];
    }
    ASSERT_TRUE(0.45 * n < hits[0] && hits[0] < 0.55 * n);
    ASSERT_TRUE((0.125 - 0.05) * n < hits[1] && hits[1] < (0.125 + 0.05) * n);
    ASSERT_TRUE((0.28125 - 0.05) * n < hits[2] && hits[2] < (0.28125 + 0.05) * n);
}

TEST(FrameSimulator, quantum_cannot_control_classical) {
    simd_bits<MAX_BITWORD_WIDTH> ref(128);

    // Quantum controlling classical operation is not allowed.
    ASSERT_THROW(
        {
            FrameSimulator::sample(
                Circuit(R"circuit(
            M 0
            CNOT 1 rec[-1]
        )circuit"),
                ref,
                1,
                SHARED_TEST_RNG());
        },
        std::invalid_argument);
    ASSERT_THROW(
        {
            FrameSimulator::sample(
                Circuit(R"circuit(
            M 0
            CY 1 rec[-1]
        )circuit"),
                ref,
                1,
                SHARED_TEST_RNG());
        },
        std::invalid_argument);
    ASSERT_THROW(
        {
            FrameSimulator::sample(
                Circuit(R"circuit(
            M 0
            YCZ rec[-1] 1
        )circuit"),
                ref,
                1,
                SHARED_TEST_RNG());
        },
        std::invalid_argument);
    ASSERT_THROW(
        {
            FrameSimulator::sample(
                Circuit(R"circuit(
            M 0
            XCZ rec[-1] 1
        )circuit"),
                ref,
                1,
                SHARED_TEST_RNG());
        },
        std::invalid_argument);
    ASSERT_THROW(
        {
            FrameSimulator::sample(
                Circuit(R"circuit(
            M 0
            SWAP 1 rec[-1]
        )circuit"),
                ref,
                1,
                SHARED_TEST_RNG());
        },
        std::invalid_argument);
}

TEST(FrameSimulator, classical_can_control_quantum) {
    simd_bits<MAX_BITWORD_WIDTH> ref(128);
    simd_bits<MAX_BITWORD_WIDTH> expected(5);
    expected.clear();
    expected[0] = true;
    expected[1] = true;
    ASSERT_EQ(
        FrameSimulator::sample(
            Circuit(R"circuit(
        X_ERROR(1) 0
        M !0
        CX rec[-1] 1
        M 1
    )circuit"),
            ref,
            1,
            SHARED_TEST_RNG())[0],
        expected);
    ASSERT_EQ(
        FrameSimulator::sample(
            Circuit(R"circuit(
        X_ERROR(1) 0
        M !0
        CY rec[-1] 1
        M 1
    )circuit"),
            ref,
            1,
            SHARED_TEST_RNG())[0],
        expected);
    ASSERT_EQ(
        FrameSimulator::sample(
            Circuit(R"circuit(
        X_ERROR(1) 0
        M !0
        XCZ 1 rec[-1]
        M 1
    )circuit"),
            ref,
            1,
            SHARED_TEST_RNG())[0],
        expected);
    ASSERT_EQ(
        FrameSimulator::sample(
            Circuit(R"circuit(
        X_ERROR(1) 0
        M !0
        YCZ 1 rec[-1]
        M 1
    )circuit"),
            ref,
            1,
            SHARED_TEST_RNG())[0],
        expected);
}

TEST(FrameSimulator, classical_controls) {
    simd_bits<MAX_BITWORD_WIDTH> ref(128);
    simd_bits<MAX_BITWORD_WIDTH> expected(5);

    expected.clear();
    ASSERT_EQ(
        FrameSimulator::sample(
            Circuit(R"circuit(
        M 0
        CX rec[-1] 1
        M 1
    )circuit"),
            ref,
            1,
            SHARED_TEST_RNG())[0],
        expected);

    expected.clear();
    ASSERT_EQ(
        FrameSimulator::sample(
            Circuit(R"circuit(
        M !0
        CX rec[-1] 1
        M 1
    )circuit"),
            ref,
            1,
            SHARED_TEST_RNG())[0],
        expected);

    expected.clear();
    expected[0] = true;
    expected[1] = true;
    ASSERT_EQ(
        FrameSimulator::sample(
            Circuit(R"circuit(
        X_ERROR(1) 0
        M !0
        CX rec[-1] 1
        M 1
    )circuit"),
            ref,
            1,
            SHARED_TEST_RNG())[0],
        expected);

    expected.clear();
    expected[0] = true;
    expected[1] = true;
    ASSERT_EQ(
        FrameSimulator::sample(
            Circuit(R"circuit(
        X_ERROR(1) 0
        M 0
        CX rec[-1] 1
        M 1
    )circuit"),
            ref,
            1,
            SHARED_TEST_RNG())[0],
        expected);
    auto r = FrameSimulator::sample(
        Circuit(R"circuit(
        X_ERROR(0.5) 0
        M 0
        CX rec[-1] 1
        M 1
    )circuit"),
        ref,
        1000,
        SHARED_TEST_RNG());
    size_t hits = 0;
    for (size_t k = 0; k < 1000; k++) {
        ASSERT_EQ(r[k][0], r[k][1]);
        hits += r[k][0];
    }
    ASSERT_TRUE(400 < hits && hits < 600);

    expected.clear();
    expected[0] = true;
    expected[1] = true;
    ASSERT_EQ(
        FrameSimulator::sample(
            Circuit(R"circuit(
        X_ERROR(1) 0
        M 0
        H 1
        CZ rec[-1] 1
        H 1
        M 1
    )circuit"),
            ref,
            1,
            SHARED_TEST_RNG())[0],
        expected);

    expected.clear();
    expected[0] = true;
    expected[1] = true;
    ASSERT_EQ(
        FrameSimulator::sample(
            Circuit(R"circuit(
        X_ERROR(1) 0
        M 0
        CY rec[-1] 1
        M 1
    )circuit"),
            ref,
            1,
            SHARED_TEST_RNG())[0],
        expected);
}

TEST(FrameSimulator, record_gets_trimmed) {
    FrameSimulator sim(100, 768, 5, SHARED_TEST_RNG());
    Circuit c = Circuit("M 0 1 2 3 4 5 6 7 8 9");
    MeasureRecordBatchWriter b(tmpfile(), 768, SAMPLE_FORMAT_B8);
    for (size_t k = 0; k < 1000; k++) {
        sim.measure_z(c.operations[0].target_data);
        sim.m_record.intermediate_write_unwritten_results_to(b, simd_bits<MAX_BITWORD_WIDTH>(0));
        ASSERT_LT(sim.m_record.storage.num_major_bits_padded(), 2500);
    }
}

TEST(FrameSimulator, stream_huge_case) {
    FILE *tmp = tmpfile();
    FrameSimulator::sample_out(
        Circuit(R"CIRCUIT(
            X_ERROR(1) 2
            REPEAT 100000 {
                M 0 1 2 3
            }
        )CIRCUIT"),
        simd_bits<MAX_BITWORD_WIDTH>(0),
        256,
        tmp,
        SAMPLE_FORMAT_B8,
        SHARED_TEST_RNG());
    rewind(tmp);
    for (size_t k = 0; k < 256 * 100000 * 4 / 8; k++) {
        ASSERT_EQ(getc(tmp), 0x44);
    }
    ASSERT_EQ(getc(tmp), EOF);
}

TEST(FrameSimulator, block_results_single_shot) {
    auto circuit = Circuit(R"circuit(
        REPEAT 10000 {
            X_ERROR(1) 0
            MR 0
            M 0 0
        }
    )circuit");
    FILE *tmp = tmpfile();
    FrameSimulator::sample_out(circuit, simd_bits<MAX_BITWORD_WIDTH>(0), 3, tmp, SAMPLE_FORMAT_01, SHARED_TEST_RNG());

    auto result = rewind_read_close(tmp);
    for (size_t k = 0; k < 30000; k += 3) {
        ASSERT_EQ(result[k], '1') << k;
        ASSERT_EQ(result[k + 1], '0') << (k + 1);
        ASSERT_EQ(result[k + 2], '0') << (k + 2);
    }
    ASSERT_EQ(result[30000], '\n');
}

TEST(FrameSimulator, block_results_triple_shot) {
    auto circuit = Circuit(R"circuit(
        REPEAT 10000 {
            X_ERROR(1) 0
            MR 0
            M 0 0
        }
    )circuit");
    FILE *tmp = tmpfile();
    FrameSimulator::sample_out(circuit, simd_bits<MAX_BITWORD_WIDTH>(0), 3, tmp, SAMPLE_FORMAT_01, SHARED_TEST_RNG());

    auto result = rewind_read_close(tmp);
    for (size_t rep = 0; rep < 3; rep++) {
        size_t s = rep * 30001;
        for (size_t k = 0; k < 30000; k += 3) {
            ASSERT_EQ(result[s + k], '1') << (s + k);
            ASSERT_EQ(result[s + k + 1], '0') << (s + k + 1);
            ASSERT_EQ(result[s + k + 2], '0') << (s + k + 2);
        }
        ASSERT_EQ(result[s + 30000], '\n');
    }
}

TEST(FrameSimulator, stream_results) {
    DebugForceResultStreamingRaii force_streaming;
    auto circuit = Circuit(R"circuit(
        REPEAT 10000 {
            X_ERROR(1) 0
            MR 0
            M 0 0
        }
    )circuit");
    FILE *tmp = tmpfile();
    FrameSimulator::sample_out(circuit, simd_bits<MAX_BITWORD_WIDTH>(0), 3, tmp, SAMPLE_FORMAT_01, SHARED_TEST_RNG());

    auto result = rewind_read_close(tmp);
    for (size_t k = 0; k < 30000; k += 3) {
        ASSERT_EQ(result[k], '1') << k;
        ASSERT_EQ(result[k + 1], '0') << (k + 1);
        ASSERT_EQ(result[k + 2], '0') << (k + 2);
    }
    ASSERT_EQ(result[30000], '\n');
}

TEST(FrameSimulator, stream_many_shots) {
    DebugForceResultStreamingRaii force_streaming;
    auto circuit = Circuit(R"circuit(
        X_ERROR(1) 1
        M 0 1 2
    )circuit");
    FILE *tmp = tmpfile();
    FrameSimulator::sample_out(circuit, simd_bits<MAX_BITWORD_WIDTH>(0), 2048, tmp, SAMPLE_FORMAT_01, SHARED_TEST_RNG());

    auto result = rewind_read_close(tmp);
    for (size_t k = 0; k < 2048 * 4; k += 4) {
        ASSERT_EQ(result[k], '0') << k;
        ASSERT_EQ(result[k + 1], '1') << (k + 1);
        ASSERT_EQ(result[k + 2], '0') << (k + 2);
        ASSERT_EQ(result[k + 3], '\n') << (k + 3);
    }
}

TEST(FrameSimulator, stream_results_triple_shot) {
    DebugForceResultStreamingRaii force_streaming;
    auto circuit = Circuit(R"circuit(
        REPEAT 10000 {
            X_ERROR(1) 0
            MR 0
            M 0 0
        }
    )circuit");
    FILE *tmp = tmpfile();
    FrameSimulator::sample_out(circuit, simd_bits<MAX_BITWORD_WIDTH>(0), 3, tmp, SAMPLE_FORMAT_01, SHARED_TEST_RNG());

    auto result = rewind_read_close(tmp);
    for (size_t rep = 0; rep < 3; rep++) {
        size_t s = rep * 30001;
        for (size_t k = 0; k < 30000; k += 3) {
            ASSERT_EQ(result[s + k], '1') << (s + k);
            ASSERT_EQ(result[s + k + 1], '0') << (s + k + 1);
            ASSERT_EQ(result[s + k + 2], '0') << (s + k + 2);
        }
        ASSERT_EQ(result[s + 30000], '\n');
    }
}

TEST(FrameSimulator, measure_y_without_reset_doesnt_reset) {
    auto r = FrameSimulator::sample_flipped_measurements(
        Circuit(R"CIRCUIT(
        RY 0
        MY 0
        MY 0
        Z_ERROR(1) 0
        MY 0
        MY 0
        Z_ERROR(1) 0
        MY 0
        MY 0
    )CIRCUIT"),
        10000,
        SHARED_TEST_RNG());
    ASSERT_EQ(r[0].popcnt(), 0);
    ASSERT_EQ(r[1].popcnt(), 0);
    ASSERT_EQ(r[2].popcnt(), 10000);
    ASSERT_EQ(r[3].popcnt(), 10000);
    ASSERT_EQ(r[4].popcnt(), 0);
    ASSERT_EQ(r[5].popcnt(), 0);

    r = FrameSimulator::sample_flipped_measurements(
        Circuit(R"CIRCUIT(
        RY 0
        MRY 0
        MRY 0
        Z_ERROR(1) 0
        MRY 0
        MRY 0
        Z_ERROR(1) 0
        MRY 0
        MRY 0
    )CIRCUIT"),
        10000,
        SHARED_TEST_RNG());
    ASSERT_EQ(r[0].popcnt(), 0);
    ASSERT_EQ(r[1].popcnt(), 0);
    ASSERT_EQ(r[2].popcnt(), 10000);
    ASSERT_EQ(r[3].popcnt(), 0);
    ASSERT_EQ(r[4].popcnt(), 10000);
    ASSERT_EQ(r[5].popcnt(), 0);
}

TEST(FrameSimulator, resets_vs_measurements) {
    auto check = [&](const char *circuit, std::vector<bool> results) {
        simd_bits<MAX_BITWORD_WIDTH> ref(results.size());
        for (size_t k = 0; k < results.size(); k++) {
            ref[k] = results[k];
        }
        simd_bit_table<MAX_BITWORD_WIDTH> t = FrameSimulator::sample(Circuit(circuit), ref, 100, SHARED_TEST_RNG());
        return !t.data.not_zero();
    };

    ASSERT_TRUE(check(
        R"circuit(
        RX 0
        RY 1
        RZ 2
        H_XZ 0
        H_YZ 1
        M 0 1 2
    )circuit",
        {
            false,
            false,
            false,
        }));

    ASSERT_TRUE(check(
        R"circuit(
        H_XZ 0 1 2
        H_YZ 3 4 5
        X_ERROR(1) 0 3 6
        Y_ERROR(1) 1 4 7
        Z_ERROR(1) 2 5 8
        MX 0 1 2
        MY 3 4 5
        MZ 6 7 8
    )circuit",
        {
            false,
            true,
            true,
            true,
            false,
            true,
            true,
            true,
            false,
        }));

    ASSERT_TRUE(check(
        R"circuit(
        H_XZ 0 1 2
        H_YZ 3 4 5
        X_ERROR(1) 0 3 6
        Y_ERROR(1) 1 4 7
        Z_ERROR(1) 2 5 8
        MX !0 !1 !2
        MY !3 !4 !5
        MZ !6 !7 !8
    )circuit",
        {
            false,
            true,
            true,
            true,
            false,
            true,
            true,
            true,
            false,
        }));

    ASSERT_TRUE(check(
        R"circuit(
        H_XZ 0 1 2
        H_YZ 3 4 5
        X_ERROR(1) 0 3 6
        Y_ERROR(1) 1 4 7
        Z_ERROR(1) 2 5 8
        MRX 0 1 2
        MRY 3 4 5
        MRZ 6 7 8
        H_XZ 0
        H_YZ 3
        M 0 3 6
    )circuit",
        {
            false,
            true,
            true,
            true,
            false,
            true,
            true,
            true,
            false,
            false,
            false,
            false,
        }));

    ASSERT_TRUE(check(
        R"circuit(
        H_XZ 0 1 2
        H_YZ 3 4 5
        X_ERROR(1) 0 3 6
        Y_ERROR(1) 1 4 7
        Z_ERROR(1) 2 5 8
        MRX !0 !1 !2
        MRY !3 !4 !5
        MRZ !6 !7 !8
        H_XZ 0
        H_YZ 3
        M 0 3 6
    )circuit",
        {
            false,
            true,
            true,
            true,
            false,
            true,
            true,
            true,
            false,
            false,
            false,
            false,
        }));

    ASSERT_TRUE(check(
        R"circuit(
        H_XZ 0
        H_YZ 1
        Z_ERROR(1) 0 1
        X_ERROR(1) 2
        MRX 0 0
        MRY 1 1
        MRZ 2 2
    )circuit",
        {
            true,
            false,
            true,
            false,
            true,
            false,
        }));
}

TEST(FrameSimulator, noisy_measurement_x) {
    auto r = FrameSimulator::sample_flipped_measurements(
        Circuit(R"CIRCUIT(
        RX 0
        MX(0.05) 0
        MX 0
    )CIRCUIT"),
        10000,
        SHARED_TEST_RNG());
    ASSERT_FALSE(r[1].not_zero());
    auto m1 = r[0].popcnt();
    ASSERT_GT(m1, 300);
    ASSERT_LT(m1, 700);

    r = FrameSimulator::sample_flipped_measurements(
        Circuit(R"CIRCUIT(
        RX 0 1
        Z_ERROR(1) 0 1
        MX(0.05) 0 1
        MX 0 1
    )CIRCUIT"),
        5000,
        SHARED_TEST_RNG());
    auto m2 = r[0].popcnt() + r[1].popcnt();
    ASSERT_LT(m2, 10000 - 300);
    ASSERT_GT(m2, 10000 - 700);
    ASSERT_EQ(r[2].popcnt(), 5000);
    ASSERT_EQ(r[3].popcnt(), 5000);
}

TEST(FrameSimulator, noisy_measurement_y) {
    auto r = FrameSimulator::sample_flipped_measurements(
        Circuit(R"CIRCUIT(
        RY 0
        MY(0.05) 0
        MY 0
    )CIRCUIT"),
        10000,
        SHARED_TEST_RNG());
    ASSERT_FALSE(r[1].not_zero());
    auto m1 = r[0].popcnt();
    ASSERT_GT(m1, 300);
    ASSERT_LT(m1, 700);

    r = FrameSimulator::sample_flipped_measurements(
        Circuit(R"CIRCUIT(
        RY 0 1
        Z_ERROR(1) 0 1
        MY(0.05) 0 1
        MY 0 1
    )CIRCUIT"),
        5000,
        SHARED_TEST_RNG());
    auto m2 = r[0].popcnt() + r[1].popcnt();
    ASSERT_LT(m2, 10000 - 300);
    ASSERT_GT(m2, 10000 - 700);
    ASSERT_EQ(r[2].popcnt(), 5000);
    ASSERT_EQ(r[3].popcnt(), 5000);
}

TEST(FrameSimulator, noisy_measurement_z) {
    auto r = FrameSimulator::sample_flipped_measurements(
        Circuit(R"CIRCUIT(
        RZ 0
        MZ(0.05) 0
        MZ 0
    )CIRCUIT"),
        10000,
        SHARED_TEST_RNG());
    ASSERT_FALSE(r[1].not_zero());
    auto m1 = r[0].popcnt();
    ASSERT_GT(m1, 300);
    ASSERT_LT(m1, 700);

    r = FrameSimulator::sample_flipped_measurements(
        Circuit(R"CIRCUIT(
        RZ 0 1
        X_ERROR(1) 0 1
        MZ(0.05) 0 1
        MZ 0 1
    )CIRCUIT"),
        5000,
        SHARED_TEST_RNG());
    auto m2 = r[0].popcnt() + r[1].popcnt();
    ASSERT_LT(m2, 10000 - 300);
    ASSERT_GT(m2, 10000 - 700);
    ASSERT_EQ(r[2].popcnt(), 5000);
    ASSERT_EQ(r[3].popcnt(), 5000);
}

TEST(FrameSimulator, noisy_measure_reset_x) {
    auto r = FrameSimulator::sample_flipped_measurements(
        Circuit(R"CIRCUIT(
        RX 0
        MRX(0.05) 0
        MRX 0
    )CIRCUIT"),
        10000,
        SHARED_TEST_RNG());
    ASSERT_FALSE(r[1].not_zero());
    auto m1 = r[0].popcnt();
    ASSERT_GT(m1, 300);
    ASSERT_LT(m1, 700);

    r = FrameSimulator::sample_flipped_measurements(
        Circuit(R"CIRCUIT(
        RX 0 1
        Z_ERROR(1) 0 1
        MRX(0.05) 0 1
        MRX 0 1
    )CIRCUIT"),
        5000,
        SHARED_TEST_RNG());
    auto m2 = r[0].popcnt() + r[1].popcnt();
    ASSERT_LT(m2, 10000 - 300);
    ASSERT_GT(m2, 10000 - 700);
    ASSERT_EQ(r[2].popcnt(), 0);
    ASSERT_EQ(r[3].popcnt(), 0);
}

TEST(FrameSimulator, noisy_measure_reset_y) {
    auto r = FrameSimulator::sample_flipped_measurements(
        Circuit(R"CIRCUIT(
        RY 0
        MRY(0.05) 0
        MRY 0
    )CIRCUIT"),
        10000,
        SHARED_TEST_RNG());
    ASSERT_FALSE(r[1].not_zero());
    auto m1 = r[0].popcnt();
    ASSERT_GT(m1, 300);
    ASSERT_LT(m1, 700);

    r = FrameSimulator::sample_flipped_measurements(
        Circuit(R"CIRCUIT(
        RY 0 1
        Z_ERROR(1) 0 1
        MRY(0.05) 0 1
        MRY 0 1
    )CIRCUIT"),
        5000,
        SHARED_TEST_RNG());
    auto m2 = r[0].popcnt() + r[1].popcnt();
    ASSERT_LT(m2, 10000 - 300);
    ASSERT_GT(m2, 10000 - 700);
    ASSERT_EQ(r[2].popcnt(), 0);
    ASSERT_EQ(r[3].popcnt(), 0);
}

TEST(FrameSimulator, noisy_measure_reset_z) {
    auto r = FrameSimulator::sample_flipped_measurements(
        Circuit(R"CIRCUIT(
        RZ 0
        MRZ(0.05) 0
        MRZ 0
    )CIRCUIT"),
        10000,
        SHARED_TEST_RNG());
    ASSERT_FALSE(r[1].not_zero());
    auto m1 = r[0].popcnt();
    ASSERT_GT(m1, 300);
    ASSERT_LT(m1, 700);

    r = FrameSimulator::sample_flipped_measurements(
        Circuit(R"CIRCUIT(
        RZ 0 1
        X_ERROR(1) 0 1
        MRZ(0.05) 0 1
        MRZ 0 1
    )CIRCUIT"),
        5000,
        SHARED_TEST_RNG());
    auto m2 = r[0].popcnt() + r[1].popcnt();
    ASSERT_LT(m2, 10000 - 300);
    ASSERT_GT(m2, 10000 - 700);
    ASSERT_EQ(r[2].popcnt(), 0);
    ASSERT_EQ(r[3].popcnt(), 0);
}

TEST(FrameSimulator, measure_pauli_product_4body) {
    auto r = FrameSimulator::sample_flipped_measurements(
        Circuit(R"CIRCUIT(
        X_ERROR(0.5) 0 1 2 3
        Z_ERROR(0.5) 0 1 2 3
        MPP X0*X1*X2*X3
        MX 0 1 2 3 4 5
        MPP X2*X3*X4*X5
        MPP Z0*Z1*Z4*Z5 !Y0*Y1*Y4*Y5
    )CIRCUIT"),
        10,
        SHARED_TEST_RNG());
    for (size_t k = 0; k < 10; k++) {
        auto x0123 = r[0][k];
        auto x0 = r[1][k];
        auto x1 = r[2][k];
        auto x2 = r[3][k];
        auto x3 = r[4][k];
        auto x4 = r[5][k];
        auto x5 = r[6][k];
        auto x2345 = r[7][k];
        auto mz0145 = r[8][k];
        auto y0145 = r[9][k];
        ASSERT_EQ(x0123, x0 ^ x1 ^ x2 ^ x3);
        ASSERT_EQ(x2345, x2 ^ x3 ^ x4 ^ x5);
        ASSERT_EQ(y0145 ^ mz0145, x0123 ^ x2345);
    }
}

TEST(FrameSimulator, non_deterministic_pauli_product_detectors) {
    auto n = FrameSimulator::sample_flipped_measurements(
                 Circuit(R"CIRCUIT(
            MPP Z8*X9
            DETECTOR rec[-1]
        )CIRCUIT"),
                 1000,
                 SHARED_TEST_RNG())[0]
                 .popcnt();
    ASSERT_TRUE(400 < n && n < 600);

    n = FrameSimulator::sample_flipped_measurements(
            Circuit(R"CIRCUIT(
            MPP X9
            DETECTOR rec[-1]
        )CIRCUIT"),
            1000,
            SHARED_TEST_RNG())[0]
            .popcnt();
    ASSERT_TRUE(400 < n && n < 600);

    n = FrameSimulator::sample_flipped_measurements(
            Circuit(R"CIRCUIT(
            MX 9
            DETECTOR rec[-1]
        )CIRCUIT"),
            1000,
            SHARED_TEST_RNG())[0]
            .popcnt();
    ASSERT_TRUE(400 < n && n < 600);
}

TEST(FrameSimulator, ignores_sweep_controls_when_given_no_sweep_data) {
    auto n = FrameSimulator::sample_flipped_measurements(
                 Circuit(R"CIRCUIT(
            CNOT sweep[0] 0
            M 0
            DETECTOR rec[-1]
        )CIRCUIT"),
                 1000,
                 SHARED_TEST_RNG())[0]
                 .popcnt();
    ASSERT_EQ(n, 0);
}
