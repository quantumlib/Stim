import {Gate} from "./gate.js"
import {draw_connector} from "./gate_draw_util.js";
import {rad} from "../draw/config.js";

function *iter_gates_sqrt_pauli_pairs() {
    yield new Gate(
        'SQRT_XX',
        2,
        true,
        false,
        new Map([
            ['IX', 'IX'],
            ['IZ', 'XY'],
            ['XI', 'XI'],
            ['ZI', 'YX'],
        ]),
        (frame, targets) => frame.do_sqrt_xx(targets),
        (op, coordFunc, ctx) => {
            let [x1, y1] = coordFunc(op.id_targets[0]);
            let [x2, y2] = coordFunc(op.id_targets[1]);

            draw_connector(ctx, x1, y1, x2, y2);

            for (let [x, y] of [[x1, y1], [x2, y2]]) {
                ctx.fillStyle = 'yellow';
                ctx.fillRect(x - rad, y - rad, rad * 2, rad * 2);
                ctx.strokeStyle = 'black';
                ctx.strokeRect(x - rad, y - rad, rad * 2, rad * 2);
                ctx.fillStyle = 'black';
                ctx.textAlign = "center";
                ctx.textBaseline = 'middle';
                ctx.fillText('√XX', x, y);
            }
        },
    )
    yield new Gate(
        'SQRT_XX_DAG',
        2,
        true,
        false,
        new Map([
            ['IX', 'IX'],
            ['IZ', 'XY'],
            ['XI', 'XI'],
            ['ZI', 'YX'],
        ]),
        (frame, targets) => frame.do_sqrt_xx(targets),
        (op, coordFunc, ctx) => {
            let [x1, y1] = coordFunc(op.id_targets[0]);
            let [x2, y2] = coordFunc(op.id_targets[1]);

            draw_connector(ctx, x1, y1, x2, y2);

            for (let [x, y] of [[x1, y1], [x2, y2]]) {
                ctx.fillStyle = 'yellow';
                ctx.fillRect(x - rad, y - rad, rad * 2, rad * 2);
                ctx.strokeStyle = 'black';
                ctx.strokeRect(x - rad, y - rad, rad * 2, rad * 2);
                ctx.fillStyle = 'black';
                ctx.textAlign = "center";
                ctx.textBaseline = 'middle';
                ctx.fillText('√XX†', x, y);
            }
        },
    )

    yield new Gate(
        'SQRT_YY',
        2,
        true,
        false,
        new Map([
            ['IX', 'YZ'],
            ['IZ', 'YX'],
            ['XI', 'ZY'],
            ['ZI', 'XY'],
        ]),
        (frame, targets) => frame.do_sqrt_yy(targets),
        (op, coordFunc, ctx) => {
            let [x1, y1] = coordFunc(op.id_targets[0]);
            let [x2, y2] = coordFunc(op.id_targets[1]);

            draw_connector(ctx, x1, y1, x2, y2);

            for (let [x, y] of [[x1, y1], [x2, y2]]) {
                ctx.fillStyle = 'yellow';
                ctx.fillRect(x - rad, y - rad, rad * 2, rad * 2);
                ctx.strokeStyle = 'black';
                ctx.strokeRect(x - rad, y - rad, rad * 2, rad * 2);
                ctx.fillStyle = 'black';
                ctx.textAlign = "center";
                ctx.textBaseline = 'middle';
                ctx.fillText('√YY', x, y);
            }
        },
    )
    yield new Gate(
        'SQRT_YY_DAG',
        2,
        true,
        false,
        new Map([
            ['IX', 'YZ'],
            ['IZ', 'YX'],
            ['XI', 'ZY'],
            ['ZI', 'XY'],
        ]),
        (frame, targets) => frame.do_sqrt_yy(targets),
        (op, coordFunc, ctx) => {
            let [x1, y1] = coordFunc(op.id_targets[0]);
            let [x2, y2] = coordFunc(op.id_targets[1]);

            draw_connector(ctx, x1, y1, x2, y2);

            for (let [x, y] of [[x1, y1], [x2, y2]]) {
                ctx.fillStyle = 'yellow';
                ctx.fillRect(x - rad, y - rad, rad * 2, rad * 2);
                ctx.strokeStyle = 'black';
                ctx.strokeRect(x - rad, y - rad, rad * 2, rad * 2);
                ctx.fillStyle = 'black';
                ctx.textAlign = "center";
                ctx.textBaseline = 'middle';
                ctx.fillText('√YY†', x, y);
            }
        },
    )

    yield new Gate(
        'SQRT_ZZ',
        2,
        true,
        false,
        new Map([
            ['IX', 'ZY'],
            ['IZ', 'IZ'],
            ['XI', 'YZ'],
            ['ZI', 'ZI'],
        ]),
        (frame, targets) => frame.do_sqrt_zz(targets),
        (op, coordFunc, ctx) => {
            let [x1, y1] = coordFunc(op.id_targets[0]);
            let [x2, y2] = coordFunc(op.id_targets[1]);

            draw_connector(ctx, x1, y1, x2, y2);

            for (let [x, y] of [[x1, y1], [x2, y2]]) {
                ctx.fillStyle = 'yellow';
                ctx.fillRect(x - rad, y - rad, rad * 2, rad * 2);
                ctx.strokeStyle = 'black';
                ctx.strokeRect(x - rad, y - rad, rad * 2, rad * 2);
                ctx.fillStyle = 'black';
                ctx.textAlign = "center";
                ctx.textBaseline = 'middle';
                ctx.fillText('√ZZ', x, y);
            }
        },
    )
    yield new Gate(
        'SQRT_ZZ_DAG',
        2,
        true,
        false,
        new Map([
            ['IX', 'ZY'],
            ['IZ', 'IZ'],
            ['XI', 'YZ'],
            ['ZI', 'ZI'],
        ]),
        (frame, targets) => frame.do_sqrt_zz(targets),
        (op, coordFunc, ctx) => {
            let [x1, y1] = coordFunc(op.id_targets[0]);
            let [x2, y2] = coordFunc(op.id_targets[1]);

            draw_connector(ctx, x1, y1, x2, y2);

            for (let [x, y] of [[x1, y1], [x2, y2]]) {
                ctx.fillStyle = 'yellow';
                ctx.fillRect(x - rad, y - rad, rad * 2, rad * 2);
                ctx.strokeStyle = 'black';
                ctx.strokeRect(x - rad, y - rad, rad * 2, rad * 2);
                ctx.fillStyle = 'black';
                ctx.textAlign = "center";
                ctx.textBaseline = 'middle';
                ctx.fillText('√ZZ†', x, y);
            }
        },
    )
}

export {iter_gates_sqrt_pauli_pairs};
