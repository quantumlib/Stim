import {Gate} from "./gate.js"
import {draw_connector, draw_swap_control, draw_iswap_control} from './gate_draw_util.js';

function *iter_gates_swaps() {
    yield new Gate(
        'ISWAP',
        2,
        true,
        false,
        new Map([
            ['IX', 'YZ'],
            ['IZ', 'ZI'],
            ['XI', 'ZY'],
            ['ZI', 'IZ'],
        ]),
        (frame, targets) => frame.do_iswap(targets),
        (op, coordFunc, ctx) => {
            let [x1, y1] = coordFunc(op.id_targets[0]);
            let [x2, y2] = coordFunc(op.id_targets[1]);
            draw_connector(ctx, x1, y1, x2, y2);
            draw_iswap_control(ctx, x1, y1);
            draw_iswap_control(ctx, x2, y2);
        },
    )
    yield new Gate(
        'ISWAP_DAG',
        2,
        true,
        false,
        new Map([
            ['IX', 'YZ'],
            ['IZ', 'ZI'],
            ['XI', 'ZY'],
            ['ZI', 'IZ'],
        ]),
        (frame, targets) => frame.do_iswap(targets),
        (op, coordFunc, ctx) => {
            let [x1, y1] = coordFunc(op.id_targets[0]);
            let [x2, y2] = coordFunc(op.id_targets[1]);
            draw_connector(ctx, x1, y1, x2, y2);
            draw_iswap_control(ctx, x1, y1);
            draw_iswap_control(ctx, x2, y2);
        },
    )
    yield new Gate(
        'SWAP',
        2,
        true,
        false,
        new Map([
            ['IX', 'XI'],
            ['IZ', 'ZI'],
            ['XI', 'IX'],
            ['ZI', 'IZ'],
        ]),
        (frame, targets) => frame.do_swap(targets),
        (op, coordFunc, ctx) => {
            let [x1, y1] = coordFunc(op.id_targets[0]);
            let [x2, y2] = coordFunc(op.id_targets[1]);
            draw_connector(ctx, x1, y1, x2, y2);
            draw_swap_control(ctx, x1, y1);
            draw_swap_control(ctx, x2, y2);
        },
    );
}

export {iter_gates_swaps};
