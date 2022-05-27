"""
Iterates over modules and classes, listing their attributes and methods in markdown.
"""

import stim

import collections
import inspect
import sys
from typing import DefaultDict, List, Union

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


def normalize_doc_string(d: str) -> str:
    lines = d.splitlines()
    indented_lines = [e for e in lines[1:] if e.strip()]
    indentation = min([len(line) - len(line.lstrip()) for line in indented_lines], default=0)
    return "\n".join(lines[:1] + [e[indentation:] for e in lines[1:]])


def indented(*, paragraph: str, indentation: str) -> str:
    return "".join(indentation + line for line in paragraph.splitlines(keepends=True))


def print_doc(*, full_name: str, parent: object, obj: object, level: int):
    doc = getattr(obj, "__doc__", None) or ""
    doc = normalize_doc_string(doc)
    if full_name.endswith("__") and len(doc.splitlines()) <= 2:
        return

    term_name = full_name.split(".")[-1]
    is_property = isinstance(obj, property)
    is_method = doc.startswith(term_name)
    if term_name == "circuit_error_locations":
        xxx = 0
    has_setter = False
    sig_name = ''
    if is_method or is_property:
        if is_property:
            print("    " * level + "@property")
        doc_lines = doc.splitlines()
        doc_lines_left = []
        new_args_name = None
        was_args = False
        sig_handled = False
        for line in doc_lines:
            if was_args and line.strip().startswith('*') and ':' in line:
                new_args_name = line[line.index('*'):line.index(':')]
            if '@overload ' in line:
                _, sig = line.split('@overload ')
                print("    " * level + "@overload")
                print("    " * level + sig)
                print("    " * level + "    pass")
            elif '@signature ' in line:
                _, sig = line.split('@signature ')
                print("    " * level + sig)
                sig_handled = True
            else:
                doc_lines_left.append(line)
            was_args = 'Args:' in line
        if is_property:
            if hasattr(obj, 'fget'):
                sig_name = term_name + obj.fget.__doc__.replace('arg0', 'self').strip()
            else:
                sig_name = f'{term_name}(self)'
            if getattr(obj, 'fset', None) is not None:
                has_setter = True
        else:
            sig_name = term_name + doc_lines_left[0][len(term_name):]
            doc_lines_left = doc_lines_left[1:]
        doc = "\n".join(doc_lines_left).lstrip()
        if "(self: " in sig_name:
            k_low = sig_name.index("(self: ") + len('(self')
            k_high = len(sig_name)
            if '->' in sig_name: k_high = sig_name.index('->', k_low, k_high)
            k_high = sig_name.index(", " if ", " in sig_name[k_low:k_high] else ")", k_low, k_high)
            sig_name = sig_name[:k_low] + sig_name[k_high:]
        is_static = '(self' not in sig_name and inspect.isclass(parent)
        if is_static:
            print("    " * level + "@staticmethod")
        sig_name = sig_name.replace(': handle', ': Any')
        sig_name = sig_name.replace('numpy.', 'np.')
        if new_args_name is not None:
            sig_name = sig_name.replace('*args', new_args_name)
        text = ""
        if not sig_handled:
            text = "    " * level + f"def {sig_name}:"
    else:
        text = "    " * level + f"class {term_name}:"
    if doc:
        text += "\n" + indented(paragraph=f"\"\"\"{doc}\n\"\"\"",
                                indentation="    " * level + "    ")
    print(text)
    if has_setter:
        if '->' in sig_name:
            setter_type = sig_name[sig_name.index('->') + 2:].strip()
        else:
            setter_type = 'Any'
        print("    " * level + f"@{term_name}.setter")
        print("    " * level + f"def {term_name}(self, value: {setter_type}):")
        print("    " * level + f"    pass")


def generate_documentation(*, obj: object, level: int, full_name: str):

    if full_name.endswith("__"):
        return
    if not inspect.ismodule(obj) and not inspect.isclass(obj):
        return

    for sub_name in dir(obj):
        if sub_name in skip:
            continue
        if sub_name.startswith("__pybind11"):
            continue
        if sub_name.startswith('_') and not sub_name.startswith('__'):
            continue
        if sub_name.endswith("__") and sub_name not in keep:
            raise ValueError("Need to classify " + sub_name + " as keep or skip.")
        sub_full_name = full_name + "." + sub_name
        sub_obj = getattr(obj, sub_name)
        print_doc(full_name=sub_full_name, obj=sub_obj, level=level + 1, parent=obj)
        generate_documentation(obj=sub_obj,
                               level=level + 1,
                               full_name=sub_full_name)


def main():
    version = stim.__version__
    if "dev" in version or version == "VERSION_INFO" or "-dev" in sys.argv:
        version = "(Development Version)"
    else:
        version = "v" + version
    print(f'''
"""Stim {version}: a fast quantum stabilizer circuit library."""
# (This a stubs file describing the classes and methods in stim.)
from typing import overload, TYPE_CHECKING, List, Dict, Tuple, Any, Union, Iterable
if TYPE_CHECKING:
    import stim
    import numpy as np
'''.strip())

    generate_documentation(obj=stim, full_name="stim", level=-1)


if __name__ == '__main__':
    main()
