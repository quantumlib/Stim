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

#include "stim/stabilizers/flow.h"

#include "stim/py/base.pybind.h"
#include "stim/stabilizers/flow.pybind.h"

using namespace stim;
using namespace stim_pybind;

pybind11::class_<Flow<MAX_BITWORD_WIDTH>> stim_pybind::pybind_flow(pybind11::module &m) {
    return pybind11::class_<Flow<MAX_BITWORD_WIDTH>>(
        m,
        "Flow",
        clean_doc_string(R"DOC(
            A stabilizer flow (e.g. "XI -> XX xor rec[-1]").

            Stabilizer circuits implement, and can be defined by, how they turn input
            stabilizers into output stabilizers mediated by measurements. These
            relationships are called stabilizer flows, and `stim.Flow` is a representation
            of such a flow. For example, a `stim.Flow` can be given to
            `stim.Circuit.has_flow` to verify that a circuit implements the flow.

            A circuit has a stabilizer flow P -> Q if it maps the instantaneous stabilizer
            P at the start of the circuit to the instantaneous stabilizer Q at the end of
            the circuit. The flow may be mediated by certain measurements. For example,
            a lattice surgery CNOT involves an MXX measurement and an MZZ measurement, and
            the CNOT flows implemented by the circuit involve these measurements.

            A flow like P -> Q means the circuit transforms P into Q.
            A flow like 1 -> P means the circuit prepares P.
            A flow like P -> 1 means the circuit measures P.
            A flow like 1 -> 1 means the circuit contains a check (could be a DETECTOR).

            References:
                Stim's gate documentation includes the stabilizer flows of each gate.

                Appendix A of https://arxiv.org/abs/2302.02192 describes how flows are
                defined and provides a circuit construction for experimentally verifying
                their presence.

            Examples:
                >>> import stim
                >>> c = stim.Circuit("CNOT 2 4")

                >>> c.has_flow(stim.Flow("__X__ -> __X_X"))
                True

                >>> c.has_flow(stim.Flow("X2*X4 -> X2"))
                True

                >>> c.has_flow(stim.Flow("Z4 -> Z4"))
                False
        )DOC")
            .data());
}

static Flow<MAX_BITWORD_WIDTH> py_init_flow(
    const pybind11::object &arg,
    const pybind11::object &input,
    const pybind11::object &output,
    const pybind11::object &measurements) {
    if (arg.is_none()) {
        Flow<MAX_BITWORD_WIDTH> result{PauliString<MAX_BITWORD_WIDTH>(0), PauliString<MAX_BITWORD_WIDTH>(0)};
        bool imag = false;
        if (!input.is_none()) {
            auto f = pybind11::cast<FlexPauliString>(input);
            imag ^= f.imag;
            result.input = std::move(f.value);
        }
        if (!output.is_none()) {
            auto f = pybind11::cast<FlexPauliString>(output);
            imag ^= f.imag;
            result.output = std::move(f.value);
        }
        if (imag) {
            throw std::invalid_argument("Anti-Hermitian flows aren't allowed.");
        }
        if (!measurements.is_none()) {
            for (const auto &h : measurements) {
                if (pybind11::isinstance<GateTarget>(h)) {
                    GateTarget g = pybind11::cast<GateTarget>(h);
                    if (!g.is_measurement_record_target()) {
                        throw std::invalid_argument("Not a measurement offset: " + g.str());
                    }
                    result.measurements.push_back(g.rec_offset());
                } else {
                    result.measurements.push_back(pybind11::cast<int32_t>(h));
                }
            }
        }
        return result;
    }

    if (!input.is_none()) {
        throw std::invalid_argument("Can't specify both a positional argument and `input=`.");
    }
    if (!output.is_none()) {
        throw std::invalid_argument("Can't specify both a positional argument and `output=`.");
    }
    if (!measurements.is_none()) {
        throw std::invalid_argument("Can't specify both a positional argument and `measurements=`.");
    }

    if (pybind11::isinstance<Flow<MAX_BITWORD_WIDTH>>(arg)) {
        return pybind11::cast<Flow<MAX_BITWORD_WIDTH>>(arg);
    } else if (pybind11::isinstance<pybind11::str>(arg)) {
        return Flow<MAX_BITWORD_WIDTH>::from_str(pybind11::cast<std::string_view>(arg));
    }

    std::stringstream ss;
    ss << "Don't know how to turn '" << arg << " into a flow.";
    throw std::invalid_argument(ss.str());
}

