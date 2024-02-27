#ifndef _STIM_SEARCH_SAT_WCNF_H
#define _STIM_SEARCH_SAT_WCNF_H

#include <cstdint>

#include "stim/dem/detector_error_model.h"

namespace stim {

/// Generates a maxSAT problem instance in .wcnf format from a DetectorErrorModel, such that the optimal value of the instance corresponds to the minimum distance of the protocol.
///
/// The .wcnf (weighted CNF) file format is widely
/// accepted by numerous maxSAT solvers. For example, the solvers in the 2023 maxSAT competition: https://maxsat-evaluations.github.io/2023/descriptions.html
/// Note that erformance can greatly vary among solvers.
/// The conversion involves encoding XOR constraints into CNF clauses using standard techniques.
///
/// Args:
///     model: The detector error model to be converted into .wcnf format for minimum distance calculation.
///     num_distinct_weights: The number of different integer weight values for quantization (default = 1).
///
/// Returns:
///     A string which is interpreted as the contents of a .wcnf file. This should be written to a file which can then be passed to various maxSAT
///     solvers to determine the minimum distance of the protocol represented by the model. The optimal value found
///     by the solver corresponds to the minimum distance of the error correction protocol. In other words, the smallest number of errors that cause a logical observable flip without any detection events.
///
/// Note:
///     The use of .wcnf format offers significant flexibility in choosing a maxSAT solver, but it also means that
///     users must separately manage the process of selecting and running the solver. This approach is designed to
///     sidestep the need for direct integration with any particular solver and allow
///     for experimentation with different solvers to achieve the best performance.
std::string shortest_undetectable_logical_error_wcnf(const DetectorErrorModel &model, size_t num_distinct_weights=1);

}  // namespace stim

#endif
