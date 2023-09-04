import {rad} from "../draw/config.js"
import {Gate} from "./gate.js"
import {draw_connector} from "./gate_draw_util.js";

/**
 * @param {!string} bases
 * @returns {!Gate}
 */
function make_mpp_gate(bases) {
    return new Gate(
        'M' + bases,
        bases.length,
        true,
        false,
        undefined,
        (frame, targets) => frame.do_mpp(bases, targets),
        (op, coordFunc, ctx) => {
            let prev_x = undefined;
            let prev_y = undefined;
            for (let k = 0; k < op.id_targets.length; k++) {
                let t = op.id_targets[k];
                let [x, y] = coordFunc(t);
                if (prev_x !== undefined) {
                    draw_connector(ctx, x, y, prev_x, prev_y);
                }

                prev_x = x;
                prev_y = y;
            }

            for (let k = 0; k < op.id_targets.length; k++) {
                let t = op.id_targets[k];
                let [x, y] = coordFunc(t);
                ctx.fillStyle = 'gray';
                ctx.fillRect(x - rad, y - rad, rad * 2, rad * 2);
                ctx.strokeStyle = 'black';
                ctx.strokeRect(x - rad, y - rad, rad * 2, rad * 2);
                ctx.fillStyle = 'black';
                ctx.textAlign = "center";
                ctx.textBaseline = 'middle';
                ctx.font = 'bold 12pt monospace'
                ctx.fillText(bases[k], x, y - 1);
                ctx.font = '5pt monospace'
                ctx.fillText('MPP', x, y + 8);
            }
        },
    );
}

export {make_mpp_gate};
