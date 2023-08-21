/**
 * Gate drawing callback.
 *
 * @callback gateDrawCallback
 * @param {!Operation} op
 * @param {!function(qubit: !int): ![!number, !number]} qubitCoordsFunc
 * @param {!CanvasRenderingContext2D} ctx
 */

/**
 * An operation without specified targets.
 */
class Gate {
    /**
     * @param {!string} name
     * @param {!int|undefined} num_qubits
     * @param {!boolean} can_fuse
     * @param {!boolean} is_marker
     * @param {!Map<!string, !string>|undefined} tableau_map
     * @param {!function(!PauliFrame, !Array<!int>)} frameDo,
     * @param {!gateDrawCallback} drawer
     * @param {undefined|!number=undefined} defaultArgument
     */
    constructor(name,
                num_qubits,
                can_fuse,
                is_marker,
                tableau_map,
                frameDo,
                drawer,
                defaultArgument = undefined) {
        this.name = name;
        this.num_qubits = num_qubits;
        this.is_marker = is_marker;
        this.can_fuse = can_fuse;
        this.tableau_map = tableau_map;
        this.frameDo = frameDo;
        this.drawer = drawer;
        this.defaultArgument = defaultArgument;
    }

    /**
     * @param {!number} newDefaultArgument
     */
    withDefaultArgument(newDefaultArgument) {
        return new Gate(
            this.name,
            this.num_qubits,
            this.can_fuse,
            this.is_marker,
            this.tableau_map,
            this.frameDo,
            this.drawer,
            newDefaultArgument);
    }
}

export {Gate};
