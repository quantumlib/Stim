"""This file is responsible for creating stim wheels.

Basically, it plays the role that "setup.py" plays in most packages.

Why does it exist? Because I got sick and tired of trying to get
setuptools to perform parallel builds and to lay out the wheel
files in the exact way that I wanted.
"""

import base64
import glob
import hashlib
import os
import pathlib
import platform
import subprocess
import sys
import sysconfig
import tempfile
import time
import zipfile

import pybind11

__version__ = '1.16.dev0'


def _get_wheel_tag() -> str:
    python_tag = {
        'cpython': f'cp{sys.version_info.major}{sys.version_info.minor}',
    }.get(sys.implementation.name)
    if python_tag is None:
        raise NotImplementedError(f"Don't know package tag for {sys.implementation.name=}")

    abi_tag = python_tag
    if sysconfig.get_config_var('Py_GIL_DISABLED'):
        abi_tag += 't'
    if sysconfig.get_config_var('Py_DEBUG'):
        abi_tag += 'd'

    mac_ver = platform.mac_ver()[0]
    if mac_ver:
        mac_ver = mac_ver.split('.')
    else:
        mac_ver = '?', '?'
    plat_tag = {
        (  'linux', 'x86_64' ): 'linux_x86_64',
        (  'linux', 'amd64'  ): 'linux_x86_64',
        (  'linux', 'aarch64'): 'linux_aarch64',
        (  'linux', 'arm64'  ): 'linux_arm64',
        ( 'darwin', 'x86_64' ): f'macosx_{mac_ver[0]}_{mac_ver[1]}_x86_64',
        ( 'darwin', 'amd64'  ): f'macosx_{mac_ver[0]}_{mac_ver[1]}_x86_64',
        ( 'darwin', 'aarch64'): f'macosx_{mac_ver[0]}_{mac_ver[1]}_aarch64',
        ( 'darwin', 'arm64'  ): f'macosx_{mac_ver[0]}_{mac_ver[1]}_arm64',
        ('windows', 'x86_64' ): 'win_amd64',
        ('windows', 'amd64'  ): 'win_amd64',
        ('windows', 'aarch64'): 'aarch64',
        ('windows', 'arm64'  ): 'arm64',
        ('windows', '????'   ): 'win32',
    }.get((platform.system().lower(), platform.machine().lower()))
    if plat_tag is None:
        raise NotImplementedError(f"Don't know platform tag for {platform.system()=} {platform.machine()=}")

    return f"{python_tag}-{abi_tag}-{plat_tag}"


def _get_content_hash(content: bytes) -> str:
    digest = hashlib.sha256(content).digest()
    hash_str = base64.urlsafe_b64encode(digest).decode().rstrip("=")
    return f"sha256={hash_str},{len(content)}"


def _run_processes_in_parallel(action_name, commands: list[list[str]]):
    """You're not gonna believe this, but this method...

    ...runs processes in parallel.

    It avoids starting more processes while cpu_count of them
    are currently running.
    """
    running = []
    try:
        cpus = os.cpu_count()
        left = len(commands)
        for step, cmd in enumerate(commands):
            # Busy-wait until fewer than n subprocesses are running.
            while len(running) == cpus:
                time.sleep(0.001)
                for k in range(cpus)[::-1]:
                    return_code = running[k].poll()
                    if return_code:
                        raise RuntimeError("A sub-process failed.")
                    if return_code is not None:
                        running[k] = running[-1]
                        running.pop()
                        left -= 1

            # Go!
            print(" ".join(cmd), file=sys.stderr)
            running.append(subprocess.Popen(cmd))
            print(f"# {action_name} (remaining={left} running={len(running)})", file=sys.stderr)

        # Wait for the remaining processes.
        while running:
            if running[-1].wait():
                raise RuntimeError("A sub-process failed.")
            running.pop()
            left -= 1
            if left:
                print(f"# {action_name} (remaining={left} running={len(running)})", file=sys.stderr)
        print(f"# done {action_name}", file=sys.stderr)
    finally:
        for r in running:
            try:
                r.kill()
            except:
                pass


