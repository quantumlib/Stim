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

#include "stim/circuit/circuit_repeat_block.pybind.h"

#include "stim/circuit/circuit.h"
#include "stim/circuit/circuit.pybind.h"
#include "stim/circuit/circuit_instruction.pybind.h"
#include "stim/py/base.pybind.h"

using namespace stim;
using namespace stim_pybind;

CircuitRepeatBlock::CircuitRepeatBlock(uint64_t repeat_count, stim::Circuit body)
    : repeat_count(repeat_count), body(body) {
    if (repeat_count == 0) {
        throw std::invalid_argument("Can't repeat 0 times.");
    }
}

Circuit CircuitRepeatBlock::body_copy() {
    return body;
}
bool CircuitRepeatBlock::operator==(const CircuitRepeatBlock &other) const {
    return repeat_count == other.repeat_count && body == other.body;
}
bool CircuitRepeatBlock::operator!=(const CircuitRepeatBlock &other) const {
    return !(*this == other);
}
std::string CircuitRepeatBlock::repr() const {
    return "stim.CircuitRepeatBlock(" + std::to_string(repeat_count) + ", " + circuit_repr(body) + ")";
}

pybind11::class_<CircuitRepeatBlock> stim_pybind::pybind_circuit_repeat_block(pybind11::module &m) {
    return pybind11::class_<CircuitRepeatBlock>(
        m,
        "CircuitRepeatBlock",
        clean_doc_string(u8R"DOC(
            A REPEAT block from a circuit.

            Examples:
                >>> import stim
                >>> circuit = stim.Circuit('''
                ...     H 0
                ...     REPEAT 5 {
                ...         CX 0 1
                ...         CZ 1 2
                ...     }
                ... ''')
                >>> repeat_block = circuit[1]
                >>> repeat_block.repeat_count
                5
                >>> repeat_block.body_copy()
                stim.Circuit('''
                    CX 0 1
                    CZ 1 2
                ''')
        )DOC")
            .data());
}

void stim_pybind::pybind_circuit_repeat_block_methods(pybind11::module &m, pybind11::class_<CircuitRepeatBlock> &c) {

    c.def(
        pybind11::init<uint64_t, Circuit>(),
        pybind11::arg("repeat_count"),
        pybind11::arg("body"),
        clean_doc_string(u8R"DOC(
            Initializes a `stim.CircuitRepeatBlock`.

            Args:
                repeat_count: The number of times to repeat the block.
                body: The body of the block, as a circuit.
        )DOC")
            .data());

    c.def_readonly(
        "repeat_count",
        &CircuitRepeatBlock::repeat_count,
        clean_doc_string(u8R"DOC(
            The repetition count of the repeat block.

            Examples:
                >>> import stim
                >>> circuit = stim.Circuit('''
                ...     H 0
                ...     REPEAT 5 {
                ...         CX 0 1
                ...         CZ 1 2
                ...     }
                ... ''')
                >>> repeat_block = circuit[1]
                >>> repeat_block.repeat_count
                5
        )DOC")
            .data());

    c.def(
        "body_copy",
        &CircuitRepeatBlock::body_copy,
        clean_doc_string(u8R"DOC(
            Returns a copy of the body of the repeat block.

            (Making a copy is enforced to make it clear that editing the result won't change
            the block's body.)

            Examples:
                >>> import stim
                >>> circuit = stim.Circuit('''
                ...     H 0
                ...     REPEAT 5 {
                ...         CX 0 1
                ...         CZ 1 2
                ...     }
                ... ''')
                >>> repeat_block = circuit[1]
                >>> repeat_block.body_copy()
                stim.Circuit('''
                    CX 0 1
                    CZ 1 2
                ''')
        )DOC")
            .data());

    c.def(pybind11::self == pybind11::self, "Determines if two `stim.CircuitRepeatBlock`s are identical.");
    c.def(pybind11::self != pybind11::self, "Determines if two `stim.CircuitRepeatBlock`s are different.");
    c.def(
        "__repr__",
        &CircuitRepeatBlock::repr,
        "Returns valid python code evaluating to an equivalent `stim.CircuitRepeatBlock`.");
}
