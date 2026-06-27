import stim
import stimzx


def test_repr():
    e = stimzx.ExternalStabilizer(input=stim.PauliString("XX"), output=stim.PauliString("Y"))
    assert eval(repr(e), {'stimzx': stimzx, 'stim': stim}) == e
