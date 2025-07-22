// Node.js script, invoked by Makefile during build:
// convert a 768-byte 96-character 8x8 pixel font to a pile of SVG files.

const fs = require('fs');
const Region2D = require('region2d').default;

const input_file = 'build/fs/font.fon';
const output_prefix = 'build/font/glyph';

fs.readFile(input_file, (err, font) => {
    for (let index = 0; index < 0x60; index++) {
        process_glyph(font, index);
    }
});

function process_glyph(font, index) {
    const width = 1024;
    const height = (width / (((388 / 644) * 4) / 3)) | 0;
    const scale = (p, f) => ((p * f) / 8) | 0;
    const pixel_rects = [];

    for (var y = 0; y < 8; y++) {
        var byte = font[y * 0x60 + index];
        for (var x = 0; x < 8; x++) {
            if (byte & (0x80 >> x)) {
                pixel_rects.push({
                    left: scale(x, width),
                    top: scale(y, height),
                    right: scale(x + 1, width),
                    bottom: scale(y + 1, height),
                });
            }
        }
    }

    const path = Region2D.fromRects(pixel_rects).getPath();
    var svg = '';

    svg += '<?xml version="1.0" encoding="UTF-8" standalone="no"?>\n';
    svg +=
        '<!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.0//EN" "http://www.w3.org/TR/SVG/DTD/svg10.dtd">\n';
    svg += `<svg width="${width}" height="${height}"><path d="`;

    for (var polygon of path) {
        var command = 'M';
        for (var point of polygon) {
            svg += `${command} ${point.x} ${point.y} `;
            command = 'L';
        }
        svg += 'Z ';
    }

    svg += '" fill="currentColor"/></svg>\n';

    const filename = `${output_prefix}-${('00' + index).slice(-2)}.svg`;
    fs.writeFile(filename, svg, function () {});
}
