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

#include "stim/simulators/frame_simulator.pybind.h"

#include "stim/py/base.pybind.h"

using namespace stim;
using namespace stim_pybind;

pybind11::class_<FrameSimulator> stim_pybind::pybind_frame_simulator(pybind11::module &m) {
    return pybind11::class_<FrameSimulator>(
        m,
        "FrameSimulator",
        clean_doc_string(u8R"DOC(class)DOC").data());
}

void stim_pybind::pybind_frame_simulator_methods(pybind11::module &m, pybind11::class_<FrameSimulator> &c) {
    c.def(
    pybind11::init([](size_t num_qubits, size_t batch_size, size_t max_lookback, const pybind11::object &seed) {
        return FrameSimulator(num_qubits, batch_size, max_lookback, make_py_seeded_rng_move(seed));
    }),
    pybind11::arg("num_qubits"),
    pybind11::arg("batch_size"),
    pybind11::arg("max_lookback"),
    pybind11::arg("seed") = pybind11::none(),
    clean_doc_string(u8R"DOC(initializer)DOC").data());
}
