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

#include "gtest/gtest.h"

#include "stim/main_namespaced.test.h"
#include "stim/util_bot/test_util.test.h"

using namespace stim;

TEST(command_m2d, m2d) {
    RaiiTempNamedFile tmp(R"CIRCUIT(
        X 0
        M 0 1
        DETECTOR rec[-2]
        DETECTOR rec[-1]
        OBSERVABLE_INCLUDE(2) rec[-1]
    )CIRCUIT");

    ASSERT_EQ(
        trim(run_captured_stim_main(
            {"m2d", "--in_format=01", "--out_format=dets", "--circuit", tmp.path.c_str(), "--append_observables"},
            "00\n01\n10\n11\n")),
        trim(R"output(
shot D0
shot D0 D1 L2
shot
shot D1 L2
            )output"));

    ASSERT_EQ(
        trim(run_captured_stim_main(
            {"m2d", "--in_format=01", "--out_format=dets", "--circuit", tmp.path.c_str()}, "00\n01\n10\n11\n")),
        trim(R"output(
shot D0
shot D0 D1
shot
shot D1
            )output"));

    ASSERT_EQ(
        trim(run_captured_stim_main(
            {"m2d", "--in_format=01", "--out_format=dets", "--circuit", tmp.path.c_str(), "--skip_reference_sample"},
            "00\n01\n10\n11\n")),
        trim(R"output(
shot
shot D1
shot D0
shot D0 D1
            )output"));
}

TEST(command_m2d, m2d_without_feedback) {
    RaiiTempNamedFile tmp(R"CIRCUIT(
        CX 0 2 1 2
        M 2
        CX rec[-1] 2
        DETECTOR rec[-1]
        TICK

        CX 0 2 1 2
        M 2
        CX rec[-1] 2
        DETECTOR rec[-1] rec[-2]
        TICK

        CX 0 2 1 2
        M 2
        CX rec[-1] 2
        DETECTOR rec[-1] rec[-2]
        TICK

        M 0 1
        DETECTOR rec[-1] rec[-2] rec[-3]
        OBSERVABLE_INCLUDE(0) rec[-1]
    )CIRCUIT");

    ASSERT_EQ(
        trim(run_captured_stim_main(
            {"m2d", "--in_format=01", "--append_observables", "--out_format=dets", "--circuit", tmp.path.c_str()},
            "00000\n10000\n01000\n00100\n00010\n00001\n")),
        trim(R"output(
shot
shot D0 D1
shot D1 D2
shot D2 D3
shot D3
shot D3 L0
            )output"));

    ASSERT_EQ(
        trim(run_captured_stim_main(
            {"m2d",
             "--in_format=01",
             "--append_observables",
             "--out_format=dets",
             "--circuit",
             tmp.path.c_str(),
             "--ran_without_feedback"},
            "00000\n11100\n01100\n00100\n00010\n00001\n")),
        trim(R"output(
shot
shot D0 D1
shot D1 D2
shot D2 D3
shot D3
shot D3 L0
            )output"));
}

TEST(command_m2d, m2d_obs_size_misalign_1_obs) {
    RaiiTempNamedFile tmp_circuit(R"CIRCUIT(
        M 0
        REPEAT 1024 {
            DETECTOR rec[-1]
        }
        OBSERVABLE_INCLUDE(0) rec[-1]
    )CIRCUIT");
    RaiiTempNamedFile tmp_obs;

    ASSERT_EQ(
        trim(run_captured_stim_main(
            {"m2d", "--in_format=01", "--obs_out", tmp_obs.path.c_str(), "--circuit", tmp_circuit.path.c_str()},
            "0\n")),
        trim(std::string(1024, '0') + "\n"));
    ASSERT_EQ(tmp_obs.read_contents(), "0\n");
}

TEST(command_m2d, m2d_obs_size_misalign_11_obs) {
    RaiiTempNamedFile tmp_circuit(R"CIRCUIT(
        M 0
        REPEAT 1024 {
            DETECTOR rec[-1]
        }
        OBSERVABLE_INCLUDE(10) rec[-1]
    )CIRCUIT");
    RaiiTempNamedFile tmp_obs;

    ASSERT_EQ(
        trim(run_captured_stim_main(
            {"m2d", "--in_format=01", "--obs_out", tmp_obs.path.c_str(), "--circuit", tmp_circuit.path.c_str()},
            "0\n")),
        trim(std::string(1024, '0') + "\n"));
    ASSERT_EQ(tmp_obs.read_contents(), "00000000000\n");
}

TEST(command_m2d, unphysical_observable_annotations) {
    RaiiTempNamedFile tmp_circuit(R"CIRCUIT(
        QUBIT_COORDS(0, 0) 0
        QUBIT_COORDS(1, 0) 1
        QUBIT_COORDS(0, 1) 2
        QUBIT_COORDS(1, 1) 3
        OBSERVABLE_INCLUDE(0) X0 X1
        OBSERVABLE_INCLUDE(1) Z0 Z2
        MPP X0*X1*X2*X3 Z0*Z1 Z2*Z3
        DEPOLARIZE1(0.001) 0 1 2 3
        MPP X0*X1*X2*X3 Z0*Z1 Z2*Z3
        DETECTOR rec[-1] rec[-4]
        DETECTOR rec[-2] rec[-5]
        DETECTOR rec[-3] rec[-6]
        OBSERVABLE_INCLUDE(0) X0 X1
        OBSERVABLE_INCLUDE(1) Z0 Z2
    )CIRCUIT");
    RaiiTempNamedFile tmp_obs;

    ASSERT_EQ(
        trim(run_captured_stim_main(
            {"m2d", "--in_format=01", "--obs_out", tmp_obs.path.c_str(), "--circuit", tmp_circuit.path.c_str()},
            "000000\n100100\n000110\n")),
        trim("000\n000\n011\n"));
    ASSERT_EQ(tmp_obs.read_contents(), "00\n00\n00\n");
}
