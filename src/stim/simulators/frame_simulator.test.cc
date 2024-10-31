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
#include "stim/mem/simd_word.test.h"
#include "stim/simulators/frame_simulator_util.h"
#include "stim/simulators/tableau_simulator.h"
#include "stim/util_bot/test_util.test.h"

using namespace stim;

TEST_EACH_WORD_SIZE_W(FrameSimulator, get_set_frame, {
    CircuitStats circuit_stats;
    circuit_stats.num_qubits = 6;
    circuit_stats.max_lookback = 999;
    FrameSimulator<W> sim(circuit_stats, FrameSimulatorMode::STORE_DETECTIONS_TO_MEMORY, 4, INDEPENDENT_TEST_RNG());
    ASSERT_EQ(sim.get_frame(0), PauliString<W>::from_str("______"));
    ASSERT_EQ(sim.get_frame(1), PauliString<W>::from_str("______"));
    ASSERT_EQ(sim.get_frame(2), PauliString<W>::from_str("______"));
    ASSERT_EQ(sim.get_frame(3), PauliString<W>::from_str("______"));
    sim.set_frame(0, PauliString<W>::from_str("_XYZ__"));
    ASSERT_EQ(sim.get_frame(0), PauliString<W>::from_str("_XYZ__"));
    ASSERT_EQ(sim.get_frame(1), PauliString<W>::from_str("______"));
    ASSERT_EQ(sim.get_frame(2), PauliString<W>::from_str("______"));
    ASSERT_EQ(sim.get_frame(3), PauliString<W>::from_str("______"));
    sim.set_frame(3, PauliString<W>::from_str("ZZZZZZ"));
    ASSERT_EQ(sim.get_frame(0), PauliString<W>::from_str("_XYZ__"));
    ASSERT_EQ(sim.get_frame(1), PauliString<W>::from_str("______"));
    ASSERT_EQ(sim.get_frame(2), PauliString<W>::from_str("______"));
    ASSERT_EQ(sim.get_frame(3), PauliString<W>::from_str("ZZZZZZ"));

    circuit_stats.num_qubits = 501;
    circuit_stats.max_lookback = 999;
    FrameSimulator<W> big_sim(
        circuit_stats, FrameSimulatorMode::STORE_DETECTIONS_TO_MEMORY, 1001, INDEPENDENT_TEST_RNG());
    big_sim.set_frame(258, PauliString<W>::from_func(false, 501, [](size_t k) {
                          return "_X"[k == 303];
                      }));
    ASSERT_EQ(big_sim.get_frame(258).ref().sparse_str(), "+X303");
    ASSERT_EQ(big_sim.get_frame(257).ref().sparse_str(), "+I");
})

template <size_t W>
bool is_bulk_frame_operation_consistent_with_tableau(const Gate &gate) {
    auto tableau = gate.tableau<W>();

    CircuitStats circuit_stats;
    circuit_stats.num_qubits = 500;
    circuit_stats.max_lookback = 10;
    size_t num_samples = 1000;
    FrameSimulator<W> sim(circuit_stats, FrameSimulatorMode::STORE_DETECTIONS_TO_MEMORY, 1000, INDEPENDENT_TEST_RNG());
    size_t num_targets = tableau.num_qubits;
    assert(num_targets == 1 || num_targets == 2);
    std::vector<GateTarget> targets{{101}, {403}, {202}, {100}};
    while (targets.size() > num_targets) {
        targets.pop_back();
    }
    auto rng = INDEPENDENT_TEST_RNG();
    for (size_t k = 7; k < num_samples; k += 101) {
        auto test_value = PauliString<W>::random(circuit_stats.num_qubits, rng);
        PauliStringRef<W> test_value_ref(test_value);
        sim.set_frame(k, test_value);
        sim.do_gate({gate.id, {}, targets, ""});
        for (size_t k2 = 0; k2 < targets.size(); k2 += num_targets) {
            size_t target_buf[2];
            if (num_targets == 1) {
                target_buf[0] = targets[k2].data;
                tableau.apply_within(test_value_ref, {&target_buf[0]});
            } else {
                target_buf[0] = targets[k2].data;
                target_buf[1] = targets[k2 + 1].data;
                tableau.apply_within(test_value_ref, {&target_buf[0], &target_buf[2]});
            }
        }
        test_value.sign = false;
        if (test_value != sim.get_frame(k)) {
            return false;
        }
    }
    return true;
}

TEST_EACH_WORD_SIZE_W(FrameSimulator, bulk_operations_consistent_with_tableau_data, {
    for (const auto &gate : GATE_DATA.items) {
        if (gate.has_known_unitary_matrix()) {
            EXPECT_TRUE(is_bulk_frame_operation_consistent_with_tableau<W>(gate)) << gate.name;
        }
    }
})

template <size_t W>
bool is_output_possible_promising_no_bare_resets(const Circuit &circuit, const simd_bits_range_ref<W> output) {
    auto tableau_sim = TableauSimulator<W>(INDEPENDENT_TEST_RNG(), circuit.count_qubits());
    size_t out_p = 0;
    bool pass = true;
    circuit.for_each_operation([&](const CircuitInstruction &op) {
        if (op.gate_type == GateType::M) {
            for (auto qf : op.targets) {
                tableau_sim.sign_bias = output[out_p] ? -1 : +1;
                tableau_sim.do_MZ(CircuitInstruction{GateType::M, {}, &qf, ""});
                if (output[out_p] != tableau_sim.measurement_record.storage.back()) {
                    pass = false;
                }
                out_p++;
            }
        } else {
            tableau_sim.do_gate(op);
        }
    });
    return pass;
}

TEST_EACH_WORD_SIZE_W(FrameSimulator, test_util_is_output_possible, {
    auto circuit = Circuit(
        "H 0\n"
        "CNOT 0 1\n"
        "X 0\n"
        "M 0\n"
        "M 1\n");
    auto data = simd_bits<W>(2);
    data.u64[0] = 0;
    ASSERT_EQ(false, is_output_possible_promising_no_bare_resets<W>(circuit, data));
    data.u64[0] = 1;
    ASSERT_EQ(true, is_output_possible_promising_no_bare_resets<W>(circuit, data));
    data.u64[0] = 2;
    ASSERT_EQ(true, is_output_possible_promising_no_bare_resets<W>(circuit, data));
    data.u64[0] = 3;
    ASSERT_EQ(false, is_output_possible_promising_no_bare_resets<W>(circuit, data));
})

