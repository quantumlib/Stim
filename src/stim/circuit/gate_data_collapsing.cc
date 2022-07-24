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

#include "stim/circuit/gate_data.h"
#include "stim/simulators/error_analyzer.h"
#include "stim/simulators/frame_simulator.h"
#include "stim/simulators/tableau_simulator.h"

using namespace stim;

void GateDataMap::add_gate_data_collapsing(bool &failed) {
    // ===================== Measure Gates. ============================
    add_gate(
        failed,
        Gate{
            "MX",
            ARG_COUNT_SYGIL_ZERO_OR_ONE,
            &TableauSimulator::measure_x,
            &FrameSimulator::measure_x,
            &ErrorAnalyzer::MX,
            (GateFlags)(GATE_PRODUCES_NOISY_RESULTS | GATE_ARGS_ARE_DISJOINT_PROBABILITIES),
            []() -> ExtraGateData {
                return {
                    "L_Collapsing Gates",
                    R"MARKDOWN(
X-basis measurement.
Projects each target qubit into `|+>` or `|->` and reports its value (false=`|+>`, true=`|->`).

Parens Arguments:

    If no parens argument is given, the measurement is perfect.
    If one parens argument is given, the measurement result is noisy.
    The argument is the probability of returning the wrong result.

Targets:

    The qubits to measure in the X basis.
    Prefixing a qubit target with `!` flips its reported measurement result.

Examples:

    # Measure qubit 5 in the X basis, and append the result into the measurement record.
    MX 5

    # Measure qubit 5 in the X basis, and append the INVERSE of its result into the measurement record.
    MX !5

    # Do a noisy measurement where the result put into the measurement record is wrong 1% of the time.
    MX(0.01) 5

    # Measure multiple qubits in the X basis, putting 3 bits into the measurement record.
    MX 2 3 5

    # Perform multiple noisy measurements. Each measurement fails independently with 2% probability.
    MX(0.02) 2 3 5
)MARKDOWN",
                    {},
                    {"X -> +m xor chance(p)", "X -> +X"},
                    R"CIRCUIT(
H 0
M 0
H 0
)CIRCUIT",
                };
            },
        });
    add_gate(
        failed,
        Gate{
            "MY",
            ARG_COUNT_SYGIL_ZERO_OR_ONE,
            &TableauSimulator::measure_y,
            &FrameSimulator::measure_y,
            &ErrorAnalyzer::MY,
            (GateFlags)(GATE_PRODUCES_NOISY_RESULTS | GATE_ARGS_ARE_DISJOINT_PROBABILITIES),
            []() -> ExtraGateData {
                return {
                    "L_Collapsing Gates",
                    R"MARKDOWN(
Y-basis measurement.
Projects each target qubit into `|i>` or `|-i>` and reports its value (false=`|i>`, true=`|-i>`).

Parens Arguments:

    If no parens argument is given, the measurement is perfect.
    If one parens argument is given, the measurement result is noisy.
    The argument is the probability of returning the wrong result.

Targets:

    The qubits to measure in the Y basis.
    Prefixing a qubit target with `!` flips its reported measurement result.

Examples:

    # Measure qubit 5 in the Y basis, and append the result into the measurement record.
    MY 5

    # Measure qubit 5 in the Y basis, and append the INVERSE of its result into the measurement record.
    MY !5

    # Do a noisy measurement where the result put into the measurement record is wrong 1% of the time.
    MY(0.01) 5

    # Measure multiple qubits in the X basis, putting 3 bits into the measurement record.
    MY 2 3 5

    # Perform multiple noisy measurements. Each measurement fails independently with 2% probability.
    MY(0.02) 2 3 5
)MARKDOWN",
                    {},
                    {"Y -> m xor chance(p)", "Y -> +Y"},
                    R"CIRCUIT(
S 0
S 0
S 0
H 0
M 0
H 0
S 0
)CIRCUIT",
                };
            },
        });
    add_gate(
        failed,
        Gate{
            "M",
            ARG_COUNT_SYGIL_ZERO_OR_ONE,
            &TableauSimulator::measure_z,
            &FrameSimulator::measure_z,
            &ErrorAnalyzer::MZ,
            (GateFlags)(GATE_PRODUCES_NOISY_RESULTS | GATE_ARGS_ARE_DISJOINT_PROBABILITIES),
            []() -> ExtraGateData {
                return {
                    "L_Collapsing Gates",
                    R"MARKDOWN(
Z-basis measurement.
Projects each target qubit into `|0>` or `|1>` and reports its value (false=`|0>`, true=`|1>`).

Parens Arguments:

    If no parens argument is given, the measurement is perfect.
    If one parens argument is given, the measurement result is noisy.
    The argument is the probability of returning the wrong result.

Targets:

    The qubits to measure in the Z basis.
    Prefixing a qubit target with `!` flips its reported measurement result.

Examples:

    # Measure qubit 5 in the Z basis, and append the result into the measurement record.
    M 5

    # 'MZ' is the same as 'M'. This also measures qubit 5 in the Z basis.
    MZ 5

    # Measure qubit 5 in the Z basis, and append the INVERSE of its result into the measurement record.
    MZ !5

    # Do a noisy measurement where the result put into the measurement record is wrong 1% of the time.
    MZ(0.01) 5

    # Measure multiple qubits in the Z basis, putting 3 bits into the measurement record.
    MZ 2 3 5

    # Perform multiple noisy measurements. Each measurement fails independently with 2% probability.
    MZ(0.02) 2 3 5
)MARKDOWN",
                    {},
                    {"Z -> m xor chance(p)", "Z -> +Z"},
                    R"CIRCUIT(
M 0
)CIRCUIT",
                };
            },
        });
    add_gate_alias(failed, "MZ", "M");

    // ===================== Measure+Reset Gates. ============================
    add_gate(
        failed,
        Gate{
            "MRX",
            ARG_COUNT_SYGIL_ZERO_OR_ONE,
            &TableauSimulator::measure_reset_x,
            &FrameSimulator::measure_reset_x,
            &ErrorAnalyzer::MRX,
            (GateFlags)(GATE_PRODUCES_NOISY_RESULTS | GATE_ARGS_ARE_DISJOINT_PROBABILITIES | GATE_IS_RESET),
            []() -> ExtraGateData {
                return {
                    "L_Collapsing Gates",
                    R"MARKDOWN(
X-basis demolition measurement (optionally noisy).
Projects each target qubit into `|+>` or `|->`, reports its value (false=`|+>`, true=`|->`), then resets to `|+>`.

Parens Arguments:

    If no parens argument is given, the demolition measurement is perfect.
    If one parens argument is given, the demolition measurement's result is noisy.
    The argument is the probability of returning the wrong result.
    The argument does not affect the fidelity of the reset.

Targets:

    The qubits to measure and reset in the X basis.
    Prefixing a qubit target with `!` flips its reported measurement result.

Examples:

    # Measure qubit 5 in the X basis, reset it to the |+> state, append the measurement result into the measurement record.
    MRX 5

    # Demolition measure qubit 5 in the X basis, but append the INVERSE of its result into the measurement record.
    MRX !5

    # Do a noisy demolition measurement where the result put into the measurement record is wrong 1% of the time.
    MRX(0.01) 5

    # Demolition measure multiple qubits in the X basis, putting 3 bits into the measurement record.
    MRX 2 3 5

    # Perform multiple noisy demolition measurements. Each measurement result is flipped independently with 2% probability.
    MRX(0.02) 2 3 5
)MARKDOWN",
                    {},
                    {"X -> m xor chance(p)", "1 -> +X"},
                    R"CIRCUIT(
H 0
M 0
R 0
H 0
)CIRCUIT",
                };
            },
        });
    add_gate(
        failed,
        Gate{
            "MRY",
            ARG_COUNT_SYGIL_ZERO_OR_ONE,
            &TableauSimulator::measure_reset_y,
            &FrameSimulator::measure_reset_y,
            &ErrorAnalyzer::MRY,
            (GateFlags)(GATE_PRODUCES_NOISY_RESULTS | GATE_ARGS_ARE_DISJOINT_PROBABILITIES | GATE_IS_RESET),
            []() -> ExtraGateData {
                return {
                    "L_Collapsing Gates",
                    R"MARKDOWN(
Y-basis demolition measurement (optionally noisy).
Projects each target qubit into `|i>` or `|-i>`, reports its value (false=`|i>`, true=`|-i>`), then resets to `|i>`.

Parens Arguments:

    If no parens argument is given, the demolition measurement is perfect.
    If one parens argument is given, the demolition measurement's result is noisy.
    The argument is the probability of returning the wrong result.
    The argument does not affect the fidelity of the reset.

Targets:

    The qubits to measure and reset in the Y basis.
    Prefixing a qubit target with `!` flips its reported measurement result.

Examples:

    # Measure qubit 5 in the Y basis, reset it to the |i> state, append the measurement result into the measurement record.
    MRY 5

    # Demolition measure qubit 5 in the Y basis, but append the INVERSE of its result into the measurement record.
    MRY !5

    # Do a noisy demolition measurement where the result put into the measurement record is wrong 1% of the time.
    MRY(0.01) 5

    # Demolition measure multiple qubits in the Y basis, putting 3 bits into the measurement record.
    MRY 2 3 5

    # Perform multiple noisy demolition measurements. Each measurement result is flipped independently with 2% probability.
    MRY(0.02) 2 3 5
)MARKDOWN",
                    {},
                    {"Y -> m xor chance(p)", "1 -> +Y"},
                    R"CIRCUIT(
S 0
S 0
S 0
H 0
R 0
M 0
H 0
S 0
)CIRCUIT",
                };
            },
        });
    add_gate(
        failed,
        Gate{
            "MR",
            ARG_COUNT_SYGIL_ZERO_OR_ONE,
            &TableauSimulator::measure_reset_z,
            &FrameSimulator::measure_reset_z,
            &ErrorAnalyzer::MRZ,
            (GateFlags)(GATE_PRODUCES_NOISY_RESULTS | GATE_ARGS_ARE_DISJOINT_PROBABILITIES | GATE_IS_RESET),
            []() -> ExtraGateData {
                return {
                    "L_Collapsing Gates",
                    R"MARKDOWN(
Z-basis demolition measurement (optionally noisy).
Projects each target qubit into `|0>` or `|1>`, reports its value (false=`|0>`, true=`|1>`), then resets to `|0>`.

Parens Arguments:

    If no parens argument is given, the demolition measurement is perfect.
    If one parens argument is given, the demolition measurement's result is noisy.
    The argument is the probability of returning the wrong result.
    The argument does not affect the fidelity of the reset.

Targets:

    The qubits to measure and reset in the Z basis.
    Prefixing a qubit target with `!` flips its reported measurement result.

Examples:

    # Measure qubit 5 in the Z basis, reset it to the |0> state, append the measurement result into the measurement record.
    MRZ 5

    # MR is also a Z-basis demolition measurement.
    MR 5

    # Demolition measure qubit 5 in the Z basis, but append the INVERSE of its result into the measurement record.
    MRZ !5

    # Do a noisy demolition measurement where the result put into the measurement record is wrong 1% of the time.
    MRZ(0.01) 5

    # Demolition measure multiple qubits in the Z basis, putting 3 bits into the measurement record.
    MRZ 2 3 5

    # Perform multiple noisy demolition measurements. Each measurement result is flipped independently with 2% probability.
    MRZ(0.02) 2 3 5
)MARKDOWN",
                    {},
                    {"Z -> m xor chance(p)", "1 -> +Z"},
                    R"CIRCUIT(
M 0
R 0
)CIRCUIT",
                };
            },
        });
    add_gate_alias(failed, "MRZ", "MR");

    // ===================== Reset Gates. ============================
    add_gate(
        failed,
        Gate{
            "RX",
            0,
            &TableauSimulator::reset_x,
            &FrameSimulator::reset_x,
            &ErrorAnalyzer::RX,
            GATE_IS_RESET,
            []() -> ExtraGateData {
                return {
                    "L_Collapsing Gates",
                    R"MARKDOWN(
X-basis reset.
Forces each target qubit into the `|+>` state by silently measuring it in the X basis and applying a `Z` gate if it ended up in the `|->` state.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    The qubits to reset in the X basis.

Examples:

    # Reset qubit 5 into the |+> state.
    RX 5

    # Result multiple qubits into the |+> state.
    RX 2 3 5
)MARKDOWN",
                    {},
                    {"1 -> +X"},
                    R"CIRCUIT(
H 0
R 0
H 0
)CIRCUIT",
                };
            },
        });
    add_gate(
        failed,
        Gate{
            "RY",
            0,
            &TableauSimulator::reset_y,
            &FrameSimulator::reset_y,
            &ErrorAnalyzer::RY,
            GATE_IS_RESET,
            []() -> ExtraGateData {
                return {
                    "L_Collapsing Gates",
                    R"MARKDOWN(
Y-basis reset.
Forces each target qubit into the `|i>` state by silently measuring it in the Y basis and applying an `X` gate if it ended up in the `|-i>` state.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    The qubits to reset in the Y basis.

Examples:

    # Reset qubit 5 into the |i> state.
    RY 5

    # Result multiple qubits into the |i> state.
    RY 2 3 5
)MARKDOWN",
                    {},
                    {"1 -> +Y"},
                    R"CIRCUIT(
S 0
S 0
S 0
H 0
R 0
H 0
S 0
)CIRCUIT",
                };
            },
        });
    add_gate(
        failed,
        Gate{
            "R",
            0,
            &TableauSimulator::reset_z,
            &FrameSimulator::reset_z,
            &ErrorAnalyzer::RZ,
            GATE_IS_RESET,
            []() -> ExtraGateData {
                return {
                    "L_Collapsing Gates",
                    R"MARKDOWN(
Z-basis reset.
Forces each target qubit into the `|0>` state by silently measuring it in the Z basis and applying an `X` gate if it ended up in the `|1>` state.

Parens Arguments:

    This instruction takes no parens arguments.

Targets:

    The qubits to reset in the Z basis.

Examples:

    # Reset qubit 5 into the |0> state.
    RZ 5

    # R means the same thing as RZ.
    R 5

    # Reset multiple qubits into the |0> state.
    RZ 2 3 5
)MARKDOWN",
                    {},
                    {"1 -> +Z"},
                    R"CIRCUIT(
R 0
)CIRCUIT",
                };
            },
        });
    add_gate_alias(failed, "RZ", "R");

    add_gate(
        failed,
        Gate{
            "MPP",
            ARG_COUNT_SYGIL_ZERO_OR_ONE,
            &TableauSimulator::MPP,
            &FrameSimulator::MPP,
            &ErrorAnalyzer::MPP,
            (GateFlags)(GATE_PRODUCES_NOISY_RESULTS | GATE_TARGETS_PAULI_STRING | GATE_TARGETS_COMBINERS | GATE_ARGS_ARE_DISJOINT_PROBABILITIES),
            []() -> ExtraGateData {
                return {
                    "L_Collapsing Gates",
                    R"MARKDOWN(
Measure Pauli products.

Parens Arguments:

    Optional.
    A single float specifying the probability of flipping each reported measurement result.

Targets:

    A series of Pauli products to measure.

    Each Pauli product is a series of Pauli targets (`[XYZ]#`) separated by combiners (`*`).
    Products can be negated by prefixing a Pauli target in the product with an inverter (`!`)

Examples:

    # Measure the two-body +X1*Y2 observable.
    MPP X1*Y2

    # Measure the one-body -Z5 observable.
    MPP !Z5

    # Measure the two-body +X1*Y2 observable and also the three-body -Z3*Z4*Z5 observable.
    MPP X1*Y2 !Z3*Z4*Z5

    # Noisily measure +Z1+Z2 and +X1*X2 (independently flip each reported result 0.1% of the time).
    MPP(0.001) Z1*Z2 X1*X2

)MARKDOWN",
                    {},
                    {"P -> m xor chance(p)", "P -> P"},
                    nullptr,
                };
            },
        });
}

