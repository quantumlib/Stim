# Copyright 2025 Google LLC
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

from __future__ import annotations

import importlib.util
import os
from pathlib import Path
import platform
import shutil
import site
import sys
import sysconfig

import time

# When pip runs with build isolation in an offline/restricted environment,
# ensure the base interpreter's site-packages (containing setuptools and wheel)
# are available on sys.path.
if importlib.util.find_spec("setuptools") is None:
    candidate_dirs = [
        sysconfig.get_paths().get("purelib"),
        sysconfig.get_paths().get("platlib"),
        site.getusersitepackages(),
    ]
    for candidate in candidate_dirs:
        if candidate and os.path.isdir(candidate) and candidate not in sys.path:
            site.addsitedir(candidate)

from setuptools import Extension, find_packages, setup
from setuptools.command.build_ext import build_ext

__version__ = "0.1.0"

PROJECT_ROOT = Path(__file__).resolve().parent


def _get_compile_and_link_args() -> tuple[list[str], list[str]]:
    system = platform.system()
    machine = (platform.machine() or platform.processor() or "").lower()
    if system.startswith("Win"):
        compile_args = ["/std:c++17", "/O2"]
        link_args: list[str] = []
    else:
        compile_args = [
            "-std=c++17",
            "-O3",
            "-fPIC",
            "-fno-strict-aliasing",
            "-g0",
        ]
        if any(arch in machine for arch in ("x86_64", "amd64", "i686", "i386", "arm64", "aarch64")):
            compile_args.append("-march=native")
        link_args = ["-O3", "-shared"]
    return compile_args, link_args


class StimsideBuildExt(build_ext):
    """Builds C++ ctypes shared libraries (.so) without Python ABI suffixes."""

    def get_ext_filename(self, ext_name: str) -> str:
        parts = ext_name.split(".")
        return os.path.join(*parts) + ".so"

    def get_export_symbols(self, ext: Extension) -> list[str]:
        # Pure C++ extern "C" shared libraries loaded via ctypes do not define PyInit_<name>.
        return []

    def run(self) -> None:
        super().run()
        self._sync_extensions_to_source_dir()

    def copy_extensions_to_source(self) -> None:
        self._sync_extensions_to_source_dir()

    def _sync_extensions_to_source_dir(self) -> None:
        package_dir_map = self.distribution.package_dir or {"": "src"}
        root_src = package_dir_map.get("", "src")
        for ext in self.extensions:
            rel_ext_path = self.get_ext_filename(ext.name)
            built_path = (Path(self.build_lib) / rel_ext_path).resolve()
            if not built_path.exists():
                continue
            max_src_mtime = max(
                (
                    (PROJECT_ROOT / src).stat().st_mtime
                    for src in ext.sources
                    if (PROJECT_ROOT / src).exists()
                ),
                default=time.time(),
            )
            target_mtime = max(time.time(), max_src_mtime + 1.0)
            os.utime(built_path, (target_mtime, target_mtime))
            dest_path = (PROJECT_ROOT / root_src / rel_ext_path).resolve()
            cache_path = (PROJECT_ROOT / "python_build_stimside" / "lib" / rel_ext_path).resolve()
            for target_path in (dest_path, cache_path):
                try:
                    target_path.parent.mkdir(parents=True, exist_ok=True)
                    if built_path != target_path:
                        tmp_dest = target_path.with_name(f".{target_path.name}.{os.getpid()}.tmp")
                        shutil.copy2(built_path, tmp_dest)
                        os.utime(tmp_dest, (target_mtime, target_mtime))
                        os.replace(tmp_dest, target_path)
                except OSError:
                    if self.inplace and target_path == dest_path:
                        raise


def _get_extensions() -> list[Extension]:
    compile_args, link_args = _get_compile_and_link_args()
    return [
        Extension(
            name="stimside.util.libcoset_kernels",
            sources=["src/stimside/util/coset_kernels.cpp"],
            language="c++",
            extra_compile_args=compile_args,
            extra_link_args=link_args,
        ),
        Extension(
            name="stimside.util.libtableside_kernels",
            sources=["src/stimside/util/tableside_kernels.cpp"],
            language="c++",
            extra_compile_args=compile_args,
            extra_link_args=link_args,
        ),
        Extension(
            name="stimside.util.libmarginal_dem_kernels",
            sources=["src/stimside/util/marginal_dem_kernels.cpp"],
            language="c++",
            extra_compile_args=compile_args,
            extra_link_args=link_args,
        ),
    ]


if __name__ != "__main__":
    # Expose PEP 517 / PEP 660 build-backend hooks when imported by pip/build.
    from setuptools.build_meta import (  # noqa: F401
        build_editable,
        build_sdist,
        build_wheel,
        prepare_metadata_for_build_editable,
        prepare_metadata_for_build_wheel,
    )

    def get_requires_for_build_wheel(config_settings=None):
        return []

    def get_requires_for_build_editable(config_settings=None):
        return []

    def get_requires_for_build_sdist(config_settings=None):
        return []

else:
    readme_path = PROJECT_ROOT / "README.md"
    long_description = (
        readme_path.read_text(encoding="utf-8") if readme_path.exists() else ""
    )

    setup(
        name="stimside",
        version=__version__,
        author="Matt McEwen, Ming Li",
        author_email="mmcewen@google.com, mliq@google.com",
        description="Stimside simulator to simulate loss and leakage using Stim.",
        long_description=long_description,
        long_description_content_type="text/markdown",
        python_requires=">=3.10",
        package_dir={"": "src"},
        packages=find_packages(where="src"),
        package_data={
            "stimside.util": ["*.cpp", "*.h", "*.so"],
        },
        include_package_data=True,
        ext_modules=_get_extensions(),
        cmdclass={"build_ext": StimsideBuildExt},
        install_requires=[
            "numpy",
            "stim",
            "sinter",
        ],
        options={"build": {"build_base": "python_build_stimside"}},
    )
