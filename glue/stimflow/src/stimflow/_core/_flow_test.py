import pytest
import stim

import stimflow


def test_with_edits():
    flow = stimflow.Flow(
        start=stimflow.PauliMap({1: 'X', 2: 'Z'}),
        end=stimflow.PauliMap(),
        measurement_indices=[1],
        center=1j,
        flags={'test'},
        sign=None,
    )

    assert flow.with_edits(start=stimflow.PauliMap()) == stimflow.Flow(
        start=stimflow.PauliMap(),
        end=stimflow.PauliMap(),
        measurement_indices=[1],
        center=1j,
        flags={'test'},
        sign=None,
    )
    assert flow.with_edits(end=stimflow.PauliMap({1: 'X'})) == stimflow.Flow(
        start=stimflow.PauliMap({1: 'X', 2: 'Z'}),
        end=stimflow.PauliMap({1: 'X'}),
        measurement_indices=[1],
        center=1j,
        flags={'test'},
        sign=None,
    )
    assert flow.with_edits(measurement_indices=[2]) == stimflow.Flow(
        start=stimflow.PauliMap({1: 'X', 2: 'Z'}),
        end=stimflow.PauliMap(),
        measurement_indices=[2],
        center=1j,
        flags={'test'},
        sign=None,
    )
    assert flow.with_edits(center=None) == stimflow.Flow(
        start=stimflow.PauliMap({1: 'X', 2: 'Z'}),
        end=stimflow.PauliMap(),
        measurement_indices=[1],
        center=None,
        flags={'test'},
        sign=None,
    )
    assert flow.with_edits(flags=set()) == stimflow.Flow(
        start=stimflow.PauliMap({1: 'X', 2: 'Z'}),
        end=stimflow.PauliMap(),
        measurement_indices=[1],
        center=1j,
        flags=set(),
        sign=None,
    )
    assert flow.with_edits(sign=True) == stimflow.Flow(
        start=stimflow.PauliMap({1: 'X', 2: 'Z'}),
        end=stimflow.PauliMap(),
        measurement_indices=[1],
        center=1j,
        flags={'test'},
        sign=True,
    )
    assert flow.with_edits(obs_name='test') == stimflow.Flow(
        start=stimflow.PauliMap({1: 'X', 2: 'Z'}, obs_name='test'),
        end=stimflow.PauliMap(obs_name='test'),
        measurement_indices=[1],
        center=1j,
        flags={'test'},
        sign=None,
    )
    assert flow.with_edits(start=stimflow.PauliMap({1: 'X'}, obs_name='test'), obs_name='test') == stimflow.Flow(
        start=stimflow.PauliMap({1: 'X'}, obs_name='test'),
        end=stimflow.PauliMap(obs_name='test'),
        measurement_indices=[1],
        center=1j,
        flags={'test'},
        sign=None,
    )
    with pytest.raises(ValueError, match='contradict'):
        flow.with_edits(start=stimflow.PauliMap(obs_name='xx'), obs_name='test')
    with pytest.raises(ValueError, match='contradict'):
        flow.with_edits(end=stimflow.PauliMap(obs_name='xx'), obs_name='test')


def test_repr():
    flow = stimflow.Flow(
        start=stimflow.PauliMap({1: 'X', 2: 'Z'}),
        end=stimflow.PauliMap(),
        measurement_indices=[1],
        center=1j,
        flags={'test'},
        sign=None,
    )
    repr_text = repr(flow)
    round_tripped = eval(repr_text, {}, {'stimflow': stimflow})
    assert round_tripped == flow
    assert repr_text == repr(round_tripped)

    flow = stimflow.Flow(
        start=stimflow.PauliMap({1: 'X', 2: 'Z'}),
    )
    assert len(repr(flow)) < len(repr_text)  # Shorter.
    repr_text = repr(flow)
    round_tripped = eval(repr_text, {}, {'stimflow': stimflow})
    assert round_tripped == flow
    assert repr_text == repr(round_tripped)


def test_equality():
    ref = stimflow.Flow(
        start=stimflow.PauliMap({1: 'X', 2: 'Z'}),
        end=stimflow.PauliMap(),
        measurement_indices=[1],
        center=1j,
        flags={'test'},
        sign=None,
    )
    ref2 = stimflow.Flow(
        start=stimflow.PauliMap({1: 'X', 2: 'Z'}),
        end=stimflow.PauliMap(),
        measurement_indices=[1],
        center=1j,
        flags={'test'},
        sign=None,
    )
    assert ref == ref2
    assert hash(ref) == hash(ref2)
    assert ref != stimflow.Flow(
        start=stimflow.PauliMap({1: 'Y', 2: 'Z'}),
        end=stimflow.PauliMap(),
        measurement_indices=[1],
        center=1j,
        flags={'test'},
        sign=None,
    )
    assert ref != stimflow.Flow(
        start=stimflow.PauliMap({1: 'X', 2: 'Z'}),
        end=stimflow.PauliMap({1: 'Z'}),
        measurement_indices=[1],
        center=1j,
        flags={'test'},
        sign=None,
    )
    assert ref != stimflow.Flow(
        start=stimflow.PauliMap({1: 'X', 2: 'Z'}),
        end=stimflow.PauliMap(),
        measurement_indices=[2],
        center=1j,
        flags={'test'},
        sign=None,
    )
    assert ref != stimflow.Flow(
        start=stimflow.PauliMap({1: 'X', 2: 'Z'}),
        end=stimflow.PauliMap(),
        measurement_indices=[1],
        center=2j,
        flags={'test'},
        sign=None,
    )
    assert ref != stimflow.Flow(
        start=stimflow.PauliMap({1: 'X', 2: 'Z'}),
        end=stimflow.PauliMap(),
        measurement_indices=[1],
        center=1j,
        flags={'test2'},
        sign=None,
    )
    assert ref != stimflow.Flow(
        start=stimflow.PauliMap({1: 'X', 2: 'Z'}),
        end=stimflow.PauliMap(),
        measurement_indices=[1],
        center=1j,
        flags={'test'},
        sign=True,
    )


