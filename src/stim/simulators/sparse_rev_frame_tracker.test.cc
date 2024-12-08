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

#include "stim/simulators/sparse_rev_frame_tracker.h"

#include "gtest/gtest.h"

#include "stim/circuit/circuit.test.h"
#include "stim/mem/simd_word.test.h"
#include "stim/util_bot/test_util.test.h"

using namespace stim;

template <size_t W>
SparseUnsignedRevFrameTracker _tracker_from_pauli_string(const char *text) {
    auto p = PauliString<W>::from_str(text);
    SparseUnsignedRevFrameTracker result(p.num_qubits, 0, 0);
    for (size_t q = 0; q < p.num_qubits; q++) {
        if (p.xs[q]) {
            result.xs[q].xor_item(DemTarget::observable_id(0));
        }
        if (p.zs[q]) {
            result.zs[q].xor_item(DemTarget::observable_id(0));
        }
    }
    return result;
}

TEST_EACH_WORD_SIZE_W(SparseUnsignedRevFrameTracker, undo_tableau_h, {
    SparseUnsignedRevFrameTracker actual(0, 0, 0);
    std::vector<uint32_t> targets{0};
    auto tableau = GATE_DATA.at("H").tableau<W>();

    actual = _tracker_from_pauli_string<W>("I");
    actual.undo_tableau<W>(tableau, targets);
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("I"));

    actual = _tracker_from_pauli_string<W>("X");
    actual.undo_tableau<W>(tableau, targets);
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("Z"));

    actual = _tracker_from_pauli_string<W>("Y");
    actual.undo_tableau<W>(tableau, targets);
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("Y"));

    actual = _tracker_from_pauli_string<W>("Z");
    actual.undo_tableau<W>(tableau, targets);
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("X"));
})

TEST_EACH_WORD_SIZE_W(SparseUnsignedRevFrameTracker, undo_tableau_s, {
    SparseUnsignedRevFrameTracker actual(0, 0, 0);
    std::vector<uint32_t> targets{0};
    auto tableau = GATE_DATA.at("S").tableau<W>();

    actual = _tracker_from_pauli_string<W>("I");
    actual.undo_tableau<W>(tableau, targets);
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("I"));

    actual = _tracker_from_pauli_string<W>("X");
    actual.undo_tableau<W>(tableau, targets);
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("Y"));

    actual = _tracker_from_pauli_string<W>("Y");
    actual.undo_tableau<W>(tableau, targets);
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("X"));

    actual = _tracker_from_pauli_string<W>("Z");
    actual.undo_tableau<W>(tableau, targets);
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("Z"));
})

TEST_EACH_WORD_SIZE_W(SparseUnsignedRevFrameTracker, undo_tableau_c_xyz, {
    SparseUnsignedRevFrameTracker actual(0, 0, 0);
    std::vector<uint32_t> targets{0};
    auto tableau = GATE_DATA.at("C_XYZ").tableau<W>();

    actual = _tracker_from_pauli_string<W>("I");
    actual.undo_tableau<W>(tableau, targets);
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("I"));

    actual = _tracker_from_pauli_string<W>("X");
    actual.undo_tableau<W>(tableau, targets);
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("Z"));

    actual = _tracker_from_pauli_string<W>("Y");
    actual.undo_tableau<W>(tableau, targets);
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("X"));

    actual = _tracker_from_pauli_string<W>("Z");
    actual.undo_tableau<W>(tableau, targets);
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("Y"));
})

TEST_EACH_WORD_SIZE_W(SparseUnsignedRevFrameTracker, undo_tableau_cx, {
    SparseUnsignedRevFrameTracker actual(0, 0, 0);
    std::vector<uint32_t> targets{0, 1};
    auto tableau = GATE_DATA.at("CX").tableau<W>();

    actual = _tracker_from_pauli_string<W>("II");
    actual.undo_tableau<W>(tableau, targets);
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("II"));

    actual = _tracker_from_pauli_string<W>("IZ");
    actual.undo_tableau<W>(tableau, targets);
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("ZZ"));

    actual = _tracker_from_pauli_string<W>("ZI");
    actual.undo_tableau<W>(tableau, targets);
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("ZI"));

    actual = _tracker_from_pauli_string<W>("XI");
    actual.undo_tableau<W>(tableau, targets);
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("XX"));

    actual = _tracker_from_pauli_string<W>("IX");
    actual.undo_tableau<W>(tableau, targets);
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("IX"));

    actual = _tracker_from_pauli_string<W>("YY");
    actual.undo_tableau<W>(tableau, targets);
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("XZ"));
})

