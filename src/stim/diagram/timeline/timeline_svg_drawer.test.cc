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

#include <fstream>
#include "stim/diagram/timeline/timeline_svg_drawer.h"

#include "gtest/gtest.h"
#include "stim/gen/circuit_gen_params.h"
#include "stim/gen/gen_rep_code.h"

using namespace stim;
using namespace stim_draw_internal;

std::string svg_diagram(const Circuit &circuit) {
    std::stringstream ss;
    DiagramTimelineSvgDrawer::make_diagram_write_to(circuit, ss);
    return ss.str();
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
    ASSERT_EQ("\n" + svg_diagram(circuit), u8R"DIAGRAM(
<svg width="448" height="288" version="1.1" xmlns="http://www.w3.org/2000/svg">
<path d="M32,32 L448,32 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="32">q0</text>
<path d="M32,96 L448,96 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="96">q1</text>
<path d="M32,160 L448,160 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="160">q2</text>
<path d="M32,224 L448,224 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="224">q3</text>
<rect x="48" y="16" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="64" y="32">I</text>
<rect x="48" y="80" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="64" y="96">X</text>
<rect x="48" y="144" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="64" y="160">Y</text>
<rect x="48" y="208" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="64" y="224">Z</text>
<rect x="112" y="16" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="128" y="32">C<tspan baseline-shift="sub" font-size="10">XYZ</tspan></text>
<rect x="112" y="80" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="128" y="96">C<tspan baseline-shift="sub" font-size="10">ZYX</tspan></text>
<rect x="112" y="144" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="128" y="160">H</text>
<rect x="112" y="208" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="128" y="224">H<tspan baseline-shift="sub" font-size="10">XY</tspan></text>
<rect x="176" y="16" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="192" y="32">H</text>
<rect x="176" y="80" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="192" y="96">H<tspan baseline-shift="sub" font-size="10">YZ</tspan></text>
<rect x="176" y="144" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="192" y="160">S</text>
<rect x="176" y="208" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="192" y="224">√X</text>
<rect x="240" y="16" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="256" y="32">√X<tspan baseline-shift="super" font-size="10">†</tspan></text>
<rect x="240" y="80" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="256" y="96">√Y</text>
<rect x="240" y="144" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="256" y="160">√Y<tspan baseline-shift="super" font-size="10">†</tspan></text>
<rect x="240" y="208" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="256" y="224">S</text>
<rect x="304" y="16" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="320" y="32">S<tspan baseline-shift="super" font-size="10">†</tspan></text>
<rect x="304" y="80" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="320" y="96">S<tspan baseline-shift="super" font-size="10">†</tspan></text>
<rect x="304" y="144" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="320" y="160">H</text>
<rect x="368" y="16" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="384" y="32">H</text>
<rect x="368" y="208" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="384" y="224">H</text>
</svg>
)DIAGRAM");
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
    )CIRCUIT");
    ASSERT_EQ("\n" + svg_diagram(circuit), u8R"DIAGRAM(
<svg width="1088" height="416" version="1.1" xmlns="http://www.w3.org/2000/svg">
<path d="M32,32 L1088,32 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="32">q0</text>
<path d="M32,96 L1088,96 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="96">q1</text>
<path d="M32,160 L1088,160 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="160">q2</text>
<path d="M32,224 L1088,224 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="224">q3</text>
<path d="M32,288 L1088,288 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="288">q4</text>
<path d="M32,352 L1088,352 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="352">q5</text>
<path d="M64,32 L64,96 " stroke="black"/>
<circle cx="64" cy="32" r="8" stroke="none" fill="black"/>
<circle cx="64" cy="96" r="8" stroke="black" fill="white"/>
<path d="M56,96 L72,96 M64,88 L64,104 " stroke="black"/>
<path d="M64,160 L64,224 " stroke="black"/>
<circle cx="64" cy="160" r="8" stroke="none" fill="black"/>
<circle cx="64" cy="224" r="8" stroke="black" fill="white"/>
<path d="M56,224 L72,224 M64,216 L64,232 " stroke="black"/>
<path d="M64,288 L64,352 " stroke="black"/>
<circle cx="64" cy="288" r="8" stroke="none" fill="black"/>
<path d="M64,362 L55.3397,347 L72.6603,347 Z" stroke="black" fill="gray"/>
<path d="M128,288 L128,352 " stroke="black"/>
<circle cx="128" cy="352" r="8" stroke="none" fill="black"/>
<path d="M128,298 L119.34,283 L136.66,283 Z" stroke="black" fill="gray"/>
<path d="M128,32 L128,160 " stroke="black"/>
<circle cx="128" cy="32" r="8" stroke="none" fill="black"/>
<circle cx="128" cy="160" r="8" stroke="none" fill="black"/>
<path d="M192,96 L192,224 " stroke="black"/>
<circle cx="192" cy="96" r="8" stroke="none" fill="gray"/>
<path d="M188,92 L196,100 M196,92 L188,100 " stroke="black"/>
<circle cx="192" cy="224" r="8" stroke="none" fill="gray"/>
<path d="M188,220 L196,228 M196,220 L188,228 " stroke="black"/>
<path d="M256,160 L256,288 " stroke="black"/>
<circle cx="256" cy="160" r="8" stroke="none" fill="gray"/>
<path d="M252,156 L260,164 M260,156 L252,164 " stroke="black"/>
<path d="M260,150 L268,150 M264,146 L264,158 " stroke="black"/>
<circle cx="256" cy="288" r="8" stroke="none" fill="gray"/>
<path d="M252,284 L260,292 M260,284 L252,292 " stroke="black"/>
<path d="M260,278 L268,278 M264,274 L264,286 " stroke="black"/>
<path d="M320,224 L320,352 " stroke="black"/>
<rect x="304" y="208" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="320" y="224">√XX</text>
<rect x="304" y="336" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="320" y="352">√XX</text>
<path d="M384,32 L384,352 " stroke="black"/>
<rect x="368" y="16" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="384" y="32">√XX<tspan baseline-shift="super" font-size="10">†</tspan></text>
<rect x="368" y="336" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="384" y="352">√XX<tspan baseline-shift="super" font-size="10">†</tspan></text>
<path d="M448,224 L448,288 " stroke="black"/>
<rect x="432" y="208" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="448" y="224">√YY</text>
<rect x="432" y="272" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="448" y="288">√YY</text>
<path d="M512,224 L512,288 " stroke="black"/>
<rect x="496" y="272" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="512" y="288">√YY</text>
<rect x="496" y="208" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="512" y="224">√YY</text>
<path d="M512,32 L512,96 " stroke="black"/>
<rect x="496" y="16" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="512" y="32">√YY<tspan baseline-shift="super" font-size="10">†</tspan></text>
<rect x="496" y="80" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="512" y="96">√YY<tspan baseline-shift="super" font-size="10">†</tspan></text>
<path d="M576,160 L576,224 " stroke="black"/>
<rect x="560" y="144" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="576" y="160">√ZZ</text>
<rect x="560" y="208" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="576" y="224">√ZZ</text>
<path d="M576,288 L576,352 " stroke="black"/>
<rect x="560" y="272" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="576" y="288">√ZZ<tspan baseline-shift="super" font-size="10">†</tspan></text>
<rect x="560" y="336" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="576" y="352">√ZZ<tspan baseline-shift="super" font-size="10">†</tspan></text>
<path d="M576,32 L576,96 " stroke="black"/>
<path d="M572,28 L580,36 M580,28 L572,36 " stroke="black"/>
<path d="M572,92 L580,100 M580,92 L572,100 " stroke="black"/>
<path d="M640,160 L640,224 " stroke="black"/>
<circle cx="640" cy="160" r="8" stroke="black" fill="white"/>
<path d="M632,160 L648,160 M640,152 L640,168 " stroke="black"/>
<circle cx="640" cy="224" r="8" stroke="black" fill="white"/>
<path d="M632,224 L648,224 M640,216 L640,232 " stroke="black"/>
<path d="M704,224 L704,288 " stroke="black"/>
<circle cx="704" cy="224" r="8" stroke="black" fill="white"/>
<path d="M696,224 L712,224 M704,216 L704,232 " stroke="black"/>
<path d="M704,298 L695.34,283 L712.66,283 Z" stroke="black" fill="gray"/>
<path d="M704,32 L704,96 " stroke="black"/>
<circle cx="704" cy="32" r="8" stroke="black" fill="white"/>
<path d="M696,32 L712,32 M704,24 L704,40 " stroke="black"/>
<circle cx="704" cy="96" r="8" stroke="none" fill="black"/>
<path d="M768,160 L768,224 " stroke="black"/>
<path d="M768,170 L759.34,155 L776.66,155 Z" stroke="black" fill="gray"/>
<circle cx="768" cy="224" r="8" stroke="black" fill="white"/>
<path d="M760,224 L776,224 M768,216 L768,232 " stroke="black"/>
<path d="M768,288 L768,352 " stroke="black"/>
<path d="M768,298 L759.34,283 L776.66,283 Z" stroke="black" fill="gray"/>
<path d="M768,362 L759.34,347 L776.66,347 Z" stroke="black" fill="gray"/>
<path d="M768,32 L768,96 " stroke="black"/>
<path d="M768,42 L759.34,27 L776.66,27 Z" stroke="black" fill="gray"/>
<circle cx="768" cy="96" r="8" stroke="none" fill="black"/>
<path d="M832,160 L832,224 " stroke="black"/>
<circle cx="832" cy="160" r="8" stroke="none" fill="black"/>
<circle cx="832" cy="224" r="8" stroke="black" fill="white"/>
<path d="M824,224 L840,224 M832,216 L832,232 " stroke="black"/>
<path d="M832,288 L832,352 " stroke="black"/>
<circle cx="832" cy="288" r="8" stroke="none" fill="black"/>
<path d="M832,362 L823.34,347 L840.66,347 Z" stroke="black" fill="gray"/>
<path d="M896,32 L896,352 " stroke="black"/>
<circle cx="896" cy="32" r="8" stroke="none" fill="black"/>
<circle cx="896" cy="352" r="8" stroke="none" fill="black"/>
<path d="M960,160 L960,224 " stroke="black"/>
<circle cx="960" cy="160" r="8" stroke="none" fill="black"/>
<circle cx="960" cy="224" r="8" stroke="none" fill="black"/>
<path d="M1024,96 L1024,288 " stroke="black"/>
<circle cx="1024" cy="96" r="8" stroke="none" fill="black"/>
<circle cx="1024" cy="288" r="8" stroke="none" fill="black"/>
</svg>
)DIAGRAM");
}

