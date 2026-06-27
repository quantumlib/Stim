import {rad} from "../draw/config.js"
import {Gate} from "./gate.js"

function *iter_gates_demolition_measurements() {
    yield new Gate(
        'MR',
        1,
        true,
        false,
        new Map([
            ['X', 'ERR:I'],
            ['Y', 'ERR:I'],
            ['Z', 'I'],
        ]),
        (frame, targets) => frame.do_demolition_measure('Z', targets),
        (frame, targets) => frame.do_demolition_measure('Z', targets),
        (op, coordFunc, ctx) => {
            let [x1, y1] = coordFunc(op.id_targets[0]);
            ctx.fillStyle = 'gray';
            ctx.fillRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.strokeStyle = 'black';
            ctx.strokeRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.fillStyle = 'black';
            ctx.textAlign = "center";
            ctx.textBaseline = 'middle';
            ctx.fillText('MR', x1, y1);
        },
    );
    yield new Gate(
        'MRY',
        1,
        true,
        false,
        new Map([
            ['X', 'ERR:I'],
            ['Y', 'I'],
            ['Z', 'ERR:I'],
        ]),
        (frame, targets) => frame.do_demolition_measure('Y', targets),
        (frame, targets) => frame.do_demolition_measure('Y', targets),
        (op, coordFunc, ctx) => {
            let [x1, y1] = coordFunc(op.id_targets[0]);
            ctx.fillStyle = 'gray';
            ctx.fillRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.strokeStyle = 'black';
            ctx.strokeRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.fillStyle = 'black';
            ctx.textAlign = "center";
            ctx.textBaseline = 'middle';
            ctx.fillText('MRY', x1, y1);
        },
    );
    yield new Gate(
        'MRX',
        1,
        true,
        false,
        new Map([
            ['X', 'I'],
            ['Y', 'ERR:I'],
            ['Z', 'ERR:I'],
        ]),
        (frame, targets) => frame.do_demolition_measure('X', targets),
        (frame, targets) => frame.do_demolition_measure('X', targets),
        (op, coordFunc, ctx) => {
            let [x1, y1] = coordFunc(op.id_targets[0]);
            ctx.fillStyle = 'gray';
            ctx.fillRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.strokeStyle = 'black';
            ctx.strokeRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.fillStyle = 'black';
            ctx.textAlign = "center";
            ctx.textBaseline = 'middle';
            ctx.fillText('MRX', x1, y1);
        },
    );
}

export {iter_gates_demolition_measurements};
