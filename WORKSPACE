load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

http_archive(
    name = "gtest",
    sha256 = "353571c2440176ded91c2de6d6cd88ddd41401d14692ec1f99e35d013feda55a",
    strip_prefix = "googletest-release-1.11.0",
    urls = ["https://github.com/google/googletest/archive/refs/tags/release-1.11.0.zip"],
)

http_archive(
    name = "pybind11",
    build_file = "@pybind11_bazel//:pybind11.BUILD",
    sha256 = "6bd528c4dbe2276635dc787b6b1f2e5316cf6b49ee3e150264e455a0d68d19c1",
    strip_prefix = "pybind11-2.9.2",
    urls = ["https://github.com/pybind/pybind11/archive/v2.9.2.tar.gz"],
)

http_archive(
    name = "rules_python",
    sha256 = "954aa89b491be4a083304a2cb838019c8b8c3720a7abb9c4cb81ac7a24230cea",
    url = "https://github.com/bazelbuild/rules_python/releases/download/0.4.0/rules_python-0.4.0.tar.gz",
)

http_archive(
    name = "pybind11_bazel",
    sha256 = "e4a9536f49d4a88e3c5a09954de49c4a18d6b1632c457a62d6ec4878c27f1b5b",
    strip_prefix = "pybind11_bazel-7f397b5d2cc2434bbd651e096548f7b40c128044",
    urls = ["https://github.com/pybind/pybind11_bazel/archive/7f397b5d2cc2434bbd651e096548f7b40c128044.zip"],
)

load("@pybind11_bazel//:python_configure.bzl", "python_configure")

python_configure(name = "local_config_python")
