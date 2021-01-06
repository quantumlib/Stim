#include <iostream>
#include <new>
#include <cassert>
#include <sstream>
#include <chrono>
#include "perf.h"

double PerfResult::rate() const {
    return total_reps / total_seconds;
}

double PerfResult::seconds_per_rep() const {
    return total_seconds / total_reps;
}

std::string PerfResult::str() const {
    std::stringstream s;
    s << *this;
    return s.str();
}

PerfResult PerfResult::from_once(const std::function<void(void)> &func) {
    auto start = std::chrono::steady_clock::now();
    func();
    auto end = std::chrono::steady_clock::now();
    auto micros = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    return {micros / 1000.0 / 1000.0, 1};
}

PerfResult PerfResult::time(const std::function<void(void)> &func) {
    auto total = PerfResult {0, 0};
    double time_target = 0.5;
    for (size_t rep_limit = 1; total.total_seconds < time_target; rep_limit *= 100) {
        double remaining_time = time_target  - total.total_seconds;
        size_t reps = (size_t)(remaining_time / (total.seconds_per_rep() + 0.00000001));
        if (reps > rep_limit) {
            reps = rep_limit;
        }
        if (reps < 1) {
            reps = 1;
        }
        auto d = PerfResult::from_once([&]() {
            for (size_t rep = 0; rep < reps; rep++) {
                func();
            }
        });
        total.total_reps += reps;
        total.total_seconds += d.total_seconds;
    }
    return total;
}

std::ostream &operator<<(std::ostream &out, const PerfResult &p) {
    std::stringstream s;
    std::string unit = "s";
    double val = p.seconds_per_rep();
    if (val < 1) {
        val *= 1000;
        unit = "ms";
    }
    if (val < 1) {
        val *= 1000;
        unit = "us";
    }
    if (val < 1) {
        val *= 1000;
        unit = "ns";
    }
    val = (size_t)(val * 10) / 10.0;
    s << val << unit;
    while (s.str().size() < 20) {
        s << " ";
    }
    val = p.rate();
    unit = "Hz";
    if (val > 1000) {
        val /= 1000;
        unit = "kHz";
    }
    if (val > 1000) {
        val /= 1000;
        unit = "MHz";
    }
    val = (size_t)(val * 10) / 10.0;
    s << val << unit;
    while (s.str().size() < 40) {
        s << " ";
    }
    out << s.str();
    out << "(";
    out << p.total_reps << " reps, total time ";
    out << p.total_seconds << "s)";
    return out;
}
