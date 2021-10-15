# Copyright 2021 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

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

version = '1.6.0'

extension_module = Extension(
    'stim',
    sources=RELEVANT_SOURCE_FILES,
    include_dirs=[pybind11.get_include(), "src"],
    language='c++',
    extra_compile_args=['-std=c++11', '-fno-strict-aliasing', '-march=native', '-O3', f'-DVERSION_INFO={version}']
)

with open('glue/python/README.md') as f:
    long_description = f.read()

setup(
    name='stim',
    version=version,
    author='Craig Gidney',
    author_email='craig.gidney@gmail.com',
    url='https://github.com/quantumlib/stim',
    license='Apache 2',
    description='A fast quantum stabilizer circuit simulator.',
    long_description=long_description,
    long_description_content_type='text/markdown',
    ext_modules=[extension_module],
    python_requires='>=3.6.0',
    data_files=['pyproject.toml', 'glue/python/README.md'] + RELEVANT_HEADERS,
    install_requires=['numpy'],
    # Needed on Windows to avoid the default `build` colliding with Bazel's `BUILD`.
    options={'build': {'build_base': 'python_build_stim'}},
)