def test_to_stim_flow():
    assert stimflow.Flow(
        start=stimflow.PauliMap({1: 'X', 2: 'Z'}, obs_name='test'),
        end=stimflow.PauliMap(obs_name='test'),
        measurement_indices=[1],
        center=1j,
        flags={'test'},
        sign=True,
    ).to_stim_flow(q2i={1: 0, 2: 1}, o2i={'test': 3}) == stim.Flow("XZ -> -rec[1] xor obs[3]")


def test_obs_name():
    assert stimflow.Flow(
        start=stimflow.PauliMap({1: 'X', 2: 'Z'}, obs_name='test'),
        end=stimflow.PauliMap(obs_name='test'),
        measurement_indices=[1],
        center=1j,
        flags={'test'},
        sign=True,
    ).obs_name == 'test'

    assert stimflow.Flow(
        start=stimflow.PauliMap({1: 'X', 2: 'Z'}),
        end=stimflow.PauliMap(),
        measurement_indices=[1],
        center=1j,
        flags={'test'},
        sign=True,
    ).obs_name is None


def test_str():
    assert str(stimflow.Flow(
        start=stimflow.PauliMap({1: 'X', 2: 'Z'}),
        end=stimflow.PauliMap({3: 'Y'}),
        measurement_indices=[1],
    )) == '''X[1+0j]*Z[2+0j] -> Y[3+0j]*rec[1]'''
    assert str(stimflow.Flow(
        start=stimflow.PauliMap({1: 'X', 2: 'Z'}, obs_name='test'),
        end=stimflow.PauliMap(obs_name='test'),
        measurement_indices=[1],
        center=1j,
        flags={'test'},
        sign=False,
    )) == '''+X[1+0j]*Z[2+0j] -> rec[1] (obs=test) (flags=['test'])'''
    assert str(stimflow.Flow(
        start=stimflow.PauliMap({1: 'X', 2: 'Z'}, obs_name='test'),
        end=stimflow.PauliMap(obs_name='test'),
        measurement_indices=[1],
        center=1j,
        flags={'test'},
        sign=True,
    )) == '''-X[1+0j]*Z[2+0j] -> rec[1] (obs=test) (flags=['test'])'''


def test_with_xz_flipped():
    assert stimflow.Flow(start=stimflow.PauliMap({1: "X", 2: "Z"}), center=0).with_xz_flipped() == stimflow.Flow(
        start=stimflow.PauliMap({1: "Z", 2: "X"}), center=0
    )
    assert stimflow.Flow(
        start=stimflow.PauliMap({1: 'X', 2: 'Z'}, obs_name='test'),
        end=stimflow.PauliMap({3: 'Y', 4: 'X'}, obs_name='test'),
        measurement_indices=[1],
        center=1j,
        flags={'test'},
        sign=True,
    ).with_xz_flipped() == stimflow.Flow(
        start=stimflow.PauliMap({1: 'Z', 2: 'X'}, obs_name='test'),
        end=stimflow.PauliMap({3: 'Y', 4: 'Z'}, obs_name='test'),
        measurement_indices=[1],
        center=1j,
        flags={'test'},
        sign=True,
    )


def test_with_transformed_coords():
    assert stimflow.Flow(
        start=stimflow.PauliMap({1: 'X', 2: 'Z'}, obs_name='test'),
        end=stimflow.PauliMap({3: 'Y', 4: 'X'}, obs_name='test'),
        measurement_indices=[1],
        center=1j,
        flags={'test'},
        sign=True,
    ).with_transformed_coords(lambda e: e*2 + 1) == stimflow.Flow(
        start=stimflow.PauliMap({3: 'X', 5: 'Z'}, obs_name='test'),
        end=stimflow.PauliMap({7: 'Y', 9: 'X'}, obs_name='test'),
        measurement_indices=[1],
        center=2j + 1,
        flags={'test'},
        sign=True,
    )

    assert stimflow.Flow(
        start=stimflow.PauliMap({1: 'X', 2: 'Z'}, obs_name='test'),
        end=stimflow.PauliMap({3: 'Y', 4: 'X'}, obs_name='test'),
        measurement_indices=[1],
        center=None,
        flags={'test'},
        sign=True,
    ).with_transformed_coords(lambda e: e*2 + 1) == stimflow.Flow(
        start=stimflow.PauliMap({3: 'X', 5: 'Z'}, obs_name='test'),
        end=stimflow.PauliMap({7: 'Y', 9: 'X'}, obs_name='test'),
        measurement_indices=[1],
        center=None,
        flags={'test'},
        sign=True,
    )
