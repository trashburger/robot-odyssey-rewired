import { diff_core } from 'fast-myers-diff';
import { pathFromRects, type Rect } from '../images/path.js';
import {
    CGA_WIDTH,
    CGA_HEIGHT,
    type DisplayListRenderer,
    type DisplayListItem,
    type CGADisplayItem,
    type RoomDisplayItem,
    type SpriteDisplayItem,
    type HLineDisplayItem,
    type VLineDisplayItem,
    type TextDisplayItem,
} from '../index.js';

// Game resolution is 160x192 with rectangular pixels, convert to 1280x960 square SVG grid.
// This is the minimum size that lets us keep to integer coordinates with correct aspect.
export const X_SCALE = 8;
export const Y_SCALE = 5;

function setXY(el: SVGElement, x: number, y: number, h: number) {
    el.setAttribute('transform', `translate(${x * X_SCALE},${(192 - y - h) * Y_SCALE})`);
}

function setWH(el: SVGElement, w: number, h: number) {
    el.setAttribute('width', (w * X_SCALE).toString());
    el.setAttribute('height', (h * Y_SCALE).toString());
}

function setXYWH(el: SVGElement, x: number, y: number, w: number, h: number) {
    setXY(el, x, y, h);
    setWH(el, w, h);
}

function colorClass(n: number): string {
    return `color-${n}`;
}

function updateClasses(el: SVGElement | HTMLElement, classes: string[]) {
    const list = el.classList;

    // Remove deleted classes
    list.forEach(function (value: string, index: number, list: DOMTokenList) {
        if (classes.indexOf(value) < 0) {
            list.remove(value);
        }
    });

    // Add new classes
    for (let cls of classes) {
        if (!list.contains(cls)) {
            list.add(cls);
        }
    }
}

// Small image converted to a path, used for sprites and rooms
class ImagePath {
    data: Uint8Array;
    path: string;

    constructor(data: Uint8Array) {
        this.data = data;
        const rects: Rect[] = [];

        if (data.length == 16) {
            // Sprite image (7x16)
            for (let y = 0; y < 16; y++) {
                const byt = data[y];
                for (let x = 0; x < 7; x++) {
                    if (byt & (0x40 >>> x)) {
                        rects.push([x * X_SCALE, (15 - y) * Y_SCALE, (x + 1) * X_SCALE, (16 - y) * Y_SCALE]);
                    }
                }
            }
            this.path = pathFromRects(rects);
        } else if (data.length == 30) {
            // Room image (20x12)
            for (let i = 0; i < 30; i++) {
                const byt = data[i];
                for (let j = 0; j < 8; j++) {
                    if ((byt >> j) & 1) {
                        const x = 8 * ((i % 10) * 2 + (j >> 2));
                        const y = 16 * (((i / 10) | 0) * 4 + (j & 3));
                        rects.push([x * X_SCALE, y * Y_SCALE, (x + 8) * X_SCALE, (y + 16) * Y_SCALE]);
                    }
                }
            }
            this.path = pathFromRects(rects);
        } else {
            this.path = '';
        }
    }
}

type CGAPalette = (string | CanvasGradient | CanvasPattern)[];

// Draw CGA framebuffers to a canvas of the same size
function cgaDraw(ctx: CanvasRenderingContext2D, palette: CGAPalette, data: Uint8Array) {
    for (let color = 0; color < 4; color++) {
        ctx.fillStyle = palette[color];
        const c2 = color << 2;
        const c4 = color << 4;
        const c6 = color << 6;

        for (let plane = 0; plane < 2; plane++) {
            let offset = plane * 0x2000;
            for (let line = 0; line < 100; line++) {
                const y = line * 2 + plane;
                for (let column = 0; column < 80; column++) {
                    const x = 4 * column;
                    const byt = data[offset++];
                    if ((byt & 0xc0) == c6) {
                        ctx.fillRect(x, y, 1, 1);
                    }
                    if ((byt & 0x30) == c4) {
                        ctx.fillRect(x + 1, y, 1, 1);
                    }
                    if ((byt & 0x0c) == c2) {
                        ctx.fillRect(x + 2, y, 1, 1);
                    }
                    if ((byt & 0x03) == color) {
                        ctx.fillRect(x + 3, y, 1, 1);
                    }
                }
            }
        }
    }
}

// Cache of path strings created from small images, for sprites and rooms
class ImagePathCache {
    map: { [key: string]: ImagePath };

    constructor() {
        this.map = {};
    }

