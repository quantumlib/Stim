#!/usr/bin/env python3

"""
Produces a .pyi file for stim, describing the contained classes and functions.
"""

from typing import Optional, Iterator, List

import stim

import inspect
import sys

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
    "__iter__",
    "__next__",
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
    return "".join(
        indentation * (line != '\n') + line
        for line in paragraph.splitlines(keepends=True)
    )


class DescribedObject:
    def __init__(self):
        self.full_name = ""
        self.level = 0
        self.lines = []


def splay_signature(sig: str) -> List[str]:
    assert sig.startswith('def')
    out = []

    level = 0

    start = sig.index('(') + 1
    mark = start
    out.append(sig[:mark])
    for k in range(mark, len(sig)):
        c = sig[k]
        if c in '([':
            level += 1
        if c in '])':
            level -= 1
        if (c == ',' and level == 0) or level < 0:
            k2 = k + (0 if level < 0 else 1)
            s = sig[mark:k2].lstrip()
            if s:
                if not s.endswith(','):
                    s += ','
                out.append('    ' + s)
            mark = k2
        if level < 0:
            break
    assert level == -1
    out.append(sig[mark:])
    return out


def print_doc(*, full_name: str, parent: object, obj: object, level: int) -> Optional[DescribedObject]:
    out_obj = DescribedObject()
    out_obj.full_name = full_name
    out_obj.level = level
    doc = getattr(obj, "__doc__", None) or ""
    doc = normalize_doc_string(doc)
    if full_name.endswith("__") and len(doc.splitlines()) <= 2:
        return None

    term_name = full_name.split(".")[-1]
    is_property = isinstance(obj, property)
    is_method = doc.startswith(term_name)
    has_setter = False
    sig_name = ''
    if is_method or is_property:
        if is_property:
            out_obj.lines.append("@property")
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
                out_obj.lines.append("@overload")
                is_static = '(self' not in sig and inspect.isclass(parent)
                if is_static:
                    out_obj.lines.append("@staticmethod")
                out_obj.lines.extend(splay_signature(sig))
                out_obj.lines.append("    pass")
            elif '@signature ' in line:
                _, sig = line.split('@signature ')
                is_static = '(self' not in sig and inspect.isclass(parent)
                if is_static:
                    out_obj.lines.append("@staticmethod")
                out_obj.lines.extend(splay_signature(sig))
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
        text = ""
        if not sig_handled:
            if "(self: " in sig_name:
                k_low = sig_name.index("(self: ") + len('(self')
                k_high = len(sig_name)
                if '->' in sig_name: k_high = sig_name.index('->', k_low, k_high)
                k_high = sig_name.index(", " if ", " in sig_name[k_low:k_high] else ")", k_low, k_high)
                sig_name = sig_name[:k_low] + sig_name[k_high:]
            if not sig_handled:
                is_static = '(self' not in sig_name and inspect.isclass(parent)
                if is_static:
                    out_obj.lines.append("@staticmethod")
            sig_name = sig_name.replace(': handle', ': Any')
            sig_name = sig_name.replace('numpy.', 'np.')
            if new_args_name is not None:
                sig_name = sig_name.replace('*args', new_args_name)
            text = "\n".join(splay_signature(f"def {sig_name}:"))
    else:
        text = f"class {term_name}:"
    if doc:
        text += "\n" + indented(paragraph=f"\"\"\"{doc}\n\"\"\"",
                                indentation="    ")
    out_obj.lines.append(text.replace('._stim_avx2', ''))
    if has_setter:
        if '->' in sig_name:
            setter_type = sig_name[sig_name.index('->') + 2:].strip().replace('._stim_avx2', '')
        else:
            setter_type = 'Any'
        out_obj.lines.append(f"@{term_name}.setter")
        out_obj.lines.append(f"def {term_name}(self, value: {setter_type}):")
        out_obj.lines.append(f"    pass")
    return out_obj


def generate_documentation(*, obj: object, level: int, full_name: str) -> Iterator[DescribedObject]:

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
        v = print_doc(full_name=sub_full_name, obj=sub_obj, level=level + 1, parent=obj)
        if v is not None:
            yield v
        yield from generate_documentation(
            obj=sub_obj,
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
    import io
    import pathlib
    import numpy as np
    import stim
'''.strip())

    for obj in generate_documentation(obj=stim, full_name="stim", level=-1):

        print('\n'.join(("    " * obj.level + line).rstrip()
                        for paragraph in obj.lines
                        for line in paragraph.splitlines()))


if __name__ == '__main__':
    main()