TEST(circuit_diagram_timeline_svg, noise_gates) {
    Circuit circuit(R"CIRCUIT(
        DEPOLARIZE1(0.125) 0 1
        DEPOLARIZE2(0.125) 0 2 4 5
        X_ERROR(0.125) 0 1 2
        Y_ERROR(0.125) 0 1 4
        Z_ERROR(0.125) 2 3 5
    )CIRCUIT");
    ASSERT_EQ("\n" + svg_diagram(circuit), u8R"DIAGRAM(
<svg width="320" height="416" version="1.1" xmlns="http://www.w3.org/2000/svg">
<path d="M32,32 L320,32 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="32">q0</text>
<path d="M32,96 L320,96 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="96">q1</text>
<path d="M32,160 L320,160 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="160">q2</text>
<path d="M32,224 L320,224 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="224">q3</text>
<path d="M32,288 L320,288 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="288">q4</text>
<path d="M32,352 L320,352 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="352">q5</text>
<rect x="48" y="16" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="64" y="32">DEP<tspan baseline-shift="sub" font-size="10">1</tspan></text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="64" y="52">0.125</text>
<rect x="48" y="80" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="64" y="96">DEP<tspan baseline-shift="sub" font-size="10">1</tspan></text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="64" y="116">0.125</text>
<path d="M128,32 L128,160 " stroke="black"/>
<rect x="112" y="16" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="128" y="32">DEP<tspan baseline-shift="sub" font-size="10">2</tspan></text>
<rect x="112" y="144" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="128" y="160">DEP<tspan baseline-shift="sub" font-size="10">2</tspan></text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="128" y="180">0.125</text>
<path d="M128,288 L128,352 " stroke="black"/>
<rect x="112" y="272" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="128" y="288">DEP<tspan baseline-shift="sub" font-size="10">2</tspan></text>
<rect x="112" y="336" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="128" y="352">DEP<tspan baseline-shift="sub" font-size="10">2</tspan></text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="128" y="372">0.125</text>
<rect x="176" y="16" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="192" y="32">ERR<tspan baseline-shift="sub" font-size="10">X</tspan></text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="192" y="52">0.125</text>
<rect x="176" y="80" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="192" y="96">ERR<tspan baseline-shift="sub" font-size="10">X</tspan></text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="192" y="116">0.125</text>
<rect x="176" y="144" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="192" y="160">ERR<tspan baseline-shift="sub" font-size="10">X</tspan></text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="192" y="180">0.125</text>
<rect x="240" y="16" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="256" y="32">ERR<tspan baseline-shift="sub" font-size="10">Y</tspan></text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="256" y="52">0.125</text>
<rect x="240" y="80" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="256" y="96">ERR<tspan baseline-shift="sub" font-size="10">Y</tspan></text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="256" y="116">0.125</text>
<rect x="240" y="272" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="256" y="288">ERR<tspan baseline-shift="sub" font-size="10">Y</tspan></text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="256" y="308">0.125</text>
<rect x="240" y="144" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="256" y="160">ERR<tspan baseline-shift="sub" font-size="10">Z</tspan></text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="256" y="180">0.125</text>
<rect x="240" y="208" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="256" y="224">ERR<tspan baseline-shift="sub" font-size="10">Z</tspan></text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="256" y="244">0.125</text>
<rect x="240" y="336" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="256" y="352">ERR<tspan baseline-shift="sub" font-size="10">Z</tspan></text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="256" y="372">0.125</text>
</svg>
)DIAGRAM");

    circuit = Circuit(R"CIRCUIT(
        E(0.25) X1 X2
        CORRELATED_ERROR(0.125) X1 Y2 Z3
        ELSE_CORRELATED_ERROR(0.25) X2 Y4 Z3
        ELSE_CORRELATED_ERROR(0.25) X5
    )CIRCUIT");
    ASSERT_EQ("\n" + svg_diagram(circuit), u8R"DIAGRAM(
<svg width="320" height="416" version="1.1" xmlns="http://www.w3.org/2000/svg">
<path d="M32,32 L320,32 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="32">q0</text>
<path d="M32,96 L320,96 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="96">q1</text>
<path d="M32,160 L320,160 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="160">q2</text>
<path d="M32,224 L320,224 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="224">q3</text>
<path d="M32,288 L320,288 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="288">q4</text>
<path d="M32,352 L320,352 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="352">q5</text>
<path d="M64,96 L64,160 " stroke="black"/>
<rect x="48" y="80" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="64" y="96">E<tspan baseline-shift="sub" font-size="10">X</tspan></text>
<rect x="48" y="144" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="64" y="160">E<tspan baseline-shift="sub" font-size="10">X</tspan></text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="64" y="180">0.25</text>
<path d="M128,96 L128,224 " stroke="black"/>
<rect x="112" y="80" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="128" y="96">E<tspan baseline-shift="sub" font-size="10">X</tspan></text>
<rect x="112" y="144" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="128" y="160">E<tspan baseline-shift="sub" font-size="10">Y</tspan></text>
<rect x="112" y="208" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="128" y="224">E<tspan baseline-shift="sub" font-size="10">Z</tspan></text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="128" y="244">0.125</text>
<path d="M192,160 L192,288 " stroke="black"/>
<rect x="176" y="144" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="192" y="160">EE<tspan baseline-shift="sub" font-size="10">X</tspan></text>
<rect x="176" y="272" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="192" y="288">EE<tspan baseline-shift="sub" font-size="10">Y</tspan></text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="192" y="308">0.25</text>
<rect x="176" y="208" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="192" y="224">EE<tspan baseline-shift="sub" font-size="10">Z</tspan></text>
<rect x="240" y="336" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="256" y="352">EE<tspan baseline-shift="sub" font-size="10">X</tspan></text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="256" y="372">0.25</text>
</svg>
)DIAGRAM");

    circuit = Circuit(R"CIRCUIT(
        PAULI_CHANNEL_1(0.125,0.25,0.125) 0 1 2 3
        PAULI_CHANNEL_2(0.01,0.01,0.01,0.02,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01) 0 1 2 4
    )CIRCUIT");
    ASSERT_EQ("\n" + svg_diagram(circuit), u8R"DIAGRAM(
<svg width="1344" height="352" version="1.1" xmlns="http://www.w3.org/2000/svg">
<path d="M32,32 L1344,32 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="32">q0</text>
<path d="M32,96 L1344,96 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="96">q1</text>
<path d="M32,160 L1344,160 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="160">q2</text>
<path d="M32,224 L1344,224 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="224">q3</text>
<path d="M32,288 L1344,288 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="288">q4</text>
<rect x="48" y="16" width="224" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="160" y="32">PAULI_CHANNEL_1</text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="160" y="52">0.125,0.25,0.125</text>
<rect x="48" y="80" width="224" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="160" y="96">PAULI_CHANNEL_1</text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="160" y="116">0.125,0.25,0.125</text>
<rect x="48" y="144" width="224" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="160" y="160">PAULI_CHANNEL_1</text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="160" y="180">0.125,0.25,0.125</text>
<rect x="48" y="208" width="224" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="160" y="224">PAULI_CHANNEL_1</text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="160" y="244">0.125,0.25,0.125</text>
<path d="M320,32 L320,96 " stroke="black"/>
<rect x="304" y="16" width="992" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="800" y="32">PAULI_CHANNEL_2<tspan baseline-shift="sub" font-size="10">0</tspan></text>
<rect x="304" y="80" width="992" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="800" y="96">PAULI_CHANNEL_2<tspan baseline-shift="sub" font-size="10">1</tspan></text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="800" y="116">0.01,0.01,0.01,0.02,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01</text>
<path d="M320,160 L320,288 " stroke="black"/>
<rect x="304" y="144" width="992" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="800" y="160">PAULI_CHANNEL_2<tspan baseline-shift="sub" font-size="10">0</tspan></text>
<rect x="304" y="272" width="992" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="800" y="288">PAULI_CHANNEL_2<tspan baseline-shift="sub" font-size="10">1</tspan></text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="800" y="308">0.01,0.01,0.01,0.02,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01</text>
</svg>
)DIAGRAM");
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
    ASSERT_EQ("\n" + svg_diagram(circuit), u8R"DIAGRAM(
<svg width="576" height="288" version="1.1" xmlns="http://www.w3.org/2000/svg">
<path d="M32,32 L576,32 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="32">q0</text>
<path d="M32,96 L576,96 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="96">q1</text>
<path d="M32,160 L576,160 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="160">q2</text>
<path d="M32,224 L576,224 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="224">q3</text>
<rect x="48" y="16" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="64" y="32">R</text>
<rect x="48" y="80" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="64" y="96">R<tspan baseline-shift="sub" font-size="10">X</tspan></text>
<rect x="48" y="144" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="64" y="160">R<tspan baseline-shift="sub" font-size="10">Y</tspan></text>
<rect x="48" y="208" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="64" y="224">R</text>
<rect x="112" y="16" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="128" y="32">M</text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="128" y="52">0.001</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="128" y="12">rec[0]</text>
<rect x="112" y="80" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="128" y="96">M</text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="128" y="116">0.001</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="128" y="76">rec[1]</text>
<rect x="176" y="80" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="192" y="96">MR</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="192" y="76">rec[2]</text>
<rect x="176" y="16" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="192" y="32">MR</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="192" y="12">rec[3]</text>
<rect x="240" y="80" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="256" y="96">MR<tspan baseline-shift="sub" font-size="10">X</tspan></text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="256" y="76">rec[4]</text>
<rect x="240" y="144" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="256" y="160">MR<tspan baseline-shift="sub" font-size="10">X</tspan></text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="256" y="140">rec[5]</text>
<rect x="240" y="16" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="256" y="32">MR<tspan baseline-shift="sub" font-size="10">Y</tspan></text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="256" y="12">rec[6]</text>
<rect x="240" y="208" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="256" y="224">MR<tspan baseline-shift="sub" font-size="10">Y</tspan></text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="256" y="204">rec[7]</text>
<rect x="304" y="80" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="320" y="96">MR<tspan baseline-shift="sub" font-size="10">Y</tspan></text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="320" y="76">rec[8]</text>
<rect x="304" y="16" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="320" y="32">MR</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="320" y="12">rec[9]</text>
<rect x="368" y="80" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="384" y="96">M<tspan baseline-shift="sub" font-size="10">X</tspan></text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="384" y="76">rec[10]</text>
<rect x="368" y="144" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="384" y="160">M<tspan baseline-shift="sub" font-size="10">Y</tspan></text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="384" y="140">rec[11]</text>
<rect x="368" y="208" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="384" y="224">M</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="384" y="204">rec[12]</text>
<path d="M448,32 L448,160 " stroke="black"/>
<rect x="432" y="16" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="448" y="32">MPP<tspan baseline-shift="sub" font-size="10">X</tspan></text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="448" y="12">rec[13]</text>
<rect x="432" y="144" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="448" y="160">MPP<tspan baseline-shift="sub" font-size="10">Y</tspan></text>
<rect x="432" y="208" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="448" y="224">MPP<tspan baseline-shift="sub" font-size="10">Z</tspan></text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="448" y="204">rec[14]</text>
<rect x="496" y="80" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="512" y="96">MPP<tspan baseline-shift="sub" font-size="10">X</tspan></text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="512" y="76">rec[15]</text>
<path d="M512,160 L512,224 " stroke="black"/>
<rect x="496" y="144" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="512" y="160">MPP<tspan baseline-shift="sub" font-size="10">Z</tspan></text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="512" y="140">rec[16]</text>
<rect x="496" y="208" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="512" y="224">MPP<tspan baseline-shift="sub" font-size="10">Y</tspan></text>
</svg>
)DIAGRAM");
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
    ASSERT_EQ("\n" + svg_diagram(circuit), u8R"DIAGRAM(
<svg width="704" height="352" version="1.1" xmlns="http://www.w3.org/2000/svg">
<path d="M32,32 L704,32 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="32">q0</text>
<path d="M32,96 L704,96 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="96">q1</text>
<path d="M32,160 L704,160 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="160">q2</text>
<path d="M32,224 L704,224 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="224">q3</text>
<path d="M32,288 L704,288 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="288">q4</text>
<rect x="48" y="16" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="64" y="32">M</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="64" y="12">rec[0]</text>
<path d="M136,0 L128,0 L128,352 L136,352 " stroke="black" fill="none"/>
<text dominant-baseline="auto" text-anchor="start" font-family="monospace" font-size="12" x="132" y="348">REP100</text>
<rect x="176" y="80" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="192" y="96">M</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="192" y="76">rec[1+iter*13]</text>
<path d="M264,4 L256,4 L256,348 L264,348 " stroke="black" fill="none"/>
<text dominant-baseline="auto" text-anchor="start" font-family="monospace" font-size="12" x="260" y="344">REP5</text>
<rect x="304" y="144" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="320" y="160">M</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="320" y="140">rec[2+iter*13+iter2]</text>
<path d="M376,4 L384,4 L384,348 L376,348 " stroke="black" fill="none"/>
<path d="M456,4 L448,4 L448,348 L456,348 " stroke="black" fill="none"/>
<text dominant-baseline="auto" text-anchor="start" font-family="monospace" font-size="12" x="452" y="344">REP7</text>
<path d="M512,224 L512,288 " stroke="black"/>
<rect x="496" y="208" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="512" y="224">MPP<tspan baseline-shift="sub" font-size="10">X</tspan></text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="512" y="204">rec[7+iter*13+iter2]</text>
<rect x="496" y="272" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="512" y="288">MPP<tspan baseline-shift="sub" font-size="10">Y</tspan></text>
<path d="M568,4 L576,4 L576,348 L568,348 " stroke="black" fill="none"/>
<path d="M632,0 L640,0 L640,352 L632,352 " stroke="black" fill="none"/>
</svg>
)DIAGRAM");
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
    ASSERT_EQ("\n" + svg_diagram(circuit), u8R"DIAGRAM(
<svg width="576" height="288" version="1.1" xmlns="http://www.w3.org/2000/svg">
<path d="M32,32 L576,32 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="32">q0</text>
<path d="M32,96 L576,96 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="96">q1</text>
<path d="M32,160 L576,160 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="160">q2</text>
<path d="M32,224 L576,224 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="224">q3</text>
<rect x="48" y="16" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="64" y="32">H</text>
<rect x="48" y="80" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="64" y="96">H</text>
<rect x="48" y="144" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="64" y="160">H</text>
<path d="M136,0 L128,0 L128,288 L136,288 " stroke="black" fill="none"/>
<text dominant-baseline="auto" text-anchor="start" font-family="monospace" font-size="12" x="132" y="284">REP5</text>
<rect x="176" y="144" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="192" y="160">R<tspan baseline-shift="sub" font-size="10">X</tspan></text>
<path d="M264,4 L256,4 L256,284 L264,284 " stroke="black" fill="none"/>
<text dominant-baseline="auto" text-anchor="start" font-family="monospace" font-size="12" x="260" y="280">REP100</text>
<rect x="304" y="16" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="320" y="32">H</text>
<rect x="304" y="80" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="320" y="96">H</text>
<rect x="304" y="208" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="320" y="224">H</text>
<rect x="368" y="208" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="384" y="224">H</text>
<path d="M440,4 L448,4 L448,284 L440,284 " stroke="black" fill="none"/>
<path d="M504,0 L512,0 L512,288 L504,288 " stroke="black" fill="none"/>
</svg>
)DIAGRAM");
}

