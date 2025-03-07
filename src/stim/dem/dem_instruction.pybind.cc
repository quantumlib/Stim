#include "stim/dem/dem_instruction.pybind.h"

#include "stim/dem/detector_error_model_target.pybind.h"
#include "stim/py/base.pybind.h"
#include "stim/util_bot/str_util.h"

using namespace stim;
using namespace stim_pybind;

std::vector<std::vector<ExposedDemTarget>> ExposedDemInstruction::target_groups() const {
    std::vector<std::vector<ExposedDemTarget>> result;
    as_dem_instruction().for_separated_targets([&](std::span<const DemTarget> group) {
        std::vector<ExposedDemTarget> copy;
        for (auto e : group) {
            copy.push_back(e);
        }
        result.push_back(copy);
    });
    return result;
}

DemInstruction ExposedDemInstruction::as_dem_instruction() const {
    return DemInstruction{arguments, targets, tag, type};
}

ExposedDemInstruction ExposedDemInstruction::from_dem_instruction(stim::DemInstruction instruction) {
    std::vector<double> arguments;
    std::vector<DemTarget> targets;
    arguments.insert(arguments.begin(), instruction.arg_data.begin(), instruction.arg_data.end());
    targets.insert(targets.begin(), instruction.target_data.begin(), instruction.target_data.end());
    return ExposedDemInstruction{arguments, targets, std::string(instruction.tag), instruction.type};
}

ExposedDemInstruction ExposedDemInstruction::from_str(std::string_view text) {
    DetectorErrorModel host;
    host.append_from_text(text);
    if (host.instructions.size() != 1 || host.instructions[0].type == DemInstructionType::DEM_REPEAT_BLOCK) {
        throw std::invalid_argument("Given text didn't parse to a single DemInstruction.");
    }
    return from_dem_instruction(host.instructions[0]);
}

std::string ExposedDemInstruction::type_name() const {
    std::stringstream out;
    out << type;
    return out.str();
}

std::string ExposedDemInstruction::str() const {
    return as_dem_instruction().str();
}

std::string ExposedDemInstruction::repr() const {
    std::stringstream out;
    out << "stim.DemInstruction('" << type << "', [";
    out << comma_sep(arguments);
    out << "], [";
    bool first = true;
    for (const auto &e : targets) {
        if (first) {
            first = false;
        } else {
            out << ", ";
        }
        if (type == DemInstructionType::DEM_SHIFT_DETECTORS) {
            out << e.data;
        } else if (e.is_relative_detector_id()) {
            out << "stim.target_relative_detector_id(" << e.raw_id() << ")";
        } else if (e.is_separator()) {
            out << "stim.target_separator()";
        } else {
            out << "stim.target_logical_observable_id(" << e.raw_id() << ")";
        }
    }
    out << "]";
    if (!tag.empty()) {
        out << ", tag=" << pybind11::cast<std::string>(pybind11::repr(pybind11::cast(tag)));
    }
    out << ")";
    return out.str();
}

bool ExposedDemInstruction::operator==(const ExposedDemInstruction &other) const {
    return type == other.type && arguments == other.arguments && targets == other.targets && tag == other.tag;
}
bool ExposedDemInstruction::operator!=(const ExposedDemInstruction &other) const {
    return !(*this == other);
}
std::vector<pybind11::object> ExposedDemInstruction::targets_copy() const {
    std::vector<pybind11::object> result;
    if (type == DemInstructionType::DEM_SHIFT_DETECTORS) {
        for (const auto &e : targets) {
            result.push_back(pybind11::cast(e.data));
        }
    } else {
        for (const auto &e : targets) {
            result.push_back(pybind11::cast(ExposedDemTarget{e}));
        }
    }
    return result;
}
std::vector<double> ExposedDemInstruction::args_copy() const {
    return arguments;
}