    get(data: Uint8Array): string {
        const key = data.toString();
        const cached = this.map[key];
        if (cached) {
            return cached.path;
        }
        return (this.map[key] = new ImagePath(data)).path;
    }
}

class SVGDisplayItem {
    dl: DisplayListItem;
    el: SVGElement;

    // Type-specific information to remember between build() and update()
    fgEl?: SVGElement;
    bgEl?: SVGElement;
    ctx?: CanvasRenderingContext2D;
    canvas?: HTMLElement;
    palette?: CGAPalette;

    constructor(dl: DisplayListItem, el: SVGElement) {
        this.dl = dl;
        this.el = el;
    }

    update(rend: SVGDisplayRenderer, replacement: DisplayListItem) {
        const el = this.el;
        const [from_union, from_classes] = this.dl;
        const [to_union, to_classes] = replacement;
        const from_key = from_union[0];
        const to_key = to_union[0];

        if (from_key === to_key) {
            switch (to_key) {
                case 'sprite': {
                    const [from_key, from_x, from_y, from_color, from_data] = from_union as SpriteDisplayItem;
                    const [to_key, to_x, to_y, to_color, to_data] = to_union as SpriteDisplayItem;
                    if (to_x !== from_x || to_y !== from_y) {
                        setXY(el, to_x, to_y, 16);
                    }
                    if (!sExpressionEqual(from_data, to_data)) {
                        const path = rend.images.get(to_data);
                        el.setAttribute('d', rend.images.get(to_data));
                    }
                    updateClasses(el, to_classes.concat([to_key, colorClass(to_color)]));
                    this.dl = replacement;
                    return;
                }

                case 'hline': {
                    const [from_key, from_x1, from_x2, from_y, from_color] = from_union as HLineDisplayItem;
                    const [to_key, to_x1, to_x2, to_y, to_color] = to_union as HLineDisplayItem;
                    if (from_x1 !== to_x1 || from_x2 !== to_x2 || from_y !== to_y) {
                        const x_low = Math.min(to_x1, to_x2);
                        const x_high = Math.max(to_x1, to_x2);
                        setXYWH(el, x_low, to_y, x_high - x_low + 1, 1);
                    }
                    updateClasses(el, to_classes.concat([to_key, colorClass(to_color)]));
                    this.dl = replacement;
                    return;
                }

                case 'vline': {
                    const [from_key, from_x, from_y1, from_y2, from_color] = from_union as VLineDisplayItem;
                    const [to_key, to_x, to_y1, to_y2, to_color] = to_union as VLineDisplayItem;
                    if (from_x !== to_x || from_y1 !== to_y1 || from_y2 !== to_y2) {
                        const y_low = Math.min(to_y1, to_y2);
                        const y_high = Math.max(to_y1, to_y2);
                        setXYWH(el, to_x, y_low, 1, y_high - y_low + 1);
                    }
                    updateClasses(el, to_classes.concat([to_key, colorClass(to_color)]));
                    this.dl = replacement;
                    return;
                }

                case 'room': {
                    const [from_key, from_fg, from_bg, from_data] = from_union as RoomDisplayItem;
                    const [to_key, to_fg, to_bg, to_data] = to_union as RoomDisplayItem;
                    if (this.bgEl && from_bg !== to_bg) {
                        updateClasses(this.bgEl, ['room-bg', colorClass(to_bg)]);
                    }
                    if (this.fgEl) {
                        updateClasses(this.fgEl, ['room-fg', colorClass(to_fg)]);
                        if (!sExpressionEqual(from_data, to_data)) {
                            this.fgEl.setAttribute('d', rend.images.get(to_data));
                        }
                    }
                    updateClasses(el, to_classes.concat(['room']));
                    this.dl = replacement;
                    return;
                }

                case 'cga': {
                    const [to_key, to_data] = to_union as CGADisplayItem;
                    if (this.ctx && this.palette) {
                        cgaDraw(this.ctx, this.palette, to_data);
                    }
                    if (this.canvas) {
                        updateClasses(this.canvas, to_classes);
                    }
                    this.dl = replacement;
                    return;
                }
            }
        }

        // No special path for this update, replace with a rebuilt element
        const rebuilt = SVGDisplayItem.build(rend, replacement);
        rend.group.replaceChild(rebuilt.el, this.el);
        Object.assign(this, rebuilt);
    }

