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

#include "stim/circuit/circuit.test.h"

#include "gtest/gtest.h"

using namespace stim;

OpDat::OpDat(uint32_t target) : targets({{target}}) {
}

OpDat::OpDat(std::vector<uint32_t> int_targets) {
    for (auto e : int_targets) {
        targets.push_back({e});
    }
}

OpDat OpDat::flipped(size_t target) {
    return OpDat(target | TARGET_INVERTED_BIT);
}

OpDat::operator OperationData() {
    return {{}, targets};
}

TEST(circuit, from_text) {
    Circuit expected;
    const auto &f = [](const char *c) {
        return Circuit(c);
    };
    ASSERT_EQ(f("# not an operation"), expected);

    expected.clear();
    expected.append_op("H", {0});
    ASSERT_EQ(f("H 0"), expected);
    ASSERT_EQ(f("h 0"), expected);
    ASSERT_EQ(f("H 0     "), expected);
    ASSERT_EQ(f("     H 0     "), expected);
    ASSERT_EQ(f("\tH 0\t\t"), expected);
    ASSERT_EQ(f("H 0  # comment"), expected);

    expected.clear();
    expected.append_op("H", {23});
    ASSERT_EQ(f("H 23"), expected);

    expected.clear();
    expected.append_op("DEPOLARIZE1", {4, 5}, 0.125);
    ASSERT_EQ(f("DEPOLARIZE1(0.125) 4 5  # comment"), expected);

    expected.clear();
    expected.append_op("ZCX", {5, 6});
    ASSERT_EQ(f("  \t Cnot 5 6  # comment   "), expected);

    ASSERT_THROW({ f("H a"); }, std::invalid_argument);
    ASSERT_THROW({ f("H(1)"); }, std::invalid_argument);
    ASSERT_THROW({ f("X_ERROR 1"); }, std::invalid_argument);
    ASSERT_THROW({ f("H 9999999999999999999999999999999999999999999"); }, std::invalid_argument);
    ASSERT_THROW({ f("H -1"); }, std::invalid_argument);
    ASSERT_THROW({ f("CNOT 0 a"); }, std::invalid_argument);
    ASSERT_THROW({ f("CNOT 0 99999999999999999999999999999999"); }, std::invalid_argument);
    ASSERT_THROW({ f("CNOT 0 -1"); }, std::invalid_argument);
    ASSERT_THROW({ f("DETECTOR 1 2"); }, std::invalid_argument);
    ASSERT_THROW({ f("CX 1 1"); }, std::invalid_argument);
    ASSERT_THROW({ f("SWAP 1 1"); }, std::invalid_argument);
    ASSERT_THROW({ f("DEPOLARIZE2(1) 1 1"); }, std::invalid_argument);
    ASSERT_THROW({ f("DETEstdCTOR rec[-1]"); }, std::invalid_argument);
    ASSERT_THROW({ f("DETECTOR rec[0]"); }, std::invalid_argument);
    ASSERT_THROW({ f("DETECTOR rec[1]"); }, std::invalid_argument);
    ASSERT_THROW({ f("DETECTOR rec[-999999999999]"); }, std::invalid_argument);
    ASSERT_THROW({ f("OBSERVABLE_INCLUDE rec[-1]"); }, std::invalid_argument);
    ASSERT_THROW({ f("OBSERVABLE_INCLUDE(-1) rec[-1]"); }, std::invalid_argument);
    ASSERT_THROW({ f("CORRELATED_ERROR(1) B1"); }, std::invalid_argument);
    ASSERT_THROW({ f("CORRELATED_ERROR(1) X 1"); }, std::invalid_argument);
    ASSERT_THROW({ f("CORRELATED_ERROR(1) X\n"); }, std::invalid_argument);
    ASSERT_THROW({ f("CORRELATED_ERROR(1) 1"); }, std::invalid_argument);
    ASSERT_THROW({ f("ELSE_CORRELATED_ERROR(1) 1 2"); }, std::invalid_argument);
    ASSERT_THROW({ f("CORRELATED_ERROR(1) 1 2"); }, std::invalid_argument);
    ASSERT_THROW({ f("CORRELATED_ERROR(1) A"); }, std::invalid_argument);

    ASSERT_THROW({ f("CNOT 0\nCNOT 1"); }, std::invalid_argument);

    expected.clear();
    ASSERT_EQ(f(""), expected);
    ASSERT_EQ(f("# Comment\n\n\n# More"), expected);

    expected.clear();
    expected.append_op("H_XZ", {0});
    ASSERT_EQ(f("H 0"), expected);

    expected.clear();
    expected.append_op("H_XZ", {0, 1});
    ASSERT_EQ(f("H 0 \n H 1"), expected);

    expected.clear();
    expected.append_op("H_XZ", {1});
    ASSERT_EQ(f("H 1"), expected);

    expected.clear();
    expected.append_op("H", {0});
    expected.append_op("ZCX", {0, 1});
    ASSERT_EQ(
        f("# EPR\n"
          "H 0\n"
          "CNOT 0 1"),
        expected);

    expected.clear();
    expected.append_op("M", {0, 0 | TARGET_INVERTED_BIT, 1, 1 | TARGET_INVERTED_BIT});
    ASSERT_EQ(f("M 0 !0 1 !1"), expected);

    // Measurement fusion.
    expected.clear();
    expected.append_op("H", {0});
    expected.append_op("M", {0, 1, 2});
    expected.append_op("SWAP", {0, 1});
    expected.append_op("M", {0, 10});
    ASSERT_EQ(
        f(R"CIRCUIT(
            H 0
            M 0
            M 1
            M 2
            SWAP 0 1
            M 0
            M 10
        )CIRCUIT"),
        expected);

    expected.clear();
    expected.append_op("X", {0});
    expected += Circuit("Y 1 2") * 2;
    ASSERT_EQ(
        f(R"CIRCUIT(
            X 0
            REPEAT 2 {
              Y 1
              Y 2 #####"
            } #####"
        )CIRCUIT"),
        expected);

    expected.clear();
    expected.append_op("DETECTOR", {5 | TARGET_RECORD_BIT});
    ASSERT_EQ(f("DETECTOR rec[-5]"), expected);
    expected.clear();
    expected.append_op("DETECTOR", {6 | TARGET_RECORD_BIT});
    ASSERT_EQ(f("DETECTOR rec[-6]"), expected);

    Circuit parsed =
        f("M 0\n"
          "REPEAT 5 {\n"
          "  M 1 2\n"
          "  M 3\n"
          "} #####");
    ASSERT_EQ(parsed.operations.size(), 2);
    ASSERT_EQ(parsed.blocks.size(), 1);
    ASSERT_EQ(parsed.blocks[0].operations.size(), 1);
    ASSERT_EQ(parsed.blocks[0].operations[0].target_data.targets.size(), 3);

    expected.clear();
    expected.append_op(
        "CORRELATED_ERROR",
        {90 | TARGET_PAULI_X_BIT,
         91 | TARGET_PAULI_X_BIT | TARGET_PAULI_Z_BIT,
         92 | TARGET_PAULI_Z_BIT,
         93 | TARGET_PAULI_X_BIT},
        0.125);
    ASSERT_EQ(
        f(R"circuit(
            CORRELATED_ERROR(0.125) X90 Y91 Z92 X93
          )circuit"),
        expected);
}