TEST(circuit_diagram_timeline_svg, classical_feedback) {
    auto circuit = Circuit(R"CIRCUIT(
        M 0
        CX rec[-1] 1
        YCZ 2 sweep[5]
    )CIRCUIT");
    ASSERT_EQ("\n" + svg_diagram(circuit), u8R"DIAGRAM(
<svg width="192" height="224" version="1.1" xmlns="http://www.w3.org/2000/svg">
<path d="M32,32 L192,32 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="32">q0</text>
<path d="M32,96 L192,96 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="96">q1</text>
<path d="M32,160 L192,160 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="160">q2</text>
<rect x="48" y="16" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="64" y="32">M</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="64" y="12">rec[0]</text>
<rect x="48" y="80" width="96" height="32" stroke="black" fill="lightgray"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="96" y="96">X<tspan baseline-shift="super" font-size="10">rec[0]</tspan></text>
<rect x="48" y="144" width="96" height="32" stroke="black" fill="lightgray"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="96" y="160">Y<tspan baseline-shift="super" font-size="10">sweep[5]</tspan></text>
</svg>
)DIAGRAM");
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
    ASSERT_EQ("\n" + svg_diagram(circuit), u8R"DIAGRAM(
<svg width="512" height="224" version="1.1" xmlns="http://www.w3.org/2000/svg">
<path d="M32,32 L512,32 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="32">q0</text>
<path d="M32,96 L512,96 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="96">q1</text>
<path d="M32,160 L512,160 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="160">q2</text>
<rect x="48" y="144" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="64" y="160">R</text>
<path d="M128,96 L128,160 " stroke="black"/>
<rect x="112" y="80" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="128" y="96">MPP<tspan baseline-shift="sub" font-size="10">X</tspan></text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="128" y="76">rec[0]</text>
<rect x="112" y="144" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="128" y="160">MPP<tspan baseline-shift="sub" font-size="10">X</tspan></text>
<path d="M192,32 L192,160 " stroke="black"/>
<rect x="176" y="16" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="192" y="32">MPP<tspan baseline-shift="sub" font-size="10">Z</tspan></text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="192" y="12">rec[1]</text>
<rect x="176" y="144" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="192" y="160">MPP<tspan baseline-shift="sub" font-size="10">Z</tspan></text>
<rect x="240" y="144" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="256" y="160">M<tspan baseline-shift="sub" font-size="10">X</tspan></text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="256" y="140">rec[2]</text>
<rect x="240" y="16" width="96" height="32" stroke="black" fill="lightgray"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="288" y="32">Z<tspan baseline-shift="super" font-size="10">rec[0]</tspan></text>
<rect x="240" y="80" width="96" height="32" stroke="black" fill="lightgray"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="288" y="96">X<tspan baseline-shift="super" font-size="10">rec[1]</tspan></text>
<rect x="368" y="16" width="96" height="32" stroke="black" fill="lightgray"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="416" y="32">Z<tspan baseline-shift="super" font-size="10">rec[2]</tspan></text>
</svg>
)DIAGRAM");
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
    ASSERT_EQ(u8"\n" + svg_diagram(circuit), u8R"DIAGRAM(
<svg width="960" height="160" version="1.1" xmlns="http://www.w3.org/2000/svg">
<path d="M32,32 L960,32 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="32">q0</text>
<path d="M32,96 L960,96 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="96">q1</text>
<rect x="48" y="16" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="64" y="32">H</text>
<rect x="112" y="16" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="128" y="32">H</text>
<path d="M35,8 L35,0 L156,0 L156,8 " stroke="black" fill="none"/>
<path d="M35,152 L35,160 L156,160 L156,152 " stroke="black" fill="none"/>
<rect x="176" y="16" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="192" y="32">H</text>
<rect x="176" y="80" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="192" y="96">H</text>
<rect x="240" y="16" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="256" y="32">H</text>
<path d="M328,0 L320,0 L320,160 L328,160 " stroke="black" fill="none"/>
<text dominant-baseline="auto" text-anchor="start" font-family="monospace" font-size="12" x="324" y="156">REP1</text>
<rect x="368" y="16" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="384" y="32">H</text>
<rect x="368" y="80" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="384" y="96">H</text>
<rect x="432" y="16" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="448" y="32">H</text>
<rect x="496" y="16" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="512" y="32">S</text>
<path d="M419,8 L419,0 L540,0 L540,8 " stroke="black" fill="none"/>
<path d="M419,152 L419,160 L540,160 L540,152 " stroke="black" fill="none"/>
<path d="M568,0 L576,0 L576,160 L568,160 " stroke="black" fill="none"/>
<rect x="624" y="16" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="640" y="32">H</text>
<rect x="688" y="16" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="704" y="32">H</text>
<rect x="752" y="16" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="768" y="32">√X</text>
<path d="M611,8 L611,0 L796,0 L796,8 " stroke="black" fill="none"/>
<path d="M611,152 L611,160 L796,160 L796,152 " stroke="black" fill="none"/>
<rect x="816" y="16" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="832" y="32">H</text>
<rect x="880" y="16" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="896" y="32">H</text>
</svg>
)DIAGRAM");
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
    ASSERT_EQ("\n" + svg_diagram(circuit), u8R"DIAGRAM(
<svg width="1216" height="416" version="1.1" xmlns="http://www.w3.org/2000/svg">
<path d="M32,32 L1216,32 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="32">q0</text>
<path d="M32,96 L1216,96 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="96">q1</text>
<path d="M32,160 L1216,160 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="160">q2</text>
<path d="M32,224 L1216,224 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="224">q3</text>
<path d="M32,288 L1216,288 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="288">q4</text>
<path d="M32,352 L1216,352 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="352">q5</text>
<rect x="48" y="80" width="224" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="160" y="96">COORDS(1,2)</text>
<rect x="48" y="16" width="224" height="32" stroke="black" fill="lightgray"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="160" y="32">DETECTOR</text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="8" x="160" y="52">coords=(4,5,6)</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="160" y="12">D0 = 1 (vacuous)</text>
<rect x="48" y="144" width="224" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="160" y="160">COORDS(11,22)</text>
<rect x="304" y="16" width="224" height="32" stroke="black" fill="lightgray"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="416" y="32">DETECTOR</text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="8" x="416" y="52">coords=(14,25,36)</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="416" y="12">D1 = 1 (vacuous)</text>
<path d="M584,0 L576,0 L576,416 L584,416 " stroke="black" fill="none"/>
<text dominant-baseline="auto" text-anchor="start" font-family="monospace" font-size="12" x="580" y="412">REP100</text>
<rect x="624" y="208" width="224" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="736" y="224">COORDS(17,28+iter*200)</text>
<rect x="624" y="272" width="224" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="736" y="288">COORDS(17,28+iter*200)</text>
<rect x="624" y="16" width="224" height="32" stroke="black" fill="lightgray"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="736" y="32">DETECTOR</text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="8" x="736" y="52">coords=(19,30+iter*200,41+iter*300)</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="736" y="12">D[2+iter] = 1 (vacuous)</text>
<path d="M888,0 L896,0 L896,416 L888,416 " stroke="black" fill="none"/>
<rect x="944" y="336" width="224" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="1056" y="352">COORDS(11,20022)</text>
<rect x="944" y="16" width="224" height="32" stroke="black" fill="lightgray"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="1056" y="32">DETECTOR</text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="8" x="1056" y="52">coords=(14,20025,30036)</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="1056" y="12">D102 = 1 (vacuous)</text>
</svg>
)DIAGRAM");
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
    ASSERT_EQ("\n" + svg_diagram(circuit), u8R"DIAGRAM(
<svg width="960" height="416" version="1.1" xmlns="http://www.w3.org/2000/svg">
<path d="M32,32 L960,32 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="32">q0</text>
<path d="M32,96 L960,96 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="96">q1</text>
<path d="M32,160 L960,160 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="160">q2</text>
<path d="M32,224 L960,224 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="224">q3</text>
<path d="M32,288 L960,288 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="288">q4</text>
<path d="M32,352 L960,352 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="352">q5</text>
<rect x="48" y="16" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="64" y="32">M</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="64" y="12">rec[0]</text>
<rect x="48" y="80" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="64" y="96">M</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="64" y="76">rec[1]</text>
<rect x="48" y="144" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="64" y="160">M</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="64" y="140">rec[2]</text>
<rect x="48" y="208" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="64" y="224">M</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="64" y="204">rec[3]</text>
<rect x="48" y="272" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="64" y="288">M</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="64" y="268">rec[4]</text>
<rect x="48" y="336" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="64" y="352">M</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="64" y="332">rec[5]</text>
<path d="M136,0 L128,0 L128,416 L136,416 " stroke="black" fill="none"/>
<text dominant-baseline="auto" text-anchor="start" font-family="monospace" font-size="12" x="132" y="412">REP100</text>
<rect x="176" y="80" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="192" y="96">M</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="192" y="76">rec[6+iter*2]</text>
<rect x="176" y="144" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="192" y="160">M</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="192" y="140">rec[7+iter*2]</text>
<path d="M248,0 L256,0 L256,416 L248,416 " stroke="black" fill="none"/>
<rect x="304" y="144" width="160" height="32" stroke="black" fill="lightgray"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="384" y="160">DETECTOR</text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="8" x="384" y="180">coords=(1)</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="384" y="140">D0 = rec[205]</text>
<rect x="304" y="80" width="160" height="32" stroke="black" fill="lightgray"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="384" y="96">DETECTOR</text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="8" x="384" y="116">coords=(2)</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="384" y="76">D1 = rec[204]</text>
<rect x="496" y="144" width="160" height="32" stroke="black" fill="lightgray"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="576" y="160">DETECTOR</text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="8" x="576" y="180">coords=(3)</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="576" y="140">D2 = rec[203]</text>
<rect x="496" y="80" width="160" height="32" stroke="black" fill="lightgray"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="576" y="96">DETECTOR</text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="8" x="576" y="116">coords=(4)</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="576" y="76">D3 = rec[202]</text>
<rect x="688" y="80" width="224" height="32" stroke="black" fill="lightgray"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="800" y="96">DETECTOR</text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="8" x="800" y="116">coords=(5)</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="800" y="76">D4 = rec[205]*rec[204]</text>
<rect x="688" y="208" width="224" height="32" stroke="black" fill="lightgray"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="800" y="224">OBS_INCLUDE(100)</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="800" y="204">L100 *= rec[5]*rec[3]</text>
</svg>
)DIAGRAM");
}

