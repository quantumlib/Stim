package(default_visibility = ["//visibility:public"])

load("@rules_python//python:packaging.bzl", "py_wheel")
load("@pybind11_bazel//:build_defs.bzl", "pybind_extension")

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
    includes = ["src/"],
    linkopts = ["-lpthread"],
)

cc_binary(
    name = "stim",
    srcs = SOURCE_FILES_NO_MAIN + glob(["src/**/main.cc"]),
    copts = [
        "-march=native",
        "-O3",
    ],
    includes = ["src/"],
    linkopts = ["-lpthread"],
)

cc_binary(
    name = "stim_benchmark",
    srcs = SOURCE_FILES_NO_MAIN + PERF_FILES,
    copts = [
        "-march=native",
        "-O3",
    ],
    includes = ["src/"],
    linkopts = ["-lpthread"],
)

cc_test(
    name = "stim_test",
    srcs = SOURCE_FILES_NO_MAIN + TEST_FILES,
    copts = [
        "-march=native",
    ],
    includes = ["src/"],
    linkopts = ["-lpthread"],
    deps = [
        "@gtest",
        "@gtest//:gtest_main",
    ],
)

cc_binary(
    name = "stim.so",
    srcs = SOURCE_FILES_NO_MAIN + PYBIND_FILES,
    copts = [
        "-O3",
        "-fvisibility=hidden",
        "-march=native",
        "-DSTIM_PYBIND11_MODULE_NAME=stim",
    ],
    includes = ["src/"],
    linkopts = ["-lpthread"],
    linkshared = 1,
    deps = ["@pybind11"],
)

py_wheel(
    name = "stim_dev_wheel",
    distribution = "stim",
    requires = ["numpy"],
    version = "dev",
    deps = [":stim.so"],
)
