import {rad} from "../draw/config.js"
import {Gate} from "./gate.js"

function *iter_gates_solo_measurements() {
    yield new Gate(
        'M',
        1,
        true,
        false,
        new Map([
            ['X', 'ERR:X'],
            ['Y', 'ERR:Y'],
            ['Z', 'Z'],
        ]),
        (frame, targets) => frame.do_measure('Z', targets),
        (op, coordFunc, ctx) => {
            let [x1, y1] = coordFunc(op.id_targets[0]);
            ctx.fillStyle = 'gray';
            ctx.fillRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.strokeStyle = 'black';
            ctx.strokeRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.fillStyle = 'black';
            ctx.textAlign = "center";
            ctx.textBaseline = 'middle';
            ctx.fillText('M', x1, y1);
            ctx.textAlign = "left";
        },
    );
    yield new Gate(
        'MX',
        1,
        true,
        false,
        new Map([
            ['X', 'X'],
            ['Y', 'ERR:Y'],
            ['Z', 'ERR:Z'],
        ]),
        (frame, targets) => frame.do_measure('X', targets),
        (op, coordFunc, ctx) => {
            let [x1, y1] = coordFunc(op.id_targets[0]);
            ctx.fillStyle = 'gray';
            ctx.fillRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.strokeStyle = 'black';
            ctx.strokeRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.fillStyle = 'black';
            ctx.textAlign = "center";
            ctx.textBaseline = 'middle';
            ctx.fillText('MX', x1, y1);
            ctx.textAlign = "left";
        },
    );
    yield new Gate(
        'MY',
        1,
        true,
        false,
        new Map([
            ['X', 'ERR:X'],
            ['Y', 'Y'],
            ['Z', 'ERR:Z'],
        ]),
        (frame, targets) => frame.do_measure('Y', targets),
        (op, coordFunc, ctx) => {
            let [x1, y1] = coordFunc(op.id_targets[0]);
            ctx.fillStyle = 'gray';
            ctx.fillRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.strokeStyle = 'black';
            ctx.strokeRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.fillStyle = 'black';
            ctx.textAlign = "center";
            ctx.textBaseline = 'middle';
            ctx.fillText('MY', x1, y1);
            ctx.textAlign = "left";
        },
    );
}

export {iter_gates_solo_measurements};
