"""
Iterates over modules and classes, listing their attributes and methods in markdown.
"""

import stim

import collections
import inspect
from typing import DefaultDict, List

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
    "__ne__",
    "__neg__",
    "__le__",
    "__len__",
    "__lt__",
    "__mul__",
    "__str__",
    "__pos__",
    "__pow__",
    "__repr__",
    "__rmul__",
    "__hash__",
}
skip = {
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
    "__setitem__",
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


def print_doc(*, full_name: str, obj: object, level: int, outs: DefaultDict[int, List[str]]):
    doc = getattr(obj, "__doc__", None) or ""
    doc = normalize_doc_string(doc)
    if not full_name.endswith("__") or len(doc.splitlines()) > 2:
        term_name = full_name.split(".")[-1]
        if doc.startswith(term_name):
            doc_lines = doc.splitlines()
            full_name = full_name + doc_lines[0][len(term_name):]
            doc = "\n".join(doc_lines[1:]).lstrip()
            if "(self: " in full_name:
                k1 = full_name.index("(self: ") + 5
                k2 = full_name.index(", " if ", " in full_name[k1:] else ")", k1)
                full_name = full_name[:k1] + full_name[k2:]
        text = f"{level * '#'} `{full_name}`"
        if doc:
            text += "\n" + indented(paragraph=f"""```
{doc}
```""", indentation="> ")
        outs[level].append(text)


def generate_documentation(*, obj: object, level: int, full_name: str, outs: DefaultDict[int, List[str]]):
    if full_name.endswith("__"):
        return
    if not inspect.ismodule(obj) and not inspect.isclass(obj):
        return

    for sub_name in dir(obj):
        if sub_name in skip:
            continue
        if sub_name.endswith("__") and sub_name not in keep:
            raise ValueError("Need to classify " + sub_name + " as keep or skip.")
        sub_full_name = full_name + "." + sub_name
        sub_obj = getattr(obj, sub_name)
        print_doc(full_name=sub_full_name, obj=sub_obj, level=level, outs=outs)
        generate_documentation(obj=sub_obj,
                               level=level + 1,
                               full_name=sub_full_name,
                               outs=outs)


def main():
    level_entries = collections.defaultdict(list)
    generate_documentation(obj=stim, full_name="stim", outs=level_entries, level=2)
    print(f"# Stim v{stim.__version__} API Reference")
    for k in sorted(level_entries.keys()):
        for entry in level_entries[k]:
            print()
            print(entry)


if __name__ == '__main__':
    main()
