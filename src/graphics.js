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

    cga[0] = 0xff000000;
    cga[1] = 0xffffff55;
    cga[2] = 0xffff55ff;
    cga[3] = 0xffffffff;

    function setSolidColor(slot, rgb) {
        patterns.fill(rgb, slot*PATTERN_SIZE, (slot+1)*PATTERN_SIZE);
    }

    function setCheckerboardColor(slot, rgb1, rgb2) {
        for (let y = 0; y < SCREEN_TILE_SIZE; y++) {
            for (let x = 0; x < SCREEN_TILE_SIZE; x++) {
                patterns[slot*PATTERN_SIZE + y*SCREEN_TILE_SIZE + x] = (x^y)&1 ? rgb1 : rgb2;
            }
        }
    }

    setSolidColor(0x00, 0xff000000);
    setSolidColor(0x01, 0xff29da00);
    setSolidColor(0x02, 0xfffc60c0);
    setSolidColor(0x03, 0xffffffff);
    setSolidColor(0x04, 0xff000000);
    setSolidColor(0x05, 0xff147bfa);
    setSolidColor(0x06, 0xfffdb200);
    setSolidColor(0x07, 0xffffffff);    
    setCheckerboardColor(0x08, 0xfffdb200, 0xff000000);
    setCheckerboardColor(0x09, 0xff147bfa, 0xff000000);
    setCheckerboardColor(0x0a, 0xfffc60c0, 0xff000000);
    setCheckerboardColor(0x0b, 0xff29da00, 0xff000000);
    setCheckerboardColor(0x0c, 0xff147bfa, 0xffffffff);
    setCheckerboardColor(0x0d, 0xfffdb200, 0xffffffff);
    setCheckerboardColor(0x0e, 0xff29da00, 0xffffffff);
    setCheckerboardColor(0x0f, 0xfffc60c0, 0xffffffff);
    setCheckerboardColor(0x10, 0xffdba6b6, 0xff000000);
    setCheckerboardColor(0x11, 0xffd5bf90, 0xff000000);
    setCheckerboardColor(0x12, 0xff8fcbd4, 0xff000000);
    setCheckerboardColor(0x13, 0xffeca3ca, 0xff000000);
    setCheckerboardColor(0x14, 0xffe9b5a2, 0xff000000);
    setSolidColor(0x15, 0xff000000);
}
