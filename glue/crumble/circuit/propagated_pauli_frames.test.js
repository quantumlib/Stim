import {test, assertThat} from "../test/test_util.js"
import {Circuit} from "./circuit.js";
import {PropagatedPauliFrames, PropagatedPauliFrameLayer} from './propagated_pauli_frames.js';

test("propagated_pauli_frames.fromMeasurements", () => {
    let propagated = PropagatedPauliFrames.fromMeasurements(Circuit.fromStimCircuit(`
        R 0
        TICK
        H 0
        TICK
        MX 0
    `), [-1]);
    assertThat(propagated).isEqualTo(new PropagatedPauliFrames(new Map([
        [0.5, new PropagatedPauliFrameLayer(
            new Map([[0, 'Z']]),
            new Set(),
            [],
        )],
        [1.5, new PropagatedPauliFrameLayer(
            new Map([[0, 'X']]),
            new Set(),
            [],
        )],
    ])));

    propagated = PropagatedPauliFrames.fromMeasurements(Circuit.fromStimCircuit(`
        RX 0
        TICK
        H 0
        TICK
        MX 0
    `), [-1]);
    assertThat(propagated).isEqualTo(new PropagatedPauliFrames(new Map([
        [0, new PropagatedPauliFrameLayer(
            new Map(),
            new Set([0]),
            [],
        )],
        [0.5, new PropagatedPauliFrameLayer(
            new Map([[0, 'Z']]),
            new Set(),
            [],
        )],
        [1.5, new PropagatedPauliFrameLayer(
            new Map([[0, 'X']]),
            new Set(),
            [],
        )],
    ])));

    propagated = PropagatedPauliFrames.fromMeasurements(Circuit.fromStimCircuit(`
        MX 0
        TICK
        H 0
        TICK
        MX 0
    `), [-1]);
    assertThat(propagated).isEqualTo(new PropagatedPauliFrames(new Map([
        [-0.5, new PropagatedPauliFrameLayer(
            new Map([[0, 'Z']]),
            new Set(),
            [],
        )],
        [-1.5, new PropagatedPauliFrameLayer(
            new Map([[0, 'Z']]),
            new Set(),
            [],
        )],
        [0, new PropagatedPauliFrameLayer(
            new Map(),
            new Set([0]),
            [],
        )],
        [0.5, new PropagatedPauliFrameLayer(
            new Map([[0, 'Z']]),
            new Set(),
            [],
        )],
        [1.5, new PropagatedPauliFrameLayer(
            new Map([[0, 'X']]),
            new Set(),
            [],
        )],
    ])));

    propagated = PropagatedPauliFrames.fromMeasurements(Circuit.fromStimCircuit(`
        RX 0 1
        TICK
        CX 0 1
        TICK
        MX 0
    `), [-1]);
    assertThat(propagated).isEqualTo(new PropagatedPauliFrames(new Map([
        [0.5, new PropagatedPauliFrameLayer(
            new Map([[0, 'X'], [1, 'X']]),
            new Set(),
            [],
        )],
        [1.5, new PropagatedPauliFrameLayer(
            new Map([[0, 'X']]),
            new Set(),
            [],
        )],
    ])));

    propagated = PropagatedPauliFrames.fromMeasurements(Circuit.fromStimCircuit(`
        CX 1 0
        MX 1
    `), [-1]);
    assertThat(propagated).isEqualTo(new PropagatedPauliFrames(new Map([
        [-1.5, new PropagatedPauliFrameLayer(
            new Map([[0, 'X'], [1, 'X']]),
            new Set(),
            [],
        )],
        [-0.5, new PropagatedPauliFrameLayer(
            new Map([[0, 'X'], [1, 'X']]),
            new Set(),
            [],
        )],
        [0.5, new PropagatedPauliFrameLayer(
            new Map([[1, 'X']]),
            new Set(),
            [],
        )],
    ])));
});

