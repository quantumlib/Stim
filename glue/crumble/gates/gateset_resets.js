import {rad} from "../draw/config.js"
import {Gate} from "./gate.js"

function *iter_gates_resets() {
    yield new Gate(
        'R',
        1,
        true,
        false,
        new Map([
            ['X', 'ERR:I'],
            ['Y', 'ERR:I'],
            ['Z', 'ERR:I'],
        ]),
        (frame, targets) => frame.do_discard(targets),
        (op, coordFunc, ctx) => {
            let [x1, y1] = coordFunc(op.id_targets[0]);
            ctx.fillStyle = 'gray';
            ctx.fillRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.strokeStyle = 'black';
            ctx.strokeRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.fillStyle = 'black';
            ctx.textAlign = "center";
            ctx.textBaseline = 'middle';
            ctx.fillText('R', x1, y1);
        },
    );
    yield new Gate(
        'RX',
        1,
        true,
        false,
        new Map([
            ['X', 'ERR:I'],
            ['Y', 'ERR:I'],
            ['Z', 'ERR:I'],
        ]),
        (frame, targets) => frame.do_discard(targets),
        (op, coordFunc, ctx) => {
            let [x1, y1] = coordFunc(op.id_targets[0]);
            ctx.fillStyle = 'gray';
            ctx.fillRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.strokeStyle = 'black';
            ctx.strokeRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.fillStyle = 'black';
            ctx.textAlign = "center";
            ctx.textBaseline = 'middle';
            ctx.fillText('RX', x1, y1);
        },
    );
    yield new Gate(
        'RY',
        1,
        true,
        false,
        new Map([
            ['X', 'ERR:I'],
            ['Y', 'ERR:I'],
            ['Z', 'ERR:I'],
        ]),
        (frame, targets) => frame.do_discard(targets),
        (op, coordFunc, ctx) => {
            let [x1, y1] = coordFunc(op.id_targets[0]);
            ctx.fillStyle = 'gray';
            ctx.fillRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.strokeStyle = 'black';
            ctx.strokeRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.fillStyle = 'black';
            ctx.textAlign = "center";
            ctx.textBaseline = 'middle';
            ctx.fillText('RY', x1, y1);
        },
    );
}

export {iter_gates_resets};
