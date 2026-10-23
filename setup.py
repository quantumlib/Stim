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
import concurrent.futures
import functools
import glob
import os
import platform
import shlex
import shutil
import subprocess
import sys
import sysconfig

from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
import pybind11

ALL_SOURCE_FILES = glob.glob("src/**/*.cc", recursive=True)
MUX_SOURCE_FILES = glob.glob("src/**/march.pybind.cc", recursive=True)
TEST_FILES = glob.glob("src/**/*.test.cc", recursive=True)
PERF_FILES = glob.glob("src/**/*.perf.cc", recursive=True)
MAIN_FILES = glob.glob("src/**/main.cc", recursive=True)
HEADER_FILES = glob.glob("src/**/*.h", recursive=True) + glob.glob("src/**/*.inl", recursive=True)
RELEVANT_SOURCE_FILES = sorted(set(ALL_SOURCE_FILES) - set(TEST_FILES + PERF_FILES + MAIN_FILES + MUX_SOURCE_FILES))

__version__ = '1.17.dev0'

if platform.system().startswith('Win'):
    common_compile_args = [
        '/std:c++20',
        # CAUTION! This is /Od instead of /O2 because of bugs in the msvc compiler!
        # See: https://github.com/quantumlib/Stim/issues/1078
        '/Od',
        f'/DVERSION_INFO={__version__}',
    ]
    arch_avx = ['/arch:AVX2']
    arch_sse = ['/arch:SSE2']
    arch_basic = []
else:
    common_compile_args = [
        '-std=c++20',
        '-fno-strict-aliasing',
        '-O3',
        '-g0',
        f'-DVERSION_INFO={__version__}',
    ]
    arch_avx = ['-mavx2']
    arch_sse = ['-msse2', '-mno-avx2']
    arch_basic = []

stim_detect_machine_architecture = Extension(
    'stim._detect_machine_architecture',
    sources=MUX_SOURCE_FILES,
    include_dirs=[pybind11.get_include(), "src"],
    language='c++',
    extra_compile_args=[
        *common_compile_args,
        *arch_basic,
    ],
)
stim_polyfill = Extension(
    'stim._stim_polyfill',
    sources=RELEVANT_SOURCE_FILES,
    include_dirs=[pybind11.get_include(), "src"],
    language='c++',
    extra_compile_args=[
        *common_compile_args,
        *arch_basic,
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
        *arch_sse,
        '-DSTIM_PYBIND11_MODULE_NAME=_stim_sse2',
    ],
)

# NOTE: disabled until https://github.com/quantumlib/Stim/issues/432 is fixed
# stim_avx2 = Extension(
#     'stim._stim_avx2',
#     sources=RELEVANT_SOURCE_FILES,
#     include_dirs=[pybind11.get_include(), "src"],
#     language='c++',
#     extra_compile_args=[
#         *common_compile_args,
#         *arch_avx,
#         '-DSTIM_PYBIND11_MODULE_NAME=_stim_avx2',
#     ],
# )

with open('glue/python/README.md', encoding='UTF-8') as f:
    long_description = f.read()

def _get_extensions():
    archs = ["x86", "i686", "i386", "amd64"]
    target = os.environ.get("ARCHFLAGS", f"{platform.processor()} {platform.machine()}").lower()
    if any(_ext in target for _ext in archs):
        # NOTE: disabled until https://github.com/quantumlib/Stim/issues/432 is fixed
        # stim_avx2,
        return [stim_detect_machine_architecture, stim_polyfill,
                # stim_avx2,
                stim_sse2]
    else:
        return [stim_detect_machine_architecture, stim_polyfill]


@functools.lru_cache(maxsize=1)
def _get_msvc_env() -> dict[str, str]:
    if shutil.which('cl.exe') and 'INCLUDE' in os.environ:
        return {k.upper(): v for k, v in os.environ.items()}
    vswhere = os.path.join(
        os.environ.get('ProgramFiles(x86)', r'C:\Program Files (x86)'),
        'Microsoft Visual Studio', 'Installer', 'vswhere.exe',
    )
    install_path = subprocess.check_output([
        vswhere, '-latest', '-prerelease',
        '-requires', 'Microsoft.VisualStudio.Component.VC.Tools.x86.x64',
        '-property', 'installationPath',
    ], text=True).strip()
    vcvarsall = os.path.join(install_path, 'VC', 'Auxiliary', 'Build', 'vcvarsall.bat')
    out = subprocess.check_output(f'call "{vcvarsall}" x64 >nul && set', shell=True, text=True)
    env = {k.upper(): v for k, v in os.environ.items()}
    for line in out.splitlines():
        if '=' in line:
            k, _, v = line.partition('=')
            if k:
                env[k.upper()] = v
    return env