TEST(circuit, parse_combiners) {
    ASSERT_THROW({ Circuit("H *"); }, std::invalid_argument);
    ASSERT_THROW({ Circuit("MPP *"); }, std::invalid_argument);
    ASSERT_THROW({ Circuit("MPP * X1"); }, std::invalid_argument);
    ASSERT_THROW({ Circuit("MPP * X1 *"); }, std::invalid_argument);
    ASSERT_THROW({ Circuit("MPP X1 *"); }, std::invalid_argument);
    ASSERT_THROW({ Circuit("MPP X1 * * Y2"); }, std::invalid_argument);
    ASSERT_THROW({ Circuit("MPP X1**Y2"); }, std::invalid_argument);
    ASSERT_THROW({ Circuit("MPP(1.1) X1**Y2"); }, std::invalid_argument);
    ASSERT_THROW({ Circuit("MPP(-0.5) X1**Y2"); }, std::invalid_argument);
    auto c = Circuit("MPP X1*Y2 Z3 * Z4\nMPP Z5");
    ASSERT_EQ(c.operations.size(), 1);
    ASSERT_EQ(c.operations[0].target_data.args.size(), 0);
    ASSERT_EQ(c.operations[0].target_data.targets.size(), 7);
    std::vector<GateTarget> expected{
        GateTarget::x(1),
        GateTarget::combiner(),
        GateTarget::y(2),
        GateTarget::z(3),
        GateTarget::combiner(),
        GateTarget::z(4),
        GateTarget::z(5),
    };
    ASSERT_EQ(c.operations[0].target_data.targets, (PointerRange<GateTarget>)expected);

    c = Circuit("MPP(0.125) X1*Y2 Z3 * Z4\nMPP Z5");
    ASSERT_EQ(c.operations[0].target_data.args.size(), 1);
    ASSERT_EQ(c.operations[0].target_data.args[0], 0.125);
}

TEST(circuit, parse_sweep_bits) {
    ASSERT_THROW({ Circuit("H sweep[0]"); }, std::invalid_argument);
    ASSERT_THROW({ Circuit("X sweep[0]"); }, std::invalid_argument);

    std::vector<GateTarget> expected{
        GateTarget::sweep_bit(2),
        GateTarget::qubit(5),
    };

    Circuit c("CNOT sweep[2] 5");
    ASSERT_EQ(c.operations.size(), 1);
    ASSERT_EQ(c.operations[0].target_data.targets, (PointerRange<GateTarget>)expected);
    ASSERT_TRUE(c.operations[0].target_data.args.empty());
}

TEST(circuit, append_circuit) {
    Circuit c1;
    c1.append_op("X", {0, 1});
    c1.append_op("M", {0, 1, 2, 4});

    Circuit c2;
    c2.append_op("M", {7});

    Circuit actual = c1;
    actual += c2;
    ASSERT_EQ(actual.operations.size(), 2);
    ASSERT_EQ(actual, Circuit("X 0 1\nM 0 1 2 4 7"));

    actual *= 4;
    ASSERT_EQ(actual.str(), R"CIRCUIT(REPEAT 4 {
    X 0 1
    M 0 1 2 4 7
})CIRCUIT");
}

TEST(circuit, append_op_fuse) {
    Circuit expected;
    Circuit actual;

    actual.clear();
    expected.append_op("H", {1, 2, 3});
    actual.append_op("H", {1});
    actual.append_op("H", {2, 3});
    ASSERT_EQ(actual, expected);
    actual.append_op("R", {0});
    actual.append_op("R", {});
    expected.append_op("R", {0});
    ASSERT_EQ(actual, expected);

    actual.clear();
    actual.append_op("DETECTOR", {2 | TARGET_RECORD_BIT, 2 | TARGET_RECORD_BIT});
    actual.append_op("DETECTOR", {1 | TARGET_RECORD_BIT, 1 | TARGET_RECORD_BIT});
    ASSERT_EQ(actual.operations.size(), 2);

    actual.clear();
    actual.append_op("TICK", {});
    actual.append_op("TICK", {});
    ASSERT_EQ(actual.operations.size(), 2);

    actual.clear();
    expected.clear();
    actual.append_op("M", {0, 1});
    actual.append_op("M", {2, 3});
    expected.append_op("M", {0, 1, 2, 3});
    ASSERT_EQ(actual, expected);

    ASSERT_THROW({ actual.append_op("CNOT", {0}); }, std::invalid_argument);
    ASSERT_THROW({ actual.append_op("X", {0}, 0.5); }, std::invalid_argument);
}