void stim_pybind::pybind_flow_methods(pybind11::module &m, pybind11::class_<Flow<MAX_BITWORD_WIDTH>> &c) {
    c.def(
        pybind11::init(&py_init_flow),
        pybind11::arg("arg") = pybind11::none(),
        pybind11::pos_only(),
        pybind11::kw_only(),
        pybind11::arg("input") = pybind11::none(),
        pybind11::arg("output") = pybind11::none(),
        pybind11::arg("measurements") = pybind11::none(),
        clean_doc_string(R"DOC(
            @signature def __init__(self, arg: Union[None, str, stim.Flow] = None, /, *, input: Optional[stim.PauliString] = None, output: Optional[stim.PauliString] = None, measurements: Optional[Iterable[Union[int, GateTarget]]] = None) -> None:
            Initializes a stim.Flow.

            When given a string, the string is parsed as flow shorthand. For example,
            the string "X_ -> ZZ xor rec[-1]" will result in a flow with input pauli string
            "X_", output pauli string "ZZ", and measurement indices [-1].

            Arguments:
                arg [position-only]: Defaults to None. Must be specified by itself if used.
                    str: Initializes a flow by parsing the given shorthand text.
                    stim.Flow: Initializes a copy of the given flow.
                    None (default): Initializes an empty flow.
                input: Defaults to None. Can be set to a stim.PauliString to directly
                    specify the flow's input stabilizer.
                output: Defaults to None. Can be set to a stim.PauliString to directly
                    specify the flow's output stabilizer.
                measurements: Can be set to a list of integers or gate targets like
                    `stim.target_rec(-1)`, to specify the measurements that mediate the
                    flow. Negative and positive measurement indices are allowed. Indexes
                    follow the python convention where -1 is the last measurement in a
                    circuit and 0 is the first measurement in a circuit.

            Examples:
                >>> import stim

                >>> stim.Flow("X2 -> -Y2*Z4 xor rec[-1]")
                stim.Flow("__X -> -__Y_Z xor rec[-1]")

                >>> stim.Flow("Z -> 1 xor rec[-1]")
                stim.Flow("Z -> rec[-1]")

                >>> stim.Flow(
                ...     input=stim.PauliString("XX"),
                ...     output=stim.PauliString("_X"),
                ...     measurements=[],
                ... )
                stim.Flow("XX -> _X")
        )DOC")
            .data());

    c.def(
        "input_copy",
        [](const Flow<MAX_BITWORD_WIDTH> &self) -> FlexPauliString {
            return FlexPauliString{self.input, false};
        },
        clean_doc_string(R"DOC(
            Returns a copy of the flow's input stabilizer.

            Examples:
                >>> import stim
                >>> f = stim.Flow(input=stim.PauliString('XX'))
                >>> f.input_copy()
                stim.PauliString("+XX")

                >>> f.input_copy() is f.input_copy()
                False
        )DOC")
            .data());

    c.def(
        "output_copy",
        [](const Flow<MAX_BITWORD_WIDTH> &self) -> FlexPauliString {
            return FlexPauliString{self.output, false};
        },
        clean_doc_string(R"DOC(
            Returns a copy of the flow's output stabilizer.

            Examples:
                >>> import stim
                >>> f = stim.Flow(output=stim.PauliString('XX'))
                >>> f.output_copy()
                stim.PauliString("+XX")

                >>> f.output_copy() is f.output_copy()
                False
        )DOC")
            .data());

    c.def(
        "measurements_copy",
        [](const Flow<MAX_BITWORD_WIDTH> &self) -> std::vector<int32_t> {
            return self.measurements;
        },
        clean_doc_string(R"DOC(
            Returns a copy of the flow's measurement indices.

            Examples:
                >>> import stim
                >>> f = stim.Flow(measurements=[-1, 2])
                >>> f.measurements_copy()
                [-1, 2]

                >>> f.measurements_copy() is f.measurements_copy()
                False
        )DOC")
            .data());

    c.def(pybind11::self == pybind11::self, "Determines if two flows have identical contents.");
    c.def(pybind11::self != pybind11::self, "Determines if two flows have non-identical contents.");
    c.def("__str__", &Flow<MAX_BITWORD_WIDTH>::str, "Returns a shorthand description of the flow.");

    c.def(
        "__repr__",
        [](const Flow<MAX_BITWORD_WIDTH> &self) {
            return "stim.Flow(\"" + self.str() + "\")";
        },
        "Returns valid python code evaluating to an equivalent `stim.Flow`.");
}
