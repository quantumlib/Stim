"""
Iterates over modules and classes, listing their attributes and methods in markdown.
"""

import stim

import sys

from util_gen_stub_file import generate_documentation


def main():
    version = stim.__version__
    if "dev" in version or version == "VERSION_INFO" or "-dev" in sys.argv:
        version = "(Development Version)"
        is_dev = True
    else:
        version = "v" + version
        is_dev = False
    objects = [
        obj
        for obj in generate_documentation(obj=stim, full_name="stim", level=0)
        if all('[DEPRECATED]' not in line for line in obj.lines)
    ]

    print(f"# Stim {version} API Reference")
    print()
    if is_dev:
        print("*CAUTION*: this API reference is for the in-development version of stim.")
        print("Methods and arguments mentioned here may not be accessible in stable versions, yet.")
        print("API references for stable versions are kept on the [stim github wiki](https://github.com/quantumlib/Stim/wiki)")
        print()
    print("## Index")
    for obj in objects:
        level = obj.level
        print((level - 1) * "    " + f"- [`{obj.full_name}`](#{obj.full_name})")

    print(f'''
```python
# Types used by the method definitions.
from typing import overload, TYPE_CHECKING, Any, Dict, Iterable, List, Optional, Tuple, Union
import io
import pathlib
import numpy as np
```
'''.strip())

    for obj in objects:
        print()
        print(f'<a name="{obj.full_name}"></a>')
        print("```python")
        print(f'# {obj.full_name}')
        print()
        if len(obj.full_name.split('.')) > 2:
            print(f'# (in class {".".join(obj.full_name.split(".")[:-1])})')
        else:
            print(f'# (at top-level in the stim module)')
        print('\n'.join(obj.lines))
        print("```")


if __name__ == '__main__':
    main()