static std::vector<GateTarget> qubit_targets(const std::vector<uint32_t> &targets) {
    std::vector<GateTarget> result;
    for (uint32_t t : targets) {
        result.push_back(GateTarget::qubit(t));
    }
    return result;
}

TEST_EACH_WORD_SIZE_W(SparseUnsignedRevFrameTracker, fuzz_all_unitary_gates_vs_tableau, {
    auto rng = INDEPENDENT_TEST_RNG();
    for (const auto &gate : GATE_DATA.items) {
        if (gate.has_known_unitary_matrix()) {
            size_t n = (gate.flags & GATE_TARGETS_PAIRS) ? 2 : 1;
            SparseUnsignedRevFrameTracker tracker_gate(n + 3, 0, 0);
            for (size_t q = 0; q < n; q++) {
                uint64_t mask = rng();
                for (size_t d = 0; d < 32; d++) {
                    if (mask & 1) {
                        tracker_gate.xs[q].sorted_items.push_back(DemTarget::relative_detector_id(d));
                    }
                    mask >>= 1;
                    if (mask & 1) {
                        tracker_gate.zs[q].sorted_items.push_back(DemTarget::relative_detector_id(d));
                    }
                    mask >>= 1;
                }
            }

            SparseUnsignedRevFrameTracker tracker_tableau = tracker_gate;
            std::vector<uint32_t> targets;
            for (size_t k = 0; k < n; k++) {
                targets.push_back(n - k + 1);
            }
            tracker_gate.undo_gate(CircuitInstruction{gate.id, {}, qubit_targets(targets), ""});
            tracker_tableau.undo_tableau<W>(gate.tableau<W>(), targets);
            EXPECT_EQ(tracker_gate, tracker_tableau) << gate.name;
        }
    }
})

TEST_EACH_WORD_SIZE_W(SparseUnsignedRevFrameTracker, RX, {
    SparseUnsignedRevFrameTracker actual(0, 0, 0);

    actual = _tracker_from_pauli_string<W>("III");
    actual.undo_RX(CircuitInstruction{GateType::RX, {}, qubit_targets({0, 2}), ""});
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("III"));

    actual = _tracker_from_pauli_string<W>("XXX");
    actual.undo_RX(CircuitInstruction{GateType::RX, {}, qubit_targets({0, 2}), ""});
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("IXI"));

    actual = _tracker_from_pauli_string<W>("XIZ");
    ASSERT_THROW({ actual.undo_RX({GateType::RX, {}, qubit_targets({0, 2}), ""}); }, std::invalid_argument);
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("XIZ"));

    actual = _tracker_from_pauli_string<W>("YIX");
    ASSERT_THROW({ actual.undo_RX({GateType::RX, {}, qubit_targets({0, 2}), ""}); }, std::invalid_argument);
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("YIX"));
})

TEST_EACH_WORD_SIZE_W(SparseUnsignedRevFrameTracker, RY, {
    SparseUnsignedRevFrameTracker actual(0, 0, 0);

    actual = _tracker_from_pauli_string<W>("III");
    actual.undo_RY({GateType::RY, {}, qubit_targets({0, 2}), ""});
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("III"));

    actual = _tracker_from_pauli_string<W>("YYY");
    actual.undo_RY({GateType::RY, {}, qubit_targets({0, 2}), ""});
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("IYI"));

    actual = _tracker_from_pauli_string<W>("YIZ");
    ASSERT_THROW({ actual.undo_RY({GateType::RY, {}, qubit_targets({0, 2}), ""}); }, std::invalid_argument);
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("YIZ"));

    actual = _tracker_from_pauli_string<W>("XIY");
    ASSERT_THROW({ actual.undo_RY({GateType::RY, {}, qubit_targets({0, 2}), ""}); }, std::invalid_argument);
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("XIY"));
})

TEST_EACH_WORD_SIZE_W(SparseUnsignedRevFrameTracker, RZ, {
    SparseUnsignedRevFrameTracker actual(0, 0, 0);

    actual = _tracker_from_pauli_string<W>("III");
    actual.undo_RZ({GateType::R, {}, qubit_targets({0, 2}), ""});
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("III"));

    actual = _tracker_from_pauli_string<W>("ZZZ");
    actual.undo_RZ({GateType::R, {}, qubit_targets({0, 2}), ""});
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("IZI"));

    actual = _tracker_from_pauli_string<W>("YIZ");
    ASSERT_THROW({ actual.undo_RZ({GateType::R, {}, qubit_targets({0, 2}), ""}); }, std::invalid_argument);
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("YIZ"));

    actual = _tracker_from_pauli_string<W>("ZIX");
    ASSERT_THROW({ actual.undo_RZ({GateType::R, {}, qubit_targets({0, 2}), ""}); }, std::invalid_argument);
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("ZIX"));
})

