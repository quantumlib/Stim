import {Gate} from "./gate.js"
import {draw_x_control, draw_y_control, draw_z_control, draw_connector} from "./gate_draw_util.js"

function *iter_gates_controlled_paulis() {
    yield new Gate(
        'CX',
        2,
        true,
        false,
        new Map([
            ['IX', 'IX'],
            ['IZ', 'ZZ'],
            ['XI', 'XX'],
            ['ZI', 'ZI'],
        ]),
        (frame, targets) => frame.do_cx(targets),
        (op, coordFunc, ctx) => {
            let [x1, y1] = coordFunc(op.id_targets[0]);
            let [x2, y2] = coordFunc(op.id_targets[1]);
            draw_connector(ctx, x1, y1, x2, y2);
            draw_z_control(ctx, x1, y1);
            draw_x_control(ctx, x2, y2);
        },
    )
    yield new Gate(
        'CY',
        2,
        true,
        false,
        new Map([
            ['IX', 'ZX'],
            ['IZ', 'ZZ'],
            ['XI', 'XY'],
            ['ZI', 'ZI'],
        ]),
        (frame, targets) => frame.do_cy(targets),
        (op, coordFunc, ctx) => {
            let [x1, y1] = coordFunc(op.id_targets[0]);
            let [x2, y2] = coordFunc(op.id_targets[1]);
            draw_connector(ctx, x1, y1, x2, y2);
            draw_z_control(ctx, x1, y1);
            draw_y_control(ctx, x2, y2);
        },
    )
    yield new Gate(
        'XCX',
        2,
        true,
        false,
        new Map([
            ['IX', 'IX'],
            ['IZ', 'XZ'],
            ['XI', 'XI'],
            ['ZI', 'ZX'],
        ]),
        (frame, targets) => frame.do_xcx(targets),
        (op, coordFunc, ctx) => {
            let [x1, y1] = coordFunc(op.id_targets[0]);
            let [x2, y2] = coordFunc(op.id_targets[1]);
            draw_connector(ctx, x1, y1, x2, y2);
            draw_x_control(ctx, x1, y1);
            draw_x_control(ctx, x2, y2);
        },
    )
    yield new Gate(
        'XCY',
        2,
        true,
        false,
        new Map([
            ['IX', 'XX'],
            ['IZ', 'XZ'],
            ['XI', 'XI'],
            ['ZI', 'ZY'],
        ]),
        (frame, targets) => frame.do_xcy(targets),
        (op, coordFunc, ctx) => {
            let [x1, y1] = coordFunc(op.id_targets[0]);
            let [x2, y2] = coordFunc(op.id_targets[1]);
            draw_connector(ctx, x1, y1, x2, y2);
            draw_x_control(ctx, x1, y1);
            draw_y_control(ctx, x2, y2);
        },
    )
    yield new Gate(
        'YCY',
        2,
        true,
        false,
        new Map([
            ['IX', 'YX'],
            ['IZ', 'YZ'],
            ['XI', 'XY'],
            ['ZI', 'ZY'],
        ]),
        (frame, targets) => frame.do_ycy(targets),
        (op, coordFunc, ctx) => {
            let [x1, y1] = coordFunc(op.id_targets[0]);
            let [x2, y2] = coordFunc(op.id_targets[1]);
            draw_connector(ctx, x1, y1, x2, y2);
            draw_y_control(ctx, x1, y1);
            draw_y_control(ctx, x2, y2);
        },
    )
    yield new Gate(
        'CZ',
        2,
        true,
        false,
        new Map([
            ['IX', 'ZX'],
            ['IZ', 'IZ'],
            ['XI', 'XZ'],
            ['ZI', 'ZI'],
        ]),
        (frame, targets) => frame.do_cz(targets),
        (op, coordFunc, ctx) => {
            let [x1, y1] = coordFunc(op.id_targets[0]);
            let [x2, y2] = coordFunc(op.id_targets[1]);
            draw_connector(ctx, x1, y1, x2, y2);
            draw_z_control(ctx, x1, y1);
            draw_z_control(ctx, x2, y2);
        },
    )
}

export {iter_gates_controlled_paulis};
