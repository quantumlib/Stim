#include "gate_data.h"

#include <complex>

#include "frame_simulator.h"
#include "tableau_simulator.h"
#include "vector_simulator.h"

const std::unordered_map<std::string, const std::string> GATE_INVERSE_NAMES{
    {"I", "I"},
    {"X", "X"},
    {"Y", "Y"},
    {"Z", "Z"},
    {"H", "H"},
    {"H_XY", "H_XY"},
    {"H_XZ", "H_XZ"},
    {"H_YZ", "H_YZ"},
    {"SQRT_X", "SQRT_X_DAG"},
    {"SQRT_X_DAG", "SQRT_X"},
    {"SQRT_Y", "SQRT_Y_DAG"},
    {"SQRT_Y_DAG", "SQRT_Y"},
    {"SQRT_Z", "SQRT_Z_DAG"},
    {"SQRT_Z_DAG", "SQRT_Z"},
    {"S", "S_DAG"},
    {"S_DAG", "S"},
    {"SWAP", "SWAP"},
    {"CNOT", "CNOT"},
    {"CX", "CX"},
    {"CY", "CY"},
    {"CZ", "CZ"},
    {"XCX", "XCX"},
    {"XCY", "XCY"},
    {"XCZ", "XCZ"},
    {"YCX", "YCX"},
    {"YCY", "YCY"},
    {"YCZ", "YCZ"},
    {"ISWAP", "ISWAP_DAG"},
    {"ISWAP_DAG", "ISWAP"},
};

const std::unordered_map<std::string, const Tableau> GATE_TABLEAUS{
    {"I", Tableau::gate1("+X", "+Z")},
    // Pauli gates.
    {"X", Tableau::gate1("+X", "-Z")},
    {"Y", Tableau::gate1("-X", "-Z")},
    {"Z", Tableau::gate1("-X", "+Z")},
    // Axis exchange gates.
    {"H", Tableau::gate1("+Z", "+X")},
    {"H_XY", Tableau::gate1("+Y", "-Z")},
    {"H_XZ", Tableau::gate1("+Z", "+X")},
    {"H_YZ", Tableau::gate1("-X", "+Y")},
    // 90 degree rotation gates.
    {"SQRT_X", Tableau::gate1("+X", "-Y")},
    {"SQRT_X_DAG", Tableau::gate1("+X", "+Y")},
    {"SQRT_Y", Tableau::gate1("-Z", "+X")},
    {"SQRT_Y_DAG", Tableau::gate1("+Z", "-X")},
    {"SQRT_Z", Tableau::gate1("+Y", "+Z")},
    {"SQRT_Z_DAG", Tableau::gate1("-Y", "+Z")},
    {"S", Tableau::gate1("+Y", "+Z")},
    {"S_DAG", Tableau::gate1("-Y", "+Z")},
    // Two qubit gates.
    {"SWAP", Tableau::gate2("+IX", "+IZ", "+XI", "+ZI")},
    {"CNOT", Tableau::gate2("+XX", "+ZI", "+IX", "+ZZ")},
    {"CX", Tableau::gate2("+XX", "+ZI", "+IX", "+ZZ")},
    {"CY", Tableau::gate2("+XY", "+ZI", "+ZX", "+ZZ")},
    {"CZ", Tableau::gate2("+XZ", "+ZI", "+ZX", "+IZ")},
    {"ISWAP", Tableau::gate2("+ZY", "+IZ", "+YZ", "+ZI")},
    {"ISWAP_DAG", Tableau::gate2("-ZY", "+IZ", "-YZ", "+ZI")},
    // Controlled interactions in other bases.
    {"XCX", Tableau::gate2("+XI", "+ZX", "+IX", "+XZ")},
    {"XCY", Tableau::gate2("+XI", "+ZY", "+XX", "+XZ")},
    {"XCZ", Tableau::gate2("+XI", "+ZZ", "+XX", "+IZ")},
    {"YCX", Tableau::gate2("+XX", "+ZX", "+IX", "+YZ")},
    {"YCY", Tableau::gate2("+XY", "+ZY", "+YX", "+YZ")},
    {"YCZ", Tableau::gate2("+XZ", "+ZZ", "+YX", "+IZ")},
};

