import {rad} from "../draw/config.js"
import {Gate} from "./gate.js"

function *iter_gates_paulis() {
    yield new Gate(
        'I',
        1,
        true,
        false,
        new Map([
            ['X', 'X'],
            ['Z', 'Z'],
        ]),
        () => {},
        (op, coordFunc, ctx) => {
            let [x1, y1] = coordFunc(op.id_targets[0]);
            ctx.fillStyle = 'white';
            ctx.fillRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.strokeStyle = 'black';
            ctx.strokeRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.fillStyle = 'black';
            ctx.textAlign = "center";
            ctx.textBaseline = 'middle';
            ctx.fillText('I', x1, y1);
        },
    )
    yield new Gate(
        'X',
        1,
        true,
        false,
        new Map([
            ['X', 'X'],
            ['Z', 'Z'],
        ]),
        () => {},
        (op, coordFunc, ctx) => {
            let [x1, y1] = coordFunc(op.id_targets[0]);
            ctx.fillStyle = 'white';
            ctx.fillRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.strokeStyle = 'black';
            ctx.strokeRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.fillStyle = 'black';
            ctx.textAlign = "center";
            ctx.textBaseline = 'middle';
            ctx.fillText('X', x1, y1);
        },
    )
    yield new Gate(
        'Y',
        1,
        true,
        false,
        new Map([
            ['X', 'X'],
            ['Z', 'Z'],
        ]),
        () => {},
        (op, coordFunc, ctx) => {
            let [x1, y1] = coordFunc(op.id_targets[0]);
            ctx.fillStyle = 'white';
            ctx.fillRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.strokeStyle = 'black';
            ctx.strokeRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.fillStyle = 'black';
            ctx.textAlign = "center";
            ctx.textBaseline = 'middle';
            ctx.fillText('Y', x1, y1);
        },
    )
    yield new Gate(
        'Z',
        1,
        true,
        false,
        new Map([
            ['X', 'X'],
            ['Z', 'Z'],
        ]),
        () => {},
        (op, coordFunc, ctx) => {
            let [x1, y1] = coordFunc(op.id_targets[0]);
            ctx.fillStyle = 'white';
            ctx.fillRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.strokeStyle = 'black';
            ctx.strokeRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.fillStyle = 'black';
            ctx.textAlign = "center";
            ctx.textBaseline = 'middle';
            ctx.fillText('Z', x1, y1);
        },
    )
}

export {iter_gates_paulis};