def find_cl_exe() -> str:
    program_files = os.environ.get("ProgramFiles(x86)")
    if program_files is None:
        return 'cl.exe'

    vswhere = pathlib.Path(program_files) / "Microsoft Visual Studio" / "Installer" / "vswhere.exe"
    if not vswhere.exists():
        return 'cl.exe'

    vs_root = subprocess.check_output([
        str(vswhere),
        "-latest",
        "-products", "*",
        "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
        "-property", "installationPath"
    ], encoding='utf-8').strip()
    if not vs_root:
        return 'cl.exe'

    msvc_root = pathlib.Path(vs_root) / "VC" / "Tools" / "MSVC"
    version = sorted(msvc_root.iterdir())[-1].name
    arch = "x64" if platform.machine().lower() in ("amd64", "x86_64") else "x86"
    cl_path = msvc_root / version / "bin" / f"Host{arch}" / arch / "cl.exe"
    if not cl_path.exists():
        return 'cl.exe'

    return str(cl_path)


def build_wheel(wheel_directory, config_settings=None, metadata_directory=None):
    wheel_name = f'stim-{__version__}-{_get_wheel_tag()}.whl'
    wheel_path = pathlib.Path(wheel_directory) / wheel_name

    # Collect source files.
    ALL_SOURCE_FILES = glob.glob("src/**/*.cc", recursive=True)
    MUX_SOURCE_FILES = glob.glob("src/**/march.pybind.cc", recursive=True)
    TEST_FILES = glob.glob("src/**/*.test.cc", recursive=True)
    PERF_FILES = glob.glob("src/**/*.perf.cc", recursive=True)
    MAIN_FILES = glob.glob("src/**/main.cc", recursive=True)
    HEADER_FILES = glob.glob("src/**/*.h", recursive=True) + glob.glob("src/**/*.inl", recursive=True)
    RELEVANT_SOURCE_FILES = sorted(set(ALL_SOURCE_FILES) - set(TEST_FILES + PERF_FILES + MAIN_FILES + MUX_SOURCE_FILES))

    is_windows = platform.system().lower().startswith('win')
    # Determine the compiler to use.
    compiler = None
    if compiler is None and config_settings is not None:
        compiler = config_settings.get("compiler", None)
    if compiler is None:
        compiler = os.environ.get('CXX', None)
    if compiler is None:
        if is_windows:
            compiler = find_cl_exe()
        else:
            compiler = 'g++'

    # Determine the linker to use.
    linker = None
    if linker is None and config_settings is not None:
        linker = config_settings.get("linker", None)
    if linker is None:
        if is_windows:
            linker = find_cl_exe()[:-6] + 'link.exe'
        else:
            linker = compiler

    # Plan out compiler and linker commands.
    configs = {
        '_detect_machine_architecture': ((), MUX_SOURCE_FILES),
        '_stim_polyfill': ((), RELEVANT_SOURCE_FILES),
        '_stim_sse2': (
            ('/arch:SSE2',) if is_windows else ('-msse2', '-mno-avx2',),
            RELEVANT_SOURCE_FILES,
        ),
        # NOTE: disabled until https://github.com/quantumlib/Stim/issues/432 is fixed
        # '_stim_avx': (('-msse2', '-mavx2',), RELEVANT_SOURCE_FILES),
    }

    if is_windows:
        so = 'pyd'
    else:
        so = 'so'

    with tempfile.TemporaryDirectory() as temp_dir:
        link_outputs = []
        temp_dir: pathlib.Path = pathlib.Path(temp_dir)
        compile_commands: list[list[str]] = []
        link_commands: list[list[str]] = []
        for name, (flags, files) in configs.items():
            object_paths = []
            for src_path in files:
                out_path: str = str(temp_dir / name / src_path) + ".o"
                object_paths.append(out_path)
                pathlib.Path(out_path).parent.mkdir(parents=True, exist_ok=True)
                if is_windows:
                    compile_commands.append([
                        compiler,
                        "/c", src_path,
                        f"/Fo{out_path}",
                        "/Isrc",
                        f"/I{pybind11.get_include()}",
                        f"/I{sysconfig.get_path('include')}",
                        "/W4",
                        "/std=c++20",
                        "/O2",
                        "/MD",
                        "/EHsc",
                        "/nologo",
                        "/DNDEBUG",
                        f"/DVERSION_INFO={__version__}",
                        f"/DSTIM_PYBIND11_MODULE_NAME={name}",
                        *flags,
                    ])
                else:
                    compile_commands.append([
                        compiler,
                        "-c", src_path,
                        "-o", out_path,
                        "-Isrc",
                        f"-I{pybind11.get_include()}",
                        f"-I{sysconfig.get_path('include')}",
                        "-Wall",
                        "-fPIC",
                        "-std=c++20",
                        "-fno-strict-aliasing",
                        "-fvisibility=hidden",
                        "-O3",
                        "-g0",
                        "-DNDEBUG",
                        f"-DVERSION_INFO={__version__}",
                        f"-DSTIM_PYBIND11_MODULE_NAME={name}",
                        *flags,
                    ])
            link_out = str(temp_dir / name / f'{name}.{so}')
            if is_windows:
                link_commands.append([
                    linker,
                    "/DLL",
                    f"/OUT:{link_out}",
                    "/nologo",
                    f"/LIBPATH:{pathlib.Path(sysconfig.get_config_var('BINDIR')) / 'libs'}",
                    *object_paths,
                ])
            else:
                osx_flags = []
                if platform.system().lower() == 'darwin':
                    osx_flags = ["-undefined", "dynamic_lookup"]
                link_commands.append([
                    linker,
                    "-shared",
                    *osx_flags,
                    "-o", link_out,
                    *object_paths,
                ])

        # Perform compilation and linking.
        _run_processes_in_parallel("compiling", compile_commands)
        _run_processes_in_parallel("linking", link_commands)

        # Define the files to put into the wheel file.
        files: dict[str, bytes] = {}
        dist_info_dir = f"stim-{__version__}.dist-info"
        files[f'{dist_info_dir}/top_level.txt'] = "stim".encode('UTF-8')
        files[f'{dist_info_dir}/WHEEL'] = f"""
Wheel-Version: 1.0
Generator: stim_custom_setup
Root-Is-Purelib: false
Tag: {_get_wheel_tag()}
""".lstrip().encode('UTF-8')
        with open('LICENSE', 'rb') as f:
            files[f'{dist_info_dir}/license/LICENSE'] = f.read()
        for file in pathlib.Path("glue/python/src/stim").iterdir():
            with open(file, 'rb') as f:
                files[f'stim/{file.name}'] = f.read()
        for name in configs.keys():
            with open(temp_dir / name / f'{name}.{so}', 'rb') as f:
                files[f'stim/{name}' + sysconfig.get_config_var('EXT_SUFFIX')] = f.read()
        files[f'{dist_info_dir}/entry_points.txt'] = """
[console_scripts]
stim = stim._main_argv:main_argv
""".strip().encode('UTF-8')
        with open('glue/python/README.md', encoding='UTF-8') as f:
            files[f'{dist_info_dir}/METADATA'] = ("""
Metadata-Version: 2.4
Name: stim
Version: 1.15.0
Summary: A fast library for analyzing with quantum stabilizer circuits.
Home-page: https://github.com/quantumlib/stim
Author: Craig Gidney
Author-email: craig.gidney@gmail.com
License: Apache 2
Requires-Python: >=3.10.0
Description-Content-Type: text/markdown
License-File: LICENSE
Requires-Dist: numpy

""".lstrip() + f.read()).encode('UTF-8')

        # Record files and their hashes in the RECORD file.
        records = [f"{dist_info_dir}/RECORD,,"]
        for k, v in files.items():
            records.append(f"{k},{_get_content_hash(v)}")
        files[f'{dist_info_dir}/RECORD'] = "\n".join(records).encode('UTF-8')

        # Write the wheel file.
        with zipfile.ZipFile(wheel_path, 'w', compression=zipfile.ZIP_DEFLATED) as f:
            for k, v in files.items():
                f.writestr(k, v)

    return wheel_name


if __name__ == '__main__':
    build_wheel('')
