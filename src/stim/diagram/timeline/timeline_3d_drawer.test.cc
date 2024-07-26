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

#include "stim/diagram/timeline/timeline_3d_drawer.h"

#include <fstream>

#include "gtest/gtest.h"

#include "stim/circuit/circuit.test.h"
#include "stim/gen/circuit_gen_params.h"
#include "stim/gen/gen_rep_code.h"
#include "stim/gen/gen_surface_code.h"
#include "stim/util_bot/test_util.test.h"

using namespace stim;
using namespace stim_draw_internal;

void expect_diagram_is_identical_to_saved_file(const Circuit &circuit, std::string_view key) {
    auto diagram = DiagramTimeline3DDrawer::circuit_to_basic_3d_diagram(circuit);
    std::stringstream actual_ss;
    diagram.to_gltf_scene().to_json().write(actual_ss);
    auto actual = actual_ss.str();

    auto path = resolve_test_file(key);
    FILE *f = fopen(path.c_str(), "rb");
    auto expected = rewind_read_close(f);

    if (expected != actual) {
        auto dot = key.rfind('.');
        std::string new_path;
        if (dot == std::string::npos) {
            new_path = path + ".new";
        } else {
            dot += path.size() - key.size();
            new_path = path.substr(0, dot) + ".new" + path.substr(dot);
        }
        std::ofstream out;
        out.open(new_path);
        out << actual;
        out.close();
        EXPECT_TRUE(false) << "Diagram didn't agree. key=" << key;
    }
}

TEST(circuit_diagram_timeline_3d, single_qubit_gates) {
    Circuit circuit(R"CIRCUIT(
        I 0
        X 1
        Y 2
        Z 3
        C_XYZ 0
        C_ZYX 1
        H 2
        H_XY 3
        H_XZ 0
        H_YZ 1
        S 2
        SQRT_X 3
        SQRT_X_DAG 0
        SQRT_Y 1
        SQRT_Y_DAG 2
        SQRT_Z 3
        SQRT_Z_DAG 0
        S_DAG 1
        H 2 0 3
    )CIRCUIT");

    expect_diagram_is_identical_to_saved_file(circuit, "single_qubits_gates.gltf");
}

TEST(circuit_diagram_timeline_3d, two_qubits_gates) {
    Circuit circuit(R"CIRCUIT(
        CNOT 0 1
        CX 2 3
        CY 4 5 5 4
        CZ 0 2
        ISWAP 1 3
        ISWAP_DAG 2 4
        SQRT_XX 3 5
        SQRT_XX_DAG 0 5
        SQRT_YY 3 4 4 3
        SQRT_YY_DAG 0 1
        SQRT_ZZ 2 3
        SQRT_ZZ_DAG 4 5
        SWAP 0 1
        XCX 2 3
        XCY 3 4
        XCZ 0 1
        YCX 2 3
        YCY 4 5
        YCZ 0 1
        ZCX 2 3
        ZCY 4 5
        ZCZ 0 5 2 3 1 4
        CXSWAP 0 1
        SWAPCX 2 3
    )CIRCUIT");
    expect_diagram_is_identical_to_saved_file(circuit, "two_qubits_gates.gltf");
}

TEST(circuit_diagram_timeline_3d, noise_gates) {
    Circuit circuit(R"CIRCUIT(
        DEPOLARIZE1(0.125) 0 1
        DEPOLARIZE2(0.125) 0 2 4 5
        X_ERROR(0.125) 0 1 2
        Y_ERROR(0.125) 0 1 4
        Z_ERROR(0.125) 2 3 5
    )CIRCUIT");
    expect_diagram_is_identical_to_saved_file(circuit, "noise_gates_1.gltf");

    circuit = Circuit(R"CIRCUIT(
        E(0.25) X1 X2
        CORRELATED_ERROR(0.125) X1 Y2 Z3
        ELSE_CORRELATED_ERROR(0.25) X2 Y4 Z3
        ELSE_CORRELATED_ERROR(0.25) X5
    )CIRCUIT");
    expect_diagram_is_identical_to_saved_file(circuit, "noise_gates_2.gltf");

    circuit = Circuit(R"CIRCUIT(
        PAULI_CHANNEL_1(0.125,0.25,0.125) 0 1 2 3
        PAULI_CHANNEL_2(0.01,0.01,0.01,0.02,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01) 0 1 2 4
    )CIRCUIT");
    expect_diagram_is_identical_to_saved_file(circuit, "noise_gates_3.gltf");
}

