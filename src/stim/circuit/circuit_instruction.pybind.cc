#include "stim/circuit/circuit_instruction.pybind.h"

#include "stim/circuit/gate_target.pybind.h"
#include "stim/gates/gates.h"
#include "stim/py/base.pybind.h"
#include "stim/util_bot/str_util.h"

using namespace stim;
using namespace stim_pybind;

PyCircuitInstruction::PyCircuitInstruction(
    std::string_view name,
    std::span<pybind11::object> init_targets,
    std::span<double> init_gate_args,
    pybind11::str tag)
    : gate_type(GATE_DATA.at(name).id), tag(tag) {
    for (const auto &obj : init_gate_args) {
        gate_args.push_back(obj);
    }
    for (const auto &obj : init_targets) {
        targets.push_back(obj_to_gate_target(obj));
    }

    as_operation_ref().validate();
}
PyCircuitInstruction::PyCircuitInstruction(
    GateType gate_type, std::vector<GateTarget> targets, std::vector<double> gate_args, pybind11::str tag)
    : gate_type(gate_type), targets(targets), gate_args(gate_args), tag(tag) {
    as_operation_ref().validate();
}

PyCircuitInstruction PyCircuitInstruction::from_str(std::string_view text) {
    Circuit host;
    host.append_from_text(text);
    if (host.operations.size() != 1 || host.operations[0].gate_type == GateType::REPEAT) {
        throw std::invalid_argument("Given text didn't parse to a single CircuitInstruction.");
    }
    return PyCircuitInstruction::from_instruction(host.operations[0]);
}

PyCircuitInstruction PyCircuitInstruction::from_instruction(CircuitInstruction instruction) {
    std::vector<double> arguments;
    std::vector<GateTarget> targets;
    arguments.insert(arguments.begin(), instruction.args.begin(), instruction.args.end());
    targets.insert(targets.begin(), instruction.targets.begin(), instruction.targets.end());
    return PyCircuitInstruction{
        instruction.gate_type,
        targets,
        arguments,
        instruction.tag,
    };
}

bool PyCircuitInstruction::operator==(const PyCircuitInstruction &other) const {
    return gate_type == other.gate_type && targets == other.targets && gate_args == other.gate_args &&
           pybind11::cast<std::string_view>(tag) == pybind11::cast<std::string_view>(other.tag);
}
bool PyCircuitInstruction::operator!=(const PyCircuitInstruction &other) const {
    return !(*this == other);
}

std::string PyCircuitInstruction::repr() const {
    std::stringstream result;
    result << "stim.CircuitInstruction('" << name() << "', [";
    bool first = true;
    for (const auto &t : targets) {
        if (first) {
            first = false;
        } else {
            result << ", ";
        }
        result << t.repr();
    }
    result << "], [" << comma_sep(gate_args) << "]";
    if (pybind11::cast<bool>(pybind11::bool_(tag))) {
        result << ", tag=";
        result << pybind11::repr(tag);
    }
    result << ")";
    return result.str();
}

std::string PyCircuitInstruction::str() const {
    std::stringstream result;
    result << as_operation_ref();
    return result.str();
}

CircuitInstruction PyCircuitInstruction::as_operation_ref() const {
    return CircuitInstruction{gate_type, gate_args, targets, pybind11::cast<std::string_view>(tag)};
}
PyCircuitInstruction::operator CircuitInstruction() const {
    return as_operation_ref();
}
std::string PyCircuitInstruction::name() const {
    return std::string(GATE_DATA[gate_type].name);
}
std::vector<uint32_t> PyCircuitInstruction::raw_targets() const {
    std::vector<uint32_t> result;
    for (const auto &t : targets) {
        result.push_back(t.data);
    }
    return result;
}

std::vector<GateTarget> PyCircuitInstruction::targets_copy() const {
    return targets;
}
std::vector<double> PyCircuitInstruction::gate_args_copy() const {
    return gate_args;
}
std::vector<std::vector<stim::GateTarget>> PyCircuitInstruction::target_groups() const {
    std::vector<std::vector<stim::GateTarget>> results;
    as_operation_ref().for_combined_target_groups([&](std::span<const GateTarget> group) {
        std::vector<stim::GateTarget> copy;
        for (auto g : group) {
            if (!g.is_combiner()) {
                copy.push_back(g);
            }
        }
        results.push_back(std::move(copy));
    });
    return results;
}

