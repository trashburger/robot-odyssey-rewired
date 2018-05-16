export function initGraphics(engine)
{
    const width = 320;
    const height = 200;
    const border = 1;

    const margin = 20;
    const aspect = 4/3.0;
    const max_w = width * 4;

    const canvas = document.getElementById('framebuffer');
    const context = canvas.getContext('2d');
    const image = context.createImageData(width, height);

    // Blank canvas, including a 1-pixel black border for consistent edge blending
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