TEST(circuit_diagram_timeline_svg, repetition_code) {
    CircuitGenParameters params(10, 3, "memory");
    auto circuit = generate_rep_code_circuit(params).circuit;
    ASSERT_EQ("\n" + svg_diagram(circuit), u8R"DIAGRAM(
<svg width="1536" height="352" version="1.1" xmlns="http://www.w3.org/2000/svg">
<path d="M32,32 L1536,32 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="32">q0</text>
<path d="M32,96 L1536,96 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="96">q1</text>
<path d="M32,160 L1536,160 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="160">q2</text>
<path d="M32,224 L1536,224 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="224">q3</text>
<path d="M32,288 L1536,288 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="32" y="288">q4</text>
<rect x="48" y="16" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="64" y="32">R</text>
<rect x="48" y="80" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="64" y="96">R</text>
<rect x="48" y="144" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="64" y="160">R</text>
<rect x="48" y="208" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="64" y="224">R</text>
<rect x="48" y="272" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="64" y="288">R</text>
<path d="M128,32 L128,96 " stroke="black"/>
<circle cx="128" cy="32" r="8" stroke="none" fill="black"/>
<circle cx="128" cy="96" r="8" stroke="black" fill="white"/>
<path d="M120,96 L136,96 M128,88 L128,104 " stroke="black"/>
<path d="M128,160 L128,224 " stroke="black"/>
<circle cx="128" cy="160" r="8" stroke="none" fill="black"/>
<circle cx="128" cy="224" r="8" stroke="black" fill="white"/>
<path d="M120,224 L136,224 M128,216 L128,232 " stroke="black"/>
<path d="M192,96 L192,160 " stroke="black"/>
<circle cx="192" cy="160" r="8" stroke="none" fill="black"/>
<circle cx="192" cy="96" r="8" stroke="black" fill="white"/>
<path d="M184,96 L200,96 M192,88 L192,104 " stroke="black"/>
<path d="M192,224 L192,288 " stroke="black"/>
<circle cx="192" cy="288" r="8" stroke="none" fill="black"/>
<circle cx="192" cy="224" r="8" stroke="black" fill="white"/>
<path d="M184,224 L200,224 M192,216 L192,232 " stroke="black"/>
<rect x="240" y="80" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="256" y="96">MR</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="256" y="76">rec[0]</text>
<rect x="240" y="208" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="256" y="224">MR</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="256" y="204">rec[1]</text>
<rect x="304" y="80" width="160" height="32" stroke="black" fill="lightgray"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="384" y="96">DETECTOR</text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="8" x="384" y="116">coords=(1,0)</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="384" y="76">D0 = rec[0]</text>
<rect x="304" y="208" width="160" height="32" stroke="black" fill="lightgray"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="384" y="224">DETECTOR</text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="8" x="384" y="244">coords=(3,0)</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="384" y="204">D1 = rec[1]</text>
<path d="M227,8 L227,0 L476,0 L476,8 " stroke="black" fill="none"/>
<path d="M227,344 L227,352 L476,352 L476,344 " stroke="black" fill="none"/>
<path d="M520,0 L512,0 L512,352 L520,352 " stroke="black" fill="none"/>
<text dominant-baseline="auto" text-anchor="start" font-family="monospace" font-size="12" x="516" y="348">REP9</text>
<path d="M640,32 L640,96 " stroke="black"/>
<circle cx="640" cy="32" r="8" stroke="none" fill="black"/>
<circle cx="640" cy="96" r="8" stroke="black" fill="white"/>
<path d="M632,96 L648,96 M640,88 L640,104 " stroke="black"/>
<path d="M640,160 L640,224 " stroke="black"/>
<circle cx="640" cy="160" r="8" stroke="none" fill="black"/>
<circle cx="640" cy="224" r="8" stroke="black" fill="white"/>
<path d="M632,224 L648,224 M640,216 L640,232 " stroke="black"/>
<path d="M704,96 L704,160 " stroke="black"/>
<circle cx="704" cy="160" r="8" stroke="none" fill="black"/>
<circle cx="704" cy="96" r="8" stroke="black" fill="white"/>
<path d="M696,96 L712,96 M704,88 L704,104 " stroke="black"/>
<path d="M704,224 L704,288 " stroke="black"/>
<circle cx="704" cy="288" r="8" stroke="none" fill="black"/>
<circle cx="704" cy="224" r="8" stroke="black" fill="white"/>
<path d="M696,224 L712,224 M704,216 L704,232 " stroke="black"/>
<rect x="752" y="80" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="768" y="96">MR</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="768" y="76">rec[2+iter*2]</text>
<rect x="752" y="208" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="768" y="224">MR</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="768" y="204">rec[3+iter*2]</text>
<rect x="816" y="80" width="224" height="32" stroke="black" fill="lightgray"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="928" y="96">DETECTOR</text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="8" x="928" y="116">coords=(1,1+iter)</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="928" y="76">D[2+iter*2] = rec[2+iter*2]*rec[0+iter*2]</text>
<rect x="816" y="208" width="224" height="32" stroke="black" fill="lightgray"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="928" y="224">DETECTOR</text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="8" x="928" y="244">coords=(3,1+iter)</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="928" y="204">D[3+iter*2] = rec[3+iter*2]*rec[1+iter*2]</text>
<path d="M739,8 L739,0 L1052,0 L1052,8 " stroke="black" fill="none"/>
<path d="M739,344 L739,352 L1052,352 L1052,344 " stroke="black" fill="none"/>
<path d="M1080,0 L1088,0 L1088,352 L1080,352 " stroke="black" fill="none"/>
<rect x="1136" y="16" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="1152" y="32">M</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="1152" y="12">rec[20]</text>
<rect x="1136" y="144" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="1152" y="160">M</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="1152" y="140">rec[21]</text>
<rect x="1136" y="272" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="1152" y="288">M</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="1152" y="268">rec[22]</text>
<rect x="1200" y="16" width="288" height="32" stroke="black" fill="lightgray"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="1344" y="32">DETECTOR</text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="8" x="1344" y="52">coords=(1,10)</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="1344" y="12">D20 = rec[21]*rec[20]*rec[18]</text>
<rect x="1200" y="144" width="288" height="32" stroke="black" fill="lightgray"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="1344" y="160">DETECTOR</text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="8" x="1344" y="180">coords=(3,10)</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="1344" y="140">D21 = rec[22]*rec[21]*rec[19]</text>
<rect x="1200" y="272" width="160" height="32" stroke="black" fill="lightgray"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="1280" y="288">OBS_INCLUDE(0)</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="1280" y="268">L0 *= rec[22]</text>
</svg>
)DIAGRAM");
}