template <size_t W>
bool is_sim_frame_consistent_with_sim_tableau(const char *program_text) {
    auto rng = INDEPENDENT_TEST_RNG();
    auto circuit = Circuit(program_text);
    auto reference_sample = TableauSimulator<W>::reference_sample_circuit(circuit);
    auto samples = sample_batch_measurements(circuit, reference_sample, 10, rng, true);

    for (size_t k = 0; k < 10; k++) {
        simd_bits_range_ref<W> sample = samples[k];
        if (!is_output_possible_promising_no_bare_resets<W>(circuit, sample)) {
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

#define EXPECT_SAMPLES_POSSIBLE(program) EXPECT_TRUE(is_sim_frame_consistent_with_sim_tableau<W>(program)) << program

TEST_EACH_WORD_SIZE_W(FrameSimulator, consistency, {
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
})

TEST_EACH_WORD_SIZE_W(FrameSimulator, sample_batch_measurements_writing_results_to_disk, {
    auto rng = INDEPENDENT_TEST_RNG();
    auto circuit = Circuit(
        "X 0\n"
        "M 1\n"
        "M 0\n"
        "M 2\n"
        "M 3\n");
    auto ref = TableauSimulator<W>::reference_sample_circuit(circuit);
    auto r = sample_batch_measurements(circuit, ref, 10, rng, true);
    for (size_t k = 0; k < 10; k++) {
        ASSERT_EQ(r[k].u64[0], 2);
    }

    FILE *tmp = tmpfile();
    sample_batch_measurements_writing_results_to_disk(circuit, ref, 5, tmp, SampleFormat::SAMPLE_FORMAT_01, rng);
    ASSERT_EQ(rewind_read_close(tmp), "0100\n0100\n0100\n0100\n0100\n");

    tmp = tmpfile();
    sample_batch_measurements_writing_results_to_disk(circuit, ref, 5, tmp, SampleFormat::SAMPLE_FORMAT_B8, rng);
    rewind(tmp);
    for (size_t k = 0; k < 5; k++) {
        ASSERT_EQ(getc(tmp), 2);
    }
    ASSERT_EQ(getc(tmp), EOF);

    tmp = tmpfile();
    sample_batch_measurements_writing_results_to_disk(circuit, ref, 64, tmp, SampleFormat::SAMPLE_FORMAT_PTB64, rng);
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
})

TEST_EACH_WORD_SIZE_W(FrameSimulator, big_circuit_measurements, {
    auto rng = INDEPENDENT_TEST_RNG();
    Circuit circuit;
    for (uint32_t k = 0; k < 1250; k += 3) {
        circuit.safe_append_u("X", {k});
    }
    for (uint32_t k = 0; k < 1250; k++) {
        circuit.safe_append_u("M", {k});
    }
    auto ref = TableauSimulator<W>::reference_sample_circuit(circuit);
    auto r = sample_batch_measurements(circuit, ref, 750, rng, true);
    for (size_t i = 0; i < 750; i++) {
        for (size_t k = 0; k < 1250; k++) {
            ASSERT_EQ(r[i][k], k % 3 == 0) << k;
        }
    }

    FILE *tmp = tmpfile();
    sample_batch_measurements_writing_results_to_disk(circuit, ref, 750, tmp, SampleFormat::SAMPLE_FORMAT_01, rng);
    rewind(tmp);
    for (size_t s = 0; s < 750; s++) {
        for (size_t k = 0; k < 1250; k++) {
            ASSERT_EQ(getc(tmp), "01"[k % 3 == 0]);
        }
        ASSERT_EQ(getc(tmp), '\n');
    }
    ASSERT_EQ(getc(tmp), EOF);

    tmp = tmpfile();
    sample_batch_measurements_writing_results_to_disk(circuit, ref, 750, tmp, SampleFormat::SAMPLE_FORMAT_B8, rng);
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
})

TEST_EACH_WORD_SIZE_W(FrameSimulator, run_length_measurement_formats, {
    auto rng = INDEPENDENT_TEST_RNG();
    Circuit circuit;
    circuit.safe_append_u("X", {100, 500, 501, 551, 1200});
    for (uint32_t k = 0; k < 1250; k++) {
        circuit.safe_append_u("M", {k});
    }
    auto ref = TableauSimulator<W>::reference_sample_circuit(circuit);

    FILE *tmp = tmpfile();
    sample_batch_measurements_writing_results_to_disk(circuit, ref, 3, tmp, SampleFormat::SAMPLE_FORMAT_HITS, rng);
    ASSERT_EQ(rewind_read_close(tmp), "100,500,501,551,1200\n100,500,501,551,1200\n100,500,501,551,1200\n");

    tmp = tmpfile();
    sample_batch_measurements_writing_results_to_disk(circuit, ref, 3, tmp, SampleFormat::SAMPLE_FORMAT_DETS, rng);
    ASSERT_EQ(
        rewind_read_close(tmp),
        "shot M100 M500 M501 M551 M1200\nshot M100 M500 M501 M551 M1200\nshot M100 M500 M501 M551 M1200\n");

    tmp = tmpfile();
    sample_batch_measurements_writing_results_to_disk(circuit, ref, 3, tmp, SampleFormat::SAMPLE_FORMAT_R8, rng);
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
})

TEST_EACH_WORD_SIZE_W(FrameSimulator, big_circuit_random_measurements, {
    auto rng = INDEPENDENT_TEST_RNG();
    Circuit circuit;
    for (uint32_t k = 0; k < 270; k++) {
        circuit.safe_append_u("H_XZ", {k});
    }
    for (uint32_t k = 0; k < 270; k++) {
        circuit.safe_append_u("M", {k});
    }
    auto ref = TableauSimulator<W>::reference_sample_circuit(circuit);
    auto r = sample_batch_measurements(circuit, ref, 1000, rng, true);
    for (size_t k = 0; k < 1000; k++) {
        ASSERT_TRUE(r[k].not_zero()) << k;
    }
})

TEST_EACH_WORD_SIZE_W(FrameSimulator, correlated_error, {
    auto rng = INDEPENDENT_TEST_RNG();
    simd_bits<W> ref(5);
    simd_bits<W> expected(5);

    expected.clear();
    ASSERT_EQ(
        sample_batch_measurements(
            Circuit(R"circuit(
        CORRELATED_ERROR(0) X0 X1
        ELSE_CORRELATED_ERROR(0) X1 X2
        ELSE_CORRELATED_ERROR(0) X2 X3
        M 0 1 2 3
    )circuit"),
            ref,
            1,
            rng,
            true)[0],
        expected);

    expected.clear();
    expected[0] = true;
    expected[1] = true;
    ASSERT_EQ(
        sample_batch_measurements(
            Circuit(R"circuit(
        CORRELATED_ERROR(1) X0 X1
        ELSE_CORRELATED_ERROR(0) X1 X2
        ELSE_CORRELATED_ERROR(0) X2 X3
        M 0 1 2 3
    )circuit"),
            ref,
            1,
            rng,
            true)[0],
        expected);

    expected.clear();
    expected[1] = true;
    expected[2] = true;
    ASSERT_EQ(
        sample_batch_measurements(
            Circuit(R"circuit(
        CORRELATED_ERROR(0) X0 X1
        ELSE_CORRELATED_ERROR(1) X1 X2
        ELSE_CORRELATED_ERROR(0) X2 X3
        M 0 1 2 3
    )circuit"),
            ref,
            1,
            rng,
            true)[0],
        expected);

    expected.clear();
    expected[2] = true;
    expected[3] = true;
    ASSERT_EQ(
        sample_batch_measurements(
            Circuit(R"circuit(
        CORRELATED_ERROR(0) X0 X1
        ELSE_CORRELATED_ERROR(0) X1 X2
        ELSE_CORRELATED_ERROR(1) X2 X3
        M 0 1 2 3
    )circuit"),
            ref,
            1,
            rng,
            true)[0],
        expected);

    expected.clear();
    expected[0] = true;
    expected[1] = true;
    ASSERT_EQ(
        sample_batch_measurements(
            Circuit(R"circuit(
        CORRELATED_ERROR(1) X0 X1
        ELSE_CORRELATED_ERROR(1) X1 X2
        ELSE_CORRELATED_ERROR(0) X2 X3
        M 0 1 2 3
    )circuit"),
            ref,
            1,
            rng,
            true)[0],
        expected);

    expected.clear();
    expected[0] = true;
    expected[1] = true;
    ASSERT_EQ(
        sample_batch_measurements(
            Circuit(R"circuit(
        CORRELATED_ERROR(1) X0 X1
        ELSE_CORRELATED_ERROR(1) X1 X2
        ELSE_CORRELATED_ERROR(1) X2 X3
        M 0 1 2 3
    )circuit"),
            ref,
            1,
            rng,
            true)[0],
        expected);

    expected.clear();
    expected[0] = true;
    expected[1] = true;
    expected[3] = true;
    expected[4] = true;
    ASSERT_EQ(
        sample_batch_measurements(
            Circuit(R"circuit(
                CORRELATED_ERROR(1) X0 X1
                ELSE_CORRELATED_ERROR(1) X1 X2
                ELSE_CORRELATED_ERROR(1) X2 X3
                CORRELATED_ERROR(1) X3 X4
                M 0 1 2 3 4
            )circuit"),
            ref,
            1,
            rng,
            true)[0],
        expected);

    int hits[3]{};
    size_t n = 10000;
    auto samples = sample_batch_measurements(
        Circuit(R"circuit(
                CORRELATED_ERROR(0.5) X0
                ELSE_CORRELATED_ERROR(0.25) X1
                ELSE_CORRELATED_ERROR(0.75) X2
                M 0 1 2
            )circuit"),
        ref,
        n,
        rng,
        true);
    for (size_t k = 0; k < n; k++) {
        hits[0] += samples[k][0];
        hits[1] += samples[k][1];
        hits[2] += samples[k][2];
    }
    ASSERT_TRUE(0.45 * n < hits[0] && hits[0] < 0.55 * n);
    ASSERT_TRUE((0.125 - 0.05) * n < hits[1] && hits[1] < (0.125 + 0.05) * n);
    ASSERT_TRUE((0.28125 - 0.05) * n < hits[2] && hits[2] < (0.28125 + 0.05) * n);
})

TEST_EACH_WORD_SIZE_W(FrameSimulator, quantum_cannot_control_classical, {
    auto rng = INDEPENDENT_TEST_RNG();
    simd_bits<W> ref(128);

    // Quantum controlling classical operation is not allowed.
    ASSERT_THROW(
        {
            sample_batch_measurements(
                Circuit(R"circuit(
            M 0
            CNOT 1 rec[-1]
        )circuit"),
                ref,
                1,
                rng,
                true);
        },
        std::invalid_argument);
    ASSERT_THROW(
        {
            sample_batch_measurements(
                Circuit(R"circuit(
            M 0
            CY 1 rec[-1]
        )circuit"),
                ref,
                1,
                rng,
                true);
        },
        std::invalid_argument);
    ASSERT_THROW(
        {
            sample_batch_measurements(
                Circuit(R"circuit(
            M 0
            YCZ rec[-1] 1
        )circuit"),
                ref,
                1,
                rng,
                true);
        },
        std::invalid_argument);
    ASSERT_THROW(
        {
            sample_batch_measurements(
                Circuit(R"circuit(
            M 0
            XCZ rec[-1] 1
        )circuit"),
                ref,
                1,
                rng,
                true);
        },
        std::invalid_argument);
    ASSERT_THROW(
        {
            sample_batch_measurements(
                Circuit(R"circuit(
            M 0
            SWAP 1 rec[-1]
        )circuit"),
                ref,
                1,
                rng,
                true);
        },
        std::invalid_argument);
})

TEST_EACH_WORD_SIZE_W(FrameSimulator, classical_can_control_quantum, {
    auto rng = INDEPENDENT_TEST_RNG();
    simd_bits<W> ref(128);
    simd_bits<W> expected(5);
    expected.clear();
    expected[0] = true;
    expected[1] = true;
    ASSERT_EQ(
        sample_batch_measurements(
            Circuit(R"circuit(
        X_ERROR(1) 0
        M !0
        CX rec[-1] 1
        M 1
    )circuit"),
            ref,
            1,
            rng,
            true)[0],
        expected);
    ASSERT_EQ(
        sample_batch_measurements(
            Circuit(R"circuit(
        X_ERROR(1) 0
        M !0
        CY rec[-1] 1
        M 1
    )circuit"),
            ref,
            1,
            rng,
            true)[0],
        expected);
    ASSERT_EQ(
        sample_batch_measurements(
            Circuit(R"circuit(
        X_ERROR(1) 0
        M !0
        XCZ 1 rec[-1]
        M 1
    )circuit"),
            ref,
            1,
            rng,
            true)[0],
        expected);
    ASSERT_EQ(
        sample_batch_measurements(
            Circuit(R"circuit(
        X_ERROR(1) 0
        M !0
        YCZ 1 rec[-1]
        M 1
    )circuit"),
            ref,
            1,
            rng,
            true)[0],
        expected);
})

TEST_EACH_WORD_SIZE_W(FrameSimulator, classical_controls, {
    auto rng = INDEPENDENT_TEST_RNG();
    simd_bits<W> ref(128);
    simd_bits<W> expected(5);

    expected.clear();
    ASSERT_EQ(
        sample_batch_measurements(
            Circuit(R"circuit(
        M 0
        CX rec[-1] 1
        M 1
    )circuit"),
            ref,
            1,
            rng,
            true)[0],
        expected);

    expected.clear();
    ASSERT_EQ(
        sample_batch_measurements(
            Circuit(R"circuit(
        M !0
        CX rec[-1] 1
        M 1
    )circuit"),
            ref,
            1,
            rng,
            true)[0],
        expected);

    expected.clear();
    expected[0] = true;
    expected[1] = true;
    ASSERT_EQ(
        sample_batch_measurements(
            Circuit(R"circuit(
        X_ERROR(1) 0
        M !0
        CX rec[-1] 1
        M 1
    )circuit"),
            ref,
            1,
            rng,
            true)[0],
        expected);

    expected.clear();
    expected[0] = true;
    expected[1] = true;
    ASSERT_EQ(
        sample_batch_measurements(
            Circuit(R"circuit(
        X_ERROR(1) 0
        M 0
        CX rec[-1] 1
        M 1
    )circuit"),
            ref,
            1,
            rng,
            true)[0],
        expected);
    auto r = sample_batch_measurements(
        Circuit(R"circuit(
        X_ERROR(0.5) 0
        M 0
        CX rec[-1] 1
        M 1
    )circuit"),
        ref,
        1000,
        rng,
        true);
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
        sample_batch_measurements(
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
            rng,
            true)[0],
        expected);

    expected.clear();
    expected[0] = true;
    expected[1] = true;
    ASSERT_EQ(
        sample_batch_measurements(
            Circuit(R"circuit(
        X_ERROR(1) 0
        M 0
        CY rec[-1] 1
        M 1
    )circuit"),
            ref,
            1,
            rng,
            true)[0],
        expected);
})

TEST_EACH_WORD_SIZE_W(FrameSimulator, record_gets_trimmed, {
    Circuit c = Circuit("M 0 1 2 3 4 5 6 7 8 9");
    FrameSimulator<W> sim(
        c.compute_stats(), FrameSimulatorMode::STREAM_MEASUREMENTS_TO_DISK, 768, INDEPENDENT_TEST_RNG());
    MeasureRecordBatchWriter b(tmpfile(), 768, SampleFormat::SAMPLE_FORMAT_B8);
    for (size_t k = 0; k < 1000; k++) {
        sim.do_MZ(c.operations[0]);
        sim.m_record.intermediate_write_unwritten_results_to(b, simd_bits<W>(0));
        ASSERT_LT(sim.m_record.storage.num_major_bits_padded(), 2500);
    }
})

TEST_EACH_WORD_SIZE_W(FrameSimulator, stream_huge_case, {
    auto rng = INDEPENDENT_TEST_RNG();
    FILE *tmp = tmpfile();
    sample_batch_measurements_writing_results_to_disk(
        Circuit(R"CIRCUIT(
            X_ERROR(1) 2
            REPEAT 100000 {
                M 0 1 2 3
            }
        )CIRCUIT"),
        simd_bits<W>(0),
        256,
        tmp,
        SampleFormat::SAMPLE_FORMAT_B8,
        rng);
    rewind(tmp);
    for (size_t k = 0; k < 256 * 100000 * 4 / 8; k++) {
        ASSERT_EQ(getc(tmp), 0x44);
    }
    ASSERT_EQ(getc(tmp), EOF);
})

TEST_EACH_WORD_SIZE_W(FrameSimulator, block_results_single_shot, {
    auto rng = INDEPENDENT_TEST_RNG();
    auto circuit = Circuit(R"circuit(
        REPEAT 10000 {
            X_ERROR(1) 0
            MR 0
            M 0 0
        }
    )circuit");
    FILE *tmp = tmpfile();
    sample_batch_measurements_writing_results_to_disk(
        circuit, simd_bits<W>(0), 3, tmp, SampleFormat::SAMPLE_FORMAT_01, rng);

    auto result = rewind_read_close(tmp);
    for (size_t k = 0; k < 30000; k += 3) {
        ASSERT_EQ(result[k], '1') << k;
        ASSERT_EQ(result[k + 1], '0') << (k + 1);
        ASSERT_EQ(result[k + 2], '0') << (k + 2);
    }
    ASSERT_EQ(result[30000], '\n');
})

TEST_EACH_WORD_SIZE_W(FrameSimulator, block_results_triple_shot, {
    auto rng = INDEPENDENT_TEST_RNG();
    auto circuit = Circuit(R"circuit(
        REPEAT 10000 {
            X_ERROR(1) 0
            MR 0
            M 0 0
        }
    )circuit");
    FILE *tmp = tmpfile();
    sample_batch_measurements_writing_results_to_disk(
        circuit, simd_bits<W>(0), 3, tmp, SampleFormat::SAMPLE_FORMAT_01, rng);

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
})

TEST_EACH_WORD_SIZE_W(FrameSimulator, stream_results, {
    auto rng = INDEPENDENT_TEST_RNG();
    DebugForceResultStreamingRaii force_streaming;
    auto circuit = Circuit(R"circuit(
        REPEAT 10000 {
            X_ERROR(1) 0
            MR 0
            M 0 0
        }
    )circuit");
    FILE *tmp = tmpfile();
    sample_batch_measurements_writing_results_to_disk(
        circuit, simd_bits<W>(0), 3, tmp, SampleFormat::SAMPLE_FORMAT_01, rng);

    auto result = rewind_read_close(tmp);
    for (size_t k = 0; k < 30000; k += 3) {
        ASSERT_EQ(result[k], '1') << k;
        ASSERT_EQ(result[k + 1], '0') << (k + 1);
        ASSERT_EQ(result[k + 2], '0') << (k + 2);
    }
    ASSERT_EQ(result[30000], '\n');
})

TEST_EACH_WORD_SIZE_W(FrameSimulator, stream_many_shots, {
    auto rng = INDEPENDENT_TEST_RNG();
    DebugForceResultStreamingRaii force_streaming;
    auto circuit = Circuit(R"circuit(
        X_ERROR(1) 1
        M 0 1 2
    )circuit");
    FILE *tmp = tmpfile();
    sample_batch_measurements_writing_results_to_disk(
        circuit, simd_bits<W>(0), 2049, tmp, SampleFormat::SAMPLE_FORMAT_01, rng);

    auto result = rewind_read_close(tmp);
    ASSERT_EQ(result.size(), 2049 * 4);
    for (size_t k = 0; k < 2049 * 4; k += 4) {
        ASSERT_EQ(result[k], '0') << k;
        ASSERT_EQ(result[k + 1], '1') << (k + 1);
        ASSERT_EQ(result[k + 2], '0') << (k + 2);
        ASSERT_EQ(result[k + 3], '\n') << (k + 3);
    }
})

TEST_EACH_WORD_SIZE_W(FrameSimulator, stream_results_triple_shot, {
    auto rng = INDEPENDENT_TEST_RNG();
    DebugForceResultStreamingRaii force_streaming;
    auto circuit = Circuit(R"circuit(
        REPEAT 10000 {
            X_ERROR(1) 0
            MR 0
            M 0 0
        }
    )circuit");
    FILE *tmp = tmpfile();
    sample_batch_measurements_writing_results_to_disk(
        circuit, simd_bits<W>(0), 3, tmp, SampleFormat::SAMPLE_FORMAT_01, rng);

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
})

TEST_EACH_WORD_SIZE_W(FrameSimulator, measure_y_without_reset_doesnt_reset, {
    auto rng = INDEPENDENT_TEST_RNG();
    auto r = sample_batch_measurements(
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
        simd_bits<W>(0),
        10000,
        rng,
        false);
    ASSERT_EQ(r[0].popcnt(), 0);
    ASSERT_EQ(r[1].popcnt(), 0);
    ASSERT_EQ(r[2].popcnt(), 10000);
    ASSERT_EQ(r[3].popcnt(), 10000);
    ASSERT_EQ(r[4].popcnt(), 0);
    ASSERT_EQ(r[5].popcnt(), 0);

    r = sample_batch_measurements(
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
        simd_bits<W>(0),
        10000,
        rng,
        false);
    ASSERT_EQ(r[0].popcnt(), 0);
    ASSERT_EQ(r[1].popcnt(), 0);
    ASSERT_EQ(r[2].popcnt(), 10000);
    ASSERT_EQ(r[3].popcnt(), 0);
    ASSERT_EQ(r[4].popcnt(), 10000);
    ASSERT_EQ(r[5].popcnt(), 0);
})

TEST_EACH_WORD_SIZE_W(FrameSimulator, resets_vs_measurements, {
    auto rng = INDEPENDENT_TEST_RNG();
    auto check = [&](const char *circuit, std::vector<bool> results) {
        simd_bits<W> ref(results.size());
        for (size_t k = 0; k < results.size(); k++) {
            ref[k] = results[k];
        }
        simd_bit_table<W> t = sample_batch_measurements(Circuit(circuit), ref, 100, rng, true);
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
})

TEST_EACH_WORD_SIZE_W(FrameSimulator, noisy_measurement_x, {
    auto rng = INDEPENDENT_TEST_RNG();
    auto r = sample_batch_measurements(
        Circuit(R"CIRCUIT(
            RX 0
            MX(0.05) 0
            MX 0
        )CIRCUIT"),
        simd_bits<W>(0),
        10000,
        rng,
        false);
    ASSERT_FALSE(r[1].not_zero());
    auto m1 = r[0].popcnt();
    ASSERT_GT(m1, 300);
    ASSERT_LT(m1, 700);

    r = sample_batch_measurements(
        Circuit(R"CIRCUIT(
            RX 0 1
            Z_ERROR(1) 0 1
            MX(0.05) 0 1
            MX 0 1
        )CIRCUIT"),
        simd_bits<W>(0),
        5000,
        rng,
        false);
    auto m2 = r[0].popcnt() + r[1].popcnt();
    ASSERT_LT(m2, 10000 - 300);
    ASSERT_GT(m2, 10000 - 700);
    ASSERT_EQ(r[2].popcnt(), 5000);
    ASSERT_EQ(r[3].popcnt(), 5000);
})

TEST_EACH_WORD_SIZE_W(FrameSimulator, noisy_measurement_y, {
    auto rng = INDEPENDENT_TEST_RNG();
    auto r = sample_batch_measurements(
        Circuit(R"CIRCUIT(
        RY 0
        MY(0.05) 0
        MY 0
    )CIRCUIT"),
        simd_bits<W>(0),
        10000,
        rng,
        false);
    ASSERT_FALSE(r[1].not_zero());
    auto m1 = r[0].popcnt();
    ASSERT_GT(m1, 300);
    ASSERT_LT(m1, 700);

    r = sample_batch_measurements(
        Circuit(R"CIRCUIT(
        RY 0 1
        Z_ERROR(1) 0 1
        MY(0.05) 0 1
        MY 0 1
    )CIRCUIT"),
        simd_bits<W>(0),
        5000,
        rng,
        false);
    auto m2 = r[0].popcnt() + r[1].popcnt();
    ASSERT_LT(m2, 10000 - 300);
    ASSERT_GT(m2, 10000 - 700);
    ASSERT_EQ(r[2].popcnt(), 5000);
    ASSERT_EQ(r[3].popcnt(), 5000);
})

TEST_EACH_WORD_SIZE_W(FrameSimulator, noisy_measurement_z, {
    auto rng = INDEPENDENT_TEST_RNG();
    auto r = sample_batch_measurements(
        Circuit(R"CIRCUIT(
        RZ 0
        MZ(0.05) 0
        MZ 0
    )CIRCUIT"),
        simd_bits<W>(0),
        10000,
        rng,
        false);
    ASSERT_FALSE(r[1].not_zero());
    auto m1 = r[0].popcnt();
    ASSERT_GT(m1, 300);
    ASSERT_LT(m1, 700);

    r = sample_batch_measurements(
        Circuit(R"CIRCUIT(
            RZ 0 1
            X_ERROR(1) 0 1
            MZ(0.05) 0 1
            MZ 0 1
        )CIRCUIT"),
        simd_bits<W>(0),
        5000,
        rng,
        false);
    auto m2 = r[0].popcnt() + r[1].popcnt();
    ASSERT_LT(m2, 10000 - 300);
    ASSERT_GT(m2, 10000 - 700);
    ASSERT_EQ(r[2].popcnt(), 5000);
    ASSERT_EQ(r[3].popcnt(), 5000);
})

TEST_EACH_WORD_SIZE_W(FrameSimulator, noisy_measurement_reset_x, {
    auto rng = INDEPENDENT_TEST_RNG();
    auto r = sample_batch_measurements(
        Circuit(R"CIRCUIT(
            RX 0
            MRX(0.05) 0
            MRX 0
        )CIRCUIT"),
        simd_bits<W>(0),
        10000,
        rng,
        false);
    ASSERT_FALSE(r[1].not_zero());
    auto m1 = r[0].popcnt();
    ASSERT_GT(m1, 300);
    ASSERT_LT(m1, 700);

    r = sample_batch_measurements(
        Circuit(R"CIRCUIT(
            RX 0 1
            Z_ERROR(1) 0 1
            MRX(0.05) 0 1
            MRX 0 1
        )CIRCUIT"),
        simd_bits<W>(0),
        5000,
        rng,
        false);
    auto m2 = r[0].popcnt() + r[1].popcnt();
    ASSERT_LT(m2, 10000 - 300);
    ASSERT_GT(m2, 10000 - 700);
    ASSERT_EQ(r[2].popcnt(), 0);
    ASSERT_EQ(r[3].popcnt(), 0);
})

TEST_EACH_WORD_SIZE_W(FrameSimulator, noisy_measurement_reset_y, {
    auto rng = INDEPENDENT_TEST_RNG();
    auto r = sample_batch_measurements(
        Circuit(R"CIRCUIT(
            RY 0
            MRY(0.05) 0
            MRY 0
        )CIRCUIT"),
        simd_bits<W>(0),
        10000,
        rng,
        false);
    ASSERT_FALSE(r[1].not_zero());
    auto m1 = r[0].popcnt();
    ASSERT_GT(m1, 300);
    ASSERT_LT(m1, 700);

    r = sample_batch_measurements(
        Circuit(R"CIRCUIT(
            RY 0 1
            Z_ERROR(1) 0 1
            MRY(0.05) 0 1
            MRY 0 1
        )CIRCUIT"),
        simd_bits<W>(0),
        5000,
        rng,
        false);
    auto m2 = r[0].popcnt() + r[1].popcnt();
    ASSERT_LT(m2, 10000 - 300);
    ASSERT_GT(m2, 10000 - 700);
    ASSERT_EQ(r[2].popcnt(), 0);
    ASSERT_EQ(r[3].popcnt(), 0);
})

TEST_EACH_WORD_SIZE_W(FrameSimulator, noisy_measurement_reset_z, {
    auto rng = INDEPENDENT_TEST_RNG();
    auto r = sample_batch_measurements(
        Circuit(R"CIRCUIT(
            RZ 0
            MRZ(0.05) 0
            MRZ 0
        )CIRCUIT"),
        simd_bits<W>(0),
        10000,
        rng,
        false);
    ASSERT_FALSE(r[1].not_zero());
    auto m1 = r[0].popcnt();
    ASSERT_GT(m1, 300);
    ASSERT_LT(m1, 700);

    r = sample_batch_measurements(
        Circuit(R"CIRCUIT(
            RZ 0 1
            X_ERROR(1) 0 1
            MRZ(0.05) 0 1
            MRZ 0 1
        )CIRCUIT"),
        simd_bits<W>(0),
        5000,
        rng,
        false);
    auto m2 = r[0].popcnt() + r[1].popcnt();
    ASSERT_LT(m2, 10000 - 300);
    ASSERT_GT(m2, 10000 - 700);
    ASSERT_EQ(r[2].popcnt(), 0);
    ASSERT_EQ(r[3].popcnt(), 0);
})

TEST_EACH_WORD_SIZE_W(FrameSimulator, measure_pauli_product_4body, {
    auto rng = INDEPENDENT_TEST_RNG();
    auto r = sample_batch_measurements(
        Circuit(R"CIRCUIT(
            X_ERROR(0.5) 0 1 2 3
            Z_ERROR(0.5) 0 1 2 3
            MPP X0*X1*X2*X3
            MX 0 1 2 3 4 5
            MPP X2*X3*X4*X5
            MPP Z0*Z1*Z4*Z5 !Y0*Y1*Y4*Y5
        )CIRCUIT"),
        simd_bits<W>(0),
        10,
        rng,
        false);
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
})

TEST_EACH_WORD_SIZE_W(FrameSimulator, non_deterministic_pauli_product_detectors, {
    auto rng = INDEPENDENT_TEST_RNG();
    auto n = sample_batch_measurements(
                 Circuit(R"CIRCUIT(
                     MPP Z8*X9
                     DETECTOR rec[-1]
                 )CIRCUIT"),
                 simd_bits<W>(0),
                 1000,
                 rng,
                 false)[0]
                 .popcnt();
    ASSERT_TRUE(400 < n && n < 600);

    n = sample_batch_measurements(
            Circuit(R"CIRCUIT(
                MPP X9
                DETECTOR rec[-1]
            )CIRCUIT"),
            simd_bits<W>(0),
            1000,
            rng,
            false)[0]
            .popcnt();
    ASSERT_TRUE(400 < n && n < 600);

    n = sample_batch_measurements(
            Circuit(R"CIRCUIT(
                MX 9
                DETECTOR rec[-1]
            )CIRCUIT"),
            simd_bits<W>(0),
            1000,
            rng,
            false)[0]
            .popcnt();
    ASSERT_TRUE(400 < n && n < 600);
})

TEST_EACH_WORD_SIZE_W(FrameSimulator, ignores_sweep_controls_when_given_no_sweep_data, {
    auto rng = INDEPENDENT_TEST_RNG();
    auto n = sample_batch_measurements(
                 Circuit(R"CIRCUIT(
                     CNOT sweep[0] 0
                     M 0
                     DETECTOR rec[-1]
                 )CIRCUIT"),
                 simd_bits<W>(0),
                 1000,
                 rng,
                 false)[0]
                 .popcnt();
    ASSERT_EQ(n, 0);
})

TEST_EACH_WORD_SIZE_W(FrameSimulator, reconfigure_for, {
    auto circuit = Circuit(R"CIRCUIT(
        X_ERROR(1) 0
        M 0
        DETECTOR rec[-1]
    )CIRCUIT");

    FrameSimulator<W> frame_sim(
        circuit.compute_stats(), FrameSimulatorMode::STORE_DETECTIONS_TO_MEMORY, 0, INDEPENDENT_TEST_RNG());
    frame_sim.configure_for(circuit.compute_stats(), FrameSimulatorMode::STORE_DETECTIONS_TO_MEMORY, 256);
    frame_sim.reset_all();
    frame_sim.do_circuit(circuit);
    ASSERT_EQ(frame_sim.det_record.storage[0].popcnt(), 256);
})

TEST_EACH_WORD_SIZE_W(FrameSimulator, mpad, {
    auto rng = INDEPENDENT_TEST_RNG();
    auto circuit = Circuit(R"CIRCUIT(
        MPAD 0 1
    )CIRCUIT");
    auto sample =
        sample_batch_measurements(circuit, TableauSimulator<W>::reference_sample_circuit(circuit), 100, rng, false);
    for (size_t k = 0; k < 100; k++) {
        ASSERT_EQ(sample[0][k], false);
        ASSERT_EQ(sample[1][k], true);
    }
})

TEST_EACH_WORD_SIZE_W(FrameSimulator, mxxyyzz_basis, {
    auto rng = INDEPENDENT_TEST_RNG();
    auto circuit = Circuit(R"CIRCUIT(
        RX 0 1
        MXX 0 1
        RY 0 1
        MYY 0 1
        RZ 0 1
        MZZ 0 1
    )CIRCUIT");
    auto sample =
        sample_batch_measurements(circuit, TableauSimulator<W>::reference_sample_circuit(circuit), 100, rng, false);
    for (size_t k = 0; k < 100; k++) {
        ASSERT_EQ(sample[0][k], false);
        ASSERT_EQ(sample[1][k], false);
        ASSERT_EQ(sample[2][k], false);
    }
})

TEST_EACH_WORD_SIZE_W(FrameSimulator, mxxyyzz_inversion, {
    auto rng = INDEPENDENT_TEST_RNG();
    auto circuit = Circuit(R"CIRCUIT(
        MXX 0 1 0 !1 !0 1 !0 !1
        MYY 0 1 0 !1 !0 1 !0 !1
        MZZ 0 1 0 !1 !0 1 !0 !1
    )CIRCUIT");
    auto sample =
        sample_batch_measurements(circuit, TableauSimulator<W>::reference_sample_circuit(circuit), 100, rng, false);
    for (size_t k = 0; k < 100; k++) {
        ASSERT_EQ(sample[1][k], !sample[0][k]);
        ASSERT_EQ(sample[2][k], !sample[0][k]);
        ASSERT_EQ(sample[3][k], sample[0][k]);
        ASSERT_EQ(sample[5][k], !sample[4][k]);
        ASSERT_EQ(sample[6][k], !sample[4][k]);
        ASSERT_EQ(sample[7][k], sample[4][k]);
        ASSERT_EQ(sample[8][k], 1 ^ sample[0][k] ^ sample[4][k]);
        ASSERT_EQ(sample[9][k], !sample[8][k]);
        ASSERT_EQ(sample[10][k], !sample[8][k]);
        ASSERT_EQ(sample[11][k], sample[8][k]);
    }
})

TEST_EACH_WORD_SIZE_W(FrameSimulator, runs_on_general_circuit, {
    auto rng = INDEPENDENT_TEST_RNG();
    auto circuit = generate_test_circuit_with_all_operations();
    auto sample =
        sample_batch_measurements(circuit, TableauSimulator<W>::reference_sample_circuit(circuit), 100, rng, false);
    ASSERT_GT(sample.num_simd_words_minor, 0);
    ASSERT_GT(sample.num_simd_words_major, 0);
})

TEST_EACH_WORD_SIZE_W(FrameSimulator, heralded_erase_detect_statistics, {
    auto circuit = Circuit(R"CIRCUIT(
        MXX 0 1
        MZZ 0 1
        HERALDED_ERASE(0.1) 0
        MXX 0 1
        MZZ 0 1
        DETECTOR rec[-1] rec[-4]
        DETECTOR rec[-2] rec[-5]
        DETECTOR rec[-3]
    )CIRCUIT");
    size_t n;
    std::array<size_t, 8> bins{};
    FrameSimulator<W> sim(
        circuit.compute_stats(), FrameSimulatorMode::STORE_DETECTIONS_TO_MEMORY, 1024, INDEPENDENT_TEST_RNG());
    for (n = 0; n < 1024 * 256; n += 1024) {
        sim.reset_all();
        sim.do_circuit(circuit);
        auto sample = sim.det_record.storage.transposed();
        for (size_t k = 0; k < 1024; k++) {
            bins[sample[k].u8[0]]++;
        }
    }
    EXPECT_NEAR(bins[0] / (double)n, 0.9, 0.05);
    EXPECT_EQ(bins[1], 0);
    EXPECT_EQ(bins[2], 0);
    EXPECT_EQ(bins[3], 0);
    EXPECT_NEAR(bins[4] / (double)n, 0.025, 0.02);
    EXPECT_NEAR(bins[5] / (double)n, 0.025, 0.02);
    EXPECT_NEAR(bins[6] / (double)n, 0.025, 0.02);
    EXPECT_NEAR(bins[7] / (double)n, 0.025, 0.02);
})

TEST_EACH_WORD_SIZE_W(FrameSimulator, heralded_pauli_channel_1_statistics, {
    auto circuit = Circuit(R"CIRCUIT(
        MXX 0 1
        MZZ 0 1
        HERALDED_PAULI_CHANNEL_1(0.05, 0.10, 0.15, 0.25) 0
        MXX 0 1
        MZZ 0 1
        DETECTOR rec[-1] rec[-4]
        DETECTOR rec[-2] rec[-5]
        DETECTOR rec[-3]
    )CIRCUIT");
    size_t n;
    std::array<size_t, 8> bins{};
    FrameSimulator<W> sim(
        circuit.compute_stats(), FrameSimulatorMode::STORE_DETECTIONS_TO_MEMORY, 1024, INDEPENDENT_TEST_RNG());
    for (n = 0; n < 1024 * 256; n += 1024) {
        sim.reset_all();
        sim.do_circuit(circuit);
        auto sample = sim.det_record.storage.transposed();
        for (size_t k = 0; k < 1024; k++) {
            bins[sample[k].u8[0]]++;
        }
    }
    EXPECT_NEAR(bins[0] / (double)n, 0.45, 0.05);
    EXPECT_EQ(bins[1], 0);
    EXPECT_EQ(bins[2], 0);
    EXPECT_EQ(bins[3], 0);
    EXPECT_NEAR(bins[4] / (double)n, 0.05, 0.04);
    EXPECT_NEAR(bins[5] / (double)n, 0.10, 0.04);
    EXPECT_NEAR(bins[6] / (double)n, 0.25, 0.04);
    EXPECT_NEAR(bins[7] / (double)n, 0.15, 0.04);
})

TEST_EACH_WORD_SIZE_W(FrameSimulator, heralded_erase_statistics_offset_by_2, {
    auto circuit = Circuit(R"CIRCUIT(
        MXX 2 3
        MZZ 2 3
        HERALDED_ERASE(0.1) 2
        MXX 2 3
        MZZ 2 3
        DETECTOR rec[-1] rec[-4]
        DETECTOR rec[-2] rec[-5]
        DETECTOR rec[-3]
    )CIRCUIT");
    size_t n;
    std::array<size_t, 8> bins{};
    FrameSimulator<W> sim(
        circuit.compute_stats(), FrameSimulatorMode::STORE_DETECTIONS_TO_MEMORY, 1024, INDEPENDENT_TEST_RNG());
    for (n = 0; n < 1024 * 256; n += 1024) {
        sim.reset_all();
        sim.do_circuit(circuit);
        auto sample = sim.det_record.storage.transposed();
        for (size_t k = 0; k < 1024; k++) {
            bins[sample[k].u8[0]]++;
        }
    }
    EXPECT_NEAR(bins[0] / (double)n, 0.9, 0.05);
    EXPECT_EQ(bins[1], 0);
    EXPECT_EQ(bins[2], 0);
    EXPECT_EQ(bins[3], 0);
    EXPECT_NEAR(bins[4] / (double)n, 0.025, 0.02);
    EXPECT_NEAR(bins[5] / (double)n, 0.025, 0.02);
    EXPECT_NEAR(bins[6] / (double)n, 0.025, 0.02);
    EXPECT_NEAR(bins[7] / (double)n, 0.025, 0.02);
})

TEST_EACH_WORD_SIZE_W(FrameSimulator, heralded_pauli_channel_1_statistics_offset_by_2, {
    auto circuit = Circuit(R"CIRCUIT(
        MXX 2 3
        MZZ 2 3
        HERALDED_PAULI_CHANNEL_1(0.05, 0.10, 0.15, 0.25) 2
        MXX 2 3
        MZZ 2 3
        DETECTOR rec[-1] rec[-4]
        DETECTOR rec[-2] rec[-5]
        DETECTOR rec[-3]
    )CIRCUIT");
    size_t n;
    std::array<size_t, 8> bins{};
    FrameSimulator<W> sim(
        circuit.compute_stats(), FrameSimulatorMode::STORE_DETECTIONS_TO_MEMORY, 1024, INDEPENDENT_TEST_RNG());
    for (n = 0; n < 1024 * 256; n += 1024) {
        sim.reset_all();
        sim.do_circuit(circuit);
        auto sample = sim.det_record.storage.transposed();
        for (size_t k = 0; k < 1024; k++) {
            bins[sample[k].u8[0]]++;
        }
    }
    EXPECT_NEAR(bins[0] / (double)n, 0.45, 0.05);
    EXPECT_EQ(bins[1], 0);
    EXPECT_EQ(bins[2], 0);
    EXPECT_EQ(bins[3], 0);
    EXPECT_NEAR(bins[4] / (double)n, 0.05, 0.04);
    EXPECT_NEAR(bins[5] / (double)n, 0.10, 0.04);
    EXPECT_NEAR(bins[6] / (double)n, 0.25, 0.04);
    EXPECT_NEAR(bins[7] / (double)n, 0.15, 0.04);
})
