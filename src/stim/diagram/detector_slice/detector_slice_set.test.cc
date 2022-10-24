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

#include "stim/diagram/detector_slice/detector_slice_set.h"

#include <fstream>

#include "gtest/gtest.h"

#include "stim/gen/circuit_gen_params.h"
#include "stim/gen/gen_rep_code.h"
#include "stim/gen/gen_surface_code.h"

using namespace stim;
using namespace stim_draw_internal;

TEST(detector_slice_set, from_circuit) {
    std::vector<double> empty_filter;
    auto slice_set = DetectorSliceSet::from_circuit_tick(
        stim::Circuit(R"CIRCUIT(
        QUBIT_COORDS(3, 5) 1
        R 0
        M 0
        DETECTOR rec[-1]
        TICK
        CX 2 1

        TICK  # Here
        H 0
        CX 1 0
        M 0
        DETECTOR rec[-1]
        M 1
        DETECTOR rec[-1]

        REPEAT 100 {
            TICK
            R 0
            M 0
            DETECTOR rec[-1]
        }
)CIRCUIT"),
        2, {&empty_filter});
    ASSERT_EQ(slice_set.coordinates, (std::map<uint64_t, std::vector<double>>{{1, {3, 5}}}));
    ASSERT_EQ(
        slice_set.slices,
        (std::map<stim::DemTarget, std::vector<stim::GateTarget>>{
            {DemTarget::relative_detector_id(1), {GateTarget::x(0), GateTarget::z(1)}},
            {DemTarget::relative_detector_id(2), {GateTarget::z(1)}},
        }));
}

TEST(detector_slice_set, big_loop_seeking) {
    stim::Circuit circuit(R"CIRCUIT(
        REPEAT 100000 {
            REPEAT 10000 {
                REPEAT 1000 {
                    REPEAT 100 {
                        REPEAT 10 {
                            RY 0
                            TICK
                            MY 0
                            DETECTOR rec[-1]
                        }
                    }
                }
                RX 1
                TICK
                MX 1
                OBSERVABLE_INCLUDE(5) rec[-1]
                DETECTOR rec[-1]
            }
        }
    )CIRCUIT");

    uint64_t inner = 10 * 100 * 1000 + 1;
    std::vector<double> empty_filter;
    auto slice_set = DetectorSliceSet::from_circuit_tick(circuit, inner * 10000ULL * 50ULL + 2ULL, {&empty_filter});
    ASSERT_EQ(slice_set.coordinates, (std::map<uint64_t, std::vector<double>>{}));
    ASSERT_EQ(
        slice_set.slices,
        (std::map<stim::DemTarget, std::vector<stim::GateTarget>>{
            {DemTarget::relative_detector_id(inner * 10000ULL * 50ULL + 1ULL), {GateTarget::y(0)}},
        }));

    slice_set =
        DetectorSliceSet::from_circuit_tick(circuit, inner * 10000ULL * 25ULL + 1000ULL * 100ULL * 10ULL + 1ULL, {&empty_filter});
    ASSERT_EQ(slice_set.coordinates, (std::map<uint64_t, std::vector<double>>{}));
    ASSERT_EQ(
        slice_set.slices,
        (std::map<stim::DemTarget, std::vector<stim::GateTarget>>{
            {DemTarget::relative_detector_id(inner * 10000ULL * 25ULL + 1000ULL * 100ULL * 10ULL), {GateTarget::x(1)}},
            {DemTarget::observable_id(5), {GateTarget::x(1)}},
        }));
}

