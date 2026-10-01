"""Shared locate-or-compile logic for the ctypes kernel libraries (coset / tableside)."""

from __future__ import annotations

from collections.abc import Callable
import ctypes
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time


def find_prebuilt_so(here: Path, lib_stem: str, cpp_path: Path) -> Path | None:
    patterns = (f"{lib_stem}.so", f"{lib_stem}*.so", f"{lib_stem}*.dylib", f"{lib_stem}*.pyd", f"{lib_stem}*.dll")
    search_dirs: list[Path] = [here]

    repo_root = here.parent.parent.parent
    if (repo_root / "setup.py").exists():
        for build_dir_name in ("python_build_stimside", "build"):
            build_root = repo_root / build_dir_name
            if build_root.is_dir():
                for sub in sorted(build_root.glob("lib*/stimside/util")):
                    search_dirs.append(sub)

    for sys_entry in sys.path:
        if not sys_entry:
            continue
        candidate_dir = Path(sys_entry) / "stimside" / "util"
        if candidate_dir != here and candidate_dir.is_dir():
            search_dirs.append(candidate_dir)

    temp_dir = Path(tempfile.gettempdir())
    if temp_dir != here:
        search_dirs.append(temp_dir)

    cpp_mtime = cpp_path.stat().st_mtime if cpp_path.exists() else None
    for d in search_dirs:
        for pat in patterns:
            for match in sorted(d.glob(pat)):
                if not match.is_file():
                    continue
                if d != here and cpp_mtime is not None and match.stat().st_mtime < cpp_mtime:
                    continue
                return match
    return None


def resolve_so(
    module_file: str,
    name: str,
    force_recompile: bool = False,
    finder: Callable[[Path, str, Path], Path | None] = find_prebuilt_so,
) -> Path:
    """Locate lib<name>.so for <name>.cpp next to `module_file` (or a prebuilt copy); compile if missing/stale."""
    here = Path(module_file).resolve().parent
    cpp_path = here / f"{name}.cpp"
    so_path = here / f"lib{name}.so"

    if not force_recompile:
        found = finder(here, f"lib{name}", cpp_path)
        if found is not None:
            if found.parent != here and os.access(here, os.W_OK):
                try:
                    tmp_copy = here / f".lib{name}.{os.getpid()}.tmp.so"
                    shutil.copy2(found, tmp_copy)
                    os.replace(tmp_copy, so_path)
                    found = so_path
                except OSError:
                    pass
            so_path = found

    in_source_tree = (
        "site-packages" not in here.parts
        and "dist-packages" not in here.parts
        and (here.parent.parent.parent / "setup.py").exists()
    )
    need_compile = (
        force_recompile
        or not so_path.exists()
        or (
            in_source_tree
            and cpp_path.exists()
            and os.access(here, os.W_OK)
            and cpp_path.stat().st_mtime > so_path.stat().st_mtime
        )
    )
    if need_compile:
        if not cpp_path.exists():
            raise FileNotFoundError(
                f"Compiled shared library '{so_path.name}' not found in {here} "
                f"and source '{cpp_path.name}' is missing. "
                f"Please run 'pip install -e .' or 'pip install .' in Stim/glue/stimside."
            )
        target_dir = here if os.access(here, os.W_OK) else Path(tempfile.gettempdir())
        so_path = target_dir / f"lib{name}.so"
        tmp_so = target_dir / f"lib{name}.{os.getpid()}.tmp.so"
        cxx = (
            shutil.which(os.environ.get("CXX", ""))
            or os.environ.get("CXX")
            or shutil.which("g++")
            or shutil.which("c++")
            or shutil.which("clang++")
            or "g++"
        )
        cmd_variants = [
            [cxx, "-O3", "-std=c++17", "-march=native", "-shared", "-fPIC", str(cpp_path), "-o", str(tmp_so)],
            [cxx, "-O3", "-std=c++17", "-shared", "-fPIC", str(cpp_path), "-o", str(tmp_so)],
        ]
        last_exc: Exception | None = None
        for cmd in cmd_variants:
            try:
                subprocess.run(cmd, check=True)
                target_mtime = max(time.time(), cpp_path.stat().st_mtime + 1.0)
                os.utime(tmp_so, (target_mtime, target_mtime))
                os.replace(tmp_so, so_path)
                last_exc = None
                break
            except Exception as exc:
                last_exc = exc
                if tmp_so.exists():
                    tmp_so.unlink(missing_ok=True)
        if last_exc is not None and (force_recompile or not so_path.exists()):
            raise RuntimeError(
                f"Failed to compile {cpp_path} with {cmd_variants[0]}. "
                f"Please build/install stimside via 'pip install -e .' or 'pip install .'."
            ) from last_exc
    return so_path


def load_cdll(resolve: Callable[..., Path], sentinel: str | None = None) -> ctypes.CDLL:
    """Load the resolved library; recompile once if loading fails or `sentinel` is not exported."""
    try:
        lib = ctypes.CDLL(str(resolve()))
        if sentinel is not None:
            getattr(lib, sentinel)
    except (OSError, AttributeError):
        lib = ctypes.CDLL(str(resolve(force_recompile=True)))
    return lib
