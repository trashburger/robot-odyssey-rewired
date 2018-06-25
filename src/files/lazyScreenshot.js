import * as Graphics from '../graphics.js';
import ScreenshotLoadingImage from '../assets/screenshot-loading.gif';

const render_stack = [];

const render_canvas = document.createElement('canvas');
render_canvas.width = Graphics.WIDTH;
render_canvas.height = Graphics.VISIBLE_HEIGHT;

function renderer()
{
    // Limit of one render per frame; keep coming back while the queue has work.
    if (render_stack.length > 1) {
        requestAnimationFrame(renderer);
    }

    const element = render_stack.pop();
    const data = element._screenshot_data;
    const file = data.loadedFile;

    const compressed = file.extension.includes('z');
    const image = data.engine.screenshotSaveFile(file.data, compressed);
    render_canvas.width = image.width;
    render_canvas.height = image.height;
    render_canvas.getContext('2d').putImageData(image, 0, 0);

    element.src = render_canvas.toDataURL();
}

function callback(entries)
{
    for (let entry of entries) {
        const element = entry.target;
        const data = element._screenshot_data;
        if (entry.isIntersecting) {
            // Stop observing after the first intersection, async load the file data
            observer.unobserve(element);
            data.file.load().then((file) => {
                data.loadedFile = file;
                data.engine.then(() => {
                    // File is loaded and engine is ready. Now rate-limit and serialize the actual rendering
                    render_stack.push(element);
                    if (render_stack.length === 1) {
                        requestAnimationFrame(renderer);
                    }
                });
            });
        }
    }
}

export function disconnect()
{
    observer.disconnect();
}

export function add(engine, file, element)
{
    element._screenshot_data = { engine, file };
    element.src = ScreenshotLoadingImage;
    observer.observe(element);
}

const observer = new IntersectionObserver(callback, {
    root: document.getElementById('modal_files'),
    rootMargin: '0px',
    threshold: 0,
});