TEST(circuit, str) {
    Circuit c;
    c.append_op("tick", {});
    c.append_op("CNOT", {2, 3});
    c.append_op("CNOT", {5 | TARGET_RECORD_BIT, 3});
    c.append_op("CY", {6 | TARGET_SWEEP_BIT, 4});
    c.append_op("M", {1, 3, 2});
    c.append_op("DETECTOR", {7 | TARGET_RECORD_BIT});
    c.append_op("OBSERVABLE_INCLUDE", {11 | TARGET_RECORD_BIT, 1 | TARGET_RECORD_BIT}, 17);
    c.append_op("X_ERROR", {19}, 0.5);
    c.append_op(
        "CORRELATED_ERROR",
        {
            23 | TARGET_PAULI_X_BIT,
            27 | TARGET_PAULI_Z_BIT,
            29 | TARGET_PAULI_X_BIT | TARGET_PAULI_Z_BIT,
        },
        0.25);
    ASSERT_EQ(c.str(), R"circuit(TICK
CX 2 3 rec[-5] 3
CY sweep[6] 4
M 1 3 2
DETECTOR rec[-7]
OBSERVABLE_INCLUDE(17) rec[-11] rec[-1]
X_ERROR(0.5) 19
E(0.25) X23 Z27 Y29)circuit");
}

TEST(circuit, append_op_validation) {
    Circuit c;
    ASSERT_THROW({ c.append_op("CNOT", {0}); }, std::invalid_argument);
    c.append_op("CNOT", {0, 1});

    ASSERT_THROW({ c.append_op("REPEAT", {100}); }, std::invalid_argument);
    ASSERT_THROW({ c.append_op("X", {0 | TARGET_PAULI_X_BIT}); }, std::invalid_argument);
    ASSERT_THROW({ c.append_op("X", {0 | TARGET_PAULI_Z_BIT}); }, std::invalid_argument);
    ASSERT_THROW({ c.append_op("X", {0 | TARGET_INVERTED_BIT}); }, std::invalid_argument);
    ASSERT_THROW({ c.append_op("X", {0}, 0.5); }, std::invalid_argument);

    ASSERT_THROW({ c.append_op("M", {0 | TARGET_PAULI_X_BIT}); }, std::invalid_argument);
    ASSERT_THROW({ c.append_op("M", {0 | TARGET_PAULI_Z_BIT}); }, std::invalid_argument);
    c.append_op("M", {0 | TARGET_INVERTED_BIT});
    c.append_op("M", {0 | TARGET_INVERTED_BIT}, 0.125);
    ASSERT_THROW({ c.append_op("M", {0}, 1.5); }, std::invalid_argument);
    ASSERT_THROW({ c.append_op("M", {0}, -1.5); }, std::invalid_argument);
    ASSERT_THROW({ c.append_op("M", {0}, {0.125, 0.25}); }, std::invalid_argument);

    c.append_op("CORRELATED_ERROR", {0 | TARGET_PAULI_X_BIT}, 0.1);
    c.append_op("CORRELATED_ERROR", {0 | TARGET_PAULI_Z_BIT}, 0.1);
    ASSERT_THROW({ c.append_op("CORRELATED_ERROR", {0 | TARGET_PAULI_X_BIT}); }, std::invalid_argument);
    ASSERT_THROW({ c.append_op("CORRELATED_ERROR", {0 | TARGET_PAULI_X_BIT}, {0.1, 0.2}); }, std::invalid_argument);
    ASSERT_THROW(
        { c.append_op("CORRELATED_ERROR", {0 | TARGET_PAULI_X_BIT | TARGET_INVERTED_BIT}); }, std::invalid_argument);
    c.append_op("X_ERROR", {0}, 0.5);

    ASSERT_THROW({ c.append_op("CNOT", {0, 0}); }, std::invalid_argument);
}

TEST(circuit, repeat_validation) {
    ASSERT_THROW({ Circuit("REPEAT 100 {"); }, std::invalid_argument);
    ASSERT_THROW({ Circuit("REPEAT 100 {{\n}"); }, std::invalid_argument);
    ASSERT_THROW({ Circuit("REPEAT {\n}"); }, std::invalid_argument);
    ASSERT_THROW({ Circuit().append_from_text("REPEAT 100 {"); }, std::invalid_argument);
    ASSERT_THROW({ Circuit().append_from_text("REPEAT {\n}"); }, std::invalid_argument);
    ASSERT_THROW({ Circuit("H {"); }, std::invalid_argument);
    ASSERT_THROW({ Circuit("H {\n}"); }, std::invalid_argument);
}

TEST(circuit, tick_validation) {
    ASSERT_THROW({ Circuit("TICK 1"); }, std::invalid_argument);
    ASSERT_THROW({ Circuit().append_op("TICK", {1}); }, std::invalid_argument);
}

TEST(circuit, detector_validation) {
    ASSERT_THROW({ Circuit("DETECTOR 1"); }, std::invalid_argument);
    ASSERT_THROW({ Circuit().append_op("DETECTOR", {1}); }, std::invalid_argument);
}

TEST(circuit, x_error_validation) {
    Circuit("X_ERROR(0) 1");
    Circuit("X_ERROR(0.1) 1");
    Circuit("X_ERROR(1) 1");
    ASSERT_THROW({ Circuit("X_ERROR 1"); }, std::invalid_argument);
    ASSERT_THROW({ Circuit("X_ERROR(-0.1) 1"); }, std::invalid_argument);
    ASSERT_THROW({ Circuit("X_ERROR(1.1) 1"); }, std::invalid_argument);
    ASSERT_THROW({ Circuit("X_ERROR(0.1, 0.1) 1"); }, std::invalid_argument);
}