constexpr std::complex<float> i = std::complex<float>(0, 1);
constexpr std::complex<float> s = 0.7071067811865475244f;
const std::unordered_map<std::string, const std::vector<std::vector<std::complex<float>>>> GATE_UNITARIES{
    {"I", {{1, 0}, {0, 1}}},
    // Pauli gates.
    {"X", {{0, 1}, {1, 0}}},
    {"Y", {{0, -i}, {i, 0}}},
    {"Z", {{1, 0}, {0, -1}}},
    // Axis exchange gates.
    {"H", {{s, s}, {s, -s}}},
    {"H_XY", {{0, s - i *s}, {s + i * s, 0}}},
    {"H_XZ", {{s, s}, {s, -s}}},
    {"H_YZ", {{s, -i *s}, {i * s, -s}}},
    // 90 degree rotation gates.
    {"SQRT_X", {{0.5f + 0.5f * i, 0.5f - 0.5f * i}, {0.5f - 0.5f * i, 0.5f + 0.5f * i}}},
    {"SQRT_X_DAG", {{0.5f - 0.5f * i, 0.5f + 0.5f * i}, {0.5f + 0.5f * i, 0.5f - 0.5f * i}}},
    {"SQRT_Y", {{0.5f + 0.5f * i, -0.5f - 0.5f * i}, {0.5f + 0.5f * i, 0.5f + 0.5f * i}}},
    {"SQRT_Y_DAG", {{0.5f - 0.5f * i, 0.5f - 0.5f * i}, {-0.5f + 0.5f * i, 0.5f - 0.5f * i}}},
    {"SQRT_Z", {{1, 0}, {0, i}}},
    {"SQRT_Z_DAG", {{1, 0}, {0, -i}}},
    {"S", {{1, 0}, {0, i}}},
    {"S_DAG", {{1, 0}, {0, -i}}},
    // Two qubit gates.
    {"CNOT", {{1, 0, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}, {0, 1, 0, 0}}},
    {"CX", {{1, 0, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}, {0, 1, 0, 0}}},
    {"CY", {{1, 0, 0, 0}, {0, 0, 0, -i}, {0, 0, 1, 0}, {0, i, 0, 0}}},
    {"CZ", {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, -1}}},
    {"SWAP", {{1, 0, 0, 0}, {0, 0, 1, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}}},
    {"ISWAP", {{1, 0, 0, 0}, {0, 0, i, 0}, {0, i, 0, 0}, {0, 0, 0, 1}}},
    {"ISWAP_DAG", {{1, 0, 0, 0}, {0, 0, -i, 0}, {0, -i, 0, 0}, {0, 0, 0, 1}}},
    // Controlled interactions in other bases.
    {"XCX",
     {{0.5f, 0.5f, 0.5f, -0.5f}, {0.5f, 0.5f, -0.5f, 0.5f}, {0.5f, -0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f, 0.5f}}},
    {"XCY",
     {{0.5f, 0.5f, -0.5f * i, 0.5f * i},
      {0.5f, 0.5f, 0.5f * i, -0.5f * i},
      {0.5f * i, -0.5f * i, 0.5f, 0.5f},
      {-0.5f * i, 0.5f * i, 0.5f, 0.5f}}},
    {"XCZ", {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}}},
    {"YCX",
     {{0.5f, -i * 0.5f, 0.5f, i * 0.5f},
      {i * 0.5f, 0.5f, -i * 0.5f, 0.5f},
      {0.5f, i * 0.5f, 0.5f, -i * 0.5f},
      {-i * 0.5f, 0.5f, i * 0.5f, 0.5f}}},
    {"YCY",
     {{0.5f, -i * 0.5f, -i * 0.5f, 0.5f},
      {i * 0.5f, 0.5f, -0.5f, -i * 0.5f},
      {i * 0.5f, -0.5f, 0.5f, -i * 0.5f},
      {0.5f, i * 0.5f, i * 0.5f, 0.5f}}},
    {"YCZ", {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, -i}, {0, 0, i, 0}}},
};

void do_nothing_pst(FrameSimulator &p, const OperationData &target_data) {}

