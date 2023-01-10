import {test, assertThat, skipRestOfTestIfHeadless} from "../test/test_util.js"
import {Chorder, ChordEvent} from "./chord.js"

test("chorder.usage", () => {
    let c = new Chorder();
    assertThat(c.curChord).isEqualTo(new Set());
    assertThat(c.curPressed).isEqualTo(new Set());
    assertThat(c.queuedEvents).isEqualTo([]);
    skipRestOfTestIfHeadless();

    c.handleKeyEvent(new KeyboardEvent('keydown', {key: 'x', ctrlKey: true}));
    assertThat(c.queuedEvents.shift()).isEqualTo(new ChordEvent(true, new Set(["x"]), false, true, false, false));
    assertThat(c.curChord).isEqualTo(new Set(["x"]));
    assertThat(c.curPressed).isEqualTo(new Set(["x"]));
    assertThat(c.curModifiers).isEqualTo(new Set(["control"]));
    assertThat(c.queuedEvents).isEqualTo([]);

    c.handleKeyEvent(new KeyboardEvent('keydown', {key: 'r', altKey: true}));
    assertThat(c.queuedEvents.shift()).isEqualTo(new ChordEvent(true, new Set(["r", "x"]), true, false, false, false));
    assertThat(c.curChord).isEqualTo(new Set(["r", "x"]));
    assertThat(c.curPressed).isEqualTo(new Set(["r", "x"]));
    assertThat(c.curModifiers).isEqualTo(new Set(["alt"]));
    assertThat(c.queuedEvents).isEqualTo([]);

    c.handleKeyEvent(new KeyboardEvent('keyup', {key: 'x'}));
    assertThat(c.queuedEvents.shift()).isEqualTo(new ChordEvent(true, new Set(["r", "x"]), true, false, false, false));
    assertThat(c.curChord).isEqualTo(new Set(["r", "x"]));
    assertThat(c.curPressed).isEqualTo(new Set(["r"]));
    assertThat(c.curModifiers).isEqualTo(new Set(["alt"]));
    assertThat(c.queuedEvents).isEqualTo([]);

    c.handleKeyEvent(new KeyboardEvent('keyup', {key: 'r'}));
    assertThat(c.queuedEvents.shift()).isEqualTo(new ChordEvent(false, new Set(["r", "x"]), true, false, false, false));
    assertThat(c.curChord).isEqualTo(new Set([]));
    assertThat(c.curPressed).isEqualTo(new Set([]));
    assertThat(c.curModifiers).isEqualTo(new Set([]));
    assertThat(c.queuedEvents).isEqualTo([]);
});
