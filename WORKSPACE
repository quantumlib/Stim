load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

http_archive(
    name = "gtest",
    sha256 = "ff7a82736e158c077e76188232eac77913a15dac0b22508c390ab3f88e6d6d86",
    strip_prefix = "googletest-b6cd405286ed8635ece71c72f118e659f4ade3fb",
    urls = ["https://github.com/google/googletest/archive/b6cd405286ed8635ece71c72f118e659f4ade3fb.zip"],  # 2019-01-07
)

http_archive(
    name = "pybind11",
    build_file = "@pybind11_bazel//:pybind11.BUILD",
    sha256 = "90b705137b69ee3b5fc655eaca66d0dc9862ea1759226f7ccd3098425ae69571",
    strip_prefix = "pybind11-2.6.0",
    urls = ["https://github.com/pybind/pybind11/archive/v2.6.0.tar.gz"],
)

git_repository(
    name = "rules_python",
    commit = "38f86fb55b698c51e8510c807489c9f4e047480e",
    remote = "https://github.com/bazelbuild/rules_python.git",
    shallow_since = "1575517988 -0500",
)

http_archive(
    name = "pybind11_bazel",
    sha256 = "e4a9536f49d4a88e3c5a09954de49c4a18d6b1632c457a62d6ec4878c27f1b5b",
    strip_prefix = "pybind11_bazel-7f397b5d2cc2434bbd651e096548f7b40c128044",
    urls = ["https://github.com/pybind/pybind11_bazel/archive/7f397b5d2cc2434bbd651e096548f7b40c128044.zip"],
)

load("@pybind11_bazel//:python_configure.bzl", "python_configure")

python_configure(name = "local_config_python")
