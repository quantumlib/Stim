import {rad} from "../draw/config.js"
import {Gate} from "./gate.js"
import {beginPathPolygon} from '../draw/draw_util.js';

/**
 * @param {!int} mi
 * @param {* | undefined} key
 * @param {!Map<!string, !int> | undefined} hitCount
 * @returns {!{wx: !number, wy: !number, dx: !number, dy: !number}}
 */
function marker_placement(mi, key, hitCount) {
    let dx, dy, wx, wy;
    if (mi < 0 && hitCount !== undefined) {
        // Detector.
        let d = hitCount.get(key)
        if (d === undefined) {
            d = 0;
        }
        hitCount.set(key, d + 1);
        dx = 9.5 - Math.round(d % 3.9 * 5);
        dy = 9.5 - Math.round(Math.floor(d / 4) % 3.8 * 5);
        wx = 3;
        wy = 3;
        if (mi < (-1 << 28)) {
            // Observable.
            dx += 2;
            wx += 4;
            dy += 2;
            wy += 4;
        }
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
        dx = Math.cos(mi * 0.6) * rad * 1.7;
        dy = Math.sin(mi * 0.6) * rad * 1.7;
        wx = 5;
        wy = 5;
        dx += wx / 2;
        dy += wy / 2;
    }
    return {dx, dy, wx, wy};
}

/**
 * @param {!string} color
 * @returns {!function}
 */
function make_marker_drawer(color) {
    return (op, coordFunc, ctx) => {
        let [x1, y1] = coordFunc(op.id_targets[0]);
        if (x1 === undefined || y1 === undefined) {
            return;
        }
        let {dx, dy, wx, wy} = marker_placement(op.args[0]);
        ctx.fillStyle = color
        if (wx === wy) {
            ctx.fillRect(x1 - dx - 2, y1 - dy - 2, wx + 4, wy + 4);
        } else {
            let x2 = x1 + (dx < 0 ? +1 : -1) * rad;
            let y2 = y1 + (dy < 0 ? +1 : -1) * rad;
            let x3 = x2 + (wx > rad ? +1 : 0) * rad * 2;
            let y3 = y2 + (wy > rad ? +1 : 0) * rad * 2;
            ctx.beginPath();
            ctx.moveTo(x1, y1);
            ctx.lineTo(x2, y2);
            ctx.lineTo(x3, y3);
            ctx.lineTo(x1, y1);
            ctx.fill();
        }
    };
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
                let [x, y] = coordFunc(t);
                x -= 0.5;
                y -= 0.5;
                transformedCoords.push([x, y]);
            }
            beginPathPolygon(ctx, transformedCoords);
            ctx.globalAlpha *= op.args[3];
            ctx.fillStyle = `rgb(${op.args[0]*255},${op.args[1]*255},${op.args[2]*255})`
            ctx.fill();
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
        'OBSERVABLE_INCLUDE',
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
        make_marker_drawer('red'),
    );
    yield new Gate(
        'MARKY',
        1,
        true,
        true,
        undefined,
        () => {},
        () => {},
        make_marker_drawer('green'),
    );
    yield new Gate(
        'MARKZ',
        1,
        true,
        true,
        undefined,
        () => {},
        () => {},
        make_marker_drawer('blue'),
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
