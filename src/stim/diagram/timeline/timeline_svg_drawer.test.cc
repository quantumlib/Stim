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

#include "stim/diagram/timeline/timeline_svg_drawer.h"

#include <fstream>

#include "gtest/gtest.h"

#include "stim/circuit/circuit.test.h"
#include "stim/gen/circuit_gen_params.h"
#include "stim/gen/gen_rep_code.h"
#include "stim/gen/gen_surface_code.h"
#include "stim/util_bot/test_util.test.h"

using namespace stim;
using namespace stim_draw_internal;

void expect_svg_diagram_is_identical_to_saved_file(const Circuit &circuit, std::string_view key) {
    std::stringstream ss;
    CoordFilter filter;
    DiagramTimelineSvgDrawer::make_diagram_write_to(
        circuit, ss, 0, UINT64_MAX, DiagramTimelineSvgDrawerMode::SVG_MODE_TIMELINE, {&filter});
    expect_string_is_identical_to_saved_file(ss.str(), key);
}

TEST(circuit_diagram_timeline_svg, single_qubit_gates) {
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

    expect_svg_diagram_is_identical_to_saved_file(circuit, "single_qubits_gates.svg");
}

TEST(circuit_diagram_timeline_svg, two_qubits_gates) {
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
    expect_svg_diagram_is_identical_to_saved_file(circuit, "two_qubits_gates.svg");
}

TEST(circuit_diagram_timeline_svg, noise_gates) {
    Circuit circuit(R"CIRCUIT(
        DEPOLARIZE1(0.125) 0 1
        DEPOLARIZE2(0.125) 0 2 4 5
        X_ERROR(0.125) 0 1 2
        Y_ERROR(0.125) 0 1 4
        Z_ERROR(0.125) 2 3 5
    )CIRCUIT");
    expect_svg_diagram_is_identical_to_saved_file(circuit, "noise_gates_1.svg");

    circuit = Circuit(R"CIRCUIT(
        E(0.25) X1 X2
        CORRELATED_ERROR(0.125) X1 Y2 Z3
        ELSE_CORRELATED_ERROR(0.25) X2 Y4 Z3
        ELSE_CORRELATED_ERROR(0.25) X5
    )CIRCUIT");
    expect_svg_diagram_is_identical_to_saved_file(circuit, "noise_gates_2.svg");

    circuit = Circuit(R"CIRCUIT(
        PAULI_CHANNEL_1(0.125,0.25,0.125) 0 1 2 3
        PAULI_CHANNEL_2(0.01,0.01,0.01,0.02,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01) 0 1 2 4
    )CIRCUIT");
    expect_svg_diagram_is_identical_to_saved_file(circuit, "noise_gates_3.svg");
}

TEST(circuit_diagram_timeline_svg, collapsing) {
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
    expect_svg_diagram_is_identical_to_saved_file(circuit, "collapsing.svg");
}

TEST(circuit_diagram_timeline_svg, measurement_looping) {
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
    expect_svg_diagram_is_identical_to_saved_file(circuit, "measurement_looping.svg");
}

TEST(circuit_diagram_timeline_svg, repeat) {
    auto circuit = Circuit(R"CIRCUIT(
        H 0 1 2
        REPEAT 5 {
            RX 2
            REPEAT 100 {
                H 0 1 3 3
            }
        }
    )CIRCUIT");
    expect_svg_diagram_is_identical_to_saved_file(circuit, "repeat.svg");
}

TEST(circuit_diagram_timeline_svg, classical_feedback) {
    auto circuit = Circuit(R"CIRCUIT(
        M 0
        CX rec[-1] 1
        YCZ 2 sweep[5]
    )CIRCUIT");
    expect_svg_diagram_is_identical_to_saved_file(circuit, "classical_feedback.svg");
}

TEST(circuit_diagram_timeline_svg, lattice_surgery_cnot) {
    auto circuit = Circuit(R"CIRCUIT(
        R 2
        MPP X1*X2
        MPP Z0*Z2
        MX 2
        CZ rec[-3] 0
        CX rec[-2] 1
        CZ rec[-1] 0
    )CIRCUIT");
    expect_svg_diagram_is_identical_to_saved_file(circuit, "lattice_surgery_cnot.svg");
}

