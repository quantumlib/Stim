from __future__ import annotations

from pathlib import Path
import shutil
import subprocess
from unittest import mock

import pytest

import stimside
from stimside.util import coset_kernels, tableside_kernels


def test_top_level_stimside_exports() -> None:
    assert hasattr(stimside, "__version__")
    assert hasattr(stimside, "FlipsideSimulator")
    assert hasattr(stimside, "TablesideSimulator")
    assert hasattr(stimside, "CosetsideSimulator")
    assert hasattr(stimside, "FlipsideSampler")
    assert hasattr(stimside, "TablesideSampler")
    assert hasattr(stimside, "CosetsideSampler")


def test_prebuilt_kernels_load_without_runtime_subprocess() -> None:
    # Ensure kernels are present (either from pip install / python_build_stimside or fallback build),
    # then verify that loading from disk never invokes subprocess.run.
    coset_kernels._resolve_coset_kernels_so()
    tableside_kernels._resolve_tableside_kernels_so()

    coset_kernels._LIB_INSTANCE = None
    tableside_kernels._LIB_INSTANCE = None

    with mock.patch.object(
        subprocess,
        "run",
        side_effect=AssertionError("subprocess.run should not be called when .so is already built"),
    ):
        coset_lib = coset_kernels.get_coset_kernels_lib()
        tableside_lib = tableside_kernels.get_tableside_kernels_lib()
        assert coset_lib is not None
        assert tableside_lib is not None


def test_wheel_install_resolution_ignores_cpp_mtime_and_missing_cpp(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    fake_pkg = tmp_path / "site-packages" / "stimside" / "util"
    fake_pkg.mkdir(parents=True)

    real_coset_so = coset_kernels._resolve_coset_kernels_so()
    real_tableside_so = tableside_kernels._resolve_tableside_kernels_so()
    shutil.copy2(real_coset_so, fake_pkg / "libcoset_kernels.so")
    shutil.copy2(real_tableside_so, fake_pkg / "libtableside_kernels.so")

    monkeypatch.setattr(coset_kernels, "__file__", str(fake_pkg / "coset_kernels.py"))
    monkeypatch.setattr(
        tableside_kernels, "__file__", str(fake_pkg / "tableside_kernels.py")
    )

    with mock.patch.object(
        subprocess,
        "run",
        side_effect=AssertionError("subprocess.run should not be called in site-packages"),
    ):
        assert coset_kernels._resolve_coset_kernels_so() == fake_pkg / "libcoset_kernels.so"
        assert (
            tableside_kernels._resolve_tableside_kernels_so()
            == fake_pkg / "libtableside_kernels.so"
        )


def test_source_tree_discovers_prebuilt_so_from_sys_path_without_subprocess(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    real_coset_so = coset_kernels._resolve_coset_kernels_so()
    real_tableside_so = tableside_kernels._resolve_tableside_kernels_so()

    fake_site = tmp_path / "installed_site"
    fake_site_util = fake_site / "stimside" / "util"
    fake_site_util.mkdir(parents=True)
    shutil.copy2(real_coset_so, fake_site_util / "libcoset_kernels.so")
    shutil.copy2(real_tableside_so, fake_site_util / "libtableside_kernels.so")

    fake_src_util = tmp_path / "repo" / "src" / "stimside" / "util"
    fake_src_util.mkdir(parents=True)
    (tmp_path / "repo" / "setup.py").write_text("# dummy\n", encoding="utf-8")

    monkeypatch.syspath_prepend(str(fake_site))
    monkeypatch.setattr(coset_kernels, "__file__", str(fake_src_util / "coset_kernels.py"))
    monkeypatch.setattr(
        tableside_kernels, "__file__", str(fake_src_util / "tableside_kernels.py")
    )

    with mock.patch.object(
        subprocess,
        "run",
        side_effect=AssertionError("subprocess.run should not be called when prebuilt .so is on sys.path"),
    ):
        resolved_coset = coset_kernels._resolve_coset_kernels_so()
        resolved_tableside = tableside_kernels._resolve_tableside_kernels_so()
        assert resolved_coset.exists()
        assert resolved_tableside.exists()


def test_missing_so_and_cpp_raises_clear_file_not_found(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    empty_util = tmp_path / "empty_util"
    empty_util.mkdir(parents=True)

    monkeypatch.setattr(coset_kernels, "_find_prebuilt_so_candidate", lambda *a, **kw: None)
    monkeypatch.setattr(tableside_kernels, "_find_prebuilt_so_candidate", lambda *a, **kw: None)
    monkeypatch.setattr(coset_kernels, "__file__", str(empty_util / "coset_kernels.py"))
    monkeypatch.setattr(
        tableside_kernels, "__file__", str(empty_util / "tableside_kernels.py")
    )

    with pytest.raises(FileNotFoundError, match="pip install"):
        coset_kernels._resolve_coset_kernels_so()

    with pytest.raises(FileNotFoundError, match="pip install"):
        tableside_kernels._resolve_tableside_kernels_so()