const std::unordered_map<std::string, std::function<void(FrameSimulator &, const OperationData &)>>
    SIM_BULK_PAULI_FRAMES_GATE_DATA{
        {"R", &FrameSimulator::reset},
        {"M", &FrameSimulator::measure},
        {"TICK", &do_nothing_pst},
        {"I", &do_nothing_pst},
        // Pauli gates (ignored because they are accounted for by the reference sample results being inverted or not.)
        {"X", &do_nothing_pst},
        {"Y", &do_nothing_pst},
        {"Z", &do_nothing_pst},
        // Axis exchange gates.
        {"H", &FrameSimulator::H_XZ},
        {"H_XY", &FrameSimulator::H_XY},
        {"H_XZ", &FrameSimulator::H_XZ},
        {"H_YZ", &FrameSimulator::H_YZ},
        // 90 degree rotation gates.
        {"SQRT_X", &FrameSimulator::H_YZ},
        {"SQRT_X_DAG", &FrameSimulator::H_YZ},
        {"SQRT_Y", &FrameSimulator::H_XZ},
        {"SQRT_Y_DAG", &FrameSimulator::H_XZ},
        {"SQRT_Z", &FrameSimulator::H_XY},
        {"SQRT_Z_DAG", &FrameSimulator::H_XY},
        {"S", &FrameSimulator::H_XY},
        {"S_DAG", &FrameSimulator::H_XY},
        // Swap gates.
        {"SWAP", &FrameSimulator::SWAP},
        {"ISWAP", &FrameSimulator::ISWAP},
        {"ISWAP_DAG", &FrameSimulator::ISWAP},
        // Controlled gates.
        {"CNOT", &FrameSimulator::CX},
        {"CX", &FrameSimulator::CX},
        {"CY", &FrameSimulator::CY},
        {"CZ", &FrameSimulator::CZ},
        // Controlled interactions in other bases.
        {"XCX", &FrameSimulator::XCX},
        {"XCY", &FrameSimulator::XCY},
        {"XCZ", &FrameSimulator::XCZ},
        {"YCX", &FrameSimulator::YCX},
        {"YCY", &FrameSimulator::YCY},
        {"YCZ", &FrameSimulator::YCZ},
    };

const std::unordered_map<std::string, std::function<void(TableauSimulator &, const OperationData &)>>
    SIM_TABLEAU_GATE_FUNC_DATA{
        {"M", &TableauSimulator::measure},
        {"R", &TableauSimulator::reset},
        {"TICK", [](auto &s, const auto &t) {}},
        {"I", [](auto &s, const auto &t) {}},
        // Pauli gates.
        {"X", &TableauSimulator::X},
        {"Y", &TableauSimulator::Y},
        {"Z", &TableauSimulator::Z},
        // Axis exchange gates.
        {"H", &TableauSimulator::H_XZ},
        {"H_XY", &TableauSimulator::H_XY},
        {"H_XZ", &TableauSimulator::H_XZ},
        {"H_YZ", &TableauSimulator::H_YZ},
        // 90 degree rotation gates.
        {"SQRT_X", &TableauSimulator::SQRT_X},
        {"SQRT_X_DAG", &TableauSimulator::SQRT_X_DAG},
        {"SQRT_Y", &TableauSimulator::SQRT_Y},
        {"SQRT_Y_DAG", &TableauSimulator::SQRT_Y_DAG},
        {"SQRT_Z", &TableauSimulator::SQRT_Z},
        {"SQRT_Z_DAG", &TableauSimulator::SQRT_Z_DAG},
        {"S", &TableauSimulator::SQRT_Z},
        {"S_DAG", &TableauSimulator::SQRT_Z_DAG},
        // Swap gates.
        {"SWAP", &TableauSimulator::SWAP},
        {"ISWAP", &TableauSimulator::ISWAP},
        {"ISWAP_DAG", &TableauSimulator::ISWAP_DAG},
        // Controlled gates.
        {"CNOT", &TableauSimulator::CX},
        {"CX", &TableauSimulator::CX},
        {"CY", &TableauSimulator::CY},
        {"CZ", &TableauSimulator::CZ},
        // Controlled interactions in other bases.
        {"XCX", &TableauSimulator::XCX},
        {"XCY", &TableauSimulator::XCY},
        {"XCZ", &TableauSimulator::XCZ},
        {"YCX", &TableauSimulator::YCX},
        {"YCY", &TableauSimulator::YCY},
        {"YCZ", &TableauSimulator::YCZ},
    };

const std::unordered_set<std::string> NOISY_GATE_NAMES{
    "DEPOLARIZE1",
    "DEPOLARIZE2",
};