TEST(circuit, pauli_err_1_validation) {
    Circuit("PAULI_CHANNEL_1(0,0,0) 1");
    Circuit("PAULI_CHANNEL_1(0.1,0.2,0.6) 1");
    Circuit("PAULI_CHANNEL_1(1,0,0) 1");
    Circuit("PAULI_CHANNEL_1(0.33333333334,0.33333333334,0.33333333334) 1");
    ASSERT_THROW({ Circuit("PAULI_CHANNEL_1 1"); }, std::invalid_argument);
    ASSERT_THROW({ Circuit("PAULI_CHANNEL_1(0.1) 1"); }, std::invalid_argument);
    ASSERT_THROW({ Circuit("PAULI_CHANNEL_1(0.1,0.1) 1"); }, std::invalid_argument);
    ASSERT_THROW({ Circuit("PAULI_CHANNEL_1(0.1,0.1,0.1,0.1) 1"); }, std::invalid_argument);
    ASSERT_THROW({ Circuit("PAULI_CHANNEL_1(-1,0,0) 1"); }, std::invalid_argument);
    ASSERT_THROW({ Circuit("PAULI_CHANNEL_1(0.1,0.5,0.6) 1"); }, std::invalid_argument);
}

TEST(circuit, pauli_err_2_validation) {
    Circuit("PAULI_CHANNEL_2(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0) 1");
    Circuit("PAULI_CHANNEL_2(0.1,0,0,0,0,0,0,0,0,0,0.1,0,0,0,0.1) 1");
    ASSERT_THROW({ Circuit("PAULI_CHANNEL_2(0.4,0,0,0,0,0.4,0,0,0,0,0,0,0,0,0.4) 1"); }, std::invalid_argument);
    ASSERT_THROW({ Circuit("PAULI_CHANNEL_2(0,0,0,0,0,0,0,0,0,0,0,0,0,0) 1"); }, std::invalid_argument);
    ASSERT_THROW({ Circuit("PAULI_CHANNEL_2(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0) 1"); }, std::invalid_argument);
}

TEST(circuit, qubit_coords) {
    ASSERT_THROW({ Circuit("TICK 1"); }, std::invalid_argument);
    ASSERT_THROW({ Circuit().append_op("TICK", {1}); }, std::invalid_argument);
}

TEST(circuit, classical_controls) {
    ASSERT_THROW(
        {
            Circuit(R"circuit(
                M 0
                XCX rec[-1] 1
             )circuit");
        },
        std::invalid_argument);

    Circuit expected;
    expected.append_op("CX", {0, 1, 1 | TARGET_RECORD_BIT, 1});
    expected.append_op("CY", {2 | TARGET_RECORD_BIT, 1});
    expected.append_op("CZ", {4 | TARGET_RECORD_BIT, 1});
    ASSERT_EQ(
        Circuit(R"circuit(ZCX 0 1
ZCX rec[-1] 1
ZCY rec[-2] 1
ZCZ rec[-4] 1)circuit"),
        expected);
}

TEST(circuit, for_each_operation) {
    Circuit c;
    c.append_from_text(R"CIRCUIT(
        H 0
        M 0 1
        REPEAT 2 {
            X 1
            REPEAT 3 {
                Y 2
            }
        }
    )CIRCUIT");

    Circuit flat;
    flat.append_op("H", {0});
    flat.append_op("M", {0, 1});
    flat.append_op("X", {1});
    flat.append_op("Y", {2});
    flat.operations.push_back(flat.operations.back());
    flat.operations.push_back(flat.operations.back());
    flat.append_op("X", {1});
    flat.append_op("Y", {2});
    flat.operations.push_back(flat.operations.back());
    flat.operations.push_back(flat.operations.back());

    std::vector<Operation> ops;
    c.for_each_operation([&](const Operation &op) {
        ops.push_back(op);
    });
    ASSERT_EQ(ops, flat.operations);
}

TEST(circuit, for_each_operation_reverse) {
    Circuit c;
    c.append_from_text(R"CIRCUIT(
        H 0
        M 0 1
        REPEAT 2 {
            X 1
            REPEAT 3 {
                Y 2
            }
        }
    )CIRCUIT");

    Circuit flat;
    flat.append_op("Y", {2});
    flat.operations.push_back(flat.operations.back());
    flat.operations.push_back(flat.operations.back());
    flat.append_op("X", {1});
    flat.append_op("Y", {2});
    flat.operations.push_back(flat.operations.back());
    flat.operations.push_back(flat.operations.back());
    flat.append_op("X", {1});
    flat.append_op("M", {0, 1});
    flat.append_op("H", {0});

    std::vector<Operation> ops;
    c.for_each_operation_reverse([&](const Operation &op) {
        ops.push_back(op);
    });
    ASSERT_EQ(ops, flat.operations);
}

TEST(circuit, count_qubits) {
    ASSERT_EQ(Circuit().count_qubits(), 0);

    ASSERT_EQ(
        Circuit(R"CIRCUIT(
        H 0
        M 0 1
        REPEAT 2 {
            X 1
            REPEAT 3 {
                Y 2
                M 2
            }
        }
    )CIRCUIT")
            .count_qubits(),
        3);

    // Ensure not unrolling to compute.
    ASSERT_EQ(
        Circuit(R"CIRCUIT(
        H 0
        M 0 1
        REPEAT 999999 {
            REPEAT 999999 {
                REPEAT 999999 {
                    REPEAT 999999 {
                        X 1
                        REPEAT 999999 {
                            Y 2
                            M 2
                        }
                    }
                }
            }
        }
    )CIRCUIT")
            .count_qubits(),
        3);
}

