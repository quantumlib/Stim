#include <cmath>
#include <iostream>
#include <sstream>

#include "arg_parse.h"
#include "benchmark_util.h"

RegisteredBenchmark *running_benchmark = nullptr;
std::vector<RegisteredBenchmark> all_registered_benchmarks{};

/// Describe quantity as an SI-prefixed value with two significant figures.
std::string si2(double val) {
    std::string unit = "";
    if (val < 1) {
        if (val < 1) {
            val *= 1000;
            unit = "m";
        }
        if (val < 1) {
            val *= 1000;
            unit = "u";
        }
        if (val < 1) {
            val *= 1000;
            unit = "n";
        }
        if (val < 1) {
            val *= 1000;
            unit = "p";
        }
    } else {
        if (val > 1000) {
            val /= 1000;
            unit = "k";
        }
        if (val > 1000) {
            val /= 1000;
            unit = "M";
        }
        if (val > 1000) {
            val /= 1000;
            unit = "G";
        }
        if (val > 1000) {
            val /= 1000;
            unit = "T";
        }
    }
    std::stringstream ss;
    if (1 <= val && val < 10) {
        ss << (size_t)val << '.' << ((size_t)(val * 10) % 10);
    } else if (10 <= val && val < 100) {
        ss << ' ' << (size_t)val;
    } else if (100 <= val && val < 1000) {
        ss << (size_t)(val / 10) * 10;
    } else {
        ss << val;
    }
    ss << ' ' << unit;
    return ss.str();
}

std::vector<const char *> known_arguments{
    "-only",
};

int main(int argc, const char **argv) {
    check_for_unknown_arguments(known_arguments.size(), known_arguments.data(), argc, argv);
    const char *only = find_argument("-only", argc, argv);
    std::vector<RegisteredBenchmark> chosen_benchmarks;
    if (only == nullptr) {
        chosen_benchmarks = all_registered_benchmarks;
    } else {
        std::string filter_text = only;
        std::vector<std::string> filters{};
        size_t s = 0;
        for (size_t k = 0;; k++) {
            if (only[k] == ',' || only[k] == '\0') {
                filters.push_back(filter_text.substr(s, k - s));
                s = k + 1;
            }
            if (only[k] == '\0') {
                break;
            }
        }

        if (filters.size() == 0) {
            std::cerr << "No filters specified.\n";
            exit(EXIT_FAILURE);
        }

        for (const auto &filter : filters) {
            bool found = false;
            for (const auto &benchmark : all_registered_benchmarks) {
                if (benchmark.name == filter) {
                    chosen_benchmarks.push_back(benchmark);
                    found = true;
                }
            }
            if (!found) {
                std::cerr << "No benchmark with name '" << filter << "'. Available benchmarks are:\n";
                for (auto &benchmark : all_registered_benchmarks) {
                    std::cerr << "    " << benchmark.name << "\n";
                }
                exit(EXIT_FAILURE);
            }
        }
    }

    for (auto &benchmark : chosen_benchmarks) {
        running_benchmark = &benchmark;
        benchmark.func();
        for (const auto &result : benchmark.results) {
            double actual_seconds_per_rep = result.total_seconds / result.total_reps;
            if (result.goal_seconds != -1) {
                int deviation = (int)round((log(result.goal_seconds) - log(actual_seconds_per_rep)) / (log(10) / 10.0));
                std::cout << "slower [";
                for (int k = -20; k <= 20; k++) {
                    if ((k < deviation && k < 0) || (k > deviation && k > 0)) {
                        std::cout << '.';
                    } else if (k == deviation) {
                        std::cout << '*';
                    } else if (k == 0) {
                        std::cout << '|';
                    } else if (deviation < 0) {
                        std::cout << '<';
                    } else {
                        std::cout << '>';
                    }
                }
                std::cout << "] faster dB | ";
                std::cout << si2(actual_seconds_per_rep) << "s";
                std::cout << " (vs " << si2(result.goal_seconds) << "s) ";
            } else {
                std::cout << si2(actual_seconds_per_rep) << "s";
            }
            for (const auto &e : result.marginal_rates) {
                const auto &multiplier = e.second;
                const auto &unit = e.first;
                std::cout << "(" << si2(result.total_reps / result.total_seconds * multiplier) << unit << "/s) ";
            }
            std::cout << benchmark.name << "\n";
            if (benchmark.results.empty()) {
                std::cerr << "`benchmark_go` was not called from BENCH(" << benchmark.name << ")";
                exit(EXIT_FAILURE);
            }
        }
    }
    return 0;
}
