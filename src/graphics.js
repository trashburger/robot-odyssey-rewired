import './graphics.css'

export function init(engine)
{
    const width = 640;
    const height = 400;
    const visible_height = 384;
    const border = 2;

    const canvas = document.getElementById('framebuffer');
    canvas.width = width + border*2;
    canvas.height = visible_height + border*2;

    const context = canvas.getContext('2d');
    const image = context.createImageData(width, height);

    engine.onRenderFrame = (rgb) => {
        image.data.set(rgb);
        context.putImageData(image, border, border);
    };
}

export function engineLoaded(engine)
{
    const colorMem = engine.getColorMemory();
    const cga = colorMem.cga;
    const patterns = colorMem.patterns;

    const SCREEN_TILE_SIZE = engine.SCREEN_TILE_SIZE;
    const PATTERN_SIZE = SCREEN_TILE_SIZE * SCREEN_TILE_SIZE;

    engine.setEmulatedCGAPalette = function(colors)
    {
        cga.set(colors);
    }

    engine.setSolidColor = function(slot, rgb)
    {
        patterns.fill(rgb, slot*PATTERN_SIZE, (slot+1)*PATTERN_SIZE);
    }

    engine.setColorTilesFromImage = function(img_src, first_slot)
    {
        return new Promise(function (ok) {
            first_slot = first_slot || 0;
            const img = document.createElement('img');

            img.onload = function() {
                const canvas = document.createElement('canvas');
                canvas.width = img.width;
                canvas.height = img.height;

                const ctx = canvas.getContext("2d");
                ctx.drawImage(img, 0, 0);
                const idata = ctx.getImageData(0, 0, img.width, img.height);
                const words = new Uint32Array(idata.data.buffer);

                patterns.subarray(first_slot * PATTERN_SIZE).set(words);

                ok();
            };

            img.crossOrigin = "Anonymous";
            img.src = img_src;
        });
    }

    engine.saveColorTilesToImage = function(first_slot, num_slots)
    {
        const total_patterns = (patterns.length / PATTERN_SIZE) | 0;
        first_slot = first_slot || 0;
        num_slots = num_slots || (total_patterns - first_slot);
        const src_words = patterns.subarray(first_slot * PATTERN_SIZE, (first_slot + num_slots) * PATTERN_SIZE);

        const canvas = document.createElement('canvas');
        canvas.width = SCREEN_TILE_SIZE;
        canvas.height = num_slots * SCREEN_TILE_SIZE;

        const ctx = canvas.getContext("2d");
        const idata = ctx.createImageData(canvas.width, canvas.height);
        const dest_words = new Uint32Array(idata.data.buffer);
        dest_words.set(src_words);
        ctx.putImageData(idata, 0, 0);

        return new Promise(function (resolve, reject) {
            canvas.toBlob( function (blob) {
                resolve(blob);
            });
        });
    }

    engine.setCheckerboardColor = function(slot, rgb1, rgb2, size)
    {
        size = size || 2;
        for (let y = 0; y < SCREEN_TILE_SIZE; y++) {
            for (let x = 0; x < SCREEN_TILE_SIZE; x++) {
                patterns[slot*PATTERN_SIZE + y*SCREEN_TILE_SIZE + x] = (x^y)&size ? rgb2 : rgb1;
            }
        }
    }

    engine.setStripedColor = function(slot, rgb1, rgb2, size)
    {
        size = size || 2;
        for (let y = 0; y < SCREEN_TILE_SIZE; y++) {
            for (let x = 0; x < SCREEN_TILE_SIZE; x++) {
                patterns[slot*PATTERN_SIZE + y*SCREEN_TILE_SIZE + x] = x&size ? rgb2 : rgb1;
            }
        }
    }

    engine.setHGRColors = function(color_table)
    {
        // Original colors available in HGR mode
        const hgr = color_table || [
            0xff000000,  // Black (0)
            0xfffe3bb9,  // Purple (1)
            0xff00ca40,  // Green (2)
            0xffd8a909,  // Blue (3)
            0xff317aed,  // Orange (4)
            0xffffffff,  // White (5)
        ];

        // Chosen colors for the CGA emulation in cutscenes;
        // compromise between the 4-color look and the HGR palette.
        engine.setEmulatedCGAPalette([
            hgr[0],      // Black (0)
            hgr[3],      // Cyan -> Blue (3)
            hgr[4],      // Magenta -> Orange (4)
            hgr[5],      // White (3)
        ]);

        // Sprite colors for the main game
        engine.setSolidColor(0x00, hgr[0]);
        engine.setSolidColor(0x01, hgr[2]);
        engine.setSolidColor(0x02, hgr[1]);
        engine.setSolidColor(0x03, hgr[5]);
        engine.setSolidColor(0x04, 0);      // Transparent black
        engine.setSolidColor(0x05, hgr[4]);
        engine.setSolidColor(0x06, hgr[3]);
        engine.setSolidColor(0x07, hgr[5]);

        // Playfield color patterns
        engine.setCheckerboardColor(0x08, hgr[3], hgr[0]);
        engine.setCheckerboardColor(0x09, hgr[4], hgr[0]);
        engine.setCheckerboardColor(0x0a, hgr[1], hgr[0]);
        engine.setCheckerboardColor(0x0b, hgr[2], hgr[0]);
        engine.setCheckerboardColor(0x0c, hgr[4], hgr[5]);
        engine.setCheckerboardColor(0x0d, hgr[3], hgr[5]);
        engine.setCheckerboardColor(0x0e, hgr[2], hgr[5]);
        engine.setCheckerboardColor(0x0f, hgr[1], hgr[5]);

        // These are basically white on black, but with different
        // color fringing. Implement these here as desaturated colors
        // mixed with black on a checkerboard, for now.
        engine.setCheckerboardColor(0x10, 0xffd4a2e4, hgr[0]);
        engine.setCheckerboardColor(0x11, 0xffb1daa2, hgr[0]);
        engine.setCheckerboardColor(0x12, 0xff8fcbd3, hgr[0]);
        engine.setCheckerboardColor(0x13, 0xfffbc3ba, hgr[0]);
        engine.setCheckerboardColor(0x14, 0xfffbc3ba, hgr[0]);

        // Unused / blank
        engine.setSolidColor(0x15, hgr[0]);

        // If you index past the end of the game's color tables,
        // various stripey patterns result based on other values
        // in memory being interpreted as byte-wide color masks.
        // We don't reproduce these patterns accurately, but to
        // reproduce the general effect initialize any otherwise
        // unused slots with appropriately colored pseudorandom bars.
        // Any pattern slots we have a specific use for can be overwritten later.

        for (let slot = 0x16; slot < 0x100; slot++) {

            if (slot == 0x87) {
                // The game actually uses this slot!

                // When you get hit by an enemy in the "frogger" sections of the Skyway level,
                // your player flashes to an alternate color. They intentionally index past the
                // end of the game's sprite palette (normally 16 entries) with entry 0x87,
                // and on the Apple II version this produces a pattern with two thin lines of
                // interference color. If the pixels are numbered left to right, an orange band
                // appears between the 2nd and 3rd, a pink band between the 5th and 6th.
                for (let x = 0; x < SCREEN_TILE_SIZE; x++) {
                    const x16 = (x / (SCREEN_TILE_SIZE / 16))|0;
                    var rgb = 0xff000000;
                    if (x16 >= 3 && x16 <= 4) rgb = 0xff4ba7bf;
                    if (x16 >= 9 && x16 <= 10) rgb = 0xffd481d5;
                    for (let y = 0; y < SCREEN_TILE_SIZE; y++) {
                        patterns[slot*PATTERN_SIZE + y*SCREEN_TILE_SIZE + x] = rgb;
                    }
                }

            } else {
                // Junk binary stripes

                var stripe = (slot % 35) + 1;
                engine.setStripedColor(slot, hgr[(stripe % 6)|0], hgr[(stripe / 6)|0], slot >> 1);
            }
        }
    }

    engine.setCGAColors = function(color_table)
    {
        // Original CGA colors
        const cga = color_table || [
            0xff000000,  // Black (0)
            0xffffff55,  // Cyan (1)
            0xffff55ff,  // Magenta (2)
            0xffffffff,  // White (3)
        ];

        // Emulated colors for CGA menus/cutscenes
        engine.setEmulatedCGAPalette(cga);

        // Sprite colors for the main game
        engine.setSolidColor(0x00, cga[0]);
        engine.setStripedColor(0x01, cga[1], cga[0]);
        engine.setStripedColor(0x02, cga[1], cga[2]);
        engine.setStripedColor(0x03, cga[3], cga[3]);
        engine.setSolidColor(0x04, 0);      // Transparent black
        engine.setStripedColor(0x05, cga[2], cga[0]);
        engine.setStripedColor(0x06, cga[1], cga[1]);
        engine.setStripedColor(0x07, cga[3], cga[3]);

        // Playfield colors
        engine.setStripedColor(0x08, cga[1], cga[3]);
        engine.setStripedColor(0x09, cga[2], cga[2]);
        engine.setStripedColor(0x0a, cga[0], cga[2]);
        engine.setStripedColor(0x0b, cga[1], cga[0]);

        engine.setStripedColor(0x0c, cga[3], cga[1]);
        engine.setStripedColor(0x0d, cga[2], cga[3]);
        engine.setStripedColor(0x0e, cga[3], cga[1]);
        engine.setStripedColor(0x0f, cga[2], cga[1]);

        engine.setStripedColor(0x10, cga[3], cga[0]);
        engine.setStripedColor(0x11, cga[3], cga[0]);
        engine.setStripedColor(0x12, cga[3], cga[0]);
        engine.setStripedColor(0x13, cga[3], cga[0]);
        engine.setStripedColor(0x14, cga[3], cga[0]);

        // Unused / blank
        engine.setStripedColor(0x15, cga[0], cga[0]);

        // If you index past the end of the game's color tables,
        // various stripey patterns result based on other values
        // in memory being interpreted as byte-wide color masks.
        // We don't reproduce these patterns accurately, but to
        // reproduce the general effect initialize any otherwise
        // unused slots with appropriately colored pseudorandom bars.
        // Any pattern slots we have a specific use for can be overwritten later.

        for (let slot = 0x16; slot < 0x100; slot++) {

            if (slot == 0x87) {
                // The game actually uses this slot!

                // When you get hit by an enemy in the "frogger" sections of the Skyway level,
                // your player flashes to an alternate color. They intentionally index past the
                // end of the game's sprite palette (normally 16 entries) with entry 0x87,
                // which on the DOS version of the game produces a byte-aligned double-wide cyan
                // and black striped pattern. The actual data represented by this color would have
                // been from the lookup table for scanline addresses in the sprite renderer.
                // This seems to have perhaps been a porting accident, since the actual color 0x87
                // matches the one used on Apple II but produces different and arbitrary results.

                engine.setStripedColor(slot, cga[1], cga[0], 4);

            } else {
                // Junk binary stripes

                var stripe = (slot % 15) + 1;
                engine.setStripedColor(slot, cga[stripe & 3], cga[stripe >> 2], slot >> 1);
            }
        }
    }

    // Built-in default palette, until the tileset (if any) loads.
    engine.setHGRColors();

    // Try to asynchronously load a partial custom tileset.
    // Any missing tiles will still refer to the HGR version.
    engine.setColorTilesFromImage(require("./rewired-tileset.png"));
}
