import stim

import stimflow


def test_mul():
    a = "IIIIXXXXYYYYZZZZ"
    b = "IXYZ" * 4
    c = "IXYZXIZYYZIXZYXI"
    a = stimflow.PauliMap({q: p for q, p in enumerate(a) if p != "I"})
    b = stimflow.PauliMap({q: p for q, p in enumerate(b) if p != "I"})
    c = stimflow.PauliMap({q: p for q, p in enumerate(c) if p != "I"})
    assert a * b == c


def test_init():
    assert stimflow.PauliMap(stim.PauliString("_XYZ_XX")) == stimflow.PauliMap(
        {"X": [1, 5, 6], "Y": [2], "Z": [3]}
    )