TEST(circuit, count_sweep_bits) {
    ASSERT_EQ(Circuit().count_sweep_bits(), 0);

    ASSERT_EQ(
        Circuit(R"CIRCUIT(
        H 0
        M 0 1
        REPEAT 2 {
            X 1
            REPEAT 3 {
                CY 100 3
                CY 80 3
                M 2
            }
        }
    )CIRCUIT")
            .count_sweep_bits(),
        0);

    ASSERT_EQ(
        Circuit(R"CIRCUIT(
        H 0
        M 0 1
        REPEAT 2 {
            X 1
            REPEAT 3 {
                CY sweep[100] 3
                CY sweep[80] 3
                M 2
            }
        }
    )CIRCUIT")
            .count_sweep_bits(),
        101);

    // Ensure not unrolling to compute.
    ASSERT_EQ(
        Circuit(R"CIRCUIT(
        H 0
        M 0 1
        REPEAT 999999 {
            REPEAT 999999 {
                REPEAT 999999 {
                    REPEAT 999999 {
                        X 1
                        REPEAT 999999 {
                            CY sweep[77] 3
                            M 2
                        }
                    }
                }
            }
        }
    )CIRCUIT")
            .count_sweep_bits(),
        78);
}

TEST(circuit, count_detectors_num_observables) {
    ASSERT_EQ(Circuit().count_detectors(), 0);
    ASSERT_EQ(Circuit().count_observables(), 0);

    ASSERT_EQ(
        Circuit(R"CIRCUIT(
        M 0 1 2
        DETECTOR rec[-1]
        OBSERVABLE_INCLUDE(5) rec[-1]
    )CIRCUIT")
            .count_detectors(),
        1);
    ASSERT_EQ(
        Circuit(R"CIRCUIT(
        M 0 1 2
        DETECTOR rec[-1]
        OBSERVABLE_INCLUDE(5) rec[-1]
    )CIRCUIT")
            .count_observables(),
        6);

    // Ensure not unrolling to compute.
    ASSERT_EQ(
        Circuit(R"CIRCUIT(
        M 0 1
        REPEAT 1000 {
            REPEAT 1000 {
                REPEAT 1000 {
                    REPEAT 1000 {
                        DETECTOR rec[-1]
                    }
                }
            }
        }
    )CIRCUIT")
            .count_detectors(),
        1000000000000ULL);
    ASSERT_EQ(
        Circuit(R"CIRCUIT(
        M 0 1
        REPEAT 1000 {
            REPEAT 1000 {
                REPEAT 1000 {
                    REPEAT 1000 {
                        OBSERVABLE_INCLUDE(2) rec[-1]
                    }
                }
            }
        }
    )CIRCUIT")
            .count_observables(),
        3);

    ASSERT_EQ(
        Circuit(R"CIRCUIT(
        M 0 1
        REPEAT 999999 {
         REPEAT 999999 {
          REPEAT 999999 {
           REPEAT 999999 {
            REPEAT 999999 {
             REPEAT 999999 {
              REPEAT 999999 {
               REPEAT 999999 {
                REPEAT 999999 {
                 REPEAT 999999 {
                  REPEAT 999999 {
                   REPEAT 999999 {
                    REPEAT 999999 {
                        DETECTOR rec[-1]
                    }
                   }
                  }
                 }
                }
               }
              }
             }
            }
           }
          }
         }
        }
    )CIRCUIT")
            .count_detectors(),
        UINT64_MAX);
}

TEST(circuit, max_lookback) {
    ASSERT_EQ(Circuit().max_lookback(), 0);
    ASSERT_EQ(
        Circuit(R"CIRCUIT(
        M 0 1 2 3 4 5 6
    )CIRCUIT")
            .max_lookback(),
        0);

    ASSERT_EQ(
        Circuit(R"CIRCUIT(
        M 0 1 2 3 4 5 6
        REPEAT 2 {
            CNOT rec[-4] 0
            REPEAT 3 {
                CNOT rec[-1] 0
            }
        }
    )CIRCUIT")
            .max_lookback(),
        4);

    // Ensure not unrolling to compute.
    ASSERT_EQ(
        Circuit(R"CIRCUIT(
        M 0 1 2 3 4 5
        REPEAT 999999 {
            REPEAT 999999 {
                REPEAT 999999 {
                    REPEAT 999999 {
                        REPEAT 999999 {
                            CNOT rec[-5] 0
                        }
                    }
                }
            }
        }
    )CIRCUIT")
            .max_lookback(),
        5);
}

TEST(circuit, count_measurements) {
    ASSERT_EQ(Circuit().count_measurements(), 0);

    ASSERT_EQ(
        Circuit(R"CIRCUIT(
        H 0
        M 0 1
        REPEAT 2 {
            X 1
            REPEAT 3 {
                Y 2
                M 2
            }
        }
    )CIRCUIT")
            .count_measurements(),
        8);

    // Ensure not unrolling to compute.
    ASSERT_EQ(
        Circuit(R"CIRCUIT(
        REPEAT 999999 {
            REPEAT 999999 {
                REPEAT 999999 {
                    M 0
                }
            }
        }
    )CIRCUIT")
            .count_measurements(),
        999999ULL * 999999ULL * 999999ULL);
    ASSERT_EQ(
        Circuit(R"CIRCUIT(
        REPEAT 999999 {
         REPEAT 999999 {
          REPEAT 999999 {
           REPEAT 999999 {
            REPEAT 999999 {
             REPEAT 999999 {
              REPEAT 999999 {
               REPEAT 999999 {
                REPEAT 999999 {
                    M 0
                }
               }
              }
             }
            }
           }
          }
         }
        }
    )CIRCUIT")
            .count_measurements(),
        UINT64_MAX);

    ASSERT_EQ(
        Circuit(R"CIRCUIT(
            MPP X0 Z1 Y2
        )CIRCUIT")
            .count_measurements(),
        3);

    ASSERT_EQ(
        Circuit(R"CIRCUIT(
            MPP X0 Z1*Y2
        )CIRCUIT")
            .count_measurements(),
        2);

    ASSERT_EQ(
        Circuit(R"CIRCUIT(
            MPP X0*X1*X2*X3*X4 Z5 Z6
        )CIRCUIT")
            .count_measurements(),
        3);

    ASSERT_EQ(
        Circuit(R"CIRCUIT(
            MPP X0*X1*X2*X3*X4 Z5 Z6
        )CIRCUIT")
            .operations[0]
            .count_measurement_results(),
        3);

    ASSERT_EQ(
        Circuit(R"CIRCUIT(
            MPP X0*X1 Z0*Z1 Y0*Y1
        )CIRCUIT")
            .count_measurements(),
        3);
}

