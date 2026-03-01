// Node.js script, invoked by Makefile during build:
// convert a 768-byte 96-character 8x8 pixel font to a pile of SVG files.

import * as fs from 'node:fs';
import * as process from 'node:process';

import { pathFromRects, type Rect } from './path.js';

const [_process, _script, input_file, output_prefix] = process.argv;

function process_glyph(font: Buffer, index: number) {
    const width = 1024;
    const height = (width / (((388 / 644) * 4) / 3)) | 0;
    const scale = (p: number, f: number) => ((p * f) / 8) | 0;
    const rects: Rect[] = [];

    for (var y = 0; y < 8; y++) {
        var byte = font[y * 0x60 + index];
        for (var x = 0; x < 8; x++) {
            if (byte & (0x80 >> x)) {
                rects.push({
                    left: scale(x, width),
                    top: scale(y, height),
                    right: scale(x + 1, width),
                    bottom: scale(y + 1, height),
                });
            }
        }
    }

    const svg =
        '<?xml version="1.0" encoding="UTF-8" standalone="no"?>' +
        '<!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.0//EN" "http://www.w3.org/TR/SVG/DTD/svg10.dtd">' +
        `<svg width="${width}" height="${height}"><path d="${pathFromRects(rects)}" fill="currentColor"/></svg>`;

    const filename = `${output_prefix}${('00' + index).slice(-2)}.svg`;
    fs.writeFileSync(filename, svg);
}

const font = fs.readFileSync(input_file);
for (let index = 0; index < 0x60; index++) {
    process_glyph(font, index);
}