TEST(circuit_diagram_timeline_svg, tick) {
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

    std::stringstream ss;
    CoordFilter filter;
    DiagramTimelineSvgDrawer::make_diagram_write_to(
        circuit, ss, 0, UINT64_MAX, DiagramTimelineSvgDrawerMode::SVG_MODE_TIMELINE, {&filter});
    expect_string_is_identical_to_saved_file(ss.str(), "tick.svg");
}

TEST(circuit_diagram_timeline_svg, shifted_coords) {
    auto circuit = Circuit(R"CIRCUIT(
        QUBIT_COORDS(1, 2) 1
        DETECTOR(4, 5, 6)
        SHIFT_COORDS(10, 20, 30, 40)
        QUBIT_COORDS(1, 2) 2
        DETECTOR(4, 5, 6)
        REPEAT 100 {
            QUBIT_COORDS(7, 8) 3 4
            DETECTOR(9, 10, 11)
            SHIFT_COORDS(0, 200, 300, 400)
        }
        QUBIT_COORDS(1, 2) 5
        DETECTOR(4, 5, 6)
    )CIRCUIT");
    expect_svg_diagram_is_identical_to_saved_file(circuit, "shifted_coords.svg");
}

TEST(circuit_diagram_timeline_svg, detector_pseudo_targets) {
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
    expect_svg_diagram_is_identical_to_saved_file(circuit, "detector_pseudo_targets.svg");
}

TEST(circuit_diagram_timeline_svg, repetition_code) {
    CircuitGenParameters params(10, 3, "memory");
    auto circuit = generate_rep_code_circuit(params).circuit;
    expect_svg_diagram_is_identical_to_saved_file(circuit, "repetition_code.svg");
}

TEST(circuit_diagram_timeline_svg, surface_code) {
    CircuitGenParameters params(10, 3, "unrotated_memory_z");
    auto circuit = generate_surface_code_circuit(params).circuit;
    expect_svg_diagram_is_identical_to_saved_file(circuit, "surface_code.svg");
}

TEST(circuit_diagram_time_detector_slice_svg, surface_code_partial) {
    CircuitGenParameters params(10, 3, "unrotated_memory_z");
    auto circuit = generate_surface_code_circuit(params).circuit.flattened();
    CoordFilter filter;
    filter.coordinates.push_back(2);
    std::stringstream ss;
    DiagramTimelineSvgDrawer::make_diagram_write_to(
        circuit, ss, 5, 11, DiagramTimelineSvgDrawerMode::SVG_MODE_TIME_DETECTOR_SLICE, {&filter});
    expect_string_is_identical_to_saved_file(ss.str(), "surface_code_time_detector_slice.svg");
}

TEST(circuit_diagram_time_detector_slice_svg, surface_code_full) {
    CircuitGenParameters params(5, 3, "unrotated_memory_z");
    auto circuit = generate_surface_code_circuit(params).circuit;
    CoordFilter filter;
    filter.coordinates.push_back(1);
    std::stringstream ss;
    DiagramTimelineSvgDrawer::make_diagram_write_to(
        circuit, ss, 0, UINT64_MAX, DiagramTimelineSvgDrawerMode::SVG_MODE_TIME_DETECTOR_SLICE, {&filter});
    expect_string_is_identical_to_saved_file(ss.str(), "surface_code_full_time_detector_slice.svg");
}

TEST(circuit_diagram_time_slice_svg, surface_code) {
    CircuitGenParameters params(10, 3, "rotated_memory_z");
    auto circuit = generate_surface_code_circuit(params).circuit;
    CoordFilter filter;
    std::stringstream ss;
    DiagramTimelineSvgDrawer::make_diagram_write_to(
        circuit, ss, 5, 11, DiagramTimelineSvgDrawerMode::SVG_MODE_TIME_SLICE, {&filter});
    expect_string_is_identical_to_saved_file(ss.str(), "surface_code_time_slice.svg");
}

TEST(circuit_diagram_timeline_svg, chained_loops) {
    auto circuit = Circuit(R"CIRCUIT(
        REPEAT 2 {
            H 0
            TICK
        }
        X 0
        TICK
        Y 0
        TICK
        Z 0
        TICK
        REPEAT 3 {
            C_XYZ 0
            TICK
        }
        X 1
        TICK
        Y 1
        TICK
        Z 1
        TICK
    )CIRCUIT");

    CoordFilter empty_filter;
    std::stringstream ss;
    DiagramTimelineSvgDrawer::make_diagram_write_to(
        circuit, ss, 0, UINT64_MAX, DiagramTimelineSvgDrawerMode::SVG_MODE_TIME_DETECTOR_SLICE, {&empty_filter});
    expect_string_is_identical_to_saved_file(ss.str(), "circuit_diagram_timeline_svg_chained_loops.svg");
}

