"""
Iterates over modules and classes, listing their attributes and methods in markdown.
"""

import stim

import collections
import inspect
import sys
from typing import DefaultDict, List, Union

from generate_stub_file import generate_documentation, DescribedObject

keep = {
    "__add__",
    "__eq__",
    "__call__",
    "__ge__",
    "__getitem__",
    "__gt__",
    "__iadd__",
    "__imul__",
    "__init__",
    "__truediv__",
    "__itruediv__",
    "__ne__",
    "__neg__",
    "__le__",
    "__len__",
    "__lt__",
    "__mul__",
    "__setitem__",
    "__str__",
    "__pos__",
    "__pow__",
    "__repr__",
    "__rmul__",
    "__hash__",
}
skip = {
    "__builtins__",
    "__cached__",
    "__getstate__",
    "__setstate__",
    "__path__",
    "__class__",
    "__delattr__",
    "__dir__",
    "__doc__",
    "__file__",
    "__format__",
    "__getattribute__",
    "__init_subclass__",
    "__loader__",
    "__module__",
    "__name__",
    "__new__",
    "__package__",
    "__reduce__",
    "__reduce_ex__",
    "__setattr__",
    "__sizeof__",
    "__spec__",
    "__subclasshook__",
    "__version__",
}


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
        print("*CAUTION*: this API reference is for the in-development version of Stim.")
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
from typing import overload, TYPE_CHECKING, List, Dict, Tuple, Any, Union, Iterable
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
