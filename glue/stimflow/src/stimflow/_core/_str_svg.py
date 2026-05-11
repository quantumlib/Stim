from __future__ import annotations

import io
import pathlib
import sys


class str_svg(str):
    """A string that will display as an SVG image in Jupyter notebooks.

    It's expected that the contents of the string will correspond to the
    contents of an SVG file.
    """

    def __str__(self) -> str:
        """Strips down to a bare string."""
        return self.encode("utf-8").decode("utf-8")

    def _repr_svg_(self) -> str:
        """This is the method Jupyter notebooks look for, to show as an SVG."""
        return self

    def write_to(self, path: str | pathlib.Path | io.IOBase):
        """Write the contents to a file, and announce that it was done.

        This method exists for quick debugging. In many contexts, such as
        in a bash terminal or in PyCharm, the printed path can be clicked
        on to open the file.
        """
        if isinstance(path, io.IOBase):
            path.write(self)
            return
        path = pathlib.Path(path)
        path.parent.mkdir(exist_ok=True, parents=True)
        if isinstance(self, bytes):
            with open(path, "wb") as f:
                print(self, file=f)
        else:
            with open(path, "w") as f:
                print(self, file=f)
        print(f"wrote file://{pathlib.Path(path).absolute()}", file=sys.stderr)
