from __future__ import annotations

from pathlib import Path
import shutil
import subprocess
from unittest import mock

import pytest

import stimside
from stimside.util import coset_kernels, tableside_kernels

KERNELS = pytest.mark.parametrize(
    "mod,name",
    [
        (coset_kernels, "coset_kernels"),
        (tableside_kernels, "tableside_kernels"),
    ],
    ids=["coset", "tablesid"],
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
