load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@pybind11_bazel//:python_configure.bzl", "python_configure")

http_archive(
    name = "pybind11",
    build_file = "@pybind11_bazel//:pybind11.BUILD",
    sha256 = "bf8f242abd1abcd375d516a7067490fb71abd79519a282d22b6e4d19282185a7",
    strip_prefix = "pybind11-2.12.0",
    urls = ["https://github.com/pybind/pybind11/archive/refs/tags/v2.12.0.tar.gz"],
)

python_configure(name = "local_config_python")
