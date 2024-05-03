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

#include <pybind11/iostream.h>
#include <pybind11/pybind11.h>

#include "stim/gates/gates.pybind.h"
#include "stim/main_namespaced.h"
#include "stim/py/base.pybind.h"
#include "stim/py/march.pybind.h"

#define xstr_literal(s) str_literal(s)
#define str_literal(s) #s

using namespace stim;
using namespace stim_pybind;

int stim_main(const std::vector<std::string> &args) {
    pybind11::scoped_ostream_redirect redirect_out(std::cout, pybind11::module_::import("sys").attr("stdout"));
    pybind11::scoped_ostream_redirect redirect_err(std::cerr, pybind11::module_::import("sys").attr("stderr"));
    std::vector<const char *> argv;
    argv.push_back("stim.main");
    for (const auto &arg : args) {
        argv.push_back(arg.c_str());
    }
    return stim::main(argv.size(), argv.data());
}

void top_level(pybind11::module &m) {
    m.def(
        "main",
        &stim_main,
        pybind11::kw_only(),
        pybind11::arg("command_line_args"),
        clean_doc_string(R"DOC(
            Runs the command line tool version of stim on the given arguments.

            Note that by default any input will be read from stdin, any output
            will print to stdout (as opposed to being intercepted). For most
            commands, you can use arguments like `--out` to write to a file
            instead of stdout and `--in` to read from a file instead of stdin.

            Returns:
                An exit code (0 means success, not zero means failure).

            Raises:
                A large variety of errors, depending on what you are doing and
                how it failed! Beware that many errors are caught by the main
                method itself and printed to stderr, with the only indication
                that something went wrong being the return code.

            Example:
                >>> import stim
                >>> import tempfile
                >>> with tempfile.TemporaryDirectory() as d:
                ...     path = f'{d}/tmp.out'
                ...     return_code = stim.main(command_line_args=[
                ...         "gen",
                ...         "--code=repetition_code",
                ...         "--task=memory",
                ...         "--rounds=1000",
                ...         "--distance=2",
                ...         "--out",
                ...         path,
                ...     ])
                ...     assert return_code == 0
                ...     with open(path) as f:
                ...         print(f.read(), end='')
                # Generated repetition_code circuit.
                # task: memory
                # rounds: 1000
                # distance: 2
                # before_round_data_depolarization: 0
                # before_measure_flip_probability: 0
                # after_reset_flip_probability: 0
                # after_clifford_depolarization: 0
                # layout:
                # L0 Z1 d2
                # Legend:
                #     d# = data qubit
                #     L# = data qubit with logical observable crossing
                #     Z# = measurement qubit
                R 0 1 2
                TICK
                CX 0 1
                TICK
                CX 2 1
                TICK
                MR 1
                DETECTOR(1, 0) rec[-1]
                REPEAT 999 {
                    TICK
                    CX 0 1
                    TICK
                    CX 2 1
                    TICK
                    MR 1
                    SHIFT_COORDS(0, 1)
                    DETECTOR(1, 0) rec[-1] rec[-2]
                }
                M 0 2
                DETECTOR(1, 1) rec[-1] rec[-2] rec[-3]
                OBSERVABLE_INCLUDE(0) rec[-1]
        )DOC")
            .data());
}

PYBIND11_MODULE(STIM_PYBIND11_MODULE_NAME, m) {
    m.attr("__version__") = xstr_literal(VERSION_INFO);
    m.doc() = R"pbdoc(
        Stim: A fast stabilizer circuit library.
    )pbdoc";

    /// class registration happens before function/method
    /// registration. If a class references another before it is
    /// registered, method signatures can get messed up.  For example,
    /// if DetectorErrorModel is defined after Circuit then
    /// Circuit.detector_error_model's return type is described as
    /// `stim::DetectorErrorModel` instead of `stim.DetectorErrorModel`.

    /// class definitions
    auto c_gate_data = pybind_gate_data(m);
    /// top level function definitions
    top_level(m);

    // method definitions
    pybind_gate_data_methods(m, c_gate_data);
}
