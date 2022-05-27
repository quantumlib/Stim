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


def print_doc(*, full_name: str, obj: object, level: int, outs: DefaultDict[int, List[str]], out_index: List[Union[List, str]]):
    doc = getattr(obj, "__doc__", None) or ""
    doc = normalize_doc_string(doc)
    if not full_name.endswith("__") or len(doc.splitlines()) > 2:
        sig_name = full_name
        term_name = full_name.split(".")[-1]
        doc_lines = doc.splitlines()
        doc_lines_left = []
        for line in doc_lines:
            if '@overload ' in line:
                pass
            elif '@signature ' in line:
                pass
            else:
                doc_lines_left.append(line)
        if doc.startswith(term_name):
            sig_name = sig_name + doc_lines_left[0][len(term_name):]
            if "(self: " in sig_name:
                k1 = sig_name.index("(self: ") + 5
                k2 = sig_name.index(", " if ", " in sig_name[k1:] else ")", k1)
                sig_name = sig_name[:k1] + sig_name[k2:]
            doc = "\n".join(doc_lines_left[1:]).lstrip()
        else:
            doc = "\n".join(doc_lines_left).lstrip()
        text = f"<a name=\"{full_name}\"></a>\n{level * '#'} `{sig_name}`"
        if "Exposed" in sig_name or "::" in sig_name:
            raise ValueError("Bad type annotation came out of pybind11:\n" + sig_name)
        if doc:
            assert '@signature' not in doc, doc
            text += "\n" + indented(paragraph=f"""```
{doc}
```""", indentation="> ")
        out_index.append(full_name)
        outs[level].append(text)


def generate_documentation(*, obj: object, level: int, full_name: str, outs: DefaultDict[int, List[str]], out_index: List[Union[List, str]]):

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
        print_doc(full_name=sub_full_name, obj=sub_obj, level=level, outs=outs, out_index=out_index)
        out_index.append([])
        generate_documentation(obj=sub_obj,
                               level=level + 1,
                               full_name=sub_full_name,
                               outs=outs,
                               out_index=out_index[-1])


def print_index(index: List[Union[List, str]], level: int = 0):
    if isinstance(index, list):
        for e in index:
            print_index(e, level + 1)
    else:
        print((level - 1) * "    " + f"- [`{index}`](#{index})")


def main():
    level_entries = collections.defaultdict(list)
    index = []
    generate_documentation(obj=stim, full_name="stim", outs=level_entries, level=2, out_index=index)
    version = stim.__version__
    if "dev" in version or version == "VERSION_INFO" or "-dev" in sys.argv:
        version = "(Development Version)"
    else:
        version = "v" + version
    print(f"# Stim {version} API Reference")
    print()
    print("## Index")
    print_index(index)

    for k in sorted(level_entries.keys()):
        for entry in level_entries[k]:
            print()
            print(entry.replace("stim._stim_avx2", "stim"))


if __name__ == '__main__':
    main()