class BuildExtDirect(build_ext):
    def run(self):
        old_inplace, self.inplace = self.inplace, False
        for ext in self.extensions:
            self.build_extension(ext)
        self.inplace = old_inplace
        if old_inplace:
            self.copy_extensions_to_source()

    def build_extension(self, ext: Extension):
        ext_path = self.get_ext_fullpath(ext.name)
        obj_dir = os.path.join(self.build_temp, ext.name)
        os.makedirs(obj_dir, exist_ok=True)
        os.makedirs(os.path.dirname(ext_path), exist_ok=True)

        include_dirs = [pybind11.get_include(), sysconfig.get_path('include'), 'src']
        sources = sorted(ext.sources)

        if sys.platform == 'win32':
            env = _get_msvc_env()
            cl = shutil.which('cl.exe', path=env['PATH']) or 'cl.exe'
            link = shutil.which('link.exe', path=env['PATH']) or 'link.exe'
            objects = [os.path.join(obj_dir, f'{k}.obj') for k in range(len(sources))]
            compile_cmds = [
                [
                    cl, '/nologo', '/c', '/EHsc', '/MD', '/DNDEBUG',
                    *(f'/I{d}' for d in include_dirs),
                    *ext.extra_compile_args,
                    f'/Tp{src}', f'/Fo{obj}',
                ]
                for src, obj in zip(sources, objects)
            ]
            mod_name = ext.name.split('.')[-1]
            link_cmd = [
                link, '/nologo', '/DLL', '/INCREMENTAL:NO',
                '/MANIFEST:EMBED,ID=2', '/MANIFESTUAC:NO',
                f'/LIBPATH:{os.path.join(sys.base_exec_prefix, "libs")}',
                f'/EXPORT:PyInit_{mod_name}',
                f'/IMPLIB:{os.path.join(obj_dir, mod_name + ".lib")}',
                *objects,
                f'/OUT:{ext_path}',
            ]
        else:
            env = os.environ.copy()
            if sys.platform == 'darwin' and 'MACOSX_DEPLOYMENT_TARGET' not in env:
                target = sysconfig.get_config_var('MACOSX_DEPLOYMENT_TARGET')
                if target:
                    env['MACOSX_DEPLOYMENT_TARGET'] = str(target)
            default_cxx = 'clang++' if sys.platform == 'darwin' else 'g++'
            cxx = shlex.split(env.get('CXX', default_cxx))
            arch_flags = shlex.split(env.get('ARCHFLAGS', ''))
            objects = [os.path.join(obj_dir, f'{k}.o') for k in range(len(sources))]
            compile_cmds = [
                [
                    *cxx, '-c', '-fPIC', '-fvisibility=hidden', '-pthread', '-DNDEBUG',
                    *(f'-I{d}' for d in include_dirs),
                    *arch_flags,
                    *ext.extra_compile_args,
                    src, '-o', obj,
                ]
                for src, obj in zip(sources, objects)
            ]
            if sys.platform == 'darwin':
                link_cmd = [*cxx, '-bundle', '-undefined', 'dynamic_lookup', *arch_flags, *objects, '-o', ext_path]
            else:
                link_cmd = [*cxx, '-shared', '-pthread', *arch_flags, *objects, '-o', ext_path]

        with concurrent.futures.ThreadPoolExecutor(max_workers=os.cpu_count() or 4) as executor:
            futures = [executor.submit(subprocess.check_call, cmd, env=env) for cmd in compile_cmds]
            for f in concurrent.futures.as_completed(futures):
                f.result()
        subprocess.check_call(link_cmd, env=env)


setup(
    name='stim',
    version=__version__,
    author='Craig Gidney',
    author_email='craig.gidney@gmail.com',
    url='https://github.com/quantumlib/stim',
    license='Apache 2',
    description='A fast library for analyzing with quantum stabilizer circuits.',
    long_description=long_description,
    long_description_content_type='text/markdown',
    ext_modules=_get_extensions(),
    cmdclass={'build_ext': BuildExtDirect},
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
