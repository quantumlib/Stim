from __future__ import annotations

import importlib.metadata

import crumpy as m


def test_version():
    assert m.__version__ in importlib.metadata.version("crumpy")
