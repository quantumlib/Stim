#ifndef GATE_DATA_H
#define GATE_DATA_H

#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>

struct TableauSimulator;
struct FrameSimulator;
struct OperationData;

extern const std::unordered_map<std::string, const std::string> GATE_CANONICAL_NAMES;
extern const std::unordered_map<std::string, const std::string> GATE_INVERSE_NAMES;
extern const std::unordered_set<std::string> NOISY_GATE_NAMES;
extern const std::unordered_map<std::string, std::function<void(TableauSimulator &, const OperationData &)>> SIM_TABLEAU_GATE_FUNC_DATA;
extern const std::unordered_map<std::string, std::function<void(FrameSimulator &, const OperationData &)>> SIM_BULK_PAULI_FRAMES_GATE_DATA;

#endif
