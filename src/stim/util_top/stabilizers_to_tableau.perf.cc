#include "stim/util_top/stabilizers_to_tableau.h"

#include "stim/perf.perf.h"

using namespace stim;

BENCHMARK(stabilizers_to_tableau_144) {
    size_t w = 24;
    size_t h = 12;
    std::vector<stim::PauliString<64>> stabilizers;
    for (size_t x = 0; x < w; x++) {
        for (size_t y = x % 2; y < h; y += 2) {
            stim::PauliString<64> ps(w * h / 2);
            stabilizers.push_back(ps);
        }
    }

    size_t dep = 0;
    Tableau<64> t2 = stabilizers_to_tableau(stabilizers, true, true, false);
    dep += t2.xs[0].zs[0];

    if (dep == 99999999) {
        std::cout << "data dependence";
    }
}
