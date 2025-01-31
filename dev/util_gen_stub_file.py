import dataclasses
import types
from typing import Any
from typing import Optional, Iterator, List

import inspect
from typing import Tuple

keep = {
    "__add__",
    "__radd__",
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
    "__firstlineno__",
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
    "__annotations__",
    "__dataclass_fields__",
    "__dataclass_params__",
    "__dict__",
    "__match_args__",
    "__post_init__",
    "__weakref__",
    "__abstractmethods__",
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
    # Maintain backwards compatibility with python 3.6
    sig = sig.replace('list[', 'List[')
    sig = sig.replace('dict[', 'Dict[')
    sig = sig.replace('tuple[', 'Tuple[')
    sig = sig.replace('set[', 'Set[')

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


def _handle_pybind_method(
    *,
    obj: Any,
    is_property: bool,
    out_obj: DescribedObject,
    parent: Any,
    full_name: str,
) -> Tuple[str, bool, str, str]:
    doc = normalize_doc_string(getattr(obj, "__doc__", ""))
    if is_property:
        out_obj.lines.append("@property")
    doc_lines = doc.splitlines()
    new_args_name = None
    was_args = False
    sig_handled = False
    has_setter = False
    doc_lines_left = []
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

    term_name = full_name.split(".")[-1]
    if is_property:
        if hasattr(obj, 'fget'):
            sig_name = term_name + obj.fget.__doc__.replace('arg0', 'self').strip()
        else:
            sig_name = f'{term_name}(self)'
        if getattr(obj, 'fset', None) is not None:
            has_setter = True
    elif doc_lines_left[0].startswith(term_name):
        sig_name = term_name + doc_lines_left[0][len(term_name):]
        doc_lines_left = doc_lines_left[1:]
    else:
        sig_name = term_name

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
    return text, has_setter, doc, sig_name


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
    is_normal_method = isinstance(obj, types.FunctionType)
    sig_name = ''
    if 'sinter' in full_name and is_normal_method:
        text = ''
        if term_name in getattr(parent, '__abstractmethods__', []):
            text += '@abc.abstractmethod\n'
        sig_name = f'{term_name}{inspect.signature(obj)}'
        text += "\n".join(splay_signature(f"def {sig_name}:"))

        # Replace default value lambdas with their source.
        if 'lambda' in str(text):
            for name, param in inspect.signature(obj).parameters.items():
                if 'lambda' in str(param.default):
                    _, lambda_src = inspect.getsource(param.default).split('lambda ')
                    lambda_src = lambda_src.strip()
                    assert lambda_src.endswith(',')
                    lambda_src = 'lambda ' + lambda_src[:-1]
                    text = text.replace(str(param.default), lambda_src)

        text = text.replace('numpy.', 'np.')
    elif is_method or is_property:
        text, has_setter, doc, sig_name = _handle_pybind_method(
            obj=obj,
            is_property=is_property,
            out_obj=out_obj,
            parent=parent,
            full_name=full_name,
        )
    elif isinstance(obj, (int, str)):
        text = f"{term_name}: {type(obj).__name__} = {obj!r}"
        doc = ''
    elif term_name == term_name.upper():
        return None  # Skip constants because they lack a doc string.
    else:
        text = f"class {term_name}"
        if inspect.isabstract(obj):
            text += '(metaclass=abc.ABCMeta)'
        text += ':'
    if doc:
        if text:
            text += "\n"
        text += indented(paragraph=f"\"\"\"{doc.rstrip()}\n\"\"\"",
                         indentation="    ")

    dataclass_fields = getattr(obj, "__dataclass_fields__", [])
    if dataclass_fields:
        dataclass_prop ='@dataclasses.dataclass'
        if getattr(obj, '__dataclass_params__').frozen:
            dataclass_prop += '(frozen=True)'
        out_obj.lines.append(dataclass_prop)

    out_obj.lines.append(text.replace('._stim_avx2', '').replace('._stim_sse2', ''))
    if has_setter:
        if '->' in sig_name:
            setter_type = sig_name[sig_name.index('->') + 2:].strip().replace('._stim_avx2', '')
        else:
            setter_type = 'Any'
        out_obj.lines.append(f"@{term_name}.setter")
        out_obj.lines.append(f"def {term_name}(self, value: {setter_type}):")
        out_obj.lines.append(f"    pass")

    if dataclass_fields:
        for f in dataclasses.fields(obj):
            if str(f.type).startswith('typing'):
                t = str(f.type).replace('typing.', '')
            else:
                t = f.type.__name__
            t = t.replace('''Union[Dict[str, ForwardRef('JSON_TYPE')], List[ForwardRef('JSON_TYPE')], str, int, float]''', 'Any')
            if f.default is dataclasses.MISSING:
                out_obj.lines.append(f'    {f.name}: {t}')
            else:
                out_obj.lines.append(f'    {f.name}: {t} = {f.default}')

    return out_obj


def generate_documentation(*, obj: object, level: int, full_name: str) -> Iterator[DescribedObject]:
    if full_name.endswith("__"):
        return
    if not inspect.ismodule(obj) and not inspect.isclass(obj):
        return

    for sub_name in dir(obj):
        if sub_name in getattr(obj, '__dataclass_fields__', []):
            continue
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