TEST_EACH_WORD_SIZE_W(SparseUnsignedRevFrameTracker, MX, {
    SparseUnsignedRevFrameTracker actual(0, 0, 0);

    actual = _tracker_from_pauli_string<W>("III");
    actual.num_measurements_in_past = 2;
    actual.undo_MX({GateType::MX, {}, qubit_targets({0, 2}), ""});
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("III"));

    actual = _tracker_from_pauli_string<W>("XXX");
    actual.num_measurements_in_past = 2;
    actual.undo_MX({GateType::MX, {}, qubit_targets({0, 2}), ""});
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("XXX"));

    actual = _tracker_from_pauli_string<W>("XIZ");
    ASSERT_THROW({ actual.undo_MX({GateType::MX, {}, qubit_targets({0, 2}), ""}); }, std::invalid_argument);
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("XIZ"));

    actual = _tracker_from_pauli_string<W>("YIX");
    ASSERT_THROW({ actual.undo_MX({GateType::MX, {}, qubit_targets({0, 2}), ""}); }, std::invalid_argument);
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("YIX"));
})

TEST_EACH_WORD_SIZE_W(SparseUnsignedRevFrameTracker, MY, {
    SparseUnsignedRevFrameTracker actual(0, 0, 0);

    actual = _tracker_from_pauli_string<W>("III");
    actual.num_measurements_in_past = 2;
    actual.undo_MY({GateType::MY, {}, qubit_targets({0, 2}), ""});
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("III"));

    actual = _tracker_from_pauli_string<W>("YYY");
    actual.num_measurements_in_past = 2;
    actual.undo_MY({GateType::MY, {}, qubit_targets({0, 2}), ""});
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("YYY"));

    actual = _tracker_from_pauli_string<W>("YIZ");
    ASSERT_THROW({ actual.undo_MY({GateType::MY, {}, qubit_targets({0, 2}), ""}); }, std::invalid_argument);
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("YIZ"));

    actual = _tracker_from_pauli_string<W>("XIY");
    ASSERT_THROW({ actual.undo_MY({GateType::MY, {}, qubit_targets({0, 2}), ""}); }, std::invalid_argument);
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("XIY"));
})

TEST_EACH_WORD_SIZE_W(SparseUnsignedRevFrameTracker, MZ, {
    SparseUnsignedRevFrameTracker actual(0, 0, 0);

    actual = _tracker_from_pauli_string<W>("III");
    actual.num_measurements_in_past = 2;
    actual.undo_MZ({GateType::M, {}, qubit_targets({0, 2}), ""});
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("III"));

    actual = _tracker_from_pauli_string<W>("ZZZ");
    actual.num_measurements_in_past = 2;
    actual.undo_MZ({GateType::M, {}, qubit_targets({0, 2}), ""});
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("ZZZ"));

    actual = _tracker_from_pauli_string<W>("YIZ");
    ASSERT_THROW({ actual.undo_MZ({GateType::M, {}, qubit_targets({0, 2}), ""}); }, std::invalid_argument);
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("YIZ"));

    actual = _tracker_from_pauli_string<W>("ZIX");
    ASSERT_THROW({ actual.undo_MZ({GateType::M, {}, qubit_targets({0, 2}), ""}); }, std::invalid_argument);
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("ZIX"));
})

TEST_EACH_WORD_SIZE_W(SparseUnsignedRevFrameTracker, MRX, {
    SparseUnsignedRevFrameTracker actual(0, 0, 0);

    actual = _tracker_from_pauli_string<W>("III");
    actual.num_measurements_in_past = 2;
    actual.undo_MRX({GateType::MRX, {}, qubit_targets({0, 2}), ""});
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("III"));

    actual = _tracker_from_pauli_string<W>("XXX");
    actual.num_measurements_in_past = 2;
    actual.undo_MRX({GateType::MRX, {}, qubit_targets({0, 2}), ""});
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("IXI"));

    actual = _tracker_from_pauli_string<W>("XIZ");
    ASSERT_THROW({ actual.undo_MRX({GateType::MRX, {}, qubit_targets({0, 2}), ""}); }, std::invalid_argument);
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("XIZ"));

    actual = _tracker_from_pauli_string<W>("YIX");
    ASSERT_THROW({ actual.undo_MRX({GateType::MRX, {}, qubit_targets({0, 2}), ""}); }, std::invalid_argument);
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("YIX"));
})

