#include <complex>
#include "gate_data.h"
#include "sim_tableau.h"
#include "sim_vector.h"
#include "sim_bulk_pauli_frame.h"

const std::unordered_map<std::string, const std::string> GATE_INVERSE_NAMES {
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

const std::unordered_map<std::string, const Tableau> GATE_TABLEAUS {
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
const std::unordered_map<std::string, const std::vector<std::vector<std::complex<float>>>> GATE_UNITARIES {
    {"I", {{1, 0}, {0, 1}}},
    // Pauli gates.
    {"X", {{0, 1}, {1, 0}}},
    {"Y", {{0, -i}, {i, 0}}},
    {"Z", {{1, 0}, {0, -1}}},
    // Axis exchange gates.
    {"H", {{s, s}, {s, -s}}},
    {"H_XY", {{0, s - i*s}, {s + i*s, 0}}},
    {"H_XZ", {{s, s}, {s, -s}}},
    {"H_YZ", {{s, -i*s}, {i*s, -s}}},
    // 90 degree rotation gates.
    {"SQRT_X", {{0.5f + 0.5f*i, 0.5f - 0.5f*i}, {0.5f - 0.5f*i, 0.5f + 0.5f*i}}},
    {"SQRT_X_DAG", {{0.5f - 0.5f*i, 0.5f + 0.5f*i}, {0.5f + 0.5f*i, 0.5f - 0.5f*i}}},
    {"SQRT_Y", {{0.5f + 0.5f*i, -0.5f - 0.5f*i}, {0.5f + 0.5f*i, 0.5f + 0.5f*i}}},
    {"SQRT_Y_DAG", {{0.5f - 0.5f*i, 0.5f - 0.5f*i}, {-0.5f + 0.5f*i, 0.5f - 0.5f*i}}},
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
    {"XCX", {{0.5f, 0.5f, 0.5f, -0.5f},
             {0.5f, 0.5f, -0.5f, 0.5f},
             {0.5f, -0.5f, 0.5f, 0.5f},
             {-0.5f, 0.5f, 0.5f, 0.5f}}},
    {"XCY", {{0.5f, 0.5f, -0.5f*i, 0.5f*i},
             {0.5f, 0.5f, 0.5f*i, -0.5f*i},
             {0.5f*i, -0.5f*i, 0.5f, 0.5f},
             {-0.5f*i, 0.5f*i, 0.5f, 0.5f}}},
    {"XCZ", {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, 1}, {0, 0, 1, 0}}},
    {"YCX", {{0.5f, -i*0.5f, 0.5f, i*0.5f},
             {i*0.5f, 0.5f, -i*0.5f, 0.5f},
             {0.5f, i*0.5f, 0.5f, -i*0.5f},
             {-i*0.5f, 0.5f, i*0.5f, 0.5f}}},
    {"YCY", {{0.5f, -i*0.5f, -i*0.5f, 0.5f},
             {i*0.5f, 0.5f, -0.5f, -i*0.5f},
             {i*0.5f, -0.5f, 0.5f, -i*0.5f},
             {0.5f, i*0.5f, i*0.5f, 0.5f}}},
    {"YCZ", {{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 0, -i}, {0, 0, i, 0}}},
};

void do_nothing_pst(SimBulkPauliFrames &p, const OperationData &target_data) {
}

const std::unordered_map<std::string, std::function<void(SimBulkPauliFrames &, const OperationData &)>> SIM_BULK_PAULI_FRAMES_GATE_DATA{
    {"R",          &SimBulkPauliFrames::reset},
    {"M_REF",      &SimBulkPauliFrames::measure_ref},
    // Pauli gates.
    {"TICK",       &do_nothing_pst},
    {"I",          &do_nothing_pst},
    {"X",          &do_nothing_pst},
    {"Y",          &do_nothing_pst},
    {"Z",          &do_nothing_pst},
    // Axis exchange gates.
    {"H", &SimBulkPauliFrames::H_XZ},
    {"H_XY",       &SimBulkPauliFrames::H_XY},
    {"H_XZ", &SimBulkPauliFrames::H_XZ},
    {"H_YZ",       &SimBulkPauliFrames::H_YZ},
    // 90 degree rotation gates.
    {"SQRT_X",     &SimBulkPauliFrames::H_YZ},
    {"SQRT_X_DAG", &SimBulkPauliFrames::H_YZ},
    {"SQRT_Y",     &SimBulkPauliFrames::H_XZ},
    {"SQRT_Y_DAG", &SimBulkPauliFrames::H_XZ},
    {"SQRT_Z",     &SimBulkPauliFrames::H_XY},
    {"SQRT_Z_DAG", &SimBulkPauliFrames::H_XY},
    {"S",          &SimBulkPauliFrames::H_XY},
    {"S_DAG",      &SimBulkPauliFrames::H_XY},
    // Swap gates.
    {"SWAP", &SimBulkPauliFrames::SWAP},
    {"ISWAP", &SimBulkPauliFrames::ISWAP},
    {"ISWAP_DAG", &SimBulkPauliFrames::ISWAP},
    // Controlled gates.
    {"CNOT", &SimBulkPauliFrames::CX},
    {"CX", &SimBulkPauliFrames::CX},
    {"CY", &SimBulkPauliFrames::CY},
    {"CZ", &SimBulkPauliFrames::CZ},
    // Controlled interactions in other bases.
    {"XCX", &SimBulkPauliFrames::XCX},
    {"XCY", &SimBulkPauliFrames::XCY},
    {"XCZ", &SimBulkPauliFrames::XCZ},
    {"YCX", &SimBulkPauliFrames::YCX},
    {"YCY", &SimBulkPauliFrames::YCY},
    {"YCZ", &SimBulkPauliFrames::YCZ},
};

const std::unordered_map<std::string, std::function<void(SimTableau &, const OperationData &)>> SIM_TABLEAU_GATE_FUNC_DATA{
        {"M",          [](auto &s, const auto &t) { s.measure(t, 0); }},
        {"M_PREFER_0", [](auto &s, const auto &t) { s.measure(t, +1); }},
        {"R",          [](auto &s, const auto &t) { s.reset(t, 0); }},
        {"R_PREFER_0", [](auto &s, const auto &t) { s.reset(t, +1); }},
        {"TICK",       [](auto &s, const auto &t) {}},
        {"I",          [](auto &s, const auto &t) {}},
        // Pauli gates.
        {"X",          &SimTableau::X},
        {"Y",          &SimTableau::Y},
        {"Z",          &SimTableau::Z},
        // Axis exchange gates.
        {"H",          &SimTableau::H_XZ},
        {"H_XY",       &SimTableau::H_XY},
        {"H_XZ",       &SimTableau::H_XZ},
        {"H_YZ",       &SimTableau::H_YZ},
        // 90 degree rotation gates.
        {"SQRT_X",     &SimTableau::SQRT_X},
        {"SQRT_X_DAG", &SimTableau::SQRT_X_DAG},
        {"SQRT_Y",     &SimTableau::SQRT_Y},
        {"SQRT_Y_DAG", &SimTableau::SQRT_Y_DAG},
        {"SQRT_Z",     &SimTableau::SQRT_Z},
        {"SQRT_Z_DAG", &SimTableau::SQRT_Z_DAG},
        {"S",          &SimTableau::SQRT_Z},
        {"S_DAG",      &SimTableau::SQRT_Z_DAG},
        // Swap gates.
        {"SWAP",       &SimTableau::SWAP},
        {"ISWAP",      &SimTableau::ISWAP},
        {"ISWAP_DAG",  &SimTableau::ISWAP_DAG},
        // Controlled gates.
        {"CNOT",       &SimTableau::CX},
        {"CX",         &SimTableau::CX},
        {"CY",         &SimTableau::CY},
        {"CZ",         &SimTableau::CZ},
        // Controlled interactions in other bases.
        {"XCX",        &SimTableau::XCX},
        {"XCY",        &SimTableau::XCY},
        {"XCZ",        &SimTableau::XCZ},
        {"YCX",        &SimTableau::YCX},
        {"YCY",        &SimTableau::YCY},
        {"YCZ",        &SimTableau::YCZ},
};

const std::unordered_set<std::string> NOISY_GATE_NAMES {
    "DEPOLARIZE1",
    "DEPOLARIZE2",
};
