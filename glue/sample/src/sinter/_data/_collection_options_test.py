import sinter


def test_repr():
    v = sinter.CollectionOptions()
    assert eval(repr(v), {"sinter": sinter}) == v
    v = sinter.CollectionOptions(max_shots=100)
    assert eval(repr(v), {"sinter": sinter}) == v
    v = sinter.CollectionOptions(max_errors=100)
    assert eval(repr(v), {"sinter": sinter}) == v
    v = sinter.CollectionOptions(start_batch_size=10)
    assert eval(repr(v), {"sinter": sinter}) == v
    v = sinter.CollectionOptions(max_batch_size=100)
    assert eval(repr(v), {"sinter": sinter}) == v
    v = sinter.CollectionOptions(max_batch_seconds=30)
    assert eval(repr(v), {"sinter": sinter}) == v
    v = sinter.CollectionOptions(max_shots=100, max_errors=90, start_batch_size=80, max_batch_size=200, max_batch_seconds=30)
    assert eval(repr(v), {"sinter": sinter}) == v


def test_combine():
    a = sinter.CollectionOptions(max_shots=200, max_batch_seconds=300)
    b = sinter.CollectionOptions(max_errors=100, max_batch_seconds=400)
    assert a.combine(b) == sinter.CollectionOptions(max_errors=100, max_shots=200, max_batch_seconds=300)
