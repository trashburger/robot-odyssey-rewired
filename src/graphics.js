export function initGraphics(engine)
{
    const fbCanvas = document.getElementById('framebuffer');
    const fbContext = fbCanvas.getContext('2d');
    const fbImage = fbContext.createImageData(320, 200);

    // Blank canvas, including a 1-pixel black border for consistent edge blending
    fbContext.fillStyle = '#000';
    fbContext.fillRect(0, 0, 322, 202);

    // Canvas resize handler
    const resize = () => {
        const aspect = 4/3.0;
        const max_w = 960;
        const w = Math.min(Math.min(max_w, window.innerWidth), aspect * window.innerHeight)|0;
        const h = (w / aspect)|0;
        fbCanvas.style.width = w + 'px';
        fbCanvas.style.height = h + 'px';
    };
    window.addEventListener('resize', resize);
    resize();

    engine.onRenderFrame = (rgbData) => {
        fbImage.data.set(rgbData);
        fbContext.putImageData(fbImage, 1, 1);
    };
}
