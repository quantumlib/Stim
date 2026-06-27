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

CircuitRepeatBlock::CircuitRepeatBlock(uint64_t repeat_count, stim::Circuit body, pybind11::str tag)
    : repeat_count(repeat_count), body(body), tag(tag) {
    if (repeat_count == 0) {
        throw std::invalid_argument("Can't repeat 0 times.");
    }
}

Circuit CircuitRepeatBlock::body_copy() {
    return body;
}
bool CircuitRepeatBlock::operator==(const CircuitRepeatBlock &other) const {
    return repeat_count == other.repeat_count && body == other.body &&
           pybind11::cast<std::string_view>(tag) == pybind11::cast<std::string_view>(other.tag);
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
        clean_doc_string(R"DOC(
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
        pybind11::init<uint64_t, Circuit, pybind11::str>(),
        pybind11::arg("repeat_count"),
        pybind11::arg("body"),
        pybind11::kw_only(),
        pybind11::arg("tag") = "",
        clean_doc_string(R"DOC(
            Initializes a `stim.CircuitRepeatBlock`.

            Args:
                repeat_count: The number of times to repeat the block.
                body: The body of the block, as a circuit.
                tag: Defaults to empty. A custom string attached to the REPEAT instruction.

            Examples:
                >>> import stim
                >>> c = stim.Circuit()
                >>> c.append(stim.CircuitRepeatBlock(100, stim.Circuit("M 0")))
                >>> c
                stim.Circuit('''
                    REPEAT 100 {
                        M 0
                    }
                ''')
        )DOC")
            .data());

    c.def_readonly(
        "repeat_count",
        &CircuitRepeatBlock::repeat_count,
        clean_doc_string(R"DOC(
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

    c.def_property_readonly(
        "name",
        [](const CircuitRepeatBlock &self) -> pybind11::str {
            return pybind11::cast("REPEAT");
        },
        clean_doc_string(R"DOC(
            Returns the name "REPEAT".

            This is a duck-typing convenience method. It exists so that code that doesn't
            know whether it has a `stim.CircuitInstruction` or a `stim.CircuitRepeatBlock`
            can check the object's name without having to do an `instanceof` check first.

            Examples:
                >>> import stim
                >>> circuit = stim.Circuit('''
                ...     H 0
                ...     REPEAT 5 {
                ...         CX 1 2
                ...     }
                ...     S 1
                ... ''')
                >>> [instruction.name for instruction in circuit]
                ['H', 'REPEAT', 'S']
        )DOC")
            .data());

    c.def_property_readonly(
        "num_measurements",
        [](const CircuitRepeatBlock &self) -> uint64_t {
            return self.body.count_measurements() * self.repeat_count;
        },
        clean_doc_string(R"DOC(
            Returns the number of bits produced when running this loop.

            Examples:
                >>> import stim
                >>> stim.CircuitRepeatBlock(
                ...     body=stim.Circuit("M 0 1"),
                ...     repeat_count=25,
                ... ).num_measurements
                50
        )DOC")
            .data());

    c.def_readonly(
        "repeat_count",
        &CircuitRepeatBlock::repeat_count,
        clean_doc_string(R"DOC(
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

    c.def_property_readonly(
        "tag",
        [](CircuitRepeatBlock &self) -> pybind11::str {
            return self.tag;
        },
        clean_doc_string(R"DOC(
            The custom tag attached to the REPEAT instruction.

            The tag is an arbitrary string.
            The default tag, when none is specified, is the empty string.

            Examples:
                >>> import stim

                >>> stim.Circuit('''
                ...     REPEAT[test] 5 {
                ...         H 0
                ...     }
                ... ''')[0].tag
                'test'

                >>> stim.Circuit('''
                ...     REPEAT 5 {
                ...         H 0
                ...     }
                ... ''')[0].tag
                ''
        )DOC")
            .data());

    c.def(
        "body_copy",
        &CircuitRepeatBlock::body_copy,
        clean_doc_string(R"DOC(
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
