import stimflow


def test_with_xz_flipped():
    assert stimflow.Flow(start=stimflow.PauliMap({1: "X", 2: "Z"}), center=0).with_xz_flipped() == stimflow.Flow(
        start=stimflow.PauliMap({1: "Z", 2: "X"}), center=0
    )