pybind11::class_<PyCircuitInstruction> stim_pybind::pybind_circuit_instruction(pybind11::module &m) {
    return pybind11::class_<PyCircuitInstruction>(
        m,
        "CircuitInstruction",
        clean_doc_string(R"DOC(
            An instruction, like `H 0 1` or `CNOT rec[-1] 5`, from a circuit.

            Examples:
                >>> import stim
                >>> circuit = stim.Circuit('''
                ...     H 0
                ...     M 0 1
                ...     X_ERROR(0.125) 5
                ... ''')
                >>> circuit[0]
                stim.CircuitInstruction('H', [stim.GateTarget(0)], [])
                >>> circuit[1]
                stim.CircuitInstruction('M', [stim.GateTarget(0), stim.GateTarget(1)], [])
                >>> circuit[2]
                stim.CircuitInstruction('X_ERROR', [stim.GateTarget(5)], [0.125])
        )DOC")
            .data());
}
void stim_pybind::pybind_circuit_instruction_methods(pybind11::module &m, pybind11::class_<PyCircuitInstruction> &c) {
    c.def(
        pybind11::init(
            [](std::string_view name, pybind11::object targets, pybind11::object gate_args, pybind11::str tag)
                -> PyCircuitInstruction {
                if (targets.is_none() and gate_args.is_none() && !pybind11::cast<bool>(pybind11::bool_(tag))) {
                    return PyCircuitInstruction::from_str(name);
                }
                std::vector<double> conv_args;
                std::vector<pybind11::object> conv_targets;
                if (!gate_args.is_none()) {
                    conv_args = pybind11::cast<std::vector<double>>(gate_args);
                }
                if (!targets.is_none()) {
                    conv_targets = pybind11::cast<std::vector<pybind11::object>>(targets);
                }
                return PyCircuitInstruction(name, conv_targets, conv_args, tag);
            }),
        pybind11::arg("name"),
        pybind11::arg("targets") = pybind11::none(),
        pybind11::arg("gate_args") = pybind11::none(),
        pybind11::kw_only(),
        pybind11::arg("tag") = "",
        clean_doc_string(R"DOC(
            @signature def __init__(self, name: str, targets: Optional[Iterable[Union[int, stim.GateTarget]]] = None, gate_args: Optional[Iterable[float]] = None, *, tag: str = "") -> None:
            Creates or parses a `stim.CircuitInstruction`.

            Args:
                name: The name of the instruction being applied.
                    If `targets` and `gate_args` aren't specified, this can be a full
                    instruction line from a stim Circuit file, like "CX 0 1".
                targets: The targets the instruction is being applied to. These can be raw
                    values like `0` and `stim.target_rec(-1)`, or instances of
                    `stim.GateTarget`.
                gate_args: The sequence of numeric arguments parameterizing a gate. For
                    noise gates this is their probabilities. For `OBSERVABLE_INCLUDE`
                    instructions it's the index of the logical observable to affect.
                tag: Defaults to "". A custom string attached to the instruction. For
                    example, for a TICK instruction, this could a string specifying an
                    amount of time which is used by custom code for adding noise to a
                    circuit. In general, stim will attempt to propagate tags across circuit
                    transformations but will otherwise completely ignore them.

            Examples:
                >>> import stim

                >>> print(stim.CircuitInstruction('DEPOLARIZE1', [5], [0.25]))
                DEPOLARIZE1(0.25) 5

                >>> stim.CircuitInstruction('CX rec[-1] 5  # comment')
                stim.CircuitInstruction('CX', [stim.target_rec(-1), stim.GateTarget(5)], [])

                >>> print(stim.CircuitInstruction('I', [2], tag='100ns'))
                I[100ns] 2
        )DOC")
            .data());

    c.def_property_readonly(
        "name",
        &PyCircuitInstruction::name,
        clean_doc_string(R"DOC(
            The name of the instruction (e.g. `H` or `X_ERROR` or `DETECTOR`).
        )DOC")
            .data());

    c.def_property_readonly(
        "tag",
        [](PyCircuitInstruction &self) -> pybind11::str {
            return self.tag;
        },
        clean_doc_string(R"DOC(
            The custom tag attached to the instruction.

            The tag is an arbitrary string.
            The default tag, when none is specified, is the empty string.

            Examples:
                >>> import stim
                >>> stim.Circuit("H[test] 0")[0].tag
                'test'
                >>> stim.Circuit("H 0")[0].tag
                ''
        )DOC")
            .data());

    c.def(
        "target_groups",
        &PyCircuitInstruction::target_groups,
        clean_doc_string(R"DOC(
            @signature def target_groups(self) -> List[List[stim.GateTarget]]:
            Splits the instruction's targets into groups depending on the type of gate.

            Single qubit gates like H get one group per target.
            Two qubit gates like CX get one group per pair of targets.
            Pauli product gates like MPP get one group per combined product.

            Returns:
                A list of groups of targets.

            Examples:
                >>> import stim
                >>> for g in stim.Circuit('H 0 1 2')[0].target_groups():
                ...     print(repr(g))
                [stim.GateTarget(0)]
                [stim.GateTarget(1)]
                [stim.GateTarget(2)]

                >>> for g in stim.Circuit('CX 0 1 2 3')[0].target_groups():
                ...     print(repr(g))
                [stim.GateTarget(0), stim.GateTarget(1)]
                [stim.GateTarget(2), stim.GateTarget(3)]

                >>> for g in stim.Circuit('MPP X0*Y1*Z2 X5*X6')[0].target_groups():
                ...     print(repr(g))
                [stim.target_x(0), stim.target_y(1), stim.target_z(2)]
                [stim.target_x(5), stim.target_x(6)]

                >>> for g in stim.Circuit('DETECTOR rec[-1] rec[-2]')[0].target_groups():
                ...     print(repr(g))
                [stim.target_rec(-1)]
                [stim.target_rec(-2)]

                >>> for g in stim.Circuit('CORRELATED_ERROR(0.1) X0 Y1')[0].target_groups():
                ...     print(repr(g))
                [stim.target_x(0), stim.target_y(1)]
        )DOC")
            .data());

    c.def(
        "targets_copy",
        &PyCircuitInstruction::targets_copy,
        clean_doc_string(R"DOC(
            Returns a copy of the targets of the instruction.

            Examples:
                >>> import stim
                >>> instruction = stim.CircuitInstruction('X_ERROR', [2, 3], [0.125])
                >>> instruction.targets_copy()
                [stim.GateTarget(2), stim.GateTarget(3)]

                >>> instruction.targets_copy() == instruction.targets_copy()
                True
                >>> instruction.targets_copy() is instruction.targets_copy()
                False
        )DOC")
            .data());

    c.def(
        "gate_args_copy",
        &PyCircuitInstruction::gate_args_copy,
        clean_doc_string(R"DOC(
            Returns the gate's arguments (numbers parameterizing the instruction).

            For noisy gates this typically a list of probabilities.
            For OBSERVABLE_INCLUDE it's a singleton list containing the logical observable
            index.

            Examples:
                >>> import stim
                >>> instruction = stim.CircuitInstruction('X_ERROR', [2, 3], [0.125])
                >>> instruction.gate_args_copy()
                [0.125]

                >>> instruction.gate_args_copy() == instruction.gate_args_copy()
                True
                >>> instruction.gate_args_copy() is instruction.gate_args_copy()
                False
        )DOC")
            .data());

    c.def_property_readonly(
        "num_measurements",
        [](const PyCircuitInstruction &self) -> uint64_t {
            return self.as_operation_ref().count_measurement_results();
        },
        clean_doc_string(R"DOC(
            Returns the number of bits produced when running this instruction.

            Examples:
                >>> import stim
                >>> stim.CircuitInstruction('H', [0]).num_measurements
                0
                >>> stim.CircuitInstruction('M', [0]).num_measurements
                1
                >>> stim.CircuitInstruction('M', [2, 3, 5, 7, 11]).num_measurements
                5
                >>> stim.CircuitInstruction('MXX', [0, 1, 4, 5, 11, 13]).num_measurements
                3
                >>> stim.Circuit('MPP X0*X1 X0*Z1*Y2')[0].num_measurements
                2
                >>> stim.CircuitInstruction('HERALDED_ERASE', [0], [0.25]).num_measurements
                1
        )DOC")
            .data());

    c.def(pybind11::self == pybind11::self, "Determines if two `stim.CircuitInstruction`s are identical.");
    c.def(pybind11::self != pybind11::self, "Determines if two `stim.CircuitInstruction`s are different.");
    c.def(
        "__repr__",
        &PyCircuitInstruction::repr,
        "Returns text that is a valid python expression evaluating to an equivalent `stim.CircuitInstruction`.");
    c.def(
        "__str__",
        &PyCircuitInstruction::str,
        "Returns a text description of the instruction as a stim circuit file line.");

    c.def("__hash__", [](const PyCircuitInstruction &self) {
        return pybind11::hash(pybind11::str(self.str()));
    });
}