pybind11::class_<ExposedDemInstruction> stim_pybind::pybind_detector_error_model_instruction(pybind11::module &m) {
    return pybind11::class_<ExposedDemInstruction>(
        m,
        "DemInstruction",
        clean_doc_string(R"DOC(
            An instruction from a detector error model.

            Examples:
                >>> import stim
                >>> model = stim.DetectorErrorModel('''
                ...     error(0.125) D0
                ...     error(0.125) D0 D1 L0
                ...     error(0.125) D1 D2
                ...     error(0.125) D2 D3
                ...     error(0.125) D3
                ... ''')
                >>> instruction = model[0]
                >>> instruction
                stim.DemInstruction('error', [0.125], [stim.target_relative_detector_id(0)])
        )DOC")
            .data());
}
void stim_pybind::pybind_detector_error_model_instruction_methods(
    pybind11::module &m, pybind11::class_<ExposedDemInstruction> &c) {
    c.def(
        pybind11::init(
            [](std::string_view type, pybind11::object &arguments, pybind11::object &targets, std::string_view tag) -> ExposedDemInstruction {
                if (arguments.is_none() && targets.is_none()) {
                    return ExposedDemInstruction::from_str(type);
                }

                std::string lower;
                for (char c : type) {
                    lower.push_back(tolower(c));
                }
                DemInstructionType conv_type;
                std::vector<DemTarget> conv_targets;
                if (lower == "error") {
                    conv_type = DemInstructionType::DEM_ERROR;
                } else if (lower == "shift_detectors") {
                    conv_type = DemInstructionType::DEM_SHIFT_DETECTORS;
                } else if (lower == "detector") {
                    conv_type = DemInstructionType::DEM_DETECTOR;
                } else if (lower == "logical_observable") {
                    conv_type = DemInstructionType::DEM_LOGICAL_OBSERVABLE;
                } else {
                    throw std::invalid_argument("Unrecognized instruction name '" + lower + "'.");
                }
                if (!targets.is_none()) {
                    if (conv_type == DemInstructionType::DEM_SHIFT_DETECTORS) {
                        for (const auto &e : targets) {
                            try {
                                conv_targets.push_back(DemTarget{pybind11::cast<uint64_t>(e)});
                            } catch (pybind11::cast_error &) {
                                throw std::invalid_argument(
                                    "Instruction '" + lower + "' only takes unsigned integer targets.");
                            }
                        }
                    } else {
                        for (const auto &e : targets) {
                            try {
                                conv_targets.push_back(pybind11::cast<ExposedDemTarget>(e).internal());
                            } catch (pybind11::cast_error &) {
                                throw std::invalid_argument(
                                    "Instruction '" + lower +
                                    "' only takes stim.target_relative_detector_id(k), "
                                    "stim.target_logical_observable_id(k), "
                                    "stim.target_separator() targets.");
                            }
                        }
                    }
                }

                std::vector<double> conv_args;
                if (!arguments.is_none()) {
                    conv_args = pybind11::cast<std::vector<double>>(arguments);
                }
                ExposedDemInstruction result{std::move(conv_args), std::move(conv_targets), std::string(tag), conv_type};
                result.as_dem_instruction().validate();
                return result;
            }),
        pybind11::arg("type"),
        pybind11::arg("args") = pybind11::none(),
        pybind11::arg("targets") = pybind11::none(),
        pybind11::kw_only(),
        pybind11::arg("tag") = "",
        clean_doc_string(R"DOC(
            @signature def __init__(self, type: str, args: Optional[Iterable[float]] = None, targets: Optional[Iterable[stim.DemTarget]] = None, *, tag: str = "") -> None:
            Creates or parses a stim.DemInstruction.

            Args:
                type: The name of the instruction type (e.g. "error" or "shift_detectors").
                    If `args` and `targets` aren't specified, this can also be set to a
                    full line of text from a dem file, like "error(0.25) D0".
                args: Numeric values parameterizing the instruction (e.g. the 0.1 in
                    "error(0.1)").
                targets: The objects the instruction involves (e.g. the "D0" and "L1" in
                    "error(0.1) D0 L1").
                tag: An arbitrary piece of text attached to the instruction.

            Examples:
                >>> import stim
                >>> instruction = stim.DemInstruction(
                ...     'error',
                ...     [0.125],
                ...     [stim.target_relative_detector_id(5)],
                ...     tag='test-tag',
                ... )
                >>> print(instruction)
                error[test-tag](0.125) D5

                >>> print(stim.DemInstruction('error(0.125) D5 L6 ^ D4  # comment'))
                error(0.125) D5 L6 ^ D4
        )DOC")
            .data());

    c.def(
        "args_copy",
        &ExposedDemInstruction::args_copy,
        clean_doc_string(R"DOC(
            @signature def args_copy(self) -> List[float]:
            Returns a copy of the list of numbers parameterizing the instruction.

            For example, this would be coordinates of a detector instruction or the
            probability of an error instruction. The result is a copy, meaning that
            editing it won't change the instruction's targets or future copies.

            Examples:
                >>> import stim
                >>> instruction = stim.DetectorErrorModel('''
                ...     error(0.125) D0
                ... ''')[0]
                >>> instruction.args_copy()
                [0.125]

                >>> instruction.args_copy() == instruction.args_copy()
                True
                >>> instruction.args_copy() is instruction.args_copy()
                False
        )DOC")
            .data());

    c.def_readonly(
        "tag",
        &ExposedDemInstruction::tag,
        clean_doc_string(R"DOC(
            Returns the arbitrary text tag attached to the instruction.

            Examples:
                >>> import stim
                >>> dem = stim.DetectorErrorModel('''
                ...     error[test-tag](0.125) D0
                ...     error(0.125) D0
                ... ''')
                >>> dem[0].tag
                'test-tag'
                >>> dem[1].tag
                ''
        )DOC")
            .data());

    c.def(
        "target_groups",
        &ExposedDemInstruction::target_groups,
        clean_doc_string(R"DOC(
            @signature def target_groups(self) -> List[List[stim.DemTarget]]:
            Returns a copy of the instruction's targets, split by target separators.

            When a detector error model instruction contains a suggested decomposition,
            its targets contain separators (`stim.DemTarget("^")`). This method splits the
            targets into groups based the separators, similar to how `str.split` works.

            Returns:
                A list of groups of targets.

            Examples:
                >>> import stim
                >>> dem = stim.DetectorErrorModel('''
                ...     error(0.01) D0 D1 ^ D2
                ...     error(0.01) D0 L0
                ...     error(0.01)
                ... ''')

                >>> dem[0].target_groups()
                [[stim.DemTarget('D0'), stim.DemTarget('D1')], [stim.DemTarget('D2')]]

                >>> dem[1].target_groups()
                [[stim.DemTarget('D0'), stim.DemTarget('L0')]]

                >>> dem[2].target_groups()
                [[]]
        )DOC")
            .data());

    c.def(
        "targets_copy",
        &ExposedDemInstruction::targets_copy,
        clean_doc_string(R"DOC(
            @signature def targets_copy(self) -> List[Union[int, stim.DemTarget]]:
            Returns a copy of the instruction's targets.

            The result is a copy, meaning that editing it won't change the instruction's
            targets or future copies.

            Examples:
                >>> import stim
                >>> instruction = stim.DetectorErrorModel('''
                ...     error(0.125) D0 L2
                ... ''')[0]
                >>> instruction.targets_copy()
                [stim.DemTarget('D0'), stim.DemTarget('L2')]

                >>> instruction.targets_copy() == instruction.targets_copy()
                True
                >>> instruction.targets_copy() is instruction.targets_copy()
                False
        )DOC")
            .data());
    c.def_property_readonly(
        "type",
        &ExposedDemInstruction::type_name,
        clean_doc_string(R"DOC(
            The name of the instruction type (e.g. "error" or "shift_detectors").
        )DOC")
            .data());
    c.def(pybind11::self == pybind11::self, "Determines if two instructions have identical contents.");
    c.def(pybind11::self != pybind11::self, "Determines if two instructions have non-identical contents.");

    c.def(
        "__str__",
        &ExposedDemInstruction::str,
        "Returns detector error model (.dem) instructions (that can be parsed by stim) for the model.");
    c.def(
        "__repr__",
        &ExposedDemInstruction::repr,
        "Returns text that is a valid python expression evaluating to an equivalent `stim.DetectorErrorModel`.");

    c.def("__hash__", [](const ExposedDemInstruction &self) {
        return pybind11::hash(pybind11::str(self.str()));
    });
}
