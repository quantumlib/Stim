import {rad} from "../draw/config.js"
import {Gate} from "./gate.js"

function *iter_gates_third_turns() {
    yield new Gate(
        'C_XYZ',
        1,
        true,
        false,
        new Map([
            ['X', 'Y'],
            ['Z', 'X'],
        ]),
        (frame, targets) => frame.do_cycle_xyz(targets),
        (op, coordFunc, ctx) => {
            let [x1, y1] = coordFunc(op.id_targets[0]);
            ctx.fillStyle = 'teal';
            ctx.fillRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.fillStyle = 'black';
            ctx.strokeStyle = 'black';
            ctx.strokeRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.textAlign = "center";
            ctx.textBaseline = 'middle';
            ctx.fillText('C', x1, y1 - rad / 3);
            ctx.fillText("XYZ", x1, y1 + rad / 3);
        },
    )
    yield new Gate(
        'C_ZYX',
        1,
        true,
        false,
        new Map([
            ['X', 'Z'],
            ['Z', 'Y'],
        ]),
        (frame, targets) => frame.do_cycle_zyx(targets),
        (op, coordFunc, ctx) => {
            let [x1, y1] = coordFunc(op.id_targets[0]);
            ctx.fillStyle = 'teal';
            ctx.fillRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.fillStyle = 'black';
            ctx.strokeStyle = 'black';
            ctx.strokeRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.textAlign = "center";
            ctx.textBaseline = 'middle';
            ctx.fillText('C', x1, y1 - rad / 3);
            ctx.fillText("ZYX", x1, y1 + rad / 3);
        },
    )
}

export {iter_gates_third_turns};
