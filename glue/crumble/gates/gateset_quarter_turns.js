import {rad} from "../draw/config.js"
import {Gate} from "./gate.js"

function *iter_gates_quarter_turns() {
    yield new Gate(
        'S',
        1,
        true,
        false,
        new Map([
            ['X', 'Y'],
            ['Z', 'Z'],
        ]),
        (frame, targets) => frame.do_exchange_xy(targets),
        (op, coordFunc, ctx) => {
            let [x1, y1] = coordFunc(op.id_targets[0]);
            ctx.fillStyle = 'yellow';
            ctx.fillRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.strokeStyle = 'black';
            ctx.strokeRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.fillStyle = 'black';
            ctx.textAlign = "center";
            ctx.textBaseline = 'middle';
            ctx.fillText('S', x1, y1);
        },
    )
    yield new Gate(
        'S_DAG',
        1,
        true,
        false,
        new Map([
            ['X', 'Y'],
            ['Z', 'Z'],
        ]),
        (frame, targets) => frame.do_exchange_xy(targets),
        (op, coordFunc, ctx) => {
            let [x1, y1] = coordFunc(op.id_targets[0]);
            ctx.fillStyle = 'yellow';
            ctx.fillRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.strokeStyle = 'black';
            ctx.strokeRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.fillStyle = 'black';
            ctx.textAlign = "center";
            ctx.textBaseline = 'middle';
            ctx.fillText('S†', x1, y1);
        },
    )

    yield new Gate(
        'SQRT_X',
        1,
        true,
        false,
        new Map([
            ['X', 'X'],
            ['Z', 'Y'],
        ]),
        (frame, targets) => frame.do_exchange_yz(targets),
        (op, coordFunc, ctx) => {
            let [x1, y1] = coordFunc(op.id_targets[0]);
            ctx.fillStyle = 'yellow';
            ctx.fillRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.strokeStyle = 'black';
            ctx.strokeRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.fillStyle = 'black';
            ctx.textAlign = "center";
            ctx.textBaseline = 'middle';
            ctx.fillText('√X', x1, y1);
        },
    )
    yield new Gate(
        'SQRT_X_DAG',
        1,
        true,
        false,
        new Map([
            ['X', 'X'],
            ['Z', 'Y'],
        ]),
        (frame, targets) => frame.do_exchange_yz(targets),
        (op, coordFunc, ctx) => {
            let [x1, y1] = coordFunc(op.id_targets[0]);
            ctx.fillStyle = 'yellow';
            ctx.fillRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.strokeStyle = 'black';
            ctx.strokeRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.fillStyle = 'black';
            ctx.textAlign = "center";
            ctx.textBaseline = 'middle';
            ctx.fillText('√X†', x1, y1);
        },
    )

    yield new Gate(
        'SQRT_Y',
        1,
        true,
        false,
        new Map([
            ['X', 'Z'],
            ['Z', 'X'],
        ]),
        (frame, targets) => frame.do_exchange_xz(targets),
        (op, coordFunc, ctx) => {
            let [x1, y1] = coordFunc(op.id_targets[0]);
            ctx.fillStyle = 'yellow';
            ctx.fillRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.strokeStyle = 'black';
            ctx.strokeRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.fillStyle = 'black';
            ctx.textAlign = "center";
            ctx.textBaseline = 'middle';
            ctx.fillText('√Y', x1, y1);
        },
    )
    yield new Gate(
        'SQRT_Y_DAG',
        1,
        true,
        false,
        new Map([
            ['X', 'Z'],
            ['Z', 'X'],
        ]),
        (frame, targets) => frame.do_exchange_xz(targets),
        (op, coordFunc, ctx) => {
            let [x1, y1] = coordFunc(op.id_targets[0]);
            ctx.fillStyle = 'yellow';
            ctx.fillRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.strokeStyle = 'black';
            ctx.strokeRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.fillStyle = 'black';
            ctx.textAlign = "center";
            ctx.textBaseline = 'middle';
            ctx.fillText('√Y†', x1, y1);
        },
    )
}

export {iter_gates_quarter_turns};
