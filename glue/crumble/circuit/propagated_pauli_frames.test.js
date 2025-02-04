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
