import {pitch, rad} from "../draw/config.js"

/**
 * @param {!CanvasRenderingContext2D} ctx
 * @param {undefined|!number} x
 * @param {undefined|!number} y
 */
function draw_x_control(ctx, x, y) {
    if (x === undefined || y === undefined) {
        return;
    }

    ctx.strokeStyle = 'black';
    ctx.fillStyle = 'white';
    ctx.beginPath();
    ctx.arc(x, y, rad, 0, 2 * Math.PI);
    ctx.fill();
    ctx.stroke();

    ctx.beginPath();
    ctx.moveTo(x, y - rad);
    ctx.lineTo(x, y + rad);
    ctx.stroke();
    ctx.beginPath();
    ctx.moveTo(x - rad, y);
    ctx.lineTo(x + rad, y);
    ctx.stroke();
}

/**
 * @param {!CanvasRenderingContext2D} ctx
 * @param {undefined|!number} x
 * @param {undefined|!number} y
 */
function draw_y_control(ctx, x, y) {
    if (x === undefined || y === undefined) {
        return;
    }
    ctx.strokeStyle = 'black';
    ctx.fillStyle = '#AAA';
    ctx.beginPath();
    ctx.moveTo(x, y + rad);
    ctx.lineTo(x + rad, y - rad);
    ctx.lineTo(x - rad, y - rad);
    ctx.lineTo(x, y + rad);
    ctx.stroke();
    ctx.fill();
}

/**
 * @param {!CanvasRenderingContext2D} ctx
 * @param {undefined|!number} x
 * @param {undefined|!number} y
 */
function draw_z_control(ctx, x, y) {
    if (x === undefined || y === undefined) {
        return;
    }
    ctx.fillStyle = 'black';
    ctx.beginPath();
    ctx.arc(x, y, rad, 0, 2 * Math.PI);
    ctx.fill();
}

/**
 * @param {!CanvasRenderingContext2D} ctx
 * @param {undefined|!number} x
 * @param {undefined|!number} y
 */
function draw_xswap_control(ctx, x, y) {
    if (x === undefined || y === undefined) {
        return;
    }
    ctx.fillStyle = 'white';
    ctx.strokeStyle = 'black';
    ctx.beginPath();
    ctx.arc(x, y, rad, 0, 2 * Math.PI);
    ctx.fill();
    ctx.stroke();

    let r = rad * 0.4;
    ctx.strokeStyle = 'black';
    ctx.lineWidth = 3;
    ctx.beginPath();
    ctx.moveTo(x - r, y - r);
    ctx.lineTo(x + r, y + r);
    ctx.stroke();
    ctx.moveTo(x - r, y + r);
    ctx.lineTo(x + r, y - r);
    ctx.stroke();
    ctx.lineWidth = 1;
}

/**
 * @param {!CanvasRenderingContext2D} ctx
 * @param {undefined|!number} x
 * @param {undefined|!number} y
 */
function draw_zswap_control(ctx, x, y) {
    if (x === undefined || y === undefined) {
        return;
    }
    ctx.fillStyle = 'black';
    ctx.strokeStyle = 'black';
    ctx.beginPath();
    ctx.arc(x, y, rad, 0, 2 * Math.PI);
    ctx.fill();
    ctx.stroke();

    let r = rad * 0.4;
    ctx.strokeStyle = 'white';
    ctx.lineWidth = 3;
    ctx.beginPath();
    ctx.moveTo(x - r, y - r);
    ctx.lineTo(x + r, y + r);
    ctx.stroke();
    ctx.moveTo(x - r, y + r);
    ctx.lineTo(x + r, y - r);
    ctx.stroke();
    ctx.lineWidth = 1;
}

/**
 * @param {!CanvasRenderingContext2D} ctx
 * @param {undefined|!number} x
 * @param {undefined|!number} y
 */
function draw_iswap_control(ctx, x, y) {
    if (x === undefined || y === undefined) {
        return;
    }
    ctx.fillStyle = '#888';
    ctx.strokeStyle = '#222';
    ctx.beginPath();
    ctx.arc(x, y, rad, 0, 2 * Math.PI);
    ctx.fill();
    ctx.stroke();

    let r = rad * 0.4;
    ctx.lineWidth = 3;
    ctx.strokeStyle = 'black';
    ctx.beginPath();
    ctx.moveTo(x - r, y - r);
    ctx.lineTo(x + r, y + r);
    ctx.stroke();
    ctx.moveTo(x - r, y + r);
    ctx.lineTo(x + r, y - r);
    ctx.stroke();
    ctx.lineWidth = 1;
}

/**
 * @param {!CanvasRenderingContext2D} ctx
 * @param {undefined|!number} x
 * @param {undefined|!number} y
 */
function draw_swap_control(ctx, x, y) {
    if (x === undefined || y === undefined) {
        return;
    }
    let r = rad / 3;
    ctx.strokeStyle = 'black';
    ctx.beginPath();
    ctx.moveTo(x - r, y - r);
    ctx.lineTo(x + r, y + r);
    ctx.stroke();
    ctx.moveTo(x - r, y + r);
    ctx.lineTo(x + r, y - r);
    ctx.stroke();
}

/**
 * @param {!CanvasRenderingContext2D} ctx
 * @param {undefined|!number} x
 * @param {undefined|!number} y
 */
function stroke_degenerate_connector(ctx, x, y) {
    if (x === undefined || y === undefined) {
        return;
    }
    let r = rad * 1.1;
    ctx.strokeRect(x - r, y - r, r * 2, r * 2);
}

/**
 * @param {!CanvasRenderingContext2D} ctx
 * @param {undefined|!number} x1
 * @param {undefined|!number} y1
 * @param {undefined|!number} x2
 * @param {undefined|!number} y2
 */
function stroke_connector_to(ctx, x1, y1, x2, y2) {
    if (x1 === undefined || y1 === undefined || x2 === undefined || y2 === undefined) {
        stroke_degenerate_connector(ctx, x1, y1);
        stroke_degenerate_connector(ctx, x2, y2);
        return;
    }
    if (x2 < x1 || (x2 === x1 && y2 < y1)) {
        stroke_connector_to(ctx, x2, y2, x1, y1);
        return;
    }

    let dx = x2 - x1;
    let dy = y2 - y1;
    let d = Math.sqrt(dx*dx + dy*dy);
    let ux = dx / d * 14;
    let uy = dy / d * 14;
    let px = uy;
    let py = -ux;

    ctx.beginPath();
    ctx.moveTo(x1, y1);
    if (d < pitch * 1.1) {
        ctx.lineTo(x2, y2);
    } else {
        ctx.bezierCurveTo(x1 + ux + px, y1 + uy + py, x2 - ux + px, y2 - uy + py, x2, y2);
    }
    ctx.stroke();
}

/**
 * @param {!CanvasRenderingContext2D} ctx
 * @param {undefined|!number} x1
 * @param {undefined|!number} y1
 * @param {undefined|!number} x2
 * @param {undefined|!number} y2
 */
function draw_connector(ctx, x1, y1, x2, y2) {
    ctx.lineWidth = 2;
    ctx.strokeStyle = 'black';
    stroke_connector_to(ctx, x1, y1, x2, y2);
    ctx.lineWidth = 1;
}

export {
    draw_x_control,
    draw_y_control,
    draw_z_control,
    draw_swap_control,
    draw_iswap_control,
    stroke_connector_to,
    draw_connector,
    draw_xswap_control,
    draw_zswap_control,
};
