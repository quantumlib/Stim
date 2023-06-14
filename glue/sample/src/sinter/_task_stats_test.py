import collections

import pytest

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
    assert v.to_csv_line() == str(v) == '        22,         3,         4,       5,pymatching,test,"{""a"":[1,2,3]}",'


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


def test_add():
    a = sinter.TaskStats(
        decoder='pymatching',
        json_metadata={'a': 2},
        strong_id='abcdef',
        shots=220,
        errors=30,
        discards=40,
        seconds=50,
        custom_counts=collections.Counter({'a': 10, 'b': 20}),
    )
    b = sinter.TaskStats(
        decoder='pymatching',
        json_metadata={'a': 2},
        strong_id='abcdef',
        shots=50,
        errors=4,
        discards=3,
        seconds=2,
        custom_counts=collections.Counter({'a': 1, 'c': 3}),
    )
    c = sinter.TaskStats(
        decoder='pymatching',
        json_metadata={'a': 2},
        strong_id='abcdef',
        shots=270,
        errors=34,
        discards=43,
        seconds=52,
        custom_counts=collections.Counter({'a': 11, 'b': 20, 'c': 3}),
    )
    assert a + b == c
    with pytest.raises(ValueError):
        a + sinter.TaskStats(
            decoder='pymatching',
            json_metadata={'a': 2},
            strong_id='abcdefDIFFERENT',
            shots=270,
            errors=34,
            discards=43,
            seconds=52,
            custom_counts=collections.Counter({'a': 11, 'b': 20, 'c': 3}),
        )
