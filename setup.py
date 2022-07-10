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
import glob
import pybind11

ALL_SOURCE_FILES = glob.glob("src/**/*.cc", recursive=True)
MUX_SOURCE_FILES = glob.glob("src/**/march.pybind.cc", recursive=True)
TEST_FILES = glob.glob("src/**/*.test.cc", recursive=True)
PERF_FILES = glob.glob("src/**/*.perf.cc", recursive=True)
MAIN_FILES = glob.glob("src/**/main.cc", recursive=True)
HEADER_FILES = glob.glob("src/**/*.h", recursive=True)
RELEVANT_SOURCE_FILES = sorted(set(ALL_SOURCE_FILES) - set(TEST_FILES + PERF_FILES + MAIN_FILES + MUX_SOURCE_FILES))

version = '1.10.dev0'

common_compile_args = [
    '-std=c++11',
    '-fno-strict-aliasing',
    '-O3',
    '-g0',
    f'-DVERSION_INFO={version}',
]
stim_detect_machine_architecture = Extension(
    'stim._detect_machine_architecture',
    sources=MUX_SOURCE_FILES,
    include_dirs=[pybind11.get_include(), "src"],
    language='c++',
    extra_compile_args=[
        *common_compile_args,
        '-mno-sse2',
        '-mno-avx2',
    ],
)
stim_polyfill = Extension(
    'stim._stim_polyfill',
    sources=RELEVANT_SOURCE_FILES,
    include_dirs=[pybind11.get_include(), "src"],
    language='c++',
    extra_compile_args=[
        *common_compile_args,
        # I would specify -mno-sse2 but that causes build failures in non-stim code...?
        '-mno-avx2',
        '-DSTIM_PYBIND11_MODULE_NAME=_stim_polyfill',
    ],
)
stim_sse2 = Extension(
    'stim._stim_sse2',
    sources=RELEVANT_SOURCE_FILES,
    include_dirs=[pybind11.get_include(), "src"],
    language='c++',
    extra_compile_args=[
        *common_compile_args,
        '-msse2',
        '-mno-avx2',
        '-DSTIM_PYBIND11_MODULE_NAME=_stim_sse2',
    ],
)
stim_avx2 = Extension(
    'stim._stim_avx2',
    sources=RELEVANT_SOURCE_FILES,
    include_dirs=[pybind11.get_include(), "src"],
    language='c++',
    extra_compile_args=[
        *common_compile_args,
        '-msse2',
        '-mavx2',
        '-DSTIM_PYBIND11_MODULE_NAME=_stim_avx2',
    ],
)

with open('glue/python/README.md', encoding='UTF-8') as f:
    long_description = f.read()

setup(
    name='stim',
    version=version,
    author='Craig Gidney',
    author_email='craig.gidney@gmail.com',
    url='https://github.com/quantumlib/stim',
    license='Apache 2',
    description='A fast library for analyzing with quantum stabilizer circuits.',
    long_description=long_description,
    long_description_content_type='text/markdown',
    ext_modules=[
        stim_detect_machine_architecture,
        stim_polyfill,
        stim_sse2,
        stim_avx2,
    ],
    python_requires='>=3.6.0',
    packages=['stim'],
    package_dir={'stim': 'glue/python/src/stim'},
    package_data={'': [*HEADER_FILES, 'glue/python/src/stim/__init__.pyi', 'glue/python/README.md', 'pyproject.toml']},
    include_package_data=True,
    install_requires=['numpy'],
    entry_points={
        'console_scripts': ['stim=stim._main_argv:main_argv'],
    },
    # Needed on Windows to avoid the default `build` colliding with Bazel's `BUILD`.
    options={'build': {'build_base': 'python_build_stim'}},
)
