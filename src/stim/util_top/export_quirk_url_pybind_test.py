import stim


def test_to_quirk_url_simple():
    c = stim.Circuit("""
        QUBIT_COORDS(2, 1) 0
        QUBIT_COORDS(2, 2) 1
        H 0
        TICK
        CX 0 1
        TICK
        C_XYZ 0
        S 1
        TICK
        MZZ 1 0
    """)
    assert c.to_quirk_url() == 'https://algassert.com/quirk#circuit={"cols":[["H"],["•","X"],["~Cxyz","Z^½"],["xpar","xpar","X"],[1,1,"ZDetectControlReset"]],"gates":[{"id":"~Cxyz","name":"Cxyz","matrix":"{{½-½i,-½-½i},{½-½i,½+½i}}"}]}'


def test_to_quirk_url_complex():
    c = stim.Circuit.generated('surface_code:rotated_memory_x', distance=2, rounds=2, after_clifford_depolarization=0.001)
    assert '"H"' in c.to_quirk_url()