    static build(rend: SVGDisplayRenderer, dl: DisplayListItem): SVGDisplayItem {
        const [dl_union, classes] = dl;
        const key = dl_union[0];

        switch (key) {
            case 'sprite': {
                const el = rend.create('path');
                const [key, x, y, color, data] = dl_union;
                setXY(el, x, y, 16);
                updateClasses(el, classes.concat([key, colorClass(color)]));
                el.setAttribute('d', rend.images.get(data));
                return new SVGDisplayItem(dl, el);
            }

            case 'hline': {
                const [key, x1, x2, y, color] = dl_union;
                const x_low = Math.min(x1, x2);
                const x_high = Math.max(x1, x2);
                if (x_high >= 160) {
                    return new SVGDisplayItem(dl, rend.create('g'));
                }
                const el = rend.create('rect');
                setXYWH(el, x_low, y, x_high - x_low + 1, 1);
                updateClasses(el, classes.concat([key, colorClass(color)]));
                return new SVGDisplayItem(dl, el);
            }

            case 'vline': {
                const [key, x, y1, y2, color] = dl_union;
                const y_low = Math.min(y1, y2);
                const y_high = Math.max(y1, y2);
                if (y_high >= 192) {
                    return new SVGDisplayItem(dl, rend.create('g'));
                }
                const el = rend.create('rect');
                setXYWH(el, x, y_low, 1, y_high - y_low + 1);
                updateClasses(el, classes.concat([key, colorClass(color)]));
                return new SVGDisplayItem(dl, el);
            }

            case 'text': {
                const [key, x, y, color, font_id, style, text] = dl_union;

                // Style 0 (normal) is monochrome and byte-aligned.
                // Style 2 (big) is zoomed 2x and supports color.
                const big = style === 2;
                const x_aligned = big ? x : x & ~1;
                const zoom = big ? 2 : 1;
                const style_color = big ? color : 3;

                if (x_aligned >= 160 || y >= 192 - 8) {
                    return new SVGDisplayItem(dl, rend.create('g'));
                }

                const el = rend.create('text');
                setXY(el, x_aligned, y, 0);
                el.style.fontFamily = font_id === 0 ? 'rofont' : `rofont-${font_id}`;
                el.style.fontSize = `${8 * zoom * Y_SCALE}px`;
                updateClasses(el, classes.concat([key, colorClass(style_color)]));

                // We can't rely on browser support for CSS "white-space: pre" on SVG text yet.
                // Put additional lines in a <tspan>, and use non-breaking spaces at the beginning
                // or end of the line, or when multiple spaces appear consecutively.
                let span_y = 0;
                for (let line of text.split(/[\n\r]/)) {
                    const line_node = rend.text(line);
                    if (span_y) {
                        const span = rend.create('tspan');
                        span.setAttribute('x', '0');
                        span.setAttribute('y', `${span_y}`);
                        span.appendChild(line_node);
                        el.appendChild(span);
                    } else {
                        el.appendChild(line_node);
                    }
                    span_y += 9 * zoom * Y_SCALE;
                }

                return new SVGDisplayItem(dl, el);
            }

            case 'room': {
                const [key, fg, bg, data] = dl_union;
                const el = rend.create('g');
                const bg_el = rend.create('rect');
                const fg_el = rend.create('path');
                setWH(bg_el, 160, 192);
                updateClasses(el, classes.concat(['room']));
                updateClasses(bg_el, ['room-bg', colorClass(bg)]);
                updateClasses(fg_el, ['room-fg', colorClass(fg)]);
                fg_el.setAttribute('d', rend.images.get(data));
                el.appendChild(bg_el);
                el.appendChild(fg_el);
                const item = new SVGDisplayItem(dl, el);
                item.fgEl = fg_el;
                item.bgEl = bg_el;
                return item;
            }

            case 'cga': {
                const [key, data] = dl_union;
                const palette = rend.cgaPalette();
                const el = rend.create('foreignObject');
                setWH(el, 160, 192);
                const canvas = document.createElement('canvas');
                canvas.width = CGA_WIDTH;
                canvas.height = CGA_HEIGHT;
                canvas.style.width = canvas.style.height = '100%';
                updateClasses(canvas, classes.concat(['cga']));
                const ctx = canvas.getContext('2d');
                el.appendChild(canvas);
                const item = new SVGDisplayItem(dl, el);
                if (ctx) {
                    cgaDraw(ctx, palette, data);
                    item.ctx = ctx;
                    item.canvas = canvas;
                    item.palette = palette;
                }
                return item;
            }

            case 'loading': {
                const el = rend.create('rect');
                updateClasses(el, classes.concat(['loading']));
                return new SVGDisplayItem(dl, el);
            }

            case 'error': {
                const [key, name, message] = dl_union;
                const el = rend.create('text');
                updateClasses(el, classes.concat(['error']));
                const name_el = rend.create('tspan');
                const message_el = rend.create('tspan');
                name_el.appendChild(rend.text(name));
                message_el.appendChild(rend.text(message));
                el.appendChild(name_el);
                el.appendChild(message_el);
                return new SVGDisplayItem(dl, el);
            }
        }
        return new SVGDisplayItem(dl, rend.create('g'));
    }
}

