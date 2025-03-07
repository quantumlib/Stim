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
            ctx.fillText("XYZ", x1, y1 + rad / 3);
        },
    )
    yield new Gate(
        'C_NXYZ',
        1,
        true,
        false,
        new Map([
            ['X', 'Y'],
            ['Z', 'X'],
        ]),
        (frame, targets) => frame.do_cycle_xyz(targets),
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
            ctx.fillText("NXYZ", x1, y1 + rad / 3);
        },
    )
    yield new Gate(
        'C_XNYZ',
        1,
        true,
        false,
        new Map([
            ['X', 'Y'],
            ['Z', 'X'],
        ]),
        (frame, targets) => frame.do_cycle_xyz(targets),
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
            ctx.fillText("XNYZ", x1, y1 + rad / 3);
        },
    )
    yield new Gate(
        'C_XYNZ',
        1,
        true,
        false,
        new Map([
            ['X', 'Y'],
            ['Z', 'X'],
        ]),
        (frame, targets) => frame.do_cycle_xyz(targets),
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
            ctx.fillText("XYNZ", x1, y1 + rad / 3);
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
            ctx.fillText("ZYX", x1, y1 + rad / 3);
        },
    )
    yield new Gate(
        'C_ZYNX',
        1,
        true,
        false,
        new Map([
            ['X', 'Z'],
            ['Z', 'Y'],
        ]),
        (frame, targets) => frame.do_cycle_zyx(targets),
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
            ctx.fillText("ZYNX", x1, y1 + rad / 3);
        },
    )
    yield new Gate(
        'C_ZNYX',
        1,
        true,
        false,
        new Map([
            ['X', 'Z'],
            ['Z', 'Y'],
        ]),
        (frame, targets) => frame.do_cycle_zyx(targets),
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
            ctx.fillText("ZNYX", x1, y1 + rad / 3);
        },
    )
    yield new Gate(
        'C_NZYX',
        1,
        true,
        false,
        new Map([
            ['X', 'Z'],
            ['Z', 'Y'],
        ]),
        (frame, targets) => frame.do_cycle_zyx(targets),
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
            ctx.fillText("NZYX", x1, y1 + rad / 3);
        },
    )
}

export {iter_gates_third_turns};
