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

#include "stim/diagram/timeline_img/timeline_svg.h"

#include "gtest/gtest.h"
#include "stim/gen/circuit_gen_params.h"
#include "stim/gen/gen_surface_code.h"
#include "stim/diagram/timeline_img/timeline_tikz.h"

using namespace stim;

TEST(timeline_tikz, repeat2XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX) {
    auto circuit = Circuit(R"CIRCUIT(
        H 0 1 2
        REPEAT 5 {
            RX 2
            SQRT_XX_DAG 0 1
            REPEAT 100 {
                H 0 1 3 3
            }
        }
        # R 0
        # RX 1
        # RY 2
        # RZ 3
        # M(0.001) 0 1
        # MR 1 0
        # MRX 1 2
        # MRY 0 3 1
        MRZ 0
        MX 1
        MY 2
        MZ 3
        MPP X0*Y2 Z3 X1 Z2*Y3
        # CNOT 0 1
        # CX 2 3
        # CY 4 5 5 4
        # CZ 0 2
        # ISWAP 1 3
        # ISWAP_DAG 2 4
        # SQRT_XX 3 5
        # SQRT_XX_DAG 0 5
        # SQRT_YY 3 4 4 3
        # SQRT_YY_DAG 0 1
        # SQRT_ZZ 2 3
        # SQRT_ZZ_DAG 4 5
        SWAP 0 1
        # XCX 2 3
        # XCY 3 4
        # XCZ 0 1
        # YCX 2 3
        # YCY 4 5
        # YCZ 0 1
        # ZCX 2 3
        ZCY 4 5
        ZCZ 0 5 2 3 1 4
    )CIRCUIT");
//    CircuitGenParameters params(999999, 5, "unrotated_memory_z");
//    circuit = generate_surface_code_circuit(params).circuit;
//
//    auto tx = circuit_diagram_timeline_svg(circuit);
//    FILE *f = fopen("/usr/local/google/home/craiggidney/tmp/x.svg", "w");
//    fprintf(f, "%s", tx.data());
//    fclose(f);
    std::cerr << circuit_diagram_timeline_tikz(circuit);
}
