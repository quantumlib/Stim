#!/usr/bin/env python3

"""Runs doctests on a module, including any objects imported into the module."""
import argparse
import doctest
import inspect
import sys
from typing import Dict


def gen(*, obj: object, fullname: str, out: Dict[str, object]) -> None:
    if inspect.isfunction(obj) or inspect.ismethod(obj):
        out[fullname] = obj
    if not inspect.ismodule(obj) and not inspect.isclass(obj):
        return
    out[fullname] = obj

    for sub_name in dir(obj):
        if sub_name.startswith('_'):
            continue
        sub_obj = getattr(obj, sub_name, None)
        if inspect.ismodule(sub_obj):
            continue
        sub_full_name = fullname + "." + sub_name
        gen(obj=sub_obj, fullname=sub_full_name, out=out)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--module",
        type=str,
        required=True,
        nargs='+',
        help="The module to test. This module will be imported, its imported values will be recursively explored, and doctests will be run on them.")
    parser.add_argument(
        '--import',
        default=(),
        nargs='*',
        type=str,
        help="Modules to import for each doctest.")
    args = parser.parse_args()

    globs = {
        k: __import__(k) for k in getattr(args, 'import')
    }
    any_failed = False
    for module_name in args.module:
        module = __import__(module_name)
        out = {}
        gen(obj=module, fullname=module_name, out=out)
        module.__test__ = {k: v for k, v in out.items() if '__' not in k}
        if doctest.testmod(module, globs=globs).failed:
            any_failed = True
    if any_failed:
        sys.exit(1)


if __name__ == '__main__':
    main()