void stim::decompose_mpp_operation(
    const OperationData &target_data,
    size_t num_qubits,
    const std::function<void(
        const OperationData &h_xz, const OperationData &h_yz, const OperationData &cnot, const OperationData &meas)>
        &callback) {
    simd_bits used(num_qubits);
    simd_bits inner_used(num_qubits);
    std::vector<GateTarget> h_xz;
    std::vector<GateTarget> h_yz;
    std::vector<GateTarget> cnot;
    std::vector<GateTarget> meas;

    auto op_dat = [](std::vector<GateTarget> &targets, ConstPointerRange<double> args) {
        return OperationData{args, targets};
    };
    size_t start = 0;
    while (start < target_data.targets.size()) {
        size_t end = start + 1;
        while (end < target_data.targets.size() && target_data.targets[end].is_combiner()) {
            end += 2;
        }

        // Determine which qubits are being touched by the next group.
        inner_used.clear();
        for (size_t i = start; i < end; i += 2) {
            auto t = target_data.targets[i];
            if (inner_used[t.qubit_value()]) {
                throw std::invalid_argument(
                    "A pauli product specified the same qubit twice.\n"
                    "The operation: MPP" +
                    target_data.str());
            }
            inner_used[t.qubit_value()] = true;
        }

        // If there's overlap with previous groups, the previous groups have to be flushed first.
        if (inner_used.intersects(used)) {
            callback(op_dat(h_xz, {}), op_dat(h_yz, {}), op_dat(cnot, {}), op_dat(meas, target_data.args));
            h_xz.clear();
            h_yz.clear();
            cnot.clear();
            meas.clear();
            used.clear();
        }
        used |= inner_used;

        // Append operations that are equivalent to the desired measurement.
        for (size_t i = start; i < end; i += 2) {
            auto t = target_data.targets[i];
            auto q = t.qubit_value();
            if (t.data & TARGET_PAULI_X_BIT) {
                if (t.data & TARGET_PAULI_Z_BIT) {
                    h_yz.push_back({q});
                } else {
                    h_xz.push_back({q});
                }
            }
            if (i == start) {
                meas.push_back({q});
            } else {
                cnot.push_back({q});
                cnot.push_back({meas.back().qubit_value()});
            }
            meas.back().data ^= t.data & TARGET_INVERTED_BIT;
        }

        start = end;
    }

    // Flush remaining groups.
    callback(op_dat(h_xz, {}), op_dat(h_yz, {}), op_dat(cnot, {}), op_dat(meas, target_data.args));
}
