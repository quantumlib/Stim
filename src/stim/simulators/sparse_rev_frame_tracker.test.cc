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

#include "stim/test_util.test.h"

using namespace stim;

SparseUnsignedRevFrameTracker _tracker_from_pauli_string(const char *text) {
    auto p = PauliString<MAX_BITWORD_WIDTH>::from_str(text);
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

TEST(SparseUnsignedRevFrameTracker, undo_tableau_h) {
    SparseUnsignedRevFrameTracker actual(0, 0, 0);
    std::vector<uint32_t> targets{0};
    auto tableau = GATE_DATA.at("H").tableau<MAX_BITWORD_WIDTH>();

    actual = _tracker_from_pauli_string("I");
    actual.undo_tableau(tableau, targets);
    ASSERT_EQ(actual, _tracker_from_pauli_string("I"));

    actual = _tracker_from_pauli_string("X");
    actual.undo_tableau(tableau, targets);
    ASSERT_EQ(actual, _tracker_from_pauli_string("Z"));

    actual = _tracker_from_pauli_string("Y");
    actual.undo_tableau(tableau, targets);
    ASSERT_EQ(actual, _tracker_from_pauli_string("Y"));

    actual = _tracker_from_pauli_string("Z");
    actual.undo_tableau(tableau, targets);
    ASSERT_EQ(actual, _tracker_from_pauli_string("X"));
}

TEST(SparseUnsignedRevFrameTracker, undo_tableau_s) {
    SparseUnsignedRevFrameTracker actual(0, 0, 0);
    std::vector<uint32_t> targets{0};
    auto tableau = GATE_DATA.at("S").tableau<MAX_BITWORD_WIDTH>();

    actual = _tracker_from_pauli_string("I");
    actual.undo_tableau(tableau, targets);
    ASSERT_EQ(actual, _tracker_from_pauli_string("I"));

    actual = _tracker_from_pauli_string("X");
    actual.undo_tableau(tableau, targets);
    ASSERT_EQ(actual, _tracker_from_pauli_string("Y"));

    actual = _tracker_from_pauli_string("Y");
    actual.undo_tableau(tableau, targets);
    ASSERT_EQ(actual, _tracker_from_pauli_string("X"));

    actual = _tracker_from_pauli_string("Z");
    actual.undo_tableau(tableau, targets);
    ASSERT_EQ(actual, _tracker_from_pauli_string("Z"));
}

TEST(SparseUnsignedRevFrameTracker, undo_tableau_c_xyz) {
    SparseUnsignedRevFrameTracker actual(0, 0, 0);
    std::vector<uint32_t> targets{0};
    auto tableau = GATE_DATA.at("C_XYZ").tableau<MAX_BITWORD_WIDTH>();

    actual = _tracker_from_pauli_string("I");
    actual.undo_tableau(tableau, targets);
    ASSERT_EQ(actual, _tracker_from_pauli_string("I"));

    actual = _tracker_from_pauli_string("X");
    actual.undo_tableau(tableau, targets);
    ASSERT_EQ(actual, _tracker_from_pauli_string("Z"));

    actual = _tracker_from_pauli_string("Y");
    actual.undo_tableau(tableau, targets);
    ASSERT_EQ(actual, _tracker_from_pauli_string("X"));

    actual = _tracker_from_pauli_string("Z");
    actual.undo_tableau(tableau, targets);
    ASSERT_EQ(actual, _tracker_from_pauli_string("Y"));
}

TEST(SparseUnsignedRevFrameTracker, undo_tableau_cx) {
    SparseUnsignedRevFrameTracker actual(0, 0, 0);
    std::vector<uint32_t> targets{0, 1};
    auto tableau = GATE_DATA.at("CX").tableau<MAX_BITWORD_WIDTH>();

    actual = _tracker_from_pauli_string("II");
    actual.undo_tableau(tableau, targets);
    ASSERT_EQ(actual, _tracker_from_pauli_string("II"));

    actual = _tracker_from_pauli_string("IZ");
    actual.undo_tableau(tableau, targets);
    ASSERT_EQ(actual, _tracker_from_pauli_string("ZZ"));

    actual = _tracker_from_pauli_string("ZI");
    actual.undo_tableau(tableau, targets);
    ASSERT_EQ(actual, _tracker_from_pauli_string("ZI"));

    actual = _tracker_from_pauli_string("XI");
    actual.undo_tableau(tableau, targets);
    ASSERT_EQ(actual, _tracker_from_pauli_string("XX"));

    actual = _tracker_from_pauli_string("IX");
    actual.undo_tableau(tableau, targets);
    ASSERT_EQ(actual, _tracker_from_pauli_string("IX"));

    actual = _tracker_from_pauli_string("YY");
    actual.undo_tableau(tableau, targets);
    ASSERT_EQ(actual, _tracker_from_pauli_string("XZ"));
}

static std::vector<GateTarget> qubit_targets(const std::vector<uint32_t> &targets) {
    std::vector<GateTarget> result;
    for (uint32_t t : targets) {
        result.push_back(GateTarget::qubit(t));
    }
    return result;
}

TEST(SparseUnsignedRevFrameTracker, fuzz_all_unitary_gates_vs_tableau) {
    auto &rng = SHARED_TEST_RNG();
    for (const auto &gate : GATE_DATA.gates()) {
        if (gate.flags & GATE_IS_UNITARY) {
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
            tracker_gate.undo_gate({gate.id, {}, qubit_targets(targets)});
            tracker_tableau.undo_tableau(gate.tableau<MAX_BITWORD_WIDTH>(), targets);
            EXPECT_EQ(tracker_gate, tracker_tableau) << gate.name;
        }
    }
}

TEST(SparseUnsignedRevFrameTracker, RX) {
    SparseUnsignedRevFrameTracker actual(0, 0, 0);

    actual = _tracker_from_pauli_string("III");
    actual.undo_RX({GateType::RX, {}, qubit_targets({0, 2})});
    ASSERT_EQ(actual, _tracker_from_pauli_string("III"));

    actual = _tracker_from_pauli_string("XXX");
    actual.undo_RX({GateType::RX, {}, qubit_targets({0, 2})});
    ASSERT_EQ(actual, _tracker_from_pauli_string("IXI"));

    actual = _tracker_from_pauli_string("XIZ");
    ASSERT_THROW({ actual.undo_RX({GateType::RX, {}, qubit_targets({0, 2})}); }, std::invalid_argument);
    ASSERT_EQ(actual, _tracker_from_pauli_string("XIZ"));

    actual = _tracker_from_pauli_string("YIX");
    ASSERT_THROW({ actual.undo_RX({GateType::RX, {}, qubit_targets({0, 2})}); }, std::invalid_argument);
    ASSERT_EQ(actual, _tracker_from_pauli_string("YIX"));
}

TEST(SparseUnsignedRevFrameTracker, RY) {
    SparseUnsignedRevFrameTracker actual(0, 0, 0);

    actual = _tracker_from_pauli_string("III");
    actual.undo_RY({GateType::RY, {}, qubit_targets({0, 2})});
    ASSERT_EQ(actual, _tracker_from_pauli_string("III"));

    actual = _tracker_from_pauli_string("YYY");
    actual.undo_RY({GateType::RY, {}, qubit_targets({0, 2})});
    ASSERT_EQ(actual, _tracker_from_pauli_string("IYI"));

    actual = _tracker_from_pauli_string("YIZ");
    ASSERT_THROW({ actual.undo_RY({GateType::RY, {}, qubit_targets({0, 2})}); }, std::invalid_argument);
    ASSERT_EQ(actual, _tracker_from_pauli_string("YIZ"));

    actual = _tracker_from_pauli_string("XIY");
    ASSERT_THROW({ actual.undo_RY({GateType::RY, {}, qubit_targets({0, 2})}); }, std::invalid_argument);
    ASSERT_EQ(actual, _tracker_from_pauli_string("XIY"));
}

TEST(SparseUnsignedRevFrameTracker, RZ) {
    SparseUnsignedRevFrameTracker actual(0, 0, 0);

    actual = _tracker_from_pauli_string("III");
    actual.undo_RZ({GateType::R, {}, qubit_targets({0, 2})});
    ASSERT_EQ(actual, _tracker_from_pauli_string("III"));

    actual = _tracker_from_pauli_string("ZZZ");
    actual.undo_RZ({GateType::R, {}, qubit_targets({0, 2})});
    ASSERT_EQ(actual, _tracker_from_pauli_string("IZI"));

    actual = _tracker_from_pauli_string("YIZ");
    ASSERT_THROW({ actual.undo_RZ({GateType::R, {}, qubit_targets({0, 2})}); }, std::invalid_argument);
    ASSERT_EQ(actual, _tracker_from_pauli_string("YIZ"));

    actual = _tracker_from_pauli_string("ZIX");
    ASSERT_THROW({ actual.undo_RZ({GateType::R, {}, qubit_targets({0, 2})}); }, std::invalid_argument);
    ASSERT_EQ(actual, _tracker_from_pauli_string("ZIX"));
}

TEST(SparseUnsignedRevFrameTracker, MX) {
    SparseUnsignedRevFrameTracker actual(0, 0, 0);

    actual = _tracker_from_pauli_string("III");
    actual.num_measurements_in_past = 2;
    actual.undo_MX({GateType::MX, {}, qubit_targets({0, 2})});
    ASSERT_EQ(actual, _tracker_from_pauli_string("III"));

    actual = _tracker_from_pauli_string("XXX");
    actual.num_measurements_in_past = 2;
    actual.undo_MX({GateType::MX, {}, qubit_targets({0, 2})});
    ASSERT_EQ(actual, _tracker_from_pauli_string("XXX"));

    actual = _tracker_from_pauli_string("XIZ");
    ASSERT_THROW({ actual.undo_MX({GateType::MX, {}, qubit_targets({0, 2})}); }, std::invalid_argument);
    ASSERT_EQ(actual, _tracker_from_pauli_string("XIZ"));

    actual = _tracker_from_pauli_string("YIX");
    ASSERT_THROW({ actual.undo_MX({GateType::MX, {}, qubit_targets({0, 2})}); }, std::invalid_argument);
    ASSERT_EQ(actual, _tracker_from_pauli_string("YIX"));
}

TEST(SparseUnsignedRevFrameTracker, MY) {
    SparseUnsignedRevFrameTracker actual(0, 0, 0);

    actual = _tracker_from_pauli_string("III");
    actual.num_measurements_in_past = 2;
    actual.undo_MY({GateType::MY, {}, qubit_targets({0, 2})});
    ASSERT_EQ(actual, _tracker_from_pauli_string("III"));

    actual = _tracker_from_pauli_string("YYY");
    actual.num_measurements_in_past = 2;
    actual.undo_MY({GateType::MY, {}, qubit_targets({0, 2})});
    ASSERT_EQ(actual, _tracker_from_pauli_string("YYY"));

    actual = _tracker_from_pauli_string("YIZ");
    ASSERT_THROW({ actual.undo_MY({GateType::MY, {}, qubit_targets({0, 2})}); }, std::invalid_argument);
    ASSERT_EQ(actual, _tracker_from_pauli_string("YIZ"));

    actual = _tracker_from_pauli_string("XIY");
    ASSERT_THROW({ actual.undo_MY({GateType::MY, {}, qubit_targets({0, 2})}); }, std::invalid_argument);
    ASSERT_EQ(actual, _tracker_from_pauli_string("XIY"));
}

TEST(SparseUnsignedRevFrameTracker, MZ) {
    SparseUnsignedRevFrameTracker actual(0, 0, 0);

    actual = _tracker_from_pauli_string("III");
    actual.num_measurements_in_past = 2;
    actual.undo_MZ({GateType::M, {}, qubit_targets({0, 2})});
    ASSERT_EQ(actual, _tracker_from_pauli_string("III"));

    actual = _tracker_from_pauli_string("ZZZ");
    actual.num_measurements_in_past = 2;
    actual.undo_MZ({GateType::M, {}, qubit_targets({0, 2})});
    ASSERT_EQ(actual, _tracker_from_pauli_string("ZZZ"));

    actual = _tracker_from_pauli_string("YIZ");
    ASSERT_THROW({ actual.undo_MZ({GateType::M, {}, qubit_targets({0, 2})}); }, std::invalid_argument);
    ASSERT_EQ(actual, _tracker_from_pauli_string("YIZ"));

    actual = _tracker_from_pauli_string("ZIX");
    ASSERT_THROW({ actual.undo_MZ({GateType::M, {}, qubit_targets({0, 2})}); }, std::invalid_argument);
    ASSERT_EQ(actual, _tracker_from_pauli_string("ZIX"));
}

TEST(SparseUnsignedRevFrameTracker, MRX) {
    SparseUnsignedRevFrameTracker actual(0, 0, 0);

    actual = _tracker_from_pauli_string("III");
    actual.num_measurements_in_past = 2;
    actual.undo_MRX({GateType::MRX, {}, qubit_targets({0, 2})});
    ASSERT_EQ(actual, _tracker_from_pauli_string("III"));

    actual = _tracker_from_pauli_string("XXX");
    actual.num_measurements_in_past = 2;
    actual.undo_MRX({GateType::MRX, {}, qubit_targets({0, 2})});
    ASSERT_EQ(actual, _tracker_from_pauli_string("IXI"));

    actual = _tracker_from_pauli_string("XIZ");
    ASSERT_THROW({ actual.undo_MRX({GateType::MRX, {}, qubit_targets({0, 2})}); }, std::invalid_argument);
    ASSERT_EQ(actual, _tracker_from_pauli_string("XIZ"));

    actual = _tracker_from_pauli_string("YIX");
    ASSERT_THROW({ actual.undo_MRX({GateType::MRX, {}, qubit_targets({0, 2})}); }, std::invalid_argument);
    ASSERT_EQ(actual, _tracker_from_pauli_string("YIX"));
}

TEST(SparseUnsignedRevFrameTracker, MRY) {
    SparseUnsignedRevFrameTracker actual(0, 0, 0);

    actual = _tracker_from_pauli_string("III");
    actual.num_measurements_in_past = 2;
    actual.undo_MRY({GateType::MRY, {}, qubit_targets({0, 2})});
    ASSERT_EQ(actual, _tracker_from_pauli_string("III"));

    actual = _tracker_from_pauli_string("YYY");
    actual.num_measurements_in_past = 2;
    actual.undo_MRY({GateType::MRY, {}, qubit_targets({0, 2})});
    ASSERT_EQ(actual, _tracker_from_pauli_string("IYI"));

    actual = _tracker_from_pauli_string("YIZ");
    ASSERT_THROW({ actual.undo_MRY({GateType::MRY, {}, qubit_targets({0, 2})}); }, std::invalid_argument);
    ASSERT_EQ(actual, _tracker_from_pauli_string("YIZ"));

    actual = _tracker_from_pauli_string("XIY");
    ASSERT_THROW({ actual.undo_MRY({GateType::MRY, {}, qubit_targets({0, 2})}); }, std::invalid_argument);
    ASSERT_EQ(actual, _tracker_from_pauli_string("XIY"));
}

TEST(SparseUnsignedRevFrameTracker, MRZ) {
    SparseUnsignedRevFrameTracker actual(0, 0, 0);

    actual = _tracker_from_pauli_string("III");
    actual.num_measurements_in_past = 2;
    actual.undo_MRZ({GateType::MR, {}, qubit_targets({0, 2})});
    ASSERT_EQ(actual, _tracker_from_pauli_string("III"));

    actual = _tracker_from_pauli_string("ZZZ");
    actual.num_measurements_in_past = 2;
    actual.undo_MRZ({GateType::MR, {}, qubit_targets({0, 2})});
    ASSERT_EQ(actual, _tracker_from_pauli_string("IZI"));

    actual = _tracker_from_pauli_string("YIZ");
    ASSERT_THROW({ actual.undo_MRZ({GateType::MR, {}, qubit_targets({0, 2})}); }, std::invalid_argument);
    ASSERT_EQ(actual, _tracker_from_pauli_string("YIZ"));

    actual = _tracker_from_pauli_string("ZIX");
    ASSERT_THROW({ actual.undo_MRZ({GateType::MR, {}, qubit_targets({0, 2})}); }, std::invalid_argument);
    ASSERT_EQ(actual, _tracker_from_pauli_string("ZIX"));
}

TEST(SparseUnsignedRevFrameTracker, feedback_from_measurement) {
    SparseUnsignedRevFrameTracker actual(0, 0, 0);

    actual = _tracker_from_pauli_string("III");
    actual.num_measurements_in_past = 2;
    actual.rec_bits[0].xor_item(DemTarget::observable_id(0));
    actual.undo_MX({GateType::MX, {}, qubit_targets({0, 2})});
    ASSERT_EQ(actual, _tracker_from_pauli_string("XII"));
    actual = _tracker_from_pauli_string("XII");
    actual.num_measurements_in_past = 2;
    actual.rec_bits[0].xor_item(DemTarget::observable_id(0));
    actual.undo_MX({GateType::MX, {}, qubit_targets({0, 2})});
    ASSERT_EQ(actual, _tracker_from_pauli_string("III"));

    actual = _tracker_from_pauli_string("III");
    actual.num_measurements_in_past = 2;
    actual.rec_bits[0].xor_item(DemTarget::observable_id(0));
    actual.undo_MY({GateType::MY, {}, qubit_targets({0, 2})});
    ASSERT_EQ(actual, _tracker_from_pauli_string("YII"));
    actual = _tracker_from_pauli_string("YII");
    actual.num_measurements_in_past = 2;
    actual.rec_bits[0].xor_item(DemTarget::observable_id(0));
    actual.undo_MY({GateType::MY, {}, qubit_targets({0, 2})});
    ASSERT_EQ(actual, _tracker_from_pauli_string("III"));

    actual = _tracker_from_pauli_string("III");
    actual.num_measurements_in_past = 2;
    actual.rec_bits[0].xor_item(DemTarget::observable_id(0));
    actual.undo_MZ({GateType::M, {}, qubit_targets({0, 2})});
    ASSERT_EQ(actual, _tracker_from_pauli_string("ZII"));
    actual = _tracker_from_pauli_string("ZII");
    actual.num_measurements_in_past = 2;
    actual.rec_bits[0].xor_item(DemTarget::observable_id(0));
    actual.undo_MZ({GateType::M, {}, qubit_targets({0, 2})});
    ASSERT_EQ(actual, _tracker_from_pauli_string("III"));

    actual = _tracker_from_pauli_string("III");
    actual.num_measurements_in_past = 2;
    actual.rec_bits[0].xor_item(DemTarget::observable_id(0));
    actual.undo_MRX({GateType::MRX, {}, qubit_targets({0, 2})});
    ASSERT_EQ(actual, _tracker_from_pauli_string("XII"));
    actual = _tracker_from_pauli_string("XII");
    actual.num_measurements_in_past = 2;
    actual.rec_bits[0].xor_item(DemTarget::observable_id(0));
    actual.undo_MRX({GateType::MRX, {}, qubit_targets({0, 2})});
    ASSERT_EQ(actual, _tracker_from_pauli_string("XII"));

    actual = _tracker_from_pauli_string("III");
    actual.num_measurements_in_past = 2;
    actual.rec_bits[0].xor_item(DemTarget::observable_id(0));
    actual.undo_MRY({GateType::MRY, {}, qubit_targets({0, 2})});
    ASSERT_EQ(actual, _tracker_from_pauli_string("YII"));
    actual = _tracker_from_pauli_string("YII");
    actual.num_measurements_in_past = 2;
    actual.rec_bits[0].xor_item(DemTarget::observable_id(0));
    actual.undo_MRY({GateType::MRY, {}, qubit_targets({0, 2})});
    ASSERT_EQ(actual, _tracker_from_pauli_string("YII"));

    actual = _tracker_from_pauli_string("III");
    actual.num_measurements_in_past = 2;
    actual.rec_bits[0].xor_item(DemTarget::observable_id(0));
    actual.undo_MRZ({GateType::MR, {}, qubit_targets({0, 2})});
    ASSERT_EQ(actual, _tracker_from_pauli_string("ZII"));
    actual = _tracker_from_pauli_string("ZII");
    actual.num_measurements_in_past = 2;
    actual.rec_bits[0].xor_item(DemTarget::observable_id(0));
    actual.undo_MRZ({GateType::MR, {}, qubit_targets({0, 2})});
    ASSERT_EQ(actual, _tracker_from_pauli_string("ZII"));
}

TEST(SparseUnsignedRevFrameTracker, feedback_into_measurement) {
    SparseUnsignedRevFrameTracker actual(0, 0, 0);
    SparseUnsignedRevFrameTracker expected(0, 0, 0);
    std::vector<GateTarget> targets{GateTarget::rec(-5), GateTarget::qubit(0)};

    actual = _tracker_from_pauli_string("XII");
    actual.num_measurements_in_past = 12;
    actual.undo_ZCX({GateType::CX, {}, targets});
    expected = _tracker_from_pauli_string("XII");
    expected.num_measurements_in_past = 12;
    ASSERT_EQ(actual, expected);

    actual = _tracker_from_pauli_string("YII");
    actual.num_measurements_in_past = 12;
    actual.undo_ZCX({GateType::CX, {}, targets});
    expected = _tracker_from_pauli_string("YII");
    expected.num_measurements_in_past = 12;
    expected.rec_bits[7].xor_item(DemTarget::observable_id(0));
    ASSERT_EQ(actual, expected);

    actual = _tracker_from_pauli_string("ZII");
    actual.num_measurements_in_past = 12;
    actual.undo_ZCX({GateType::CX, {}, targets});
    expected = _tracker_from_pauli_string("ZII");
    expected.num_measurements_in_past = 12;
    expected.rec_bits[7].xor_item(DemTarget::observable_id(0));
    ASSERT_EQ(actual, expected);

    actual = _tracker_from_pauli_string("XII");
    actual.num_measurements_in_past = 12;
    actual.undo_ZCY({GateType::CY, {}, targets});
    expected = _tracker_from_pauli_string("XII");
    expected.num_measurements_in_past = 12;
    expected.rec_bits[7].xor_item(DemTarget::observable_id(0));
    ASSERT_EQ(actual, expected);

    actual = _tracker_from_pauli_string("YII");
    actual.num_measurements_in_past = 12;
    actual.undo_ZCY({GateType::CY, {}, targets});
    expected = _tracker_from_pauli_string("YII");
    expected.num_measurements_in_past = 12;
    ASSERT_EQ(actual, expected);

    actual = _tracker_from_pauli_string("ZII");
    actual.num_measurements_in_past = 12;
    actual.undo_ZCY({GateType::CY, {}, targets});
    expected = _tracker_from_pauli_string("ZII");
    expected.num_measurements_in_past = 12;
    expected.rec_bits[7].xor_item(DemTarget::observable_id(0));
    ASSERT_EQ(actual, expected);

    actual = _tracker_from_pauli_string("XII");
    actual.num_measurements_in_past = 12;
    actual.undo_ZCZ({GateType::CZ, {}, targets});
    expected = _tracker_from_pauli_string("XII");
    expected.num_measurements_in_past = 12;
    expected.rec_bits[7].xor_item(DemTarget::observable_id(0));
    ASSERT_EQ(actual, expected);

    actual = _tracker_from_pauli_string("YII");
    actual.num_measurements_in_past = 12;
    actual.undo_ZCZ({GateType::CZ, {}, targets});
    expected = _tracker_from_pauli_string("YII");
    expected.num_measurements_in_past = 12;
    expected.rec_bits[7].xor_item(DemTarget::observable_id(0));
    ASSERT_EQ(actual, expected);

    actual = _tracker_from_pauli_string("ZII");
    actual.num_measurements_in_past = 12;
    actual.undo_ZCZ({GateType::CZ, {}, targets});
    expected = _tracker_from_pauli_string("ZII");
    expected.num_measurements_in_past = 12;
    ASSERT_EQ(actual, expected);
}

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
