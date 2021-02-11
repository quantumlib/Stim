from setuptools import setup, Extension
import pybind11
import glob

ALL_SOURCE_FILES = glob.glob("src/**/*.cc", recursive=True)
TEST_FILES = glob.glob("src/**/*.test.cc", recursive=True)
PERF_FILES = glob.glob("src/**/*.perf.cc", recursive=True)
MAIN_FILES = glob.glob("src/**/main.cc", recursive=True)
RELEVANT_SOURCE_FILES = sorted(set(ALL_SOURCE_FILES) - set(TEST_FILES + PERF_FILES + MAIN_FILES))

ALL_HEADERS = glob.glob("src/**/*.h", recursive=True)
TEST_HEADERS = glob.glob("src/**/*.test.h", recursive=True)
PERF_HEADERS = glob.glob("src/**/*.perf.h", recursive=True)
RELEVANT_HEADERS = sorted(set(ALL_HEADERS) - set(TEST_HEADERS + PERF_HEADERS))

extension_module = Extension(
    'stim',
    sources=RELEVANT_SOURCE_FILES,
    include_dirs=[pybind11.get_include()],
    language='c++',
    extra_compile_args=['-std=c++11', '-march=native', '-O3']
)

setup(
    name='stim',
    version='0.1',
    author='Craig Gidney',
    author_email='craig.gidney@gmail.com',
    url='https://github.com/Strilanc/Stim',
    license='Apache 2',
    description='A fast quantum stabilizer circuit simulator.',
    ext_modules=[extension_module],
    python_requires='>=3.5.0',
    data_files=['pyproject.toml', 'README.md'] + RELEVANT_HEADERS,
    requires=['numpy'],
    # Needed on Windows to avoid the default `build` colliding with Bazel's `BUILD`.
    options={'build': {'build_base': 'python_build_stim'}},
)
