import {describe} from "../base/describe.js";
import {equate} from "../base/equate.js";

class ChordEvent {
    /**
     * @param {!boolean} inProgress
     * @param {!Set<!string>} chord
     * @param {!boolean} altKey
     * @param {!boolean} ctrlKey
     * @param {!boolean} metaKey
     * @param {!boolean} shiftKey
     */
    constructor(inProgress, chord, altKey, ctrlKey, metaKey, shiftKey) {
        this.inProgress = inProgress;
        this.chord = chord;
        this.altKey = altKey;
        this.shiftKey = shiftKey;
        this.ctrlKey = ctrlKey;
        this.metaKey = metaKey;
    }

    /**
     * @param {*} other
     * @return {!boolean}
     */
    isEqualTo(other) {
        return other instanceof ChordEvent &&
            this.inProgress === other.inProgress &&
            equate(this.chord, other.chord) &&
            this.altKey === other.altKey &&
            this.shiftKey === other.shiftKey &&
            this.ctrlKey === other.ctrlKey &&
            this.metaKey === other.metaKey;
    }

    /**
     * @return {!string}
     */
    toString() {
        return `ChordEvent(
    inProgress=${this.inProgress},
    chord=${describe(this.chord)},
    altKey=${this.altKey},
    shiftKey=${this.shiftKey},
    ctrlKey=${this.ctrlKey},
    metaKey=${this.metaKey},
)`;
    }
}

const MODIFIER_KEYS = new Set(["alt", "shift", "control", "meta"]);
const ACTION_KEYS = new Set(['1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\\', '`']);

class Chorder {
    constructor() {
        this.curModifiers = /** @type {!Set<!string>} */ new Set();
        this.curPressed = /** @type {!Set<!string>} */ new Set();
        this.curChord = /** @type {!Set<!string>} */ new Set();
        this.queuedEvents = /** @type {!Array<!ChordEvent>} */ [];
    }

    /**
     * @param {!boolean} inProgress
     */
    toEvent(inProgress) {
        return new ChordEvent(
            inProgress,
            new Set(this.curChord),
            this.curModifiers.has("alt"),
            this.curModifiers.has("control"),
            this.curModifiers.has("meta"),
            this.curModifiers.has("shift")
        );
    }

    /**
     * @param {!boolean} inProgress
     * @private
     */
    _queueEvent(inProgress) {
        this.queuedEvents.push(this.toEvent(inProgress));
    }

    handleFocusChanged() {
        this.curPressed.clear();
        this.curChord.clear();
        this.curModifiers.clear();
    }

    /**
     * @param {!KeyboardEvent} ev
     */
    handleKeyEvent(ev) {
        let key = ev.key.toLowerCase();
        if (key === 'escape') {
            this.handleFocusChanged();
        }
        if (ev.type === 'keydown') {
            let flag_key_pairs = [
                [ev.altKey, "alt"],
                [ev.shiftKey, "shift"],
                [ev.ctrlKey, "control"],
                [ev.metaKey, "meta"],
            ];
            for (let [b, k] of flag_key_pairs) {
                if (b) {
                    this.curModifiers.add(k);
                } else {
                    this.curModifiers.delete(k);
                }
            }
            if (!MODIFIER_KEYS.has(key)) {
                this.curPressed.add(key);
                this.curChord.add(key);
            }
            this._queueEvent(true);
        } else if (ev.type === 'keyup') {
            if (!MODIFIER_KEYS.has(key)) {
                this.curPressed.delete(key);
                this._queueEvent(this.curPressed.size > 0 && !ACTION_KEYS.has(key));
                if (ACTION_KEYS.has(key)) {
                    this.curChord.delete(key);
                }
                if (this.curPressed.size === 0) {
                    this.curModifiers.clear();
                    this.curChord.clear();
                }
            }
        } else {
            throw new Error("Not a recognized key event type: " + ev.type);
        }
    }
}

export {Chorder, ChordEvent};
