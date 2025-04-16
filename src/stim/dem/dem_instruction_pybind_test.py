import stim
import pytest


def test_args_copy():
    assert stim.DemInstruction(
        "error", [0.25], [stim.target_relative_detector_id(3)]
    ).args_copy() == [0.25]
    assert stim.DemInstruction(
        "error", [0.125], [stim.target_relative_detector_id(3)]
    ).args_copy() == [0.125]
    assert stim.DemInstruction("shift_detectors", [], [1]).args_copy() == []
    assert stim.DemInstruction("shift_detectors", [0.125, 0.25], [1]).args_copy() == [
        0.125,
        0.25,
    ]


def test_targets_copy():
    t1 = [
        stim.target_relative_detector_id(3),
        stim.target_separator(),
        stim.target_logical_observable_id(2),
    ]
    assert stim.DemInstruction("error", [0.25], t1).targets_copy() == t1
    assert stim.DemInstruction("shift_detectors", [], [1]).targets_copy() == [1]
    t2 = [stim.target_logical_observable_id(3)]
    assert stim.DemInstruction("logical_observable", [], t2).targets_copy() == t2


def test_type():
    assert (
        stim.DemInstruction("error", [0.25], [stim.target_relative_detector_id(3)]).type
        == "error"
    )
    assert (
        stim.DemInstruction("ERROR", [0.25], [stim.target_relative_detector_id(3)]).type
        == "error"
    )
    assert stim.DemInstruction("shift_detectors", [], [1]).type == "shift_detectors"
    assert (
        stim.DemInstruction("detector", [], [stim.target_relative_detector_id(3)]).type
        == "detector"
    )
    assert (
        stim.DemInstruction(
            "logical_observable", [], [stim.target_logical_observable_id(3)]
        ).type
        == "logical_observable"
    )


def test_equality():
    e1 = stim.DemInstruction("error", [0.25], [stim.target_relative_detector_id(3)])
    assert e1 == stim.DemInstruction(
        "error", [0.25], [stim.target_relative_detector_id(3)]
    )
    assert not (
        e1
        != stim.DemInstruction("error", [0.25], [stim.target_relative_detector_id(3)])
    )
    assert e1 != stim.DemInstruction(
        "error", [0.35], [stim.target_relative_detector_id(3)]
    )
    assert not (
        e1
        == stim.DemInstruction("error", [0.35], [stim.target_relative_detector_id(3)])
    )
    assert e1 != stim.DemInstruction(
        "error", [0.35], [stim.target_relative_detector_id(4)]
    )
    assert e1 != stim.DemInstruction("shift_detectors", [0.35], [3])


def test_validation():
    with pytest.raises(ValueError, match="takes 1 arg"):
        stim.DemInstruction("error", [], [stim.target_relative_detector_id(3)])
    with pytest.raises(ValueError, match="takes 1 arg"):
        stim.DemInstruction("error", [0.5, 0.5], [stim.target_relative_detector_id(3)])
    with pytest.raises(ValueError, match="last target.+separator"):
        stim.DemInstruction("error", [0.25], [stim.target_separator()])
    with pytest.raises(ValueError, match="0 to 1"):
        stim.DemInstruction("error", [-0.1], [stim.target_relative_detector_id(3)])
    with pytest.raises(ValueError, match="0 to 1"):
        stim.DemInstruction("error", [1.1], [stim.target_relative_detector_id(3)])
    with pytest.raises(ValueError, match="detector.+targets"):
        stim.DemInstruction("error", [0.25], [3])

    with pytest.raises(ValueError, match="integer targets"):
        stim.DemInstruction(
            "shift_detectors", [1.1], [stim.target_relative_detector_id(3)]
        )


def test_str():
    v = stim.DemInstruction(
        "ERROR",
        [0.125],
        [stim.target_relative_detector_id(3), stim.target_logical_observable_id(6)],
    )
    assert str(v) == "error(0.125) D3 L6"
    v = stim.DemInstruction("Shift_detectors", [1.5, 2.5, 5.5], [6])
    assert str(v) == "shift_detectors(1.5, 2.5, 5.5) 6"


def test_repr():
    v = stim.DemInstruction(
        "error",
        [0.25],
        [stim.target_relative_detector_id(3), stim.target_logical_observable_id(6)],
    )
    assert eval(repr(v), {"stim": stim}) == v
    v = stim.DemInstruction("shift_detectors", [1.5, 2.5, 5.5], [6])
    assert eval(repr(v), {"stim": stim}) == v


def test_hashable():
    a = stim.DemInstruction("error", [0.25], [stim.target_relative_detector_id(3)])
    b = stim.DemInstruction("error", [0.125], [stim.target_relative_detector_id(3)])
    c = stim.DemInstruction("error", [0.25], [stim.target_relative_detector_id(3)])
    assert hash(a) == hash(c)
    assert len({a, b, c}) == 2


def test_target_groups():
    dem = stim.DetectorErrorModel("detector D0")
    assert dem[0].target_groups() == [[stim.DemTarget("D0")]]


def test_init_from_str():
    assert stim.DemInstruction("detector D0") == stim.DemInstruction(
        "detector", [], [stim.target_relative_detector_id(0)]
    )

    with pytest.raises(ValueError, match="single DemInstruction"):
        stim.DemInstruction("")

    with pytest.raises(ValueError, match="single DemInstruction"):
        stim.DemInstruction(
            """
            repeat 5 {
                error(0.25) D0
                shift_detectors 1
            }
        """
        )

    with pytest.raises(ValueError, match="single DemInstruction"):
        stim.DemInstruction(
            """
            detector D0
            detector D1
        """
        )


def test_tag():
    assert stim.DemInstruction("error[test](0.25) D1").tag == "test"
    assert (
        stim.DemInstruction("error", [0.25], [stim.DemTarget("D1")], tag="test").tag
        == "test"
    )
    dem = stim.DetectorErrorModel(
        """
        error[test-tag](0.125) D0
        error(0.125) D0
    """
    )
    assert dem[0].tag == "test-tag"
    assert dem[1].tag == ""
