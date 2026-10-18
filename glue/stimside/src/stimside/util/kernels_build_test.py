from __future__ import annotations

import ctypes
import functools
import os
from pathlib import Path
import shutil
import subprocess
from unittest import mock

import pytest

import stimside
from stimside.util import _kernel_loader, coset_kernels, marginal_dem_kernels, tableside_kernels

KERNELS = pytest.mark.parametrize(
    "mod,name",
    [
        (coset_kernels, "coset_kernels"),
        (tableside_kernels, "tableside_kernels"),
        (marginal_dem_kernels, "marginal_dem_kernels"),
    ],
    ids=["coset", "tableside", "marginal_dem"],
)


def _resolve(mod, name: str) -> Path:
    return getattr(mod, f"_resolve_{name}_so")()


def _forbid_subprocess(msg: str):
    return mock.patch.object(subprocess, "run", side_effect=AssertionError(msg))


def test_top_level_stimside_exports() -> None:
    assert hasattr(stimside, "__version__")
    assert hasattr(stimside, "FlipsideSimulator")
    assert hasattr(stimside, "TablesideSimulator")
    assert hasattr(stimside, "CosetsideSimulator")
    assert hasattr(stimside, "FlipsideSampler")
    assert hasattr(stimside, "TablesideSampler")
    assert hasattr(stimside, "CosetsideSampler")
    assert hasattr(stimside, "MarginalLeakageDemGenerator")


@KERNELS
def test_prebuilt_kernels_load_without_runtime_subprocess(mod, name) -> None:
    # Ensure kernels are present (either from pip install / python_build_stimside or fallback build),
    # then verify that loading from disk never invokes subprocess.run.
    _resolve(mod, name)
    mod._LIB_INSTANCE = None
    with _forbid_subprocess("subprocess.run should not be called when .so is already built"):
        assert getattr(mod, f"get_{name}_lib")() is not None


@KERNELS
def test_wheel_install_resolution_ignores_cpp_mtime_and_missing_cpp(
    mod, name, tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    fake_pkg = tmp_path / "site-packages" / "stimside" / "util"
    fake_pkg.mkdir(parents=True)
    shutil.copy2(_resolve(mod, name), fake_pkg / f"lib{name}.so")
    monkeypatch.setattr(mod, "__file__", str(fake_pkg / f"{name}.py"))
    with _forbid_subprocess("subprocess.run should not be called in site-packages"):
        assert _resolve(mod, name) == fake_pkg / f"lib{name}.so"


@KERNELS
def test_source_tree_discovers_prebuilt_so_from_sys_path_without_subprocess(
    mod, name, tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    fake_site_util = tmp_path / "installed_site" / "stimside" / "util"
    fake_site_util.mkdir(parents=True)
    shutil.copy2(_resolve(mod, name), fake_site_util / f"lib{name}.so")
    fake_src_util = tmp_path / "repo" / "src" / "stimside" / "util"
    fake_src_util.mkdir(parents=True)
    (tmp_path / "repo" / "setup.py").write_text("# dummy\n", encoding="utf-8")
    monkeypatch.syspath_prepend(str(tmp_path / "installed_site"))
    monkeypatch.setattr(mod, "__file__", str(fake_src_util / f"{name}.py"))
    with _forbid_subprocess("subprocess.run should not be called when prebuilt .so is on sys.path"):
        assert _resolve(mod, name).exists()


@KERNELS
def test_missing_so_and_cpp_raises_clear_file_not_found(
    mod, name, tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    empty_util = tmp_path / "empty_util"
    empty_util.mkdir(parents=True)
    monkeypatch.setattr(mod, "_find_prebuilt_so_candidate", lambda *a, **kw: None)
    monkeypatch.setattr(mod, "__file__", str(empty_util / f"{name}.py"))
    with pytest.raises(FileNotFoundError, match="pip install"):
        _resolve(mod, name)


def _stale_tiny_library(tmp_path: Path) -> tuple[functools.partial[Path], Path]:
    # A prebuilt library from an older layout, not older than the .cpp (so only the sentinel
    # can tell).
    util = tmp_path / "src" / "stimside" / "util"
    util.mkdir(parents=True)
    (tmp_path / "setup.py").write_text("# dummy\n", encoding="utf-8")
    cpp = util / "tiny.cpp"
    resolve = functools.partial(_kernel_loader.resolve_so, str(util / "tiny.py"), "tiny")
    cpp.write_text('extern "C" int tiny_abi_v1() { return 1; }\n', encoding="utf-8")
    so = resolve(force_recompile=True)
    cpp.write_text('extern "C" int tiny_abi_v2() { return 2; }\n', encoding="utf-8")
    t = cpp.stat().st_mtime + 10
    os.utime(so, (t, t))
    return resolve, so


def test_library_without_the_sentinel_is_rebuilt_and_used(tmp_path: Path) -> None:
    # The rebuilt library must be returned, not the stale one already loaded.
    resolve, _ = _stale_tiny_library(tmp_path)
    assert _kernel_loader.load_cdll(resolve, "tiny_abi_v2").tiny_abi_v2() == 2


def test_stale_library_that_stays_loaded_raises_instead_of_being_used(tmp_path: Path) -> None:
    resolve, so = _stale_tiny_library(tmp_path)
    held = ctypes.CDLL(str(so))  # another handle, so the stale library cannot be unloaded
    with pytest.raises(RuntimeError, match="restart Python"):
        _kernel_loader.load_cdll(resolve, "tiny_abi_v2")
    assert not hasattr(held, "tiny_abi_v2")