TEST_EACH_WORD_SIZE_W(SparseUnsignedRevFrameTracker, MRY, {
    SparseUnsignedRevFrameTracker actual(0, 0, 0);

    actual = _tracker_from_pauli_string<W>("III");
    actual.num_measurements_in_past = 2;
    actual.undo_MRY({GateType::MRY, {}, qubit_targets({0, 2}), ""});
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("III"));

    actual = _tracker_from_pauli_string<W>("YYY");
    actual.num_measurements_in_past = 2;
    actual.undo_MRY({GateType::MRY, {}, qubit_targets({0, 2}), ""});
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("IYI"));

    actual = _tracker_from_pauli_string<W>("YIZ");
    ASSERT_THROW({ actual.undo_MRY({GateType::MRY, {}, qubit_targets({0, 2}), ""}); }, std::invalid_argument);
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("YIZ"));

    actual = _tracker_from_pauli_string<W>("XIY");
    ASSERT_THROW({ actual.undo_MRY({GateType::MRY, {}, qubit_targets({0, 2}), ""}); }, std::invalid_argument);
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("XIY"));
})

TEST_EACH_WORD_SIZE_W(SparseUnsignedRevFrameTracker, MRZ, {
    SparseUnsignedRevFrameTracker actual(0, 0, 0);

    actual = _tracker_from_pauli_string<W>("III");
    actual.num_measurements_in_past = 2;
    actual.undo_MRZ({GateType::MR, {}, qubit_targets({0, 2}), ""});
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("III"));

    actual = _tracker_from_pauli_string<W>("ZZZ");
    actual.num_measurements_in_past = 2;
    actual.undo_MRZ({GateType::MR, {}, qubit_targets({0, 2}), ""});
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("IZI"));

    actual = _tracker_from_pauli_string<W>("YIZ");
    ASSERT_THROW({ actual.undo_MRZ({GateType::MR, {}, qubit_targets({0, 2}), ""}); }, std::invalid_argument);
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("YIZ"));

    actual = _tracker_from_pauli_string<W>("ZIX");
    ASSERT_THROW({ actual.undo_MRZ({GateType::MR, {}, qubit_targets({0, 2}), ""}); }, std::invalid_argument);
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("ZIX"));
})

TEST_EACH_WORD_SIZE_W(SparseUnsignedRevFrameTracker, feedback_from_measurement, {
    SparseUnsignedRevFrameTracker actual(0, 0, 0);

    actual = _tracker_from_pauli_string<W>("III");
    actual.num_measurements_in_past = 2;
    actual.rec_bits[0].xor_item(DemTarget::observable_id(0));
    actual.undo_MX({GateType::MX, {}, qubit_targets({0, 2}), ""});
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("XII"));
    actual = _tracker_from_pauli_string<W>("XII");
    actual.num_measurements_in_past = 2;
    actual.rec_bits[0].xor_item(DemTarget::observable_id(0));
    actual.undo_MX({GateType::MX, {}, qubit_targets({0, 2}), ""});
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("III"));

    actual = _tracker_from_pauli_string<W>("III");
    actual.num_measurements_in_past = 2;
    actual.rec_bits[0].xor_item(DemTarget::observable_id(0));
    actual.undo_MY({GateType::MY, {}, qubit_targets({0, 2}), ""});
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("YII"));
    actual = _tracker_from_pauli_string<W>("YII");
    actual.num_measurements_in_past = 2;
    actual.rec_bits[0].xor_item(DemTarget::observable_id(0));
    actual.undo_MY({GateType::MY, {}, qubit_targets({0, 2}), ""});
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("III"));

    actual = _tracker_from_pauli_string<W>("III");
    actual.num_measurements_in_past = 2;
    actual.rec_bits[0].xor_item(DemTarget::observable_id(0));
    actual.undo_MZ({GateType::M, {}, qubit_targets({0, 2}), ""});
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("ZII"));
    actual = _tracker_from_pauli_string<W>("ZII");
    actual.num_measurements_in_past = 2;
    actual.rec_bits[0].xor_item(DemTarget::observable_id(0));
    actual.undo_MZ({GateType::M, {}, qubit_targets({0, 2}), ""});
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("III"));

    actual = _tracker_from_pauli_string<W>("III");
    actual.num_measurements_in_past = 2;
    actual.rec_bits[0].xor_item(DemTarget::observable_id(0));
    actual.undo_MRX({GateType::MRX, {}, qubit_targets({0, 2}), ""});
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("XII"));
    actual = _tracker_from_pauli_string<W>("XII");
    actual.num_measurements_in_past = 2;
    actual.rec_bits[0].xor_item(DemTarget::observable_id(0));
    actual.undo_MRX({GateType::MRX, {}, qubit_targets({0, 2}), ""});
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("XII"));

    actual = _tracker_from_pauli_string<W>("III");
    actual.num_measurements_in_past = 2;
    actual.rec_bits[0].xor_item(DemTarget::observable_id(0));
    actual.undo_MRY({GateType::MRY, {}, qubit_targets({0, 2}), ""});
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("YII"));
    actual = _tracker_from_pauli_string<W>("YII");
    actual.num_measurements_in_past = 2;
    actual.rec_bits[0].xor_item(DemTarget::observable_id(0));
    actual.undo_MRY({GateType::MRY, {}, qubit_targets({0, 2}), ""});
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("YII"));

    actual = _tracker_from_pauli_string<W>("III");
    actual.num_measurements_in_past = 2;
    actual.rec_bits[0].xor_item(DemTarget::observable_id(0));
    actual.undo_MRZ({GateType::MR, {}, qubit_targets({0, 2}), ""});
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("ZII"));
    actual = _tracker_from_pauli_string<W>("ZII");
    actual.num_measurements_in_past = 2;
    actual.rec_bits[0].xor_item(DemTarget::observable_id(0));
    actual.undo_MRZ({GateType::MR, {}, qubit_targets({0, 2}), ""});
    ASSERT_EQ(actual, _tracker_from_pauli_string<W>("ZII"));
})

