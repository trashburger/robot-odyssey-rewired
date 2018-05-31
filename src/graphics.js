export function initGraphics(engine)
{
    const width = 640;
    const height = 400;
    const border = 2;

    const margin = 20;
    const aspect = 4/3.0;
    const max_w = width * 4;

    const canvas = document.getElementById('framebuffer');
    const context = canvas.getContext('2d');
    const image = context.createImageData(width, height);

    // Blank canvas, including a black border for consistent edge blending
    context.fillStyle = '#000';
    context.fillRect(0, 0, width + 2*border, height + 2*border);

    // Canvas resize handler
    const resize = () => {
        const w_box = document.documentElement.clientWidth - margin;
        const h_box = window.innerHeight - margin;
        const w = Math.min(Math.min(max_w, w_box), aspect * h_box)|0;
        const h = (w / aspect)|0;
        canvas.style.width = w + 'px';
        canvas.style.height = h + 'px';
    };
    window.addEventListener('resize', resize);
    resize();

    engine.onRenderFrame = (rgb) => {
        image.data.set(rgb);
        context.putImageData(image, border, border);
    };
}

export function initGraphicsAfterEngineLoads(engine)
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

    engine.setColorTilesFromImage = function(first_slot, img_src)
    {
        const img = document.createElement('img');
        img.src = img_src;

        return img.decode().then(function() {

            const canvas = document.createElement('canvas');
            canvas.width = img.width;
            canvas.height = img.height;

            const ctx = canvas.getContext("2d");
            ctx.drawImage(img, 0, 0);
            const idata = ctx.getImageData(0, 0, img.width, img.height);
            const words = new Uint32Array(idata.data.buffer);

            patterns.subarray(first_slot * PATTERN_SIZE).set(words);
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
        engine.setSolidColor(0x04, hgr[0]);
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
        engine.setSolidColor(0x04, cga[0]);
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
    }

    engine.setHGRColors();
}
