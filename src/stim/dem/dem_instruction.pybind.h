#ifndef _STIM_DEM_DETECTOR_ERROR_MODEL_INSTRUCTION_PYBIND_H
#define _STIM_DEM_DETECTOR_ERROR_MODEL_INSTRUCTION_PYBIND_H

#include <pybind11/pybind11.h>

#include "stim/dem/detector_error_model_target.pybind.h"

namespace stim_pybind {

struct ExposedDemInstruction {
    std::vector<double> arguments;
    std::vector<stim::DemTarget> targets;
    std::string tag;
    stim::DemInstructionType type;

    static ExposedDemInstruction from_str(std::string_view text);
    static ExposedDemInstruction from_dem_instruction(stim::DemInstruction instruction);

    std::vector<std::vector<stim_pybind::ExposedDemTarget>> target_groups() const;
    std::vector<double> args_copy() const;
    std::vector<pybind11::object> targets_copy() const;
    stim::DemInstruction as_dem_instruction() const;
    std::string type_name() const;
    std::string str() const;
    std::string repr() const;
    bool operator==(const ExposedDemInstruction &other) const;
    bool operator!=(const ExposedDemInstruction &other) const;
};

pybind11::class_<ExposedDemInstruction> pybind_detector_error_model_instruction(pybind11::module &m);
void pybind_detector_error_model_instruction_methods(pybind11::module &m, pybind11::class_<ExposedDemInstruction> &c);

}  // namespace stim_pybind
#endif
