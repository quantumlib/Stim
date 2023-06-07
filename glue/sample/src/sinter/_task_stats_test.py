import sinter


def test_repr():
    v = sinter.TaskStats(
        strong_id='test',
        json_metadata={'a': [1, 2, 3]},
        decoder='pymatching',
        shots=22,
        errors=3,
        discards=4,
        seconds=5,
    )
    assert eval(repr(v), {"sinter": sinter}) == v


def test_to_csv_line():
    v = sinter.TaskStats(
        strong_id='test',
        json_metadata={'a': [1, 2, 3]},
        decoder='pymatching',
        shots=22,
        errors=3,
        discards=4,
        seconds=5,
    )
    assert v.to_csv_line() == str(v) == '        22,         3,         4,       5,pymatching,test,"{""a"":[1,2,3]}"'


def test_to_anon_stats():
    v = sinter.TaskStats(
        strong_id='test',
        json_metadata={'a': [1, 2, 3]},
        decoder='pymatching',
        shots=22,
        errors=3,
        discards=4,
        seconds=5,
    )
    assert v.to_anon_stats() == sinter.AnonTaskStats(shots=22, errors=3, discards=4, seconds=5)
