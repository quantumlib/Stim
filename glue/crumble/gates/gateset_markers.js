import {rad} from "../draw/config.js"
import {Gate} from "./gate.js"
import {beginPathPolygon} from '../draw/draw_util.js';

/**
 * @param {!int} mi
 * @returns {!{wx: !number, wy: !number, dx: !number, dy: !number}}
 */
function marker_placement(mi) {
    let dx, dy, wx, wy;
    if (mi < 0) {
        dx = 10 - ~mi % 4 * 5;
        dy = 10 - Math.floor(~mi / 4) % 4 * 5;
        wx = 3;
        wy = 3;
    } else if (mi === 0) {
        dx = rad;
        dy = rad + 5;
        wx = rad * 2;
        wy = 5;
    } else if (mi === 1) {
        dx = -rad;
        dy = rad;
        wx = 5;
        wy = rad * 2;
    } else if (mi === 2) {
        dx = rad;
        dy = -rad;
        wx = rad * 2;
        wy = 5;
    } else if (mi === 3) {
        dx = rad + 5;
        dy = rad;
        wx = 5;
        wy = rad * 2;
    } else {
        dx = Math.cos(mi / 5) * rad;
        dy = Math.sin(mi / 5) * rad;
        wx = 5;
        wy = 5;
    }
    return {dx, dy, wx, wy};
}

function *iter_gates_markers() {
    yield new Gate(
        'POLYGON',
        undefined,
        false,
        true,
        undefined,
        () => {},
        () => {},
        (op, coordFunc, ctx) => {
            let transformedCoords = []
            for (let t of op.id_targets) {
                transformedCoords.push(coordFunc(t));
            }
            beginPathPolygon(ctx, transformedCoords);
            ctx.globalAlpha *= op.args[3];
            ctx.fillStyle = `rgb(${op.args[0]*255},${op.args[1]*255},${op.args[2]*255})`
            ctx.strokeStyle = `rgb(${op.args[0]*32},${op.args[1]*32},${op.args[2]*32})`
            ctx.fill();
            ctx.stroke();
        },
    );
    yield new Gate(
        'DETECTOR',
        undefined,
        false,
        true,
        undefined,
        () => {},
        () => {},
        (op, coordFunc, ctx) => {
        },
    );
    yield new Gate(
        'MARKX',
        1,
        true,
        true,
        undefined,
        () => {},
        () => {},
        (op, coordFunc, ctx) => {
            let [x1, y1] = coordFunc(op.id_targets[0]);
            if (x1 === undefined || y1 === undefined) {
                return;
            }
            let {dx, dy, wx, wy} = marker_placement(op.args[0]);
            let x2, y2, x3, y3;
            if (wx === wy) {
                x2 = x1 + dx;
                y2 = y1 + dy;
                x3 = x2 + dx + wx;
                y3 = y2 + dy + wy;
            } else {
                x2 = x1 + (dx < 0 ? +1 : -1) * rad;
                y2 = y1 + (dy < 0 ? +1 : -1) * rad;
                x3 = x2 + (wx > rad ? +1 : 0) * rad * 2;
                y3 = y2 + (wy > rad ? +1 : 0) * rad * 2;
            }
            ctx.fillStyle = 'red'
            ctx.beginPath();
            ctx.moveTo(x1, y1);
            ctx.lineTo(x2, y2);
            ctx.lineTo(x3, y3);
            ctx.lineTo(x1, y1);
            ctx.fill();
        }
    );
    yield new Gate(
        'MARKY',
        1,
        true,
        true,
        undefined,
        () => {},
        () => {},
        (op, coordFunc, ctx) => {
            let [x1, y1] = coordFunc(op.id_targets[0]);
            if (x1 === undefined || y1 === undefined) {
                return;
            }
            let {dx, dy, wx, wy} = marker_placement(op.args[0]);
            let x2, y2, x3, y3;
            if (wx === wy) {
                x2 = x1 + dx;
                y2 = y1 + dy;
                x3 = x2 + dx + wx;
                y3 = y2 + dy + wy;
            } else {
                x2 = x1 + (dx < 0 ? +1 : -1) * rad;
                y2 = y1 + (dy < 0 ? +1 : -1) * rad;
                x3 = x2 + (wx > rad ? +1 : 0) * rad * 2;
                y3 = y2 + (wy > rad ? +1 : 0) * rad * 2;
            }
            ctx.fillStyle = 'green'
            ctx.beginPath();
            ctx.moveTo(x1, y1);
            ctx.lineTo(x2, y2);
            ctx.lineTo(x3, y3);
            ctx.lineTo(x1, y1);
            ctx.fill();
        }
    );
    yield new Gate(
        'MARKZ',
        1,
        true,
        true,
        undefined,
        () => {},
        () => {},
        (op, coordFunc, ctx) => {
            let [x1, y1] = coordFunc(op.id_targets[0]);
            if (x1 === undefined || y1 === undefined) {
                return;
            }
            let {dx, dy, wx, wy} = marker_placement(op.args[0]);
            let x2, y2, x3, y3;
            if (wx === wy) {
                x2 = x1 + dx;
                y2 = y1 + dy;
                x3 = x2 + dx + wx;
                y3 = y2 + dy + wy;
            } else {
                x2 = x1 + (dx < 0 ? +1 : -1) * rad;
                y2 = y1 + (dy < 0 ? +1 : -1) * rad;
                x3 = x2 + (wx > rad ? +1 : 0) * rad * 2;
                y3 = y2 + (wy > rad ? +1 : 0) * rad * 2;
            }
            ctx.fillStyle = 'blue'
            ctx.beginPath();
            ctx.moveTo(x1, y1);
            ctx.lineTo(x2, y2);
            ctx.lineTo(x3, y3);
            ctx.lineTo(x1, y1);
            ctx.fill();
        }
    );
    yield new Gate(
        'MARK',
        1,
        false,
        true,
        undefined,
        () => {},
        () => {},
        (op, coordFunc, ctx) => {
            let [x1, y1] = coordFunc(op.id_targets[0]);
            if (x1 === undefined || y1 === undefined) {
                return;
            }
            ctx.fillStyle = 'magenta'
            ctx.fillRect(x1 - rad, y1 - rad, rad, rad);
        }
    );
}

export {iter_gates_markers, marker_placement};