TEST(diagram_timeline_svg_drawer, make_diagram_write_to) {
    CircuitGenParameters params(2, 3, "rotated_memory_x");
    auto circuit = generate_surface_code_circuit(params).circuit;
    std::vector<CoordFilter> coord_filter{CoordFilter{}};
    std::stringstream ss;
    DiagramTimelineSvgDrawer::make_diagram_write_to(
        circuit,
        ss,
        0,
        circuit.count_ticks(),
        DiagramTimelineSvgDrawerMode::SVG_MODE_TIME_DETECTOR_SLICE,
        coord_filter);
    expect_string_is_identical_to_saved_file(ss.str(), "detslice-with-ops_surface_code.svg");
}

TEST(diagram_timeline_svg_drawer, test_circuit_all_ops_time_slice) {
    auto circuit = generate_test_circuit_with_all_operations();
    CoordFilter filter;
    std::stringstream ss;
    DiagramTimelineSvgDrawer::make_diagram_write_to(
        circuit, ss, 0, UINT64_MAX, DiagramTimelineSvgDrawerMode::SVG_MODE_TIME_SLICE, {&filter});
    expect_string_is_identical_to_saved_file(ss.str(), "circuit_all_ops_timeslice.svg");
}

TEST(diagram_timeline_svg_drawer, test_circuit_all_ops_time_line) {
    auto circuit = generate_test_circuit_with_all_operations();
    CoordFilter filter;
    std::stringstream ss;
    DiagramTimelineSvgDrawer::make_diagram_write_to(
        circuit, ss, 0, UINT64_MAX, DiagramTimelineSvgDrawerMode::SVG_MODE_TIMELINE, {&filter});
    expect_string_is_identical_to_saved_file(ss.str(), "circuit_all_ops_timeline.svg");
}

TEST(diagram_timeline_svg_drawer, test_circuit_all_ops_detslice) {
    CoordFilter empty_filter;
    std::stringstream ss;
    auto circuit = generate_test_circuit_with_all_operations();
    DiagramTimelineSvgDrawer::make_diagram_write_to(
        circuit, ss, 0, UINT64_MAX, DiagramTimelineSvgDrawerMode::SVG_MODE_TIME_DETECTOR_SLICE, {&empty_filter});
    expect_string_is_identical_to_saved_file(ss.str(), "circuit_all_ops_detslice.svg");
}

TEST(diagram_timeline_svg_drawer, anticommuting_detector_circuit) {
    CoordFilter empty_filter;
    std::stringstream ss;
    auto circuit = Circuit(R"CIRCUIT(
        TICK
        R 0
        TICK
        R 0
        TICK
        MXX 0 1
        M 2
        DETECTOR rec[-1]
        DETECTOR rec[-2]
    )CIRCUIT");
    DiagramTimelineSvgDrawer::make_diagram_write_to(
        circuit, ss, 0, UINT64_MAX, DiagramTimelineSvgDrawerMode::SVG_MODE_TIME_DETECTOR_SLICE, {&empty_filter});
    expect_string_is_identical_to_saved_file(ss.str(), "anticommuting_detslice.svg");
}

TEST(diagram_timeline_svg_drawer, bezier_curves) {
    CoordFilter empty_filter;
    std::stringstream ss;
    auto circuit = Circuit(R"CIRCUIT(
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(1, 0) 1
        QUBIT_COORDS(2, 0) 2
        QUBIT_COORDS(3, 0) 3
        CX 0 1
        CX 2 3
        TICK
        CX 0 2
        CX 1 3
    )CIRCUIT");
    DiagramTimelineSvgDrawer::make_diagram_write_to(
        circuit, ss, 0, UINT64_MAX, DiagramTimelineSvgDrawerMode::SVG_MODE_TIME_SLICE, {&empty_filter});
    expect_string_is_identical_to_saved_file(ss.str(), "bezier_time_slice.svg");
}
