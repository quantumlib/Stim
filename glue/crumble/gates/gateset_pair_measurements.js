import {rad} from "../draw/config.js"
import {Gate} from "./gate.js"
import {draw_connector} from "./gate_draw_util.js";

function *iter_gates_pair_measurements() {
    yield new Gate(
        'MXX',
        2,
        true,
        false,
        new Map([
            ['II', 'II'],
            ['IX', 'IX'],
            ['IY', 'ERR:IY'],
            ['IZ', 'ERR:IZ'],

            ['XI', 'XI'],
            ['XX', 'XX'],
            ['XY', 'ERR:XY'],
            ['XZ', 'ERR:XZ'],

            ['YI', 'ERR:YI'],
            ['YX', 'ERR:YX'],
            ['YY', 'YY'],
            ['YZ', 'YZ'],

            ['ZI', 'ERR:ZI'],
            ['ZX', 'ERR:ZX'],
            ['ZY', 'ZY'],
            ['ZZ', 'ZZ'],
        ]),
        (frame, targets) => frame.do_measure('XX', targets),
        (op, coordFunc, ctx) => {
            let [x1, y1] = coordFunc(op.id_targets[0]);
            let [x2, y2] = coordFunc(op.id_targets[1]);

            draw_connector(ctx, x1, y1, x2, y2);

            ctx.fillStyle = 'gray';
            ctx.fillRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.fillRect(x2 - rad, y2 - rad, rad*2, rad*2);
            ctx.strokeStyle = 'black';
            ctx.strokeRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.strokeRect(x2 - rad, y2 - rad, rad*2, rad*2);
            ctx.fillStyle = 'black';
            ctx.textAlign = "center";
            ctx.textBaseline = 'middle';
            ctx.fillText('MXX', x1, y1);
            ctx.fillText('MXX', x2, y2);
        },
    );
    yield new Gate(
        'MYY',
        2,
        true,
        false,
        new Map([
            ['II', 'II'],
            ['IX', 'ERR:IX'],
            ['IY', 'IY'],
            ['IZ', 'ERR:IZ'],

            ['XI', 'ERR:XI'],
            ['XX', 'XX'],
            ['XY', 'ERR:XY'],
            ['XZ', 'XZ'],

            ['YI', 'YI'],
            ['YX', 'ERR:YX'],
            ['YY', 'YY'],
            ['YZ', 'ERR:YZ'],

            ['ZI', 'ERR:ZI'],
            ['ZX', 'ZX'],
            ['ZY', 'ERR:ZY'],
            ['ZZ', 'ZZ'],
        ]),
        (frame, targets) => frame.do_measure('YY', targets),
        (op, coordFunc, ctx) => {
            let [x1, y1] = coordFunc(op.id_targets[0]);
            let [x2, y2] = coordFunc(op.id_targets[1]);

            draw_connector(ctx, x1, y1, x2, y2);

            ctx.fillStyle = 'gray';
            ctx.fillRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.fillRect(x2 - rad, y2 - rad, rad*2, rad*2);
            ctx.strokeStyle = 'black';
            ctx.strokeRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.strokeRect(x2 - rad, y2 - rad, rad*2, rad*2);
            ctx.fillStyle = 'black';
            ctx.textAlign = "center";
            ctx.textBaseline = 'middle';
            ctx.fillText('MYY', x1, y1);
            ctx.fillText('MYY', x2, y2);
        },
    );
    yield new Gate(
        'MZZ',
        2,
        true,
        false,
        new Map([
            ['II', 'II'],
            ['IX', 'ERR:IX'],
            ['IY', 'ERR:IY'],
            ['IZ', 'IZ'],

            ['XI', 'ERR:XI'],
            ['XX', 'XX'],
            ['XY', 'XY'],
            ['XZ', 'ERR:XZ'],

            ['YI', 'ERR:YI'],
            ['YX', 'YX'],
            ['YY', 'YY'],
            ['YZ', 'ERR:YZ'],

            ['ZI', 'ZI'],
            ['ZX', 'ERR:ZX'],
            ['ZY', 'ERR:ZY'],
            ['ZZ', 'ZZ'],
        ]),
        (frame, targets) => frame.do_measure('ZZ', targets),
        (op, coordFunc, ctx) => {
            let [x1, y1] = coordFunc(op.id_targets[0]);
            let [x2, y2] = coordFunc(op.id_targets[1]);

            draw_connector(ctx, x1, y1, x2, y2);

            ctx.fillStyle = 'gray';
            ctx.fillRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.fillRect(x2 - rad, y2 - rad, rad*2, rad*2);
            ctx.strokeStyle = 'black';
            ctx.strokeRect(x1 - rad, y1 - rad, rad*2, rad*2);
            ctx.strokeRect(x2 - rad, y2 - rad, rad*2, rad*2);
            ctx.fillStyle = 'black';
            ctx.textAlign = "center";
            ctx.textBaseline = 'middle';
            ctx.fillText('MZZ', x1, y1);
            ctx.fillText('MZZ', x2, y2);
        },
    );
}

export {iter_gates_pair_measurements};
