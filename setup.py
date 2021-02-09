from distutils.core import setup, Extension
import pybind11
import glob

ALL_SOURCE_FILES = glob.glob("**/*.cc", recursive=True)
TEST_FILES = glob.glob("**/*.test.cc", recursive=True)
PERF_FILES = glob.glob("**/*.perf.cc", recursive=True)
MAIN_FILES = glob.glob("**/main.cc", recursive=True)

extension_module = Extension(
    'stim',
    sources=sorted(set(ALL_SOURCE_FILES) - set(TEST_FILES + PERF_FILES + MAIN_FILES)),
    include_dirs=[pybind11.get_include()],
    language='c++',
    extra_compile_args=['-std=c++11', '-march=native', '-O3']
)

setup(
    name='stim',
    version='0.1',
    description='A fast quantum stabilizer circuit simulator.',
    ext_modules=[extension_module],
    # Needed on Windows to avoid default `build` colliding with Bazel's `BUILD`.
    options={'build': {'build_base': 'python_build_stim'}}
)