TEST(circuit, preserves_repetition_blocks) {
    Circuit c = Circuit(R"CIRCUIT(
        H 0
        M 0 1
        REPEAT 2 {
            X 1
            REPEAT 3 {
                Y 2
                M 2
                X 0
            }
        }
    )CIRCUIT");
    ASSERT_EQ(c.operations.size(), 3);
    ASSERT_EQ(c.blocks.size(), 1);
    ASSERT_EQ(c.blocks[0].operations.size(), 2);
    ASSERT_EQ(c.blocks[0].blocks.size(), 1);
    ASSERT_EQ(c.blocks[0].blocks[0].operations.size(), 3);
    ASSERT_EQ(c.blocks[0].blocks[0].blocks.size(), 0);
}

TEST(circuit, multiplication_repeats) {
    Circuit c = Circuit(R"CIRCUIT(
        H 0
        M 0 1
    )CIRCUIT");
    ASSERT_EQ((c * 2).str(), R"CIRCUIT(REPEAT 2 {
    H 0
    M 0 1
})CIRCUIT");

    ASSERT_EQ(c * 0, Circuit());
    ASSERT_EQ(c * 1, c);
    Circuit copy = c;
    c *= 1;
    ASSERT_EQ(c, copy);
    c *= 0;
    ASSERT_EQ(c, Circuit());
}

TEST(circuit, self_addition) {
    Circuit c = Circuit(R"CIRCUIT(
        X 0
    )CIRCUIT");
    c += c;
    ASSERT_EQ(c.operations.size(), 1);
    ASSERT_EQ(c.blocks.size(), 0);
    ASSERT_EQ(c, Circuit("X 0 0"));

    c = Circuit(R"CIRCUIT(
        X 0
        Y 0
    )CIRCUIT");
    c += c;
    ASSERT_EQ(c.operations.size(), 4);
    ASSERT_EQ(c.blocks.size(), 0);
    ASSERT_EQ(c.operations[0], c.operations[2]);
    ASSERT_EQ(c.operations[1], c.operations[3]);
    ASSERT_EQ(c, Circuit("X 0\nY 0\nX 0\nY 0"));

    c = Circuit(R"CIRCUIT(
        X 0
        REPEAT 2 {
            Y 0
        }
    )CIRCUIT");
    c += c;
    ASSERT_EQ(c.operations.size(), 4);
    ASSERT_EQ(c.blocks.size(), 1);
    ASSERT_EQ(c.operations[0], c.operations[2]);
    ASSERT_EQ(c.operations[1], c.operations[3]);
}

TEST(circuit, addition_shares_blocks) {
    Circuit c1 = Circuit(R"CIRCUIT(
        X 0
        REPEAT 2 {
            X 1
        }
    )CIRCUIT");
    Circuit c2 = Circuit(R"CIRCUIT(
        X 2
        REPEAT 2 {
            X 3
        }
    )CIRCUIT");
    Circuit c3 = Circuit(R"CIRCUIT(
        X 0
        REPEAT 2 {
            X 1
        }
        X 2
        REPEAT 2 {
            X 3
        }
    )CIRCUIT");
    ASSERT_EQ(c1 + c2, c3);
    c1 += c2;
    ASSERT_EQ(c1, c3);
}

TEST(circuit, big_rep_count) {
    Circuit c = Circuit(R"CIRCUIT(
        REPEAT 1234567890123456789 {
            M 1
        }
    )CIRCUIT");
    ASSERT_EQ(c.operations[0].target_data.targets.size(), 3);
    ASSERT_EQ(c.operations[0].target_data.targets[0].data, 0);
    ASSERT_EQ(c.operations[0].target_data.targets[1].data, 1234567890123456789ULL & 0xFFFFFFFFULL);
    ASSERT_EQ(c.operations[0].target_data.targets[2].data, 1234567890123456789ULL >> 32);
    ASSERT_EQ(c.str(), "REPEAT 1234567890123456789 {\n    M 1\n}");
    ASSERT_EQ(c.count_measurements(), 1234567890123456789ULL);

    ASSERT_THROW({ c * 12345ULL; }, std::invalid_argument);
}

TEST(circuit, zero_repetitions_not_allowed) {
    ASSERT_ANY_THROW({
        Circuit(R"CIRCUIT(
            REPEAT 0 {
                M 0
                OBSERVABLE_INCLUDE(0) rec[-1]
            }
        )CIRCUIT");
    });
}

TEST(circuit, negative_float_coordinates) {
    auto c = Circuit(R"CIRCUIT(
        SHIFT_COORDS(-1, -2, -3)
        QUBIT_COORDS(1, -2) 1
        QUBIT_COORDS(-3.5) 1
    )CIRCUIT");
    ASSERT_EQ(c.operations[0].target_data.args[2], -3);
    ASSERT_EQ(c.operations[2].target_data.args[0], -3.5);
    ASSERT_ANY_THROW({ Circuit("M(-0.1) 0"); });
    c = Circuit("QUBIT_COORDS(1e20) 0");
    ASSERT_EQ(c.operations[0].target_data.args[0], 1e20);
    c = Circuit("QUBIT_COORDS(1E+20) 0");
    ASSERT_EQ(c.operations[0].target_data.args[0], 1E+20);
    ASSERT_ANY_THROW({ Circuit("QUBIT_COORDS(1e10000) 0"); });
}

