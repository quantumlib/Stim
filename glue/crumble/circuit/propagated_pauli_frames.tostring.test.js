import {test, assertThat} from "../test/test_util.js";
import {PropagatedPauliFrameLayer} from "./propagated_pauli_frames.js";

test("propagated_pauli_frame_layer.toString_handles_object_crossings", () => {
    const layer = new PropagatedPauliFrameLayer(
        new Map([[0, 'X']]),
        new Set(),
        [{q1: 0, q2: 1, color: 'X'}],
    );
    assertThat(() => String(layer)).runsWithoutThrowingAnException();
});

