import * as Graphics from '../graphics.js';
import ScreenshotLoadingImage from '../assets/screenshot-loading.png';

const screenshot_render_queue = [];
let screenshot_render_timer = null;

const render_canvas = document.createElement('canvas');
render_canvas.width = Graphics.WIDTH;
render_canvas.height = Graphics.VISIBLE_HEIGHT;

function renderer()
{
    if (!screenshot_render_queue.length) {
        clearInterval(screenshot_render_timer);
        screenshot_render_timer = null;
        return;
    }

    const element = screenshot_render_queue.pop();
    const data = element._screenshot_data;
    const file = data.loadedFile;

    const image = data.engine.screenshotSaveFile(file.data, file.name.includes('z'));
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
                    screenshot_render_queue.push(element);
                    if (screenshot_render_timer === null) {
                        screenshot_render_timer = setInterval(renderer, 10);
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
    root: document.getElementById('game_area'),
    rootMargin: '0px',
    threshold: 0,
});
