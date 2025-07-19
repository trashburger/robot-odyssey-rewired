import * as Graphics from '../graphics.js';
import * as EngineLoader from '../engineLoader.js';
import { isCompressed } from './storage.js';
import ScreenshotLoadingImage from '../assets/screenshot-loading.gif';
import ScreenshotBrokenImage from '../assets/screenshot-broken.png';

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

    const engine = EngineLoader.instance;
    const image = engine.screenshotSaveFile(file.data, isCompressed(file));
    if (image) {
        render_canvas.getContext('2d').putImageData(image, 0, 0);
        element.src = render_canvas.toDataURL();
    } else {
        element.src = ScreenshotBrokenImage;
    }
}

async function thumbnailIsVisible(element)
{
    // Wait for the file contents to load from indexedDb
    const data = element._screenshot_data;
    data.loadedFile = await data.file.load();

    // Wait for the engine to load before queueing frames
    await EngineLoader.complete;
    render_stack.push(element);
    if (render_stack.length === 1) {
        requestAnimationFrame(renderer);
    }
}

function callback(entries)
{
    for (let entry of entries) {
        const element = entry.target;
        if (entry.isIntersecting) {
            // Stop observing after the first intersection, async load the file data
            observer.unobserve(element);
            thumbnailIsVisible(element);
        }
    }
}

export function disconnect()
{
    observer.disconnect();
}

export function add(file, element)
{
    element._screenshot_data = { file };
    element.src = ScreenshotLoadingImage;
    observer.observe(element);
}

const observer = new IntersectionObserver(callback, {
    root: document.getElementById('modal_files'),
    rootMargin: '0px',
    threshold: 0,
});
