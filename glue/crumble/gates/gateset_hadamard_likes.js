import {rad} from "../draw/config.js"
import {Gate} from "./gate.js"

function *iter_gates_hadamard_likes() {
    yield new Gate(
        'H',
        1,
        true,
        false,
        new Map([
            ['X', 'Z'],
            ['Z', 'X'],
        ]),
        (frame, targets) => frame.do_exchange_xz(targets),
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
            ctx.fillText('H', x1, y1);
        },
    );
    yield new Gate(
        'H_NXZ',
        1,
        true,
        false,
        new Map([
            ['X', 'Z'],
            ['Z', 'X'],
        ]),
        (frame, targets) => frame.do_exchange_xz(targets),
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
            ctx.fillText('H', x1, y1 - rad / 3);
            ctx.fillText("NXZ", x1, y1 + rad / 3);
        },
    );
    yield new Gate(
        'H_XY',
        1,
        true,
        false,
        new Map([
            ['X', 'Y'],
            ['Z', 'Z'],  // -Z technically
        ]),
        (frame, targets) => frame.do_exchange_xy(targets),
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
            ctx.fillText('H', x1, y1 - rad / 3);
            ctx.fillText("XY", x1, y1 + rad / 3);
        },
    );
    yield new Gate(
        'H_NXY',
        1,
        true,
        false,
        new Map([
            ['X', 'Y'],
            ['Z', 'Z'],
        ]),
        (frame, targets) => frame.do_exchange_xy(targets),
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
            ctx.fillText('H', x1, y1 - rad / 3);
            ctx.fillText("NXY", x1, y1 + rad / 3);
        },
    );
    yield new Gate(
        'H_YZ',
        1,
        true,
        false,
        new Map([
            ['X', 'X'],  // -X technically
            ['Z', 'Y'],
        ]),
        (frame, targets) => frame.do_exchange_yz(targets),
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
            ctx.fillText('H', x1, y1 - rad / 3);
            ctx.fillText("YZ", x1, y1 + rad / 3);
        },
    );
    yield new Gate(
        'H_NYZ',
        1,
        true,
        false,
        new Map([
            ['X', 'X'],  // -X technically
            ['Z', 'Y'],
        ]),
        (frame, targets) => frame.do_exchange_yz(targets),
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
            ctx.fillText('H', x1, y1 - rad / 3);
            ctx.fillText("NYZ", x1, y1 + rad / 3);
        },
    );
}

export {iter_gates_hadamard_likes};