TEST(circuit, py_get_slice) {
    Circuit c(R"CIRCUIT(
        H 0
        CNOT 0 1
        M(0.125) 0 1
        REPEAT 100 {
            E(0.25) X0 Y5
            REPEAT 20 {
                H 0
            }
        }
        H 1
        REPEAT 999 {
            H 2
        }
    )CIRCUIT");
    ASSERT_EQ(c.py_get_slice(0, 1, 6), c);
    ASSERT_EQ(c.py_get_slice(0, 1, 4), Circuit(R"CIRCUIT(
        H 0
        CNOT 0 1
        M(0.125) 0 1
        REPEAT 100 {
            E(0.25) X0 Y5
            REPEAT 20 {
                H 0
            }
        }
    )CIRCUIT"));
    ASSERT_EQ(c.py_get_slice(2, 1, 3), Circuit(R"CIRCUIT(
        M(0.125) 0 1
        REPEAT 100 {
            E(0.25) X0 Y5
            REPEAT 20 {
                H 0
            }
        }
        H 1
    )CIRCUIT"));

    ASSERT_EQ(c.py_get_slice(4, -1, 3), Circuit(R"CIRCUIT(
        H 1
        REPEAT 100 {
            E(0.25) X0 Y5
            REPEAT 20 {
                H 0
            }
        }
        M(0.125) 0 1
    )CIRCUIT"));

    ASSERT_EQ(c.py_get_slice(5, -2, 3), Circuit(R"CIRCUIT(
        REPEAT 999 {
            H 2
        }
        REPEAT 100 {
            E(0.25) X0 Y5
            REPEAT 20 {
                H 0
            }
        }
        CNOT 0 1
    )CIRCUIT"));

    Circuit c2 = c;
    Circuit c3 = c2.py_get_slice(0, 1, 6);
    c2.clear();
    ASSERT_EQ(c, c3);
}

TEST(circuit, append_repeat_block) {
    Circuit c;
    Circuit b("X 0");
    Circuit a("Y 0");

    c.append_repeat_block(100, b);
    ASSERT_EQ(c, Circuit(R"CIRCUIT(
        REPEAT 100 {
            X 0
        }
    )CIRCUIT"));

    c.append_repeat_block(200, a);
    ASSERT_EQ(c, Circuit(R"CIRCUIT(
        REPEAT 100 {
            X 0
        }
        REPEAT 200 {
            Y 0
        }
    )CIRCUIT"));

    c.append_repeat_block(400, std::move(b));
    ASSERT_TRUE(b.operations.empty());
    ASSERT_FALSE(a.operations.empty());
    ASSERT_EQ(c, Circuit(R"CIRCUIT(
        REPEAT 100 {
            X 0
        }
        REPEAT 200 {
            Y 0
        }
        REPEAT 400 {
            X 0
        }
    )CIRCUIT"));

    ASSERT_THROW({ c.append_repeat_block(0, a); }, std::invalid_argument);
    ASSERT_THROW({ c.append_repeat_block(0, std::move(a)); }, std::invalid_argument);
}

TEST(circuit, aliased_noiseless_circuit) {
    Circuit initial(R"CIRCUIT(
        H 0
        X_ERROR(0.1) 0
        M(0.05) 0
        REPEAT 100 {
            CNOT 0 1
            DEPOLARIZE2(0.1) 0 1
            MPP(0.1) X0*X1 Z0 Z1
        }
    )CIRCUIT");
    Circuit noiseless = initial.aliased_noiseless_circuit();
    ASSERT_EQ(noiseless, Circuit(R"CIRCUIT(
        H 0
        M 0
        REPEAT 100 {
            CNOT 0 1
            MPP X0*X1 Z0 Z1
        }
    )CIRCUIT"));

    Circuit c1 = initial.without_noise();
    Circuit c2 = c1;
    ASSERT_EQ(c1, noiseless);
    initial.clear();
    ASSERT_EQ(c1, c2);
}

TEST(circuit, validate_nan_probability) {
    Circuit c;
    ASSERT_THROW({ c.append_op("X_ERROR", {0}, NAN); }, std::invalid_argument);
}

TEST(circuit, get_final_qubit_coords) {
    ASSERT_EQ(Circuit().get_final_qubit_coords(), (std::map<uint64_t, std::vector<double>>{}));
    ASSERT_EQ(
        Circuit(R"CIRCUIT(
                  QUBIT_COORDS(1, 2) 3
              )CIRCUIT")
            .get_final_qubit_coords(),
        (std::map<uint64_t, std::vector<double>>{
            {3, {1, 2}},
        }));
    ASSERT_EQ(
        Circuit(R"CIRCUIT(
                  SHIFT_COORDS(10, 20, 30)
                  QUBIT_COORDS(4, 5) 3
              )CIRCUIT")
            .get_final_qubit_coords(),
        (std::map<uint64_t, std::vector<double>>{
            {3, {14, 25}},
        }));
    ASSERT_EQ(
        Circuit(R"CIRCUIT(
                  QUBIT_COORDS(1, 2, 3, 4) 1
                  REPEAT 100 {
                      SHIFT_COORDS(10, 20, 30)
                  }
                  QUBIT_COORDS(4, 5) 3
                  QUBIT_COORDS(6) 4
              )CIRCUIT")
            .get_final_qubit_coords(),
        (std::map<uint64_t, std::vector<double>>{
            {1, {1, 2, 3, 4}},
            {3, {1004, 2005}},
            {4, {1006}},
        }));
}

TEST(circuit, get_final_qubit_coords_huge_repetition_count_efficiency) {
    auto actual = Circuit(R"CIRCUIT(
        QUBIT_COORDS(0) 0
        REPEAT 1000 {
            QUBIT_COORDS(1, 1) 1
            REPEAT 2000 {
                QUBIT_COORDS(2, 0.5) 2
                REPEAT 4000 {
                    QUBIT_COORDS(3) 3
                    REPEAT 8000 {
                        QUBIT_COORDS(4) 4
                        SHIFT_COORDS(100)
                        QUBIT_COORDS(5) 5
                    }
                    SHIFT_COORDS(10)
                    QUBIT_COORDS(6) 6
                }
                QUBIT_COORDS(7) 7
            }
            QUBIT_COORDS(8) 8
        }
        QUBIT_COORDS(9) 9
    )CIRCUIT")
                      .get_final_qubit_coords();

    ASSERT_EQ(
        actual,
        (std::map<uint64_t, std::vector<double>>{
            {0, {0}},
            {1, {6400080000000001 - 6400080000000, 1}},
            {2, {6400080000000002 - 3200040000, 0.5}},
            {3, {6400080000000003 - 800010}},
            {4, {6400080000000004 - 110}},
            {5, {6400080000000005 - 10}},
            {6, {6400080000000006}},
            {7, {6400080000000007}},
            {8, {6400080000000008}},
            {9, {6400080000000009}},
        }));
    // Precision sanity check.
    ASSERT_EQ(6400080000000009, (uint64_t)(double)6400080000000009);
}

TEST(circuit, count_ticks) {
    ASSERT_EQ(Circuit().count_ticks(), 0);

    ASSERT_EQ(
        Circuit(R"CIRCUIT(
            TICK
        )CIRCUIT")
            .count_ticks(),
        1);

    ASSERT_EQ(
        Circuit(R"CIRCUIT(
            TICK
            TICK
        )CIRCUIT")
            .count_ticks(),
        2);

    ASSERT_EQ(
        Circuit(R"CIRCUIT(
            TICK
            H 0
            TICK
        )CIRCUIT")
            .count_ticks(),
        2);

    ASSERT_EQ(
        Circuit(R"CIRCUIT(
            TICK
            REPEAT 1000 {
                REPEAT 2000 {
                    REPEAT 1000 {
                        TICK
                    }
                    TICK
                    TICK
                    TICK
                }
            }
            TICK
        )CIRCUIT")
            .count_ticks(),
        2006000002);
}

TEST(circuit, coords_of_detector) {
    Circuit c(R"CIRCUIT(
        TICK
        REPEAT 1000 {
            REPEAT 2000 {
                REPEAT 1000 {
                    DETECTOR(0, 0, 0, 4)
                    SHIFT_COORDS(1, 0, 0)
                }
                DETECTOR(0, 0, 0, 3)
                SHIFT_COORDS(0, 1, 0)
            }
            DETECTOR(0, 0, 0, 2)
            SHIFT_COORDS(0, 0, 1)
        }
        DETECTOR(0, 0, 0, 1)
    )CIRCUIT");
    ASSERT_EQ(c.coords_of_detector(0), (std::vector<double>{0, 0, 0, 4}));
    ASSERT_EQ(c.coords_of_detector(1), (std::vector<double>{1, 0, 0, 4}));
    ASSERT_EQ(c.coords_of_detector(999), (std::vector<double>{999, 0, 0, 4}));
    ASSERT_EQ(c.coords_of_detector(1000), (std::vector<double>{1000, 0, 0, 3}));
    ASSERT_EQ(c.coords_of_detector(1001), (std::vector<double>{1000, 1, 0, 4}));
    ASSERT_EQ(c.coords_of_detector(1002), (std::vector<double>{1001, 1, 0, 4}));

    ASSERT_THROW({ c.get_detector_coordinates({4000000000}); }, std::invalid_argument);

    auto result = c.get_detector_coordinates({
        0,
        1,
        999,
        1000,
        1001,
        1002,
    });
    ASSERT_EQ(
        result,
        (std::map<uint64_t, std::vector<double>>{
            {0, {0, 0, 0, 4}},
            {1, {1, 0, 0, 4}},
            {999, {999, 0, 0, 4}},
            {1000, {1000, 0, 0, 3}},
            {1001, {1000, 1, 0, 4}},
            {1002, {1001, 1, 0, 4}},
        }));
}

TEST(circuit, final_coord_shift) {
    Circuit c(R"CIRCUIT(
        REPEAT 1000 {
            REPEAT 2000 {
                REPEAT 3000 {
                    SHIFT_COORDS(0, 0, 1)
                }
                SHIFT_COORDS(1)
            }
            SHIFT_COORDS(0, 1)
        }
    )CIRCUIT");
    ASSERT_EQ(c.final_coord_shift(), (std::vector<double>{2000000, 1000, 6000000000}));
}

TEST(circuit, concat_fuse) {
    Circuit c1("H 0");
    Circuit c2("H 1");
    Circuit c3 = c1 + c2;
    ASSERT_EQ(c3.operations.size(), 1);
    ASSERT_EQ(c3, Circuit("H 0 1"));
}

TEST(circuit, concat_self_fuse) {
    Circuit c("H 0");
    c += c;
    ASSERT_EQ(c.operations.size(), 1);
    ASSERT_EQ(c, Circuit("H 0 0"));
}

TEST(circuit, assignment_copies_operations) {
    Circuit c1("H 0 1\nS 0");
    Circuit c2("TICK");
    ASSERT_EQ(c1.operations.size(), 2);
    ASSERT_EQ(c2.operations.size(), 1);
    c2 = c1;
    ASSERT_EQ(c2.operations.size(), 2);
    ASSERT_EQ(c1, c2);
}
