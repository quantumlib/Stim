import {Circuit} from "../circuit/circuit.js";
import {Layer} from "../circuit/layer.js";

/**
 * A copy of the editor state which can be used for tasks such as drawing previews of changes.
 *
 * Technically not immutable, but should be treated as immutable. Should never be mutated.
 */
class StateSnapshot {
    /**
     * @param {!Circuit} circuit
     * @param {!int} curLayer
     * @param {!Map<!string, ![!number, !number]>} focusedSet
     * @param {!Map<!string, ![!number, !number]>} timelineSet
     * @param {!number} curMouseX
     * @param {!number} curMouseY
     * @param {!number} mouseDownX
     * @param {!number} mouseDownY
     * @param {!Array<![!number, !number]>} boxHighlightPreview
     */
    constructor(circuit, curLayer, focusedSet, timelineSet, curMouseX, curMouseY, mouseDownX, mouseDownY, boxHighlightPreview) {
        this.circuit = circuit.copy();
        this.curLayer = curLayer;
        this.focusedSet = new Map(focusedSet.entries());
        this.timelineSet = new Map(timelineSet.entries());
        this.curMouseX = curMouseX;
        this.curMouseY = curMouseY;
        this.mouseDownX = mouseDownX;
        this.mouseDownY = mouseDownY;
        this.boxHighlightPreview = [...boxHighlightPreview];

        while (this.circuit.layers.length <= this.curLayer) {
            this.circuit.layers.push(new Layer());
        }
    }

    /**
     * @returns {!Set<!int>}
     */
    id_usedQubits() {
        return this.circuit.allQubits();
    }

    /**
     * @returns {!Array<!int>}
     */
    timelineQubits() {
        let used = this.id_usedQubits();
        let qubits = [];
        if (this.timelineSet.size > 0) {
            let c2q = this.circuit.coordToQubitMap();
            for (let key of this.timelineSet.keys()) {
                let q = c2q.get(key);
                if (q !== undefined) {
                    qubits.push(q);
                }
            }
        } else {
            qubits.push(...used.values());
        }
        return qubits.filter(q => used.has(q));
    }
}

export {StateSnapshot}
