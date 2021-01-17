#ifndef PERF_H
#define PERF_H

#include <iostream>
#include <functional>

struct PerfResult {
    double total_seconds;
    size_t total_reps;

    double rate() const;
    double seconds_per_rep() const;

    std::string str() const;

    static PerfResult from_once(const std::function<void(void)> &func);
    static PerfResult time(const std::function<void(void)> &func, float target_seconds = 0.5);
};

std::ostream &operator<<(std::ostream &out, const PerfResult &ps);

#endif
