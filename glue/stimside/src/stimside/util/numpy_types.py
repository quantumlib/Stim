from typing import Any

import numpy as np

BoolNDArray = np.ndarray[Any, np.dtype[np.bool_]]
Bool1DArray = np.ndarray[tuple[int], np.dtype[np.bool_]]
Bool2DArray = np.ndarray[tuple[int, int], np.dtype[np.bool_]]

Float2DArray = np.ndarray[tuple[int, int], np.dtype[np.float64]]
Int2DArray = np.ndarray[tuple[int, int], np.dtype[np.int_]]
Uint82DArray = np.ndarray[tuple[int, int], np.dtype[np.uint8]]