test("propagated_pauli_frames.fromCircuit_demolitionMeasurementMarkers", () => {
    // Operator processing for demolition measurements (MR/MRX/MRY)

    // Uncaptured: the Z reaches the MR's reset and is reported as lost at that layer.
    let propagated = PropagatedPauliFrames.fromCircuit(Circuit.fromStimCircuit(`
        R 0
        MARKZ(0) 0
        TICK
        MR 0
        TICK
        M 0
    `), 0);
    assertThat(propagated).isEqualTo(new PropagatedPauliFrames(new Map([
        [0.5, new PropagatedPauliFrameLayer(
            new Map([[0, 'Z']]),
            new Set(),
            [],
        )],
        [1, new PropagatedPauliFrameLayer(
            new Map(),
            new Set([0]),
            [],
        )],
    ])));

    // Captured: a co-located MARKZ (measured basis) captures the incoming Z, so it ends cleanly.
    propagated = PropagatedPauliFrames.fromCircuit(Circuit.fromStimCircuit(`
        R 0
        MARKZ(0) 0
        TICK
        MR 0
        MARKZ(0) 0
        TICK
        M 0
    `), 0);
    assertThat(propagated).isEqualTo(new PropagatedPauliFrames(new Map([
        [0.5, new PropagatedPauliFrameLayer(
            new Map([[0, 'Z']]),
            new Set(),
            [],
        )],
    ])));

    // Outgoing: with nothing flowing in, a MARKZ at the MR introduces a Z on the reset's output,
    // which propagates forward (here it passes through the following M).
    propagated = PropagatedPauliFrames.fromCircuit(Circuit.fromStimCircuit(`
        MR 0
        MARKZ(0) 0
        TICK
        M 0
    `), 0);
    assertThat(propagated).isEqualTo(new PropagatedPauliFrames(new Map([
        [0.5, new PropagatedPauliFrameLayer(
            new Map([[0, 'Z']]),
            new Set(),
            [],
        )],
        [1.5, new PropagatedPauliFrameLayer(
            new Map([[0, 'Z']]),
            new Set(),
            [],
        )],
    ])));

    // The measured basis is per gate: MRX captures an X.
    propagated = PropagatedPauliFrames.fromCircuit(Circuit.fromStimCircuit(`
        RX 0
        MARKX(0) 0
        TICK
        MRX 0
        MARKX(0) 0
        TICK
        MX 0
    `), 0);
    assertThat(propagated).isEqualTo(new PropagatedPauliFrames(new Map([
        [0.5, new PropagatedPauliFrameLayer(
            new Map([[0, 'X']]),
            new Set(),
            [],
        )],
    ])));

    // MRY reports an uncaptured Y.
    propagated = PropagatedPauliFrames.fromCircuit(Circuit.fromStimCircuit(`
        RY 0
        MARKY(0) 0
        TICK
        MRY 0
        TICK
        MY 0
    `), 0);
    assertThat(propagated).isEqualTo(new PropagatedPauliFrames(new Map([
        [0.5, new PropagatedPauliFrameLayer(
            new Map([[0, 'Y']]),
            new Set(),
            [],
        )],
        [1, new PropagatedPauliFrameLayer(
            new Map(),
            new Set([0]),
            [],
        )],
    ])));

    // A marker in the wrong basis does not capture the incoming operator: it is still reported.
    propagated = PropagatedPauliFrames.fromCircuit(Circuit.fromStimCircuit(`
        R 0
        MARKZ(0) 0
        TICK
        MR 0
        MARKX(0) 0
        TICK
        M 0
    `), 0);
    assertThat(propagated).isEqualTo(new PropagatedPauliFrames(new Map([
        [0.5, new PropagatedPauliFrameLayer(
            new Map([[0, 'Z']]),
            new Set(),
            [],
        )],
        [1, new PropagatedPauliFrameLayer(
            new Map(),
            new Set([0]),
            [],
        )],
    ])));

    // An operator that anticommutes with the measured basis (X into an MR) cannot be captured and
    // is reported at the measurement.
    propagated = PropagatedPauliFrames.fromCircuit(Circuit.fromStimCircuit(`
        RX 0
        MARKX(0) 0
        TICK
        MR 0
        TICK
        M 0
    `), 0);
    assertThat(propagated).isEqualTo(new PropagatedPauliFrames(new Map([
        [0.5, new PropagatedPauliFrameLayer(
            new Map([[0, 'X']]),
            new Set(),
            [],
        )],
        [1, new PropagatedPauliFrameLayer(
            new Map(),
            new Set([0]),
            [],
        )],
    ])));
});
