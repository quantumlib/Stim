import {rad} from './config.js';

/**
 * @param {!CanvasRenderingContext2D} ctx
 * @param {!Array<![!number, !number]>} coords
 */
function beginPathPolygon(ctx, coords) {
    ctx.beginPath();
    if (coords.length === 0) {
        return;
    }
    let n = coords.length;
    if (n === 1) {
        let [[x0, y0]] = coords;
        ctx.arc(x0, y0, rad * 1.7, 0, 2 * Math.PI);
    } else if (n === 2) {
        let [[x0, y0], [x1, y1]] = coords;
        let dx = x1 - x0;
        let dy = y1 - y0;
        let cx = (x1 + x0) / 2;
        let cy = (y1 + y0) / 2;
        let px = -dy;
        let py = dx;
        let pa = px*px + py*py;
        if (pa > 50*50) {
            let s = 50 / Math.sqrt(pa);
            px *= s;
            py *= s;
        }
        let ac1x = cx + px * 0.2 - dx * 0.2;
        let ac1y = cy + py * 0.2 - dy * 0.2;
        let ac2x = cx + px * 0.2 + dx * 0.2;
        let ac2y = cy + py * 0.2 + dy * 0.2;
        let bc1x = cx - px * 0.2 - dx * 0.2;
        let bc1y = cy - py * 0.2 - dy * 0.2;
        let bc2x = cx - px * 0.2 + dx * 0.2;
        let bc2y = cy - py * 0.2 + dy * 0.2;
        ctx.moveTo(x0, y0);
        ctx.bezierCurveTo(ac1x, ac1y, ac2x, ac2y, x1, y1);
        ctx.bezierCurveTo(bc2x, bc2y, bc1x, bc1y, x0, y0);
    } else {
        let [xn, yn] = coords[n - 1];
        ctx.moveTo(xn, yn);
        for (let k = 0; k < n; k++) {
            let [xk, yk] = coords[k];
            ctx.lineTo(xk, yk);
        }
    }
}

export {beginPathPolygon}
