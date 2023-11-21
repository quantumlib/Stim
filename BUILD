package(default_visibility = ["//visibility:public"])

load("@rules_python//python:packaging.bzl", "py_wheel")

SOURCE_FILES_NO_MAIN = glob(
    [
        "src/**/*.cc",
        "src/**/*.h",
        "src/**/*.inl",
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
    copts = [
        "-std=c++20",
    ],
    includes = ["src/"],
)

cc_binary(
    name = "stim",
    srcs = SOURCE_FILES_NO_MAIN + glob(["src/**/main.cc"]),
    copts = [
        "-std=c++20",
        "-march=native",
        "-O3",
    ],
    includes = ["src/"],
)

cc_binary(
    name = "stim_benchmark",
    srcs = SOURCE_FILES_NO_MAIN + PERF_FILES,
    copts = [
        "-std=c++20",
        "-march=native",
        "-O3",
    ],
    includes = ["src/"],
)

cc_test(
    name = "stim_test",
    srcs = SOURCE_FILES_NO_MAIN + TEST_FILES,
    copts = [
        "-std=c++20",
        "-march=native",
    ],
    data = glob(["testdata/**"]),
    includes = ["src/"],
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
        "-std=c++20",
        "-fvisibility=hidden",
        "-march=native",
        "-DSTIM_PYBIND11_MODULE_NAME=stim",
        "-DVERSION_INFO=0.0.dev0",
    ],
    includes = ["src/"],
    linkshared = 1,
    deps = ["@pybind11"],
)

genrule(
    name = "stim_wheel_files",
    srcs = ["doc/stim.pyi"],
    outs = ["stim.pyi"],
    cmd = "cp $(location doc/stim.pyi) $@",
)

py_wheel(
    name = "stim_dev_wheel",
    distribution = "stim",
    requires = ["numpy"],
    version = "0.0.dev0",
    deps = [
        ":stim.so",
        ":stim_wheel_files",
    ],
)