type SExpression = SExpression[] | Uint8Array | string | number;

function sExpressionEqual(a: SExpression, b: SExpression) {
    const t = typeof a;
    if (t !== typeof b) {
        return false;
    }
    if (t === 'string' || t === 'number') {
        return a === b;
    }
    const lA = a as SExpression[] | Uint8Array;
    const lB = b as SExpression[] | Uint8Array;
    const l = lA.length;
    if (l !== lB.length) {
        return false;
    }
    for (let i = 0; i < l; i++) {
        if (!sExpressionEqual(lA[i], lB[i])) {
            return false;
        }
    }
    return true;
}

export class SVGDisplayRenderer implements DisplayListRenderer {
    svg: SVGSVGElement;
    cgaProbe: SVGElement[];
    items: SVGDisplayItem[];
    images: ImagePathCache;
    group: SVGGElement;

    constructor(svg: SVGSVGElement) {
        this.svg = svg;
        this.cgaProbe = [];
        this.items = [];
        this.images = new ImagePathCache();
        this.group = document.createElementNS(svg.namespaceURI, 'g') as SVGGElement;

        // Mapping to regular color table from the 4-color CGA palette in cutscenes
        const cgaColors = [
            0, // Black
            6, // Cyan -> Blue
            5, // Magenta -> Orange
            3, // White
        ];

        // Set up probe elements that we will use in cgaPalette()
        for (let color of cgaColors) {
            const probe = this.create('path');
            probe.setAttribute('d', '');
            probe.classList.add('cga');
            probe.classList.add(colorClass(color));
            svg.appendChild(probe);
            this.cgaProbe.push(probe);
        }

        svg.appendChild(this.group);
        svg.setAttribute('viewBox', `0 0 ${160 * X_SCALE} ${192 * Y_SCALE}`);
        svg.classList.add('robot-odyssey-rewired');
    }

    cgaPalette(): CGAPalette {
        const result = [];
        for (let el of this.cgaProbe) {
            result.push(getComputedStyle(el)['fill']);
        }
        return result;
    }

    create(name: string): SVGElement {
        return document.createElementNS(this.svg.namespaceURI, name) as SVGElement;
    }

    text(line: string): Text {
        return document.createTextNode(line.replace(/^ | $| (?= )|(?<= ) /g, '\u00A0'));
    }

    present(dl: DisplayListItem[]) {
        const items = this.items;
        let offset = 0;

        // Display lists are s-expressions with stable toString() representation, use that for fast compare
        function eq(i: number, j: number) {
            return sExpressionEqual(items[i].dl, dl[j]);
        }

        // The generic fast-myers-diff core yields 4-tuples that represent contiguous runs
        // of deletions and/or insertions at one location.

        const ops = Array.from(diff_core(0, items.length, 0, dl.length, eq));
        ops.sort((a: number[], b: number[]) => a[0] - b[0]);

        for (let op of ops) {
            const [sx, ex, sy, ey] = op;

            // Plan to in-place update elements that are in both x and y
            const x_count = ex - sx;
            const y_count = ey - sy;
            const num_update = Math.min(x_count, y_count);
            const num_delete = x_count - num_update;
            const num_insert = y_count - num_update;

            // Updates
            for (let i = 0; i < num_update; i++) {
                items[offset + sx + i].update(this, dl[sy + i]);
            }

            // Deletions
            if (num_delete > 0) {
                const lower = offset + sx + num_update;
                const upper = lower + num_delete;
                for (let i = lower; i < upper; i++) {
                    this.group.removeChild(items[i].el);
                }
                items.splice(lower, num_delete);
                offset -= num_delete;
            }

            // Insertions
            for (let i = 0; i < num_insert; i++) {
                const item = SVGDisplayItem.build(this, dl[sy + num_update + i]);
                const x = offset + sx + num_update;
                items.splice(x, 0, item);
                offset += 1;
                if (x === 0) {
                    this.group.insertAdjacentElement('afterbegin', item.el);
                } else {
                    items[x - 1].el.insertAdjacentElement('afterend', item.el);
                }
            }
        }
    }
}
