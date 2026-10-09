from __future__ import annotations

import ctypes
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time

u64_p = ctypes.POINTER(ctypes.c_uint64)
u8_p = ctypes.POINTER(ctypes.c_uint8)
i8_p = ctypes.POINTER(ctypes.c_int8)
i32_p = ctypes.POINTER(ctypes.c_int32)
i64_p = ctypes.POINTER(ctypes.c_int64)
int_p = ctypes.POINTER(ctypes.c_int)
void_p = ctypes.c_void_p

_LIB_INSTANCE: ctypes.CDLL | None = None


def _find_prebuilt_so_candidate(here: Path, lib_stem: str, cpp_path: Path) -> Path | None:
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


def _resolve_coset_kernels_so(force_recompile: bool = False) -> Path:
    here = Path(__file__).resolve().parent
    cpp_path = here / "coset_kernels.cpp"
    so_path = here / "libcoset_kernels.so"

    if not force_recompile:
        found = _find_prebuilt_so_candidate(here, "libcoset_kernels", cpp_path)
        if found is not None:
            if found.parent != here and os.access(here, os.W_OK):
                try:
                    tmp_copy = here / f".libcoset_kernels.{os.getpid()}.tmp.so"
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
        so_path = target_dir / "libcoset_kernels.so"
        tmp_so = target_dir / f"libcoset_kernels.{os.getpid()}.tmp.so"
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


def get_coset_kernels_lib() -> ctypes.CDLL:
    """Locate (or compile if needed) and return the cached ctypes CDLL for coset_kernels.cpp."""
    global _LIB_INSTANCE
    if _LIB_INSTANCE is not None:
        return _LIB_INSTANCE

    so_path = _resolve_coset_kernels_so()
    try:
        lib = ctypes.CDLL(str(so_path))
    except OSError:
        so_path = _resolve_coset_kernels_so(force_recompile=True)
        lib = ctypes.CDLL(str(so_path))

    lib.engine_create.argtypes = [
        ctypes.c_int,
        ctypes.c_int,
        ctypes.c_uint64,
        u64_p,
    ]
    lib.engine_create.restype = void_p

    lib.engine_destroy.argtypes = [void_p]
    lib.engine_destroy.restype = None

    lib.engine_set_rng_states.argtypes = [void_p, u64_p]
    lib.engine_set_rng_states.restype = None

    lib.engine_clear.argtypes = [void_p]
    lib.engine_clear.restype = None

    lib.engine_num_active.argtypes = [void_p]
    lib.engine_num_active.restype = ctypes.c_int

    lib.engine_get_active_shot_indices.argtypes = [void_p, i32_p, i32_p]
    lib.engine_get_active_shot_indices.restype = ctypes.c_int

    lib.engine_export_shot.argtypes = [
        void_p,
        ctypes.c_int,
        u64_p,
        u64_p,
        u64_p,
        u8_p,
        u8_p,
        i32_p,
    ]
    lib.engine_export_shot.restype = None

    lib.engine_import_shot.argtypes = [
        void_p,
        ctypes.c_int,
        ctypes.c_int,
        u64_p,
        u64_p,
        u64_p,
        u8_p,
        u8_p,
        i32_p,
    ]
    lib.engine_import_shot.restype = None

    lib.engine_batch_inject_cz.argtypes = [
        void_p,
        ctypes.c_int,
        i64_p,
        i64_p,
        u8_p,
        u8_p,
        u64_p,
        u64_p,
        u8_p,
        u64_p,
        u8_p,
        u8_p,
        int_p,
        int_p,
    ]
    lib.engine_batch_inject_cz.restype = None

    lib.engine_batch_inject_general_clifford.argtypes = [
        void_p,
        ctypes.c_int,
        ctypes.c_int,
        ctypes.c_int,
        i64_p,
        u8_p,
        u8_p,
        u8_p,
        u8_p,
        u8_p,
        u8_p,
        u64_p,
        u64_p,
        u8_p,
        u8_p,
        u8_p,
        u64_p,
        u64_p,
        u8_p,
        u64_p,
        u64_p,
        u8_p,
        u8_p,
        int_p,
        int_p,
    ]
    lib.engine_batch_inject_general_clifford.restype = None

    lib.engine_batch_det_meas_step.argtypes = [
        void_p,
        ctypes.c_int,
        i64_p,
        u8_p,
        u64_p,
        u8_p,
        u8_p,
        u64_p,
        u64_p,
        u8_p,
        u8_p,
        int_p,
        int_p,
        u8_p,
        int_p,
    ]
    lib.engine_batch_det_meas_step.restype = None

    lib.engine_batch_mixed_substep.argtypes = [
        void_p,
        ctypes.c_int,
        ctypes.c_int64,
        ctypes.c_uint8,
        ctypes.c_int64,
        u64_p,
        u64_p,
        u64_p,
        ctypes.c_int,
        ctypes.c_int,
        i64_p,
        ctypes.c_int,
        i64_p,
        u8_p,
        u8_p,
        u8_p,
        u8_p,
        int_p,
        int_p,
        u8_p,
        int_p,
    ]
    lib.engine_batch_mixed_substep.restype = None

    lib.engine_batch_evict.argtypes = [
        void_p,
        u64_p,
        u64_p,
        u8_p,
        u8_p,
        u8_p,
        u8_p,
        int_p,
        int_p,
    ]
    lib.engine_batch_evict.restype = None

    lib.engine_batch_unmatched_noisy_reset_z.argtypes = [
        void_p,
        u8_p,
        u8_p,
        u8_p,
        u64_p,
        u64_p,
        u8_p,
        u64_p,
        u8_p,
        u8_p,
        int_p,
        int_p,
    ]
    lib.engine_batch_unmatched_noisy_reset_z.restype = None

    lib.engine_peek_pauli_batch.argtypes = [
        void_p,
        ctypes.c_int,
        i64_p,
        ctypes.c_int,
        u8_p,
        u8_p,
        u64_p,
        u64_p,
        u64_p,
        u64_p,
        u8_p,
        u8_p,
        i8_p,
    ]
    lib.engine_peek_pauli_batch.restype = None

    _LIB_INSTANCE = lib
    return lib
