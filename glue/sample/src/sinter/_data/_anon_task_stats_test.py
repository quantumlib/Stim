import collections

import sinter


def test_repr():
    v = sinter.AnonTaskStats(shots=22, errors=3, discards=4, seconds=5)
    assert eval(repr(v), {"sinter": sinter}) == v
    v = sinter.AnonTaskStats()
    assert eval(repr(v), {"sinter": sinter}) == v
    v = sinter.AnonTaskStats(shots=22)
    assert eval(repr(v), {"sinter": sinter}) == v
    v = sinter.AnonTaskStats(shots=21, errors=4)
    assert eval(repr(v), {"sinter": sinter}) == v
    v = sinter.AnonTaskStats(shots=21, discards=4)
    assert eval(repr(v), {"sinter": sinter}) == v
    v = sinter.AnonTaskStats(seconds=4)
    assert eval(repr(v), {"sinter": sinter}) == v


def test_add():
    a0 = sinter.AnonTaskStats(shots=220, errors=30, discards=40, seconds=50)
    b0 = sinter.AnonTaskStats(shots=50, errors=4, discards=3, seconds=2)
    assert a0 + b0 == sinter.AnonTaskStats(shots=270, errors=34, discards=43, seconds=52)
    assert a0 + sinter.AnonTaskStats() == a0

    a = sinter.AnonTaskStats(shots=220, errors=30, discards=40, seconds=50, custom_counts=collections.Counter({'a': 10, 'b': 20}))
    b = sinter.AnonTaskStats(shots=50, errors=4, discards=3, seconds=2, custom_counts=collections.Counter({'a': 1, 'c': 3}))
    assert a + b == sinter.AnonTaskStats(shots=270, errors=34, discards=43, seconds=52, custom_counts=collections.Counter({'a': 11, 'b': 20, 'c': 3}))

    assert a + sinter.AnonTaskStats() == a
    assert sinter.AnonTaskStats() + b == b

    assert a + b0 == sinter.AnonTaskStats(shots=270, errors=34, discards=43, seconds=52, custom_counts=collections.Counter({'a': 10, 'b': 20}))
    assert a0 + b == sinter.AnonTaskStats(shots=270, errors=34, discards=43, seconds=52, custom_counts=collections.Counter({'a': 1, 'c': 3}))
