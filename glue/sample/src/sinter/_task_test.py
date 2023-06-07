import numpy as np

import stim

import sinter


def test_repr():
    circuit = stim.Circuit("""
        X_ERROR(0.1) 0 1 2
        M 0 1 2
        DETECTOR rec[-1] rec[-2]
        DETECTOR rec[-2] rec[-3]
        OBSERVABLE_INCLUDE(0) rec[-1]
    """)
    v = sinter.Task(circuit=circuit)
    assert eval(repr(v), {"stim": stim, "sinter": sinter, "np": np}) == v

    v = sinter.Task(circuit=circuit, detector_error_model=circuit.detector_error_model())
    assert eval(repr(v), {"stim": stim, "sinter": sinter, "np": np}) == v

    v = sinter.Task(circuit=circuit, postselection_mask=np.array([1], dtype=np.uint8))
    assert eval(repr(v), {"stim": stim, "sinter": sinter, "np": np}) == v

    v = sinter.Task(circuit=circuit, postselection_mask=np.array([2], dtype=np.uint8))
    assert eval(repr(v), {"stim": stim, "sinter": sinter, "np": np}) == v

    v = sinter.Task(circuit=circuit, postselected_observables_mask=np.array([1], dtype=np.uint8))
    assert eval(repr(v), {"stim": stim, "sinter": sinter, "np": np}) == v

    v = sinter.Task(circuit=circuit, collection_options=sinter.CollectionOptions(max_shots=10))
    assert eval(repr(v), {"stim": stim, "sinter": sinter, "np": np}) == v

    v = sinter.Task(circuit=circuit, json_metadata={'a': 5})
    assert eval(repr(v), {"stim": stim, "sinter": sinter, "np": np}) == v

    v = sinter.Task(circuit=circuit, decoder='pymatching')
    assert eval(repr(v), {"stim": stim, "sinter": sinter, "np": np}) == v