TEST(detector_slice_set_text_diagram, repetition_code) {
    std::vector<double> empty_filter;
    CircuitGenParameters params(10, 5, "memory");
    auto circuit = generate_rep_code_circuit(params).circuit;
    auto slice_set = DetectorSliceSet::from_circuit_tick(circuit, 9, {&empty_filter});
    ASSERT_EQ(slice_set.slices.size(), circuit.count_qubits());
    ASSERT_EQ("\n" + slice_set.str() + "\n", R"DIAGRAM(
q0: --------Z:D12----------------------------
            |
q1: -Z:D8---Z:D12----------------------------
            |
q2: --------Z:D12--Z:D13---------------------
                   |
q3: -Z:D9----------Z:D13---------------------
                   |
q4: ---------------Z:D13--Z:D14--------------
                          |
q5: -Z:D10----------------Z:D14--------------
                          |
q6: ----------------------Z:D14--Z:D15-------
                                 |
q7: -Z:D11-----------------------Z:D15-------
                                 |
q8: -----------------------------Z:D15--Z:L0-
)DIAGRAM");

    ASSERT_EQ("\n" + DetectorSliceSet::from_circuit_tick(circuit, 11, {&empty_filter}).str() + "\n", R"DIAGRAM(
q0: --------Z:D16-
            |
q1: -Z:D12--Z:D16-
     |
q2: -Z:D12--Z:D17-
            |
q3: -Z:D13--Z:D17-
     |
q4: -Z:D13--Z:D18-
            |
q5: -Z:D14--Z:D18-
     |
q6: -Z:D14--Z:D19-
            |
q7: -Z:D15--Z:D19-
     |
q8: -Z:D15--Z:L0--
)DIAGRAM");
}

TEST(detector_slice_set_text_diagram, surface_code) {
    std::vector<double> empty_filter;
    CircuitGenParameters params(10, 2, "unrotated_memory_z");
    auto circuit = generate_surface_code_circuit(params).circuit;
    auto slice_set = DetectorSliceSet::from_circuit_tick(circuit, 11, {&empty_filter});
    ASSERT_EQ(slice_set.slices.size(), circuit.count_qubits());
    ASSERT_EQ("\n" + slice_set.str() + "\n", R"DIAGRAM(
q0:(0, 0) -X:D2--Z:D3--------------------------------Z:L0-
           |     |                                   |
q1:(1, 0) -X:D2--|-----------------X:D6--Z:D7--------Z:L0-
           |     |                 |     |           |
q2:(2, 0) -|-----|-----Z:D4--------X:D6--|-----------Z:L0-
           |     |     |           |     |
q3:(0, 1) -X:D2--Z:D3--|-----------|-----Z:D7-------------
                       |           |     |
q4:(1, 1) -------------Z:D4--X:D5--X:D6--Z:D7-------------
                       |     |           |
q5:(2, 1) -------------Z:D4--|-----------|-----Z:D8--X:D9-
                       |     |           |     |     |
q6:(0, 2) -------------|-----X:D5--------Z:D7--|-----|----
                       |     |                 |     |
q7:(1, 2) -------------Z:D4--X:D5--------------|-----X:D9-
                                               |     |
q8:(2, 2) -------------------------------------Z:D8--X:D9-
)DIAGRAM");
}