TEST_EACH_WORD_SIZE_W(SparseUnsignedRevFrameTracker, feedback_into_measurement, {
    SparseUnsignedRevFrameTracker actual(0, 0, 0);
    SparseUnsignedRevFrameTracker expected(0, 0, 0);
    std::vector<GateTarget> targets{GateTarget::rec(-5), GateTarget::qubit(0)};

    actual = _tracker_from_pauli_string<W>("XII");
    actual.num_measurements_in_past = 12;
    actual.undo_ZCX({GateType::CX, {}, targets, ""});
    expected = _tracker_from_pauli_string<W>("XII");
    expected.num_measurements_in_past = 12;
    ASSERT_EQ(actual, expected);

    actual = _tracker_from_pauli_string<W>("YII");
    actual.num_measurements_in_past = 12;
    actual.undo_ZCX({GateType::CX, {}, targets, ""});
    expected = _tracker_from_pauli_string<W>("YII");
    expected.num_measurements_in_past = 12;
    expected.rec_bits[7].xor_item(DemTarget::observable_id(0));
    ASSERT_EQ(actual, expected);

    actual = _tracker_from_pauli_string<W>("ZII");
    actual.num_measurements_in_past = 12;
    actual.undo_ZCX({GateType::CX, {}, targets, ""});
    expected = _tracker_from_pauli_string<W>("ZII");
    expected.num_measurements_in_past = 12;
    expected.rec_bits[7].xor_item(DemTarget::observable_id(0));
    ASSERT_EQ(actual, expected);

    actual = _tracker_from_pauli_string<W>("XII");
    actual.num_measurements_in_past = 12;
    actual.undo_ZCY({GateType::CY, {}, targets, ""});
    expected = _tracker_from_pauli_string<W>("XII");
    expected.num_measurements_in_past = 12;
    expected.rec_bits[7].xor_item(DemTarget::observable_id(0));
    ASSERT_EQ(actual, expected);

    actual = _tracker_from_pauli_string<W>("YII");
    actual.num_measurements_in_past = 12;
    actual.undo_ZCY({GateType::CY, {}, targets, ""});
    expected = _tracker_from_pauli_string<W>("YII");
    expected.num_measurements_in_past = 12;
    ASSERT_EQ(actual, expected);

    actual = _tracker_from_pauli_string<W>("ZII");
    actual.num_measurements_in_past = 12;
    actual.undo_ZCY({GateType::CY, {}, targets, ""});
    expected = _tracker_from_pauli_string<W>("ZII");
    expected.num_measurements_in_past = 12;
    expected.rec_bits[7].xor_item(DemTarget::observable_id(0));
    ASSERT_EQ(actual, expected);

    actual = _tracker_from_pauli_string<W>("XII");
    actual.num_measurements_in_past = 12;
    actual.undo_ZCZ({GateType::CZ, {}, targets, ""});
    expected = _tracker_from_pauli_string<W>("XII");
    expected.num_measurements_in_past = 12;
    expected.rec_bits[7].xor_item(DemTarget::observable_id(0));
    ASSERT_EQ(actual, expected);

    actual = _tracker_from_pauli_string<W>("YII");
    actual.num_measurements_in_past = 12;
    actual.undo_ZCZ({GateType::CZ, {}, targets, ""});
    expected = _tracker_from_pauli_string<W>("YII");
    expected.num_measurements_in_past = 12;
    expected.rec_bits[7].xor_item(DemTarget::observable_id(0));
    ASSERT_EQ(actual, expected);

    actual = _tracker_from_pauli_string<W>("ZII");
    actual.num_measurements_in_past = 12;
    actual.undo_ZCZ({GateType::CZ, {}, targets, ""});
    expected = _tracker_from_pauli_string<W>("ZII");
    expected.num_measurements_in_past = 12;
    ASSERT_EQ(actual, expected);
})

