package(default_visibility = ["//visibility:public"])

SOURCE_FILES_NO_MAIN = glob(
    [
        "src/**/*.cc",
        "src/**/*.h",
    ],
    exclude = glob([
        "src/**/*.test.cc",
        "src/**/*.test.h",
        "src/**/*.perf.cc",
        "src/**/*.perf.h",
        "src/**/*.pybind.cc",
        "src/**/*.pybind.h",
        "src/**/main.cc",
    ]),
)

TEST_FILES = glob(
    [
        "src/**/*.test.cc",
        "src/**/*.test.h",
    ],
)

PERF_FILES = glob(
    [
        "src/**/*.perf.cc",
        "src/**/*.perf.h",
    ],
)

PYBIND_FILES = glob(
    [
        "src/**/*.pybind.cc",
        "src/**/*.pybind.h",
    ],
)

cc_library(
    name = "stim_lib",
    srcs = SOURCE_FILES_NO_MAIN,
    linkopts = ["-lpthread"],
)

cc_binary(
    name = "stim",
    srcs = SOURCE_FILES_NO_MAIN + glob(["src/**/main.cc"]),
    copts = [
        "-march=native",
        "-O3",
    ],
    linkopts = ["-lpthread"],
)

cc_binary(
    name = "stim_benchmark",
    srcs = SOURCE_FILES_NO_MAIN + PERF_FILES,
    copts = [
        "-march=native",
        "-O3",
    ],
    linkopts = ["-lpthread"],
)

cc_test(
    name = "stim_test",
    srcs = SOURCE_FILES_NO_MAIN + TEST_FILES,
    copts = [
        "-march=native",
    ],
    linkopts = ["-lpthread"],
    deps = [
        "@gtest",
        "@gtest//:gtest_main",
    ],
)

cc_binary(
    name = "stim_pybind.so",
    srcs = SOURCE_FILES_NO_MAIN + PYBIND_FILES,
    copts = [
        "-O3",
        "-fvisibility=hidden",
    ],
    linkopts = ["-lpthread"],
    linkshared = 1,
    deps = [
        "@pybind11",
    ],
)