TEST(circuit_diagram_timeline_3d, collapsing) {
    Circuit circuit(R"CIRCUIT(
        R 0
        RX 1
        RY 2
        RZ 3
        M(0.001) 0 1
        MR 1 0
        MRX 1 2
        MRY 0 3 1
        MRZ 0
        MX 1
        MY 2
        MZ 3
        MPP X0*Y2 Z3 X1 Z2*Y3
    )CIRCUIT");
    expect_diagram_is_identical_to_saved_file(circuit, "collapsing.gltf");
}

TEST(circuit_diagram_timeline_3d, measurement_looping) {
    Circuit circuit(R"CIRCUIT(
        M 0
        REPEAT 100 {
            M 1
            REPEAT 5 {
                M 2
            }
            REPEAT 7 {
                MPP X3*Y4
            }
        }
    )CIRCUIT");
    expect_diagram_is_identical_to_saved_file(circuit, "measurement_looping.gltf");
}

TEST(circuit_diagram_timeline_3d, repeat) {
    auto circuit = Circuit(R"CIRCUIT(
        H 0 1 2
        REPEAT 5 {
            RX 2
            REPEAT 100 {
                H 0 1 3 3
            }
        }
    )CIRCUIT");
    expect_diagram_is_identical_to_saved_file(circuit, "repeat.gltf");
}

TEST(circuit_diagram_timeline_3d, classical_feedback) {
    auto circuit = Circuit(R"CIRCUIT(
        M 0
        CX rec[-1] 1
        YCZ 2 sweep[5]
    )CIRCUIT");
    expect_diagram_is_identical_to_saved_file(circuit, "classical_feedback.gltf");
}

TEST(circuit_diagram_timeline_3d, lattice_surgery_cnot) {
    auto circuit = Circuit(R"CIRCUIT(
        R 2
        MPP X1*X2
        MPP Z0*Z2
        MX 2
        CZ rec[-3] 0
        CX rec[-2] 1
        CZ rec[-1] 0
    )CIRCUIT");
    expect_diagram_is_identical_to_saved_file(circuit, "lattice_surgery_cnot.gltf");
}

TEST(circuit_diagram_timeline_3d, tick) {
    auto circuit = Circuit(R"CIRCUIT(
        H 0 0
        TICK
        H 0 1
        TICK
        H 0
        REPEAT 1 {
            H 0 1
            TICK
            H 0
            S 0
        }
        H 0 0
        SQRT_X 0
        TICK
        H 0 0
    )CIRCUIT");
    expect_diagram_is_identical_to_saved_file(circuit, "tick.gltf");
}

TEST(circuit_diagram_timeline_3d, detector_pseudo_targets) {
    auto circuit = Circuit(R"CIRCUIT(
        M 0 1 2 3 4 5
        REPEAT 100 {
            M 1 2
        }
        DETECTOR(1) rec[-1]
        DETECTOR(2) rec[-2]
        DETECTOR(3) rec[-3]
        DETECTOR(4) rec[-4]
        DETECTOR(5) rec[-1] rec[-2]
        OBSERVABLE_INCLUDE(100) rec[-201] rec[-203]
    )CIRCUIT");
    expect_diagram_is_identical_to_saved_file(circuit, "detector_pseudo_targets.gltf");
}

TEST(circuit_diagram_timeline_3d, repetition_code) {
    CircuitGenParameters params(10, 3, "memory");
    auto circuit = generate_rep_code_circuit(params).circuit;
    expect_diagram_is_identical_to_saved_file(circuit, "repetition_code.gltf");
}

TEST(circuit_diagram_timeline_3d, surface_code) {
    CircuitGenParameters params(10, 3, "unrotated_memory_z");
    auto circuit = generate_surface_code_circuit(params).circuit;
    expect_diagram_is_identical_to_saved_file(circuit, "surface_code.gltf");
}

TEST(circuit_diagram_timeline_3d, test_circuit_all_ops) {
    expect_diagram_is_identical_to_saved_file(generate_test_circuit_with_all_operations(), "circuit_all_ops_3d.gltf");
}