TEST(SparseUnsignedRevFrameTracker, undo_circuit_feedback) {
    SparseUnsignedRevFrameTracker actual(20, 100, 10);
    actual.undo_circuit(Circuit(R"CIRCUIT(
        MR 0
        CX rec[-1] 0
        M 0
        DETECTOR rec[-1]
    )CIRCUIT"));
    SparseUnsignedRevFrameTracker expected(20, 98, 9);
    expected.zs[0].xor_item(DemTarget::relative_detector_id(9));
    ASSERT_EQ(actual, expected);
}

TEST(SparseUnsignedRevFrameTracker, equal_shifted) {
    SparseUnsignedRevFrameTracker actual(10, 200, 300);
    SparseUnsignedRevFrameTracker expected(10, 2000, 3000);
    ASSERT_TRUE(actual.is_shifted_copy(expected));

    actual.rec_bits[200 - 5].xor_item(DemTarget::observable_id(2));
    ASSERT_FALSE(actual.is_shifted_copy(expected));
    expected.rec_bits[2000 - 5].xor_item(DemTarget::observable_id(2));
    ASSERT_TRUE(actual.is_shifted_copy(expected));

    actual.rec_bits[200 - 5].xor_item(DemTarget::relative_detector_id(300 - 7));
    ASSERT_FALSE(actual.is_shifted_copy(expected));
    expected.rec_bits[2000 - 5].xor_item(DemTarget::relative_detector_id(300 - 7));
    ASSERT_FALSE(actual.is_shifted_copy(expected));
    expected.rec_bits[2000 - 5].xor_item(DemTarget::relative_detector_id(300 - 7));
    expected.rec_bits[2000 - 5].xor_item(DemTarget::relative_detector_id(3000 - 7));
    ASSERT_TRUE(actual.is_shifted_copy(expected));

    actual.xs[5].xor_item(DemTarget::observable_id(3));
    ASSERT_FALSE(actual.is_shifted_copy(expected));
    expected.xs[5].xor_item(DemTarget::observable_id(3));
    ASSERT_TRUE(actual.is_shifted_copy(expected));

    actual.zs[6].xor_item(DemTarget::observable_id(3));
    ASSERT_FALSE(actual.is_shifted_copy(expected));
    expected.zs[6].xor_item(DemTarget::observable_id(3));
    ASSERT_TRUE(actual.is_shifted_copy(expected));

    actual.xs[5].xor_item(DemTarget::relative_detector_id(300 - 13));
    ASSERT_FALSE(actual.is_shifted_copy(expected));
    expected.xs[5].xor_item(DemTarget::relative_detector_id(3000 - 13));
    ASSERT_TRUE(actual.is_shifted_copy(expected));

    actual.zs[6].xor_item(DemTarget::relative_detector_id(300 - 13));
    ASSERT_FALSE(actual.is_shifted_copy(expected));
    expected.zs[6].xor_item(DemTarget::relative_detector_id(3000 - 13));
    ASSERT_TRUE(actual.is_shifted_copy(expected));

    actual.shift(2000 - 200, 3000 - 300);
    ASSERT_EQ(actual, expected);
}

TEST(SparseUnsignedRevFrameTracker, undo_circuit_loop_big_period) {
    SparseUnsignedRevFrameTracker actual(20, 5000000000000ULL, 4000000000000ULL);
    SparseUnsignedRevFrameTracker expected = actual;
    Circuit body(R"CIRCUIT(
        CX 0 1 1 2 2 3 3 0
        M 0 0 1
        DETECTOR rec[-2] rec[-3]
        OBSERVABLE_INCLUDE(3) rec[-1]
    )CIRCUIT");

    actual.undo_loop(body, 500);
    expected.undo_loop_by_unrolling(body, 500);

    ASSERT_EQ(actual, expected);

    // Make test timeout if folding fails.
    actual.undo_loop(body, 1000000000001ULL);
    ASSERT_EQ(actual.zs[0].sorted_items, std::vector<DemTarget>{DemTarget::observable_id(3)});
    ASSERT_EQ(actual.num_measurements_in_past, 5000000000000ULL - 3000000000003ULL - 1500);
    ASSERT_EQ(actual.num_detectors_in_past, 4000000000000ULL - 1000000000001ULL - 500);
}

TEST_EACH_WORD_SIZE_W(SparseUnsignedRevFrameTracker, mpad, {
    SparseUnsignedRevFrameTracker actual(0, 0, 0);
    SparseUnsignedRevFrameTracker expected(0, 0, 0);

    actual = _tracker_from_pauli_string<W>("IIZ");
    actual.num_measurements_in_past = 2;
    actual.rec_bits[1].xor_item(DemTarget::relative_detector_id(5));
    actual.undo_MPAD({GateType::MRY, {}, qubit_targets({0}), ""});

    expected = _tracker_from_pauli_string<W>("IIZ");
    expected.num_measurements_in_past = 1;

    ASSERT_EQ(actual, expected);
})

TEST_EACH_WORD_SIZE_W(SparseUnsignedRevFrameTracker, mxx, {
    SparseUnsignedRevFrameTracker actual(0, 0, 0);
    SparseUnsignedRevFrameTracker expected(0, 0, 0);

    actual = _tracker_from_pauli_string<W>("III");
    actual.num_measurements_in_past = 2;
    actual.rec_bits[1].xor_item(DemTarget::observable_id(0));
    actual.undo_circuit(Circuit("MXX 0 1"));
    expected = _tracker_from_pauli_string<W>("XXI");
    expected.num_measurements_in_past = 1;
    ASSERT_EQ(actual, expected);

    actual = _tracker_from_pauli_string<W>("ZZI");
    actual.num_measurements_in_past = 2;
    actual.rec_bits[1].xor_item(DemTarget::observable_id(0));
    actual.undo_circuit(Circuit("MXX !0 !1"));
    expected = _tracker_from_pauli_string<W>("YYI");
    expected.num_measurements_in_past = 1;
    ASSERT_EQ(actual, expected);

    actual = _tracker_from_pauli_string<W>("ZXI");
    actual.num_measurements_in_past = 2;
    actual.rec_bits[1].xor_item(DemTarget::observable_id(0));
    ASSERT_THROW({ actual.undo_circuit(Circuit("MXX 0 1")); }, std::invalid_argument);
})

TEST_EACH_WORD_SIZE_W(SparseUnsignedRevFrameTracker, myy, {
    SparseUnsignedRevFrameTracker actual(0, 0, 0);
    SparseUnsignedRevFrameTracker expected(0, 0, 0);

    actual = _tracker_from_pauli_string<W>("III");
    actual.num_measurements_in_past = 2;
    actual.rec_bits[1].xor_item(DemTarget::observable_id(0));
    actual.undo_circuit(Circuit("MYY 0 1"));
    expected = _tracker_from_pauli_string<W>("YYI");
    expected.num_measurements_in_past = 1;
    ASSERT_EQ(actual, expected);

    actual = _tracker_from_pauli_string<W>("ZZI");
    actual.num_measurements_in_past = 2;
    actual.rec_bits[1].xor_item(DemTarget::observable_id(0));
    actual.undo_circuit(Circuit("MYY !0 !1"));
    expected = _tracker_from_pauli_string<W>("XXI");
    expected.num_measurements_in_past = 1;
    ASSERT_EQ(actual, expected);

    actual = _tracker_from_pauli_string<W>("ZYI");
    actual.num_measurements_in_past = 2;
    actual.rec_bits[1].xor_item(DemTarget::observable_id(0));
    ASSERT_THROW({ actual.undo_circuit(Circuit("MYY 0 1")); }, std::invalid_argument);
})

TEST_EACH_WORD_SIZE_W(SparseUnsignedRevFrameTracker, mzz, {
    SparseUnsignedRevFrameTracker actual(0, 0, 0);
    SparseUnsignedRevFrameTracker expected(0, 0, 0);

    actual = _tracker_from_pauli_string<W>("III");
    actual.num_measurements_in_past = 2;
    actual.rec_bits[1].xor_item(DemTarget::observable_id(0));
    actual.undo_circuit(Circuit("MZZ 0 1"));
    expected = _tracker_from_pauli_string<W>("ZZI");
    expected.num_measurements_in_past = 1;
    ASSERT_EQ(actual, expected);

    actual = _tracker_from_pauli_string<W>("XXI");
    actual.num_measurements_in_past = 2;
    actual.rec_bits[1].xor_item(DemTarget::observable_id(0));
    actual.undo_circuit(Circuit("MZZ !0 !1"));
    expected = _tracker_from_pauli_string<W>("YYI");
    expected.num_measurements_in_past = 1;
    ASSERT_EQ(actual, expected);

    actual = _tracker_from_pauli_string<W>("ZYI");
    actual.num_measurements_in_past = 2;
    actual.rec_bits[1].xor_item(DemTarget::observable_id(0));
    ASSERT_THROW({ actual.undo_circuit(Circuit("MZZ 0 1")); }, std::invalid_argument);
})

TEST(SparseUnsignedRevFrameTracker, runs_on_general_circuit) {
    auto circuit = generate_test_circuit_with_all_operations();
    SparseUnsignedRevFrameTracker s(circuit.count_qubits(), circuit.count_measurements(), circuit.count_detectors());
    s.undo_circuit(circuit);
    ASSERT_EQ(s.num_measurements_in_past, 0);
    ASSERT_EQ(s.num_detectors_in_past, 0);
}

TEST(SparseUnsignedRevFrameTracker, tracks_anticommutation) {
    Circuit circuit(R"CIRCUIT(
        R 0 1 2
        H 0
        CX 0 1 0 2
        MX 0 1 2
        DETECTOR rec[-1]
        DETECTOR rec[-1] rec[-2] rec[-3]
        DETECTOR rec[-2] rec[-3]
        OBSERVABLE_INCLUDE(2) rec[-3]
        OBSERVABLE_INCLUDE(1) rec[-1] rec[-2] rec[-3]
    )CIRCUIT");

    SparseUnsignedRevFrameTracker rev(
        circuit.count_qubits(), circuit.count_measurements(), circuit.count_detectors(), false);
    rev.undo_circuit(circuit);
    ASSERT_EQ(
        rev.anticommutations,
        (std::set<std::pair<DemTarget, GateTarget>>{
            {DemTarget::relative_detector_id(0), GateTarget::x(2)},
            {DemTarget::relative_detector_id(2), GateTarget::x(2)},
            {DemTarget::observable_id(2), GateTarget::x(1)},
            {DemTarget::observable_id(2), GateTarget::x(2)}}));

    SparseUnsignedRevFrameTracker rev2(circuit.count_qubits(), circuit.count_measurements(), circuit.count_detectors());
    ASSERT_THROW({ rev.undo_circuit(circuit); }, std::invalid_argument);
}

TEST(SparseUnsignedRevFrameTracker, MZZ) {
    Circuit circuit(R"CIRCUIT(
        MZZ 0 1 1 2
        M 2
        DETECTOR rec[-1]
    )CIRCUIT");

    SparseUnsignedRevFrameTracker rev(
        circuit.count_qubits(), circuit.count_measurements(), circuit.count_detectors(), false);
    rev.undo_circuit(circuit);
    ASSERT_TRUE(rev.xs[0].empty());
    ASSERT_TRUE(rev.xs[1].empty());
    ASSERT_TRUE(rev.xs[2].empty());
    ASSERT_TRUE(rev.zs[0].empty());
    ASSERT_TRUE(rev.zs[1].empty());
    ASSERT_EQ(rev.zs[2].sorted_items, (std::vector<DemTarget>{DemTarget::relative_detector_id(0)}));
}

TEST(SparseUnsignedRevFrameTracker, fail_anticommute) {
    Circuit circuit(R"CIRCUIT(
        RX 0 1 2
        M 2
        DETECTOR rec[-1]
    )CIRCUIT");

    SparseUnsignedRevFrameTracker rev(
        circuit.count_qubits(), circuit.count_measurements(), circuit.count_detectors(), true);
    ASSERT_THROW({ rev.undo_circuit(circuit); }, std::invalid_argument);
}

TEST(SparseUnsignedRevFrameTracker, OBS_INCLUDE_PAULIS) {
    SparseUnsignedRevFrameTracker rev(4, 4, 4);

    rev.undo_circuit(Circuit("OBSERVABLE_INCLUDE(5) X1 Y2 Z3 rec[-1]"));
    ASSERT_TRUE(rev.xs[0].empty());
    ASSERT_TRUE(rev.zs[0].empty());
    ASSERT_EQ(rev.xs[1], (std::vector<DemTarget>{DemTarget::observable_id(5)}));
    ASSERT_TRUE(rev.zs[1].empty());
    ASSERT_EQ(rev.xs[2], (std::vector<DemTarget>{DemTarget::observable_id(5)}));
    ASSERT_EQ(rev.zs[2], (std::vector<DemTarget>{DemTarget::observable_id(5)}));
    ASSERT_TRUE(rev.xs[3].empty());
    ASSERT_EQ(rev.zs[3], (std::vector<DemTarget>{DemTarget::observable_id(5)}));
}