TEST(detector_slice_set_svg_diagram, surface_code) {
    std::vector<double> empty_filter;
    CircuitGenParameters params(10, 2, "rotated_memory_z");
    auto circuit = generate_surface_code_circuit(params).circuit;
    auto slice_set = DetectorSliceSet::from_circuit_tick(circuit, 8, {&empty_filter});
    std::stringstream ss;
    slice_set.write_svg_diagram_to(ss);
    ASSERT_EQ(
        "\n" + ss.str() + "\n",
        u8R"SVG(
<svg viewBox="0 0 77.2548 122.51" xmlns="http://www.w3.org/2000/svg">
<path d="M38.6274,16 61.2548,38.6274 16,38.6274 Z" stroke="none" fill-opacity="0.75" fill="#AAAAAA" />
<defs>
<radialGradient id="xgrad"><stop offset="50%" stop-color="#FF4444" stop-opacity="1"/><stop offset="100%" stop-color="#AAAAAA" stop-opacity="0"/></radialGradient>
<radialGradient id="ygrad"><stop offset="50%" stop-color="#40FF40" stop-opacity="1"/><stop offset="100%" stop-color="#AAAAAA" stop-opacity="0"/></radialGradient>
<radialGradient id="zgrad"><stop offset="50%" stop-color="#4848FF" stop-opacity="1"/><stop offset="100%" stop-color="#AAAAAA" stop-opacity="0"/></radialGradient>
</defs>
<clipPath id="clip0"><path d="M38.6274,16 61.2548,38.6274 16,38.6274 Z" /></clipPath>
<circle clip-path="url(#clip0)" cx="16" cy="38.6274" r="20" stroke="none" fill="url('#xgrad')"/>
<circle clip-path="url(#clip0)" cx="38.6274" cy="16" r="20" stroke="none" fill="url('#zgrad')"/>
<circle clip-path="url(#clip0)" cx="61.2548" cy="38.6274" r="20" stroke="none" fill="url('#xgrad')"/>
<path d="M38.6274,16 61.2548,38.6274 16,38.6274 Z" stroke="black" fill="none" />
<path d="M16,38.6274 61.2548,38.6274 38.6274,61.2548 61.2548,83.8822 16,83.8822 Z" stroke="none" fill-opacity="0.75" fill="#4848FF" />
<path d="M16,38.6274 61.2548,38.6274 38.6274,61.2548 61.2548,83.8822 16,83.8822 Z" stroke="black" fill="none" />
<path d="M16,83.8822 61.2548,83.8822 38.6274,106.51 Z" stroke="none" fill-opacity="0.75" fill="#AAAAAA" />
<clipPath id="clip1"><path d="M16,83.8822 61.2548,83.8822 38.6274,106.51 Z" /></clipPath>
<circle clip-path="url(#clip1)" cx="16" cy="83.8822" r="20" stroke="none" fill="url('#xgrad')"/>
<circle clip-path="url(#clip1)" cx="61.2548" cy="83.8822" r="20" stroke="none" fill="url('#xgrad')"/>
<circle clip-path="url(#clip1)" cx="38.6274" cy="106.51" r="20" stroke="none" fill="url('#zgrad')"/>
<path d="M16,83.8822 61.2548,83.8822 38.6274,106.51 Z" stroke="black" fill="none" />
<path d="M32.6274,16 a 6 6 0 0 0 12 0 a 6 6 0 0 0 -12 0" stroke="none" fill-opacity="1" fill="#4848FF" />
<path d="M32.6274,16 a 6 6 0 0 0 12 0 a 6 6 0 0 0 -12 0" stroke="black" fill="none" />
<path d="M32.6274,61.2548 a 6 6 0 0 0 12 0 a 6 6 0 0 0 -12 0" stroke="none" fill-opacity="1" fill="#4848FF" />
<path d="M32.6274,61.2548 a 6 6 0 0 0 12 0 a 6 6 0 0 0 -12 0" stroke="black" fill="none" />
<path d="M32.6274,106.51 a 6 6 0 0 0 12 0 a 6 6 0 0 0 -12 0" stroke="none" fill-opacity="1" fill="#4848FF" />
<path d="M32.6274,106.51 a 6 6 0 0 0 12 0 a 6 6 0 0 0 -12 0" stroke="black" fill="none" />
<circle cx="16" cy="38.6274" r="2" stroke="none" fill="black" />
<circle cx="38.6274" cy="16" r="2" stroke="none" fill="black" />
<circle cx="61.2548" cy="38.6274" r="2" stroke="none" fill="black" />
<circle cx="16" cy="83.8822" r="2" stroke="none" fill="black" />
<circle cx="38.6274" cy="61.2548" r="2" stroke="none" fill="black" />
<circle cx="61.2548" cy="83.8822" r="2" stroke="none" fill="black" />
<circle cx="38.6274" cy="106.51" r="2" stroke="none" fill="black" />
</svg>
)SVG");
}
