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

#include "stim/gen/circuit_gen_params.h"
#include "stim/gen/gen_rep_code.h"

using namespace stim;
using namespace stim_draw_internal;

std::string svg_diagram(const Circuit &circuit) {
    std::stringstream ss;
    ss << '\n';
    DiagramTimelineSvgDrawer::make_diagram_write_to(circuit, ss);
    ss << '\n';
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
    ASSERT_EQ(
        svg_diagram(circuit),
        u8R"DIAGRAM(
<svg viewBox="0 0 512 352"  version="1.1" xmlns="http://www.w3.org/2000/svg">
<path d="M64,64 L480,64 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="64">q0</text>
<path d="M64,128 L480,128 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="128">q1</text>
<path d="M64,192 L480,192 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="192">q2</text>
<path d="M64,256 L480,256 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="256">q3</text>
<rect x="80" y="48" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="96" y="64">I</text>
<rect x="80" y="112" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="96" y="128">X</text>
<rect x="80" y="176" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="96" y="192">Y</text>
<rect x="80" y="240" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="96" y="256">Z</text>
<rect x="144" y="48" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="160" y="64">C<tspan baseline-shift="sub" font-size="10">XYZ</tspan></text>
<rect x="144" y="112" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="160" y="128">C<tspan baseline-shift="sub" font-size="10">ZYX</tspan></text>
<rect x="144" y="176" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="160" y="192">H</text>
<rect x="144" y="240" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="160" y="256">H<tspan baseline-shift="sub" font-size="10">XY</tspan></text>
<rect x="208" y="48" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="224" y="64">H</text>
<rect x="208" y="112" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="224" y="128">H<tspan baseline-shift="sub" font-size="10">YZ</tspan></text>
<rect x="208" y="176" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="224" y="192">S</text>
<rect x="208" y="240" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="224" y="256">√X</text>
<rect x="272" y="48" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="288" y="64">√X<tspan baseline-shift="super" font-size="10">†</tspan></text>
<rect x="272" y="112" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="288" y="128">√Y</text>
<rect x="272" y="176" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="288" y="192">√Y<tspan baseline-shift="super" font-size="10">†</tspan></text>
<rect x="272" y="240" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="288" y="256">S</text>
<rect x="336" y="48" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="352" y="64">S<tspan baseline-shift="super" font-size="10">†</tspan></text>
<rect x="336" y="112" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="352" y="128">S<tspan baseline-shift="super" font-size="10">†</tspan></text>
<rect x="336" y="176" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="352" y="192">H</text>
<rect x="400" y="48" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="416" y="64">H</text>
<rect x="400" y="240" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="416" y="256">H</text>
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
    ASSERT_EQ(
        svg_diagram(circuit),
        u8R"DIAGRAM(
<svg viewBox="0 0 1152 480"  version="1.1" xmlns="http://www.w3.org/2000/svg">
<path d="M64,64 L1120,64 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="64">q0</text>
<path d="M64,128 L1120,128 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="128">q1</text>
<path d="M64,192 L1120,192 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="192">q2</text>
<path d="M64,256 L1120,256 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="256">q3</text>
<path d="M64,320 L1120,320 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="320">q4</text>
<path d="M64,384 L1120,384 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="384">q5</text>
<path d="M96,64 L96,128 " stroke="black"/>
<circle cx="96" cy="64" r="8" stroke="none" fill="black"/>
<circle cx="96" cy="128" r="8" stroke="black" fill="white"/>
<path d="M88,128 L104,128 M96,120 L96,136 " stroke="black"/>
<path d="M96,192 L96,256 " stroke="black"/>
<circle cx="96" cy="192" r="8" stroke="none" fill="black"/>
<circle cx="96" cy="256" r="8" stroke="black" fill="white"/>
<path d="M88,256 L104,256 M96,248 L96,264 " stroke="black"/>
<path d="M96,320 L96,384 " stroke="black"/>
<circle cx="96" cy="320" r="8" stroke="none" fill="black"/>
<path d="M96,394 L87.3397,379 L104.66,379 Z" stroke="black" fill="gray"/>
<path d="M160,320 L160,384 " stroke="black"/>
<circle cx="160" cy="384" r="8" stroke="none" fill="black"/>
<path d="M160,330 L151.34,315 L168.66,315 Z" stroke="black" fill="gray"/>
<path d="M160,64 L160,192 " stroke="black"/>
<circle cx="160" cy="64" r="8" stroke="none" fill="black"/>
<circle cx="160" cy="192" r="8" stroke="none" fill="black"/>
<path d="M224,128 L224,256 " stroke="black"/>
<circle cx="224" cy="128" r="8" stroke="none" fill="gray"/>
<path d="M220,124 L228,132 M228,124 L220,132 " stroke="black"/>
<circle cx="224" cy="256" r="8" stroke="none" fill="gray"/>
<path d="M220,252 L228,260 M228,252 L220,260 " stroke="black"/>
<path d="M288,192 L288,320 " stroke="black"/>
<circle cx="288" cy="192" r="8" stroke="none" fill="gray"/>
<path d="M284,188 L292,196 M292,188 L284,196 " stroke="black"/>
<path d="M292,182 L300,182 M296,178 L296,190 " stroke="black"/>
<circle cx="288" cy="320" r="8" stroke="none" fill="gray"/>
<path d="M284,316 L292,324 M292,316 L284,324 " stroke="black"/>
<path d="M292,310 L300,310 M296,306 L296,318 " stroke="black"/>
<path d="M352,256 L352,384 " stroke="black"/>
<rect x="336" y="240" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="352" y="256">√XX</text>
<rect x="336" y="368" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="352" y="384">√XX</text>
<path d="M416,64 L416,384 " stroke="black"/>
<rect x="400" y="48" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="416" y="64">√XX<tspan baseline-shift="super" font-size="10">†</tspan></text>
<rect x="400" y="368" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="416" y="384">√XX<tspan baseline-shift="super" font-size="10">†</tspan></text>
<path d="M480,256 L480,320 " stroke="black"/>
<rect x="464" y="240" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="480" y="256">√YY</text>
<rect x="464" y="304" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="480" y="320">√YY</text>
<path d="M544,256 L544,320 " stroke="black"/>
<rect x="528" y="304" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="544" y="320">√YY</text>
<rect x="528" y="240" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="544" y="256">√YY</text>
<path d="M544,64 L544,128 " stroke="black"/>
<rect x="528" y="48" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="544" y="64">√YY<tspan baseline-shift="super" font-size="10">†</tspan></text>
<rect x="528" y="112" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="544" y="128">√YY<tspan baseline-shift="super" font-size="10">†</tspan></text>
<path d="M608,192 L608,256 " stroke="black"/>
<rect x="592" y="176" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="608" y="192">√ZZ</text>
<rect x="592" y="240" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="608" y="256">√ZZ</text>
<path d="M608,320 L608,384 " stroke="black"/>
<rect x="592" y="304" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="608" y="320">√ZZ<tspan baseline-shift="super" font-size="10">†</tspan></text>
<rect x="592" y="368" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="608" y="384">√ZZ<tspan baseline-shift="super" font-size="10">†</tspan></text>
<path d="M608,64 L608,128 " stroke="black"/>
<path d="M604,60 L612,68 M612,60 L604,68 " stroke="black"/>
<path d="M604,124 L612,132 M612,124 L604,132 " stroke="black"/>
<path d="M672,192 L672,256 " stroke="black"/>
<circle cx="672" cy="192" r="8" stroke="black" fill="white"/>
<path d="M664,192 L680,192 M672,184 L672,200 " stroke="black"/>
<circle cx="672" cy="256" r="8" stroke="black" fill="white"/>
<path d="M664,256 L680,256 M672,248 L672,264 " stroke="black"/>
<path d="M736,256 L736,320 " stroke="black"/>
<circle cx="736" cy="256" r="8" stroke="black" fill="white"/>
<path d="M728,256 L744,256 M736,248 L736,264 " stroke="black"/>
<path d="M736,330 L727.34,315 L744.66,315 Z" stroke="black" fill="gray"/>
<path d="M736,64 L736,128 " stroke="black"/>
<circle cx="736" cy="64" r="8" stroke="black" fill="white"/>
<path d="M728,64 L744,64 M736,56 L736,72 " stroke="black"/>
<circle cx="736" cy="128" r="8" stroke="none" fill="black"/>
<path d="M800,192 L800,256 " stroke="black"/>
<path d="M800,202 L791.34,187 L808.66,187 Z" stroke="black" fill="gray"/>
<circle cx="800" cy="256" r="8" stroke="black" fill="white"/>
<path d="M792,256 L808,256 M800,248 L800,264 " stroke="black"/>
<path d="M800,320 L800,384 " stroke="black"/>
<path d="M800,330 L791.34,315 L808.66,315 Z" stroke="black" fill="gray"/>
<path d="M800,394 L791.34,379 L808.66,379 Z" stroke="black" fill="gray"/>
<path d="M800,64 L800,128 " stroke="black"/>
<path d="M800,74 L791.34,59 L808.66,59 Z" stroke="black" fill="gray"/>
<circle cx="800" cy="128" r="8" stroke="none" fill="black"/>
<path d="M864,192 L864,256 " stroke="black"/>
<circle cx="864" cy="192" r="8" stroke="none" fill="black"/>
<circle cx="864" cy="256" r="8" stroke="black" fill="white"/>
<path d="M856,256 L872,256 M864,248 L864,264 " stroke="black"/>
<path d="M864,320 L864,384 " stroke="black"/>
<circle cx="864" cy="320" r="8" stroke="none" fill="black"/>
<path d="M864,394 L855.34,379 L872.66,379 Z" stroke="black" fill="gray"/>
<path d="M928,64 L928,384 " stroke="black"/>
<circle cx="928" cy="64" r="8" stroke="none" fill="black"/>
<circle cx="928" cy="384" r="8" stroke="none" fill="black"/>
<path d="M992,192 L992,256 " stroke="black"/>
<circle cx="992" cy="192" r="8" stroke="none" fill="black"/>
<circle cx="992" cy="256" r="8" stroke="none" fill="black"/>
<path d="M1056,128 L1056,320 " stroke="black"/>
<circle cx="1056" cy="128" r="8" stroke="none" fill="black"/>
<circle cx="1056" cy="320" r="8" stroke="none" fill="black"/>
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
    ASSERT_EQ(
        svg_diagram(circuit),
        u8R"DIAGRAM(
<svg viewBox="0 0 384 480"  version="1.1" xmlns="http://www.w3.org/2000/svg">
<path d="M64,64 L352,64 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="64">q0</text>
<path d="M64,128 L352,128 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="128">q1</text>
<path d="M64,192 L352,192 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="192">q2</text>
<path d="M64,256 L352,256 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="256">q3</text>
<path d="M64,320 L352,320 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="320">q4</text>
<path d="M64,384 L352,384 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="384">q5</text>
<rect x="80" y="48" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="96" y="64">DEP<tspan baseline-shift="sub" font-size="10">1</tspan></text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="96" y="84">0.125</text>
<rect x="80" y="112" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="96" y="128">DEP<tspan baseline-shift="sub" font-size="10">1</tspan></text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="96" y="148">0.125</text>
<path d="M160,64 L160,192 " stroke="black"/>
<rect x="144" y="48" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="160" y="64">DEP<tspan baseline-shift="sub" font-size="10">2</tspan></text>
<rect x="144" y="176" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="160" y="192">DEP<tspan baseline-shift="sub" font-size="10">2</tspan></text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="160" y="212">0.125</text>
<path d="M160,320 L160,384 " stroke="black"/>
<rect x="144" y="304" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="160" y="320">DEP<tspan baseline-shift="sub" font-size="10">2</tspan></text>
<rect x="144" y="368" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="160" y="384">DEP<tspan baseline-shift="sub" font-size="10">2</tspan></text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="160" y="404">0.125</text>
<rect x="208" y="48" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="224" y="64">ERR<tspan baseline-shift="sub" font-size="10">X</tspan></text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="224" y="84">0.125</text>
<rect x="208" y="112" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="224" y="128">ERR<tspan baseline-shift="sub" font-size="10">X</tspan></text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="224" y="148">0.125</text>
<rect x="208" y="176" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="224" y="192">ERR<tspan baseline-shift="sub" font-size="10">X</tspan></text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="224" y="212">0.125</text>
<rect x="272" y="48" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="288" y="64">ERR<tspan baseline-shift="sub" font-size="10">Y</tspan></text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="288" y="84">0.125</text>
<rect x="272" y="112" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="288" y="128">ERR<tspan baseline-shift="sub" font-size="10">Y</tspan></text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="288" y="148">0.125</text>
<rect x="272" y="304" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="288" y="320">ERR<tspan baseline-shift="sub" font-size="10">Y</tspan></text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="288" y="340">0.125</text>
<rect x="272" y="176" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="288" y="192">ERR<tspan baseline-shift="sub" font-size="10">Z</tspan></text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="288" y="212">0.125</text>
<rect x="272" y="240" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="288" y="256">ERR<tspan baseline-shift="sub" font-size="10">Z</tspan></text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="288" y="276">0.125</text>
<rect x="272" y="368" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="288" y="384">ERR<tspan baseline-shift="sub" font-size="10">Z</tspan></text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="288" y="404">0.125</text>
</svg>
)DIAGRAM");

    circuit = Circuit(R"CIRCUIT(
        E(0.25) X1 X2
        CORRELATED_ERROR(0.125) X1 Y2 Z3
        ELSE_CORRELATED_ERROR(0.25) X2 Y4 Z3
        ELSE_CORRELATED_ERROR(0.25) X5
    )CIRCUIT");
    ASSERT_EQ(
        svg_diagram(circuit),
        u8R"DIAGRAM(
<svg viewBox="0 0 384 480"  version="1.1" xmlns="http://www.w3.org/2000/svg">
<path d="M64,64 L352,64 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="64">q0</text>
<path d="M64,128 L352,128 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="128">q1</text>
<path d="M64,192 L352,192 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="192">q2</text>
<path d="M64,256 L352,256 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="256">q3</text>
<path d="M64,320 L352,320 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="320">q4</text>
<path d="M64,384 L352,384 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="384">q5</text>
<path d="M96,128 L96,192 " stroke="black"/>
<rect x="80" y="112" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="96" y="128">E<tspan baseline-shift="sub" font-size="10">X</tspan></text>
<rect x="80" y="176" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="96" y="192">E<tspan baseline-shift="sub" font-size="10">X</tspan></text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="96" y="212">0.25</text>
<path d="M160,128 L160,256 " stroke="black"/>
<rect x="144" y="112" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="160" y="128">E<tspan baseline-shift="sub" font-size="10">X</tspan></text>
<rect x="144" y="176" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="160" y="192">E<tspan baseline-shift="sub" font-size="10">Y</tspan></text>
<rect x="144" y="240" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="160" y="256">E<tspan baseline-shift="sub" font-size="10">Z</tspan></text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="160" y="276">0.125</text>
<path d="M224,192 L224,320 " stroke="black"/>
<rect x="208" y="176" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="224" y="192">EE<tspan baseline-shift="sub" font-size="10">X</tspan></text>
<rect x="208" y="304" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="224" y="320">EE<tspan baseline-shift="sub" font-size="10">Y</tspan></text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="224" y="340">0.25</text>
<rect x="208" y="240" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="224" y="256">EE<tspan baseline-shift="sub" font-size="10">Z</tspan></text>
<rect x="272" y="368" width="32" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="288" y="384">EE<tspan baseline-shift="sub" font-size="10">X</tspan></text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="288" y="404">0.25</text>
</svg>
)DIAGRAM");

    circuit = Circuit(R"CIRCUIT(
        PAULI_CHANNEL_1(0.125,0.25,0.125) 0 1 2 3
        PAULI_CHANNEL_2(0.01,0.01,0.01,0.02,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01) 0 1 2 4
    )CIRCUIT");
    ASSERT_EQ(
        svg_diagram(circuit),
        u8R"DIAGRAM(
<svg viewBox="0 0 1408 416"  version="1.1" xmlns="http://www.w3.org/2000/svg">
<path d="M64,64 L1376,64 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="64">q0</text>
<path d="M64,128 L1376,128 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="128">q1</text>
<path d="M64,192 L1376,192 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="192">q2</text>
<path d="M64,256 L1376,256 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="256">q3</text>
<path d="M64,320 L1376,320 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="320">q4</text>
<rect x="80" y="48" width="224" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="192" y="64">PAULI_CHANNEL_1</text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="192" y="84">0.125,0.25,0.125</text>
<rect x="80" y="112" width="224" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="192" y="128">PAULI_CHANNEL_1</text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="192" y="148">0.125,0.25,0.125</text>
<rect x="80" y="176" width="224" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="192" y="192">PAULI_CHANNEL_1</text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="192" y="212">0.125,0.25,0.125</text>
<rect x="80" y="240" width="224" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="192" y="256">PAULI_CHANNEL_1</text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="192" y="276">0.125,0.25,0.125</text>
<path d="M352,64 L352,128 " stroke="black"/>
<rect x="336" y="48" width="992" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="832" y="64">PAULI_CHANNEL_2<tspan baseline-shift="sub" font-size="10">0</tspan></text>
<rect x="336" y="112" width="992" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="832" y="128">PAULI_CHANNEL_2<tspan baseline-shift="sub" font-size="10">1</tspan></text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="832" y="148">0.01,0.01,0.01,0.02,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01</text>
<path d="M352,192 L352,320 " stroke="black"/>
<rect x="336" y="176" width="992" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="832" y="192">PAULI_CHANNEL_2<tspan baseline-shift="sub" font-size="10">0</tspan></text>
<rect x="336" y="304" width="992" height="32" stroke="black" fill="pink"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="832" y="320">PAULI_CHANNEL_2<tspan baseline-shift="sub" font-size="10">1</tspan></text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="832" y="340">0.01,0.01,0.01,0.02,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01,0.01</text>
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
    ASSERT_EQ(
        svg_diagram(circuit),
        u8R"DIAGRAM(
<svg viewBox="0 0 640 352"  version="1.1" xmlns="http://www.w3.org/2000/svg">
<path d="M64,64 L608,64 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="64">q0</text>
<path d="M64,128 L608,128 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="128">q1</text>
<path d="M64,192 L608,192 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="192">q2</text>
<path d="M64,256 L608,256 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="256">q3</text>
<rect x="80" y="48" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="96" y="64">R</text>
<rect x="80" y="112" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="96" y="128">R<tspan baseline-shift="sub" font-size="10">X</tspan></text>
<rect x="80" y="176" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="96" y="192">R<tspan baseline-shift="sub" font-size="10">Y</tspan></text>
<rect x="80" y="240" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="96" y="256">R</text>
<rect x="144" y="48" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="160" y="64">M</text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="160" y="84">0.001</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="160" y="44">rec[0]</text>
<rect x="144" y="112" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="160" y="128">M</text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="10" stroke="red" x="160" y="148">0.001</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="160" y="108">rec[1]</text>
<rect x="208" y="112" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="224" y="128">MR</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="224" y="108">rec[2]</text>
<rect x="208" y="48" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="224" y="64">MR</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="224" y="44">rec[3]</text>
<rect x="272" y="112" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="288" y="128">MR<tspan baseline-shift="sub" font-size="10">X</tspan></text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="288" y="108">rec[4]</text>
<rect x="272" y="176" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="288" y="192">MR<tspan baseline-shift="sub" font-size="10">X</tspan></text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="288" y="172">rec[5]</text>
<rect x="272" y="48" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="288" y="64">MR<tspan baseline-shift="sub" font-size="10">Y</tspan></text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="288" y="44">rec[6]</text>
<rect x="272" y="240" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="288" y="256">MR<tspan baseline-shift="sub" font-size="10">Y</tspan></text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="288" y="236">rec[7]</text>
<rect x="336" y="112" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="352" y="128">MR<tspan baseline-shift="sub" font-size="10">Y</tspan></text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="352" y="108">rec[8]</text>
<rect x="336" y="48" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="352" y="64">MR</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="352" y="44">rec[9]</text>
<rect x="400" y="112" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="416" y="128">M<tspan baseline-shift="sub" font-size="10">X</tspan></text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="416" y="108">rec[10]</text>
<rect x="400" y="176" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="416" y="192">M<tspan baseline-shift="sub" font-size="10">Y</tspan></text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="416" y="172">rec[11]</text>
<rect x="400" y="240" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="416" y="256">M</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="416" y="236">rec[12]</text>
<path d="M480,64 L480,192 " stroke="black"/>
<rect x="464" y="48" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="480" y="64">MPP<tspan baseline-shift="sub" font-size="10">X</tspan></text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="480" y="44">rec[13]</text>
<rect x="464" y="176" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="480" y="192">MPP<tspan baseline-shift="sub" font-size="10">Y</tspan></text>
<rect x="464" y="240" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="480" y="256">MPP<tspan baseline-shift="sub" font-size="10">Z</tspan></text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="480" y="236">rec[14]</text>
<rect x="528" y="112" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="544" y="128">MPP<tspan baseline-shift="sub" font-size="10">X</tspan></text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="544" y="108">rec[15]</text>
<path d="M544,192 L544,256 " stroke="black"/>
<rect x="528" y="176" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="544" y="192">MPP<tspan baseline-shift="sub" font-size="10">Z</tspan></text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="544" y="172">rec[16]</text>
<rect x="528" y="240" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="544" y="256">MPP<tspan baseline-shift="sub" font-size="10">Y</tspan></text>
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
    ASSERT_EQ(
        svg_diagram(circuit),
        u8R"DIAGRAM(
<svg viewBox="0 0 768 416"  version="1.1" xmlns="http://www.w3.org/2000/svg">
<path d="M64,64 L736,64 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="64">q0</text>
<path d="M64,128 L736,128 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="128">q1</text>
<path d="M64,192 L736,192 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="192">q2</text>
<path d="M64,256 L736,256 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="256">q3</text>
<path d="M64,320 L736,320 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="320">q4</text>
<rect x="80" y="48" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="96" y="64">M</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="96" y="44">rec[0]</text>
<path d="M168,32 L160,32 L160,384 L168,384 " stroke="black" fill="none"/>
<text dominant-baseline="auto" text-anchor="start" font-family="monospace" font-size="12" x="164" y="380">REP100</text>
<rect x="208" y="112" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="224" y="128">M</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="224" y="108">rec[1+iter*13]</text>
<path d="M296,36 L288,36 L288,380 L296,380 " stroke="black" fill="none"/>
<text dominant-baseline="auto" text-anchor="start" font-family="monospace" font-size="12" x="292" y="376">REP5</text>
<rect x="336" y="176" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="352" y="192">M</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="352" y="172">rec[2+iter*13+iter2]</text>
<path d="M408,36 L416,36 L416,380 L408,380 " stroke="black" fill="none"/>
<path d="M488,36 L480,36 L480,380 L488,380 " stroke="black" fill="none"/>
<text dominant-baseline="auto" text-anchor="start" font-family="monospace" font-size="12" x="484" y="376">REP7</text>
<path d="M544,256 L544,320 " stroke="black"/>
<rect x="528" y="240" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="544" y="256">MPP<tspan baseline-shift="sub" font-size="10">X</tspan></text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="544" y="236">rec[7+iter*13+iter2]</text>
<rect x="528" y="304" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="544" y="320">MPP<tspan baseline-shift="sub" font-size="10">Y</tspan></text>
<path d="M600,36 L608,36 L608,380 L600,380 " stroke="black" fill="none"/>
<path d="M664,32 L672,32 L672,384 L664,384 " stroke="black" fill="none"/>
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
    ASSERT_EQ(
        svg_diagram(circuit),
        u8R"DIAGRAM(
<svg viewBox="0 0 640 352"  version="1.1" xmlns="http://www.w3.org/2000/svg">
<path d="M64,64 L608,64 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="64">q0</text>
<path d="M64,128 L608,128 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="128">q1</text>
<path d="M64,192 L608,192 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="192">q2</text>
<path d="M64,256 L608,256 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="256">q3</text>
<rect x="80" y="48" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="96" y="64">H</text>
<rect x="80" y="112" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="96" y="128">H</text>
<rect x="80" y="176" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="96" y="192">H</text>
<path d="M168,32 L160,32 L160,320 L168,320 " stroke="black" fill="none"/>
<text dominant-baseline="auto" text-anchor="start" font-family="monospace" font-size="12" x="164" y="316">REP5</text>
<rect x="208" y="176" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="224" y="192">R<tspan baseline-shift="sub" font-size="10">X</tspan></text>
<path d="M296,36 L288,36 L288,316 L296,316 " stroke="black" fill="none"/>
<text dominant-baseline="auto" text-anchor="start" font-family="monospace" font-size="12" x="292" y="312">REP100</text>
<rect x="336" y="48" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="352" y="64">H</text>
<rect x="336" y="112" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="352" y="128">H</text>
<rect x="336" y="240" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="352" y="256">H</text>
<rect x="400" y="240" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="416" y="256">H</text>
<path d="M472,36 L480,36 L480,316 L472,316 " stroke="black" fill="none"/>
<path d="M536,32 L544,32 L544,320 L536,320 " stroke="black" fill="none"/>
</svg>
)DIAGRAM");
}

TEST(circuit_diagram_timeline_svg, classical_feedback) {
    auto circuit = Circuit(R"CIRCUIT(
        M 0
        CX rec[-1] 1
        YCZ 2 sweep[5]
    )CIRCUIT");
    ASSERT_EQ(
        svg_diagram(circuit),
        u8R"DIAGRAM(
<svg viewBox="0 0 256 288"  version="1.1" xmlns="http://www.w3.org/2000/svg">
<path d="M64,64 L224,64 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="64">q0</text>
<path d="M64,128 L224,128 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="128">q1</text>
<path d="M64,192 L224,192 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="192">q2</text>
<rect x="80" y="48" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="96" y="64">M</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="96" y="44">rec[0]</text>
<rect x="80" y="112" width="96" height="32" stroke="black" fill="lightgray"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="128" y="128">X<tspan baseline-shift="super" font-size="10">rec[0]</tspan></text>
<rect x="80" y="176" width="96" height="32" stroke="black" fill="lightgray"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="128" y="192">Y<tspan baseline-shift="super" font-size="10">sweep[5]</tspan></text>
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
    ASSERT_EQ(
        svg_diagram(circuit),
        u8R"DIAGRAM(
<svg viewBox="0 0 576 288"  version="1.1" xmlns="http://www.w3.org/2000/svg">
<path d="M64,64 L544,64 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="64">q0</text>
<path d="M64,128 L544,128 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="128">q1</text>
<path d="M64,192 L544,192 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="192">q2</text>
<rect x="80" y="176" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="96" y="192">R</text>
<path d="M160,128 L160,192 " stroke="black"/>
<rect x="144" y="112" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="160" y="128">MPP<tspan baseline-shift="sub" font-size="10">X</tspan></text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="160" y="108">rec[0]</text>
<rect x="144" y="176" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="160" y="192">MPP<tspan baseline-shift="sub" font-size="10">X</tspan></text>
<path d="M224,64 L224,192 " stroke="black"/>
<rect x="208" y="48" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="224" y="64">MPP<tspan baseline-shift="sub" font-size="10">Z</tspan></text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="224" y="44">rec[1]</text>
<rect x="208" y="176" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="12" x="224" y="192">MPP<tspan baseline-shift="sub" font-size="10">Z</tspan></text>
<rect x="272" y="176" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="288" y="192">M<tspan baseline-shift="sub" font-size="10">X</tspan></text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="288" y="172">rec[2]</text>
<rect x="272" y="48" width="96" height="32" stroke="black" fill="lightgray"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="320" y="64">Z<tspan baseline-shift="super" font-size="10">rec[0]</tspan></text>
<rect x="272" y="112" width="96" height="32" stroke="black" fill="lightgray"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="320" y="128">X<tspan baseline-shift="super" font-size="10">rec[1]</tspan></text>
<rect x="400" y="48" width="96" height="32" stroke="black" fill="lightgray"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="448" y="64">Z<tspan baseline-shift="super" font-size="10">rec[2]</tspan></text>
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
    ASSERT_EQ(
        svg_diagram(circuit),
        u8R"DIAGRAM(
<svg viewBox="0 0 1024 224"  version="1.1" xmlns="http://www.w3.org/2000/svg">
<path d="M64,64 L992,64 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="64">q0</text>
<path d="M64,128 L992,128 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="128">q1</text>
<rect x="80" y="48" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="96" y="64">H</text>
<rect x="144" y="48" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="160" y="64">H</text>
<path d="M68,40 L68,32 L188,32 L188,40 " stroke="black" fill="none"/>
<path d="M68,184 L68,192 L188,192 L188,184 " stroke="black" fill="none"/>
<rect x="208" y="48" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="224" y="64">H</text>
<rect x="208" y="112" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="224" y="128">H</text>
<rect x="272" y="48" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="288" y="64">H</text>
<path d="M360,32 L352,32 L352,192 L360,192 " stroke="black" fill="none"/>
<text dominant-baseline="auto" text-anchor="start" font-family="monospace" font-size="12" x="356" y="188">REP1</text>
<rect x="400" y="48" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="416" y="64">H</text>
<rect x="400" y="112" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="416" y="128">H</text>
<rect x="464" y="48" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="480" y="64">H</text>
<rect x="528" y="48" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="544" y="64">S</text>
<path d="M452,40 L452,32 L572,32 L572,40 " stroke="black" fill="none"/>
<path d="M452,184 L452,192 L572,192 L572,184 " stroke="black" fill="none"/>
<path d="M600,32 L608,32 L608,192 L600,192 " stroke="black" fill="none"/>
<rect x="656" y="48" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="672" y="64">H</text>
<rect x="720" y="48" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="736" y="64">H</text>
<rect x="784" y="48" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="800" y="64">√X</text>
<path d="M644,40 L644,32 L828,32 L828,40 " stroke="black" fill="none"/>
<path d="M644,184 L644,192 L828,192 L828,184 " stroke="black" fill="none"/>
<rect x="848" y="48" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="864" y="64">H</text>
<rect x="912" y="48" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="928" y="64">H</text>
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
    ASSERT_EQ(
        svg_diagram(circuit),
        u8R"DIAGRAM(
<svg viewBox="0 0 1280 480"  version="1.1" xmlns="http://www.w3.org/2000/svg">
<path d="M64,64 L1248,64 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="64">q0</text>
<path d="M64,128 L1248,128 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="128">q1</text>
<path d="M64,192 L1248,192 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="192">q2</text>
<path d="M64,256 L1248,256 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="256">q3</text>
<path d="M64,320 L1248,320 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="320">q4</text>
<path d="M64,384 L1248,384 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="384">q5</text>
<rect x="80" y="112" width="224" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="192" y="128">COORDS(1,2)</text>
<rect x="80" y="48" width="224" height="32" stroke="black" fill="lightgray"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="192" y="64">DETECTOR</text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="8" x="192" y="84">coords=(4,5,6)</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="192" y="44">D0 = 1 (vacuous)</text>
<rect x="80" y="176" width="224" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="192" y="192">COORDS(11,22)</text>
<rect x="336" y="48" width="224" height="32" stroke="black" fill="lightgray"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="448" y="64">DETECTOR</text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="8" x="448" y="84">coords=(14,25,36)</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="448" y="44">D1 = 1 (vacuous)</text>
<path d="M616,32 L608,32 L608,448 L616,448 " stroke="black" fill="none"/>
<text dominant-baseline="auto" text-anchor="start" font-family="monospace" font-size="12" x="612" y="444">REP100</text>
<rect x="656" y="240" width="224" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="768" y="256">COORDS(17,28+iter*200)</text>
<rect x="656" y="304" width="224" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="768" y="320">COORDS(17,28+iter*200)</text>
<rect x="656" y="48" width="224" height="32" stroke="black" fill="lightgray"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="768" y="64">DETECTOR</text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="8" x="768" y="84">coords=(19,30+iter*200,41+iter*300)</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="768" y="44">D[2+iter] = 1 (vacuous)</text>
<path d="M920,32 L928,32 L928,448 L920,448 " stroke="black" fill="none"/>
<rect x="976" y="368" width="224" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="1088" y="384">COORDS(11,20022)</text>
<rect x="976" y="48" width="224" height="32" stroke="black" fill="lightgray"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="1088" y="64">DETECTOR</text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="8" x="1088" y="84">coords=(14,20025,30036)</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="1088" y="44">D102 = 1 (vacuous)</text>
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
    ASSERT_EQ(
        svg_diagram(circuit),
        u8R"DIAGRAM(
<svg viewBox="0 0 1024 480"  version="1.1" xmlns="http://www.w3.org/2000/svg">
<path d="M64,64 L992,64 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="64">q0</text>
<path d="M64,128 L992,128 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="128">q1</text>
<path d="M64,192 L992,192 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="192">q2</text>
<path d="M64,256 L992,256 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="256">q3</text>
<path d="M64,320 L992,320 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="320">q4</text>
<path d="M64,384 L992,384 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="384">q5</text>
<rect x="80" y="48" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="96" y="64">M</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="96" y="44">rec[0]</text>
<rect x="80" y="112" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="96" y="128">M</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="96" y="108">rec[1]</text>
<rect x="80" y="176" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="96" y="192">M</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="96" y="172">rec[2]</text>
<rect x="80" y="240" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="96" y="256">M</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="96" y="236">rec[3]</text>
<rect x="80" y="304" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="96" y="320">M</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="96" y="300">rec[4]</text>
<rect x="80" y="368" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="96" y="384">M</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="96" y="364">rec[5]</text>
<path d="M168,32 L160,32 L160,448 L168,448 " stroke="black" fill="none"/>
<text dominant-baseline="auto" text-anchor="start" font-family="monospace" font-size="12" x="164" y="444">REP100</text>
<rect x="208" y="112" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="224" y="128">M</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="224" y="108">rec[6+iter*2]</text>
<rect x="208" y="176" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="224" y="192">M</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="224" y="172">rec[7+iter*2]</text>
<path d="M280,32 L288,32 L288,448 L280,448 " stroke="black" fill="none"/>
<rect x="336" y="176" width="160" height="32" stroke="black" fill="lightgray"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="416" y="192">DETECTOR</text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="8" x="416" y="212">coords=(1)</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="416" y="172">D0 = rec[205]</text>
<rect x="336" y="112" width="160" height="32" stroke="black" fill="lightgray"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="416" y="128">DETECTOR</text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="8" x="416" y="148">coords=(2)</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="416" y="108">D1 = rec[204]</text>
<rect x="528" y="176" width="160" height="32" stroke="black" fill="lightgray"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="608" y="192">DETECTOR</text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="8" x="608" y="212">coords=(3)</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="608" y="172">D2 = rec[203]</text>
<rect x="528" y="112" width="160" height="32" stroke="black" fill="lightgray"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="608" y="128">DETECTOR</text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="8" x="608" y="148">coords=(4)</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="608" y="108">D3 = rec[202]</text>
<rect x="720" y="112" width="224" height="32" stroke="black" fill="lightgray"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="832" y="128">DETECTOR</text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="8" x="832" y="148">coords=(5)</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="832" y="108">D4 = rec[205]*rec[204]</text>
<rect x="720" y="240" width="224" height="32" stroke="black" fill="lightgray"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="832" y="256">OBS_INCLUDE(100)</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="832" y="236">L100 *= rec[5]*rec[3]</text>
</svg>
)DIAGRAM");
}

TEST(circuit_diagram_timeline_svg, repetition_code) {
    CircuitGenParameters params(10, 3, "memory");
    auto circuit = generate_rep_code_circuit(params).circuit;
    ASSERT_EQ(
        svg_diagram(circuit),
        u8R"DIAGRAM(
<svg viewBox="0 0 1600 416"  version="1.1" xmlns="http://www.w3.org/2000/svg">
<path d="M64,64 L1568,64 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="64">q0</text>
<path d="M64,128 L1568,128 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="128">q1</text>
<path d="M64,192 L1568,192 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="192">q2</text>
<path d="M64,256 L1568,256 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="256">q3</text>
<path d="M64,320 L1568,320 " stroke="black"/>
<text dominant-baseline="central" text-anchor="end" font-family="monospace" font-size="12" x="64" y="320">q4</text>
<rect x="80" y="48" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="96" y="64">R</text>
<rect x="80" y="112" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="96" y="128">R</text>
<rect x="80" y="176" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="96" y="192">R</text>
<rect x="80" y="240" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="96" y="256">R</text>
<rect x="80" y="304" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="96" y="320">R</text>
<path d="M160,64 L160,128 " stroke="black"/>
<circle cx="160" cy="64" r="8" stroke="none" fill="black"/>
<circle cx="160" cy="128" r="8" stroke="black" fill="white"/>
<path d="M152,128 L168,128 M160,120 L160,136 " stroke="black"/>
<path d="M160,192 L160,256 " stroke="black"/>
<circle cx="160" cy="192" r="8" stroke="none" fill="black"/>
<circle cx="160" cy="256" r="8" stroke="black" fill="white"/>
<path d="M152,256 L168,256 M160,248 L160,264 " stroke="black"/>
<path d="M224,128 L224,192 " stroke="black"/>
<circle cx="224" cy="192" r="8" stroke="none" fill="black"/>
<circle cx="224" cy="128" r="8" stroke="black" fill="white"/>
<path d="M216,128 L232,128 M224,120 L224,136 " stroke="black"/>
<path d="M224,256 L224,320 " stroke="black"/>
<circle cx="224" cy="320" r="8" stroke="none" fill="black"/>
<circle cx="224" cy="256" r="8" stroke="black" fill="white"/>
<path d="M216,256 L232,256 M224,248 L224,264 " stroke="black"/>
<rect x="272" y="112" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="288" y="128">MR</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="288" y="108">rec[0]</text>
<rect x="272" y="240" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="288" y="256">MR</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="288" y="236">rec[1]</text>
<rect x="336" y="112" width="160" height="32" stroke="black" fill="lightgray"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="416" y="128">DETECTOR</text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="8" x="416" y="148">coords=(1,0)</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="416" y="108">D0 = rec[0]</text>
<rect x="336" y="240" width="160" height="32" stroke="black" fill="lightgray"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="416" y="256">DETECTOR</text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="8" x="416" y="276">coords=(3,0)</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="416" y="236">D1 = rec[1]</text>
<path d="M260,40 L260,32 L508,32 L508,40 " stroke="black" fill="none"/>
<path d="M260,376 L260,384 L508,384 L508,376 " stroke="black" fill="none"/>
<path d="M552,32 L544,32 L544,384 L552,384 " stroke="black" fill="none"/>
<text dominant-baseline="auto" text-anchor="start" font-family="monospace" font-size="12" x="548" y="380">REP9</text>
<path d="M672,64 L672,128 " stroke="black"/>
<circle cx="672" cy="64" r="8" stroke="none" fill="black"/>
<circle cx="672" cy="128" r="8" stroke="black" fill="white"/>
<path d="M664,128 L680,128 M672,120 L672,136 " stroke="black"/>
<path d="M672,192 L672,256 " stroke="black"/>
<circle cx="672" cy="192" r="8" stroke="none" fill="black"/>
<circle cx="672" cy="256" r="8" stroke="black" fill="white"/>
<path d="M664,256 L680,256 M672,248 L672,264 " stroke="black"/>
<path d="M736,128 L736,192 " stroke="black"/>
<circle cx="736" cy="192" r="8" stroke="none" fill="black"/>
<circle cx="736" cy="128" r="8" stroke="black" fill="white"/>
<path d="M728,128 L744,128 M736,120 L736,136 " stroke="black"/>
<path d="M736,256 L736,320 " stroke="black"/>
<circle cx="736" cy="320" r="8" stroke="none" fill="black"/>
<circle cx="736" cy="256" r="8" stroke="black" fill="white"/>
<path d="M728,256 L744,256 M736,248 L736,264 " stroke="black"/>
<rect x="784" y="112" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="800" y="128">MR</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="800" y="108">rec[2+iter*2]</text>
<rect x="784" y="240" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="800" y="256">MR</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="800" y="236">rec[3+iter*2]</text>
<rect x="848" y="112" width="224" height="32" stroke="black" fill="lightgray"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="960" y="128">DETECTOR</text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="8" x="960" y="148">coords=(1,1+iter)</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="960" y="108">D[2+iter*2] = rec[2+iter*2]*rec[0+iter*2]</text>
<rect x="848" y="240" width="224" height="32" stroke="black" fill="lightgray"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="960" y="256">DETECTOR</text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="8" x="960" y="276">coords=(3,1+iter)</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="960" y="236">D[3+iter*2] = rec[3+iter*2]*rec[1+iter*2]</text>
<path d="M772,40 L772,32 L1084,32 L1084,40 " stroke="black" fill="none"/>
<path d="M772,376 L772,384 L1084,384 L1084,376 " stroke="black" fill="none"/>
<path d="M1112,32 L1120,32 L1120,384 L1112,384 " stroke="black" fill="none"/>
<rect x="1168" y="48" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="1184" y="64">M</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="1184" y="44">rec[20]</text>
<rect x="1168" y="176" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="1184" y="192">M</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="1184" y="172">rec[21]</text>
<rect x="1168" y="304" width="32" height="32" stroke="black" fill="white"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="1184" y="320">M</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="1184" y="300">rec[22]</text>
<rect x="1232" y="48" width="288" height="32" stroke="black" fill="lightgray"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="1376" y="64">DETECTOR</text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="8" x="1376" y="84">coords=(1,10)</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="1376" y="44">D20 = rec[21]*rec[20]*rec[18]</text>
<rect x="1232" y="176" width="288" height="32" stroke="black" fill="lightgray"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="1376" y="192">DETECTOR</text>
<text dominant-baseline="hanging" text-anchor="middle" font-family="monospace" font-size="8" x="1376" y="212">coords=(3,10)</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="1376" y="172">D21 = rec[22]*rec[21]*rec[19]</text>
<rect x="1232" y="304" width="160" height="32" stroke="black" fill="lightgray"/>
<text dominant-baseline="central" text-anchor="middle" font-family="monospace" font-size="16" x="1312" y="320">OBS_INCLUDE(0)</text>
<text text-anchor="middle" font-family="monospace" font-size="8" x="1312" y="300">L0 *= rec[22]</text>
</svg>
)DIAGRAM");
}
