#include "tableau.h"

#include "../benchmark_util.h"

BENCHMARK(tableau_random_10) {
    auto n = 10;
    Tableau t(n);
    std::mt19937_64 rng(0);
    benchmark_go([&]() { t = Tableau::random(n, rng); }).goal_micros(60);
}

BENCHMARK(tableau_random_100) {
    auto n = 100;
    Tableau t(n);
    std::mt19937_64 rng(0);
    benchmark_go([&]() { t = Tableau::random(n, rng); }).goal_millis(2);
}

BENCHMARK(tableau_random_256) {
    auto n = 256;
    Tableau t(n);
    std::mt19937_64 rng(0);
    benchmark_go([&]() { t = Tableau::random(n, rng); }).goal_millis(12);
}

BENCHMARK(tableau_random_1000) {
    auto n = 1000;
    Tableau t(n);
    std::mt19937_64 rng(0);
    benchmark_go([&]() { t = Tableau::random(n, rng); }).goal_millis(200);
}

BENCHMARK(tableau_cnot_10Kqubits) {
    auto n = 10 * 1000;
    Tableau t(n);
    benchmark_go([&]() { t.prepend_CX(5, 20); }).goal_nanos(120);
}
