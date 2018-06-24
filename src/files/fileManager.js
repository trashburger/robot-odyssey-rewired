import relativeDate from 'relative-date';
import 'intersection-observer';
import * as GameMenu from '../gameMenu.js';

const modal_files = document.getElementById('modal_files');

let files_close_state = null;

const screenshot_render_queue = [];
let screenshot_render_timer = null;

const filters = {
    game: (file) => ['gsv', 'gsvz'].includes(file.extension),
    lab: (file) => ['lsv', 'lsvz'].includes(file.extension),
    chip: (file) => file.extension == 'csv',
};

const onclick = {
    game: clickedSavedFile,
    lab: clickedSavedFile,
};

function clickedSavedFile(engine, file)
{
    file.load().then((file) => {
        if (engine.setSaveFile(file.data, file.name.includes('z')) && engine.loadGame()) {
            close(GameMenu.States.LOADING);
        }
    });
}

function lazyScreenshotRenderer()
{
    if (!screenshot_render_queue.length) {
        clearInterval(screenshot_render_timer);
        screenshot_render_timer = null;
        return;
    }

    const element = screenshot_render_queue.pop();
    const engine = element.roEngine;
    const file = element.roLoadedFile;
    const ctx = element.getContext('2d');

    ctx.putImageData(engine.screenshotSaveFile(file.data, file.name.includes('z')), 0, 0);
}

// Start loading screenshots only when they're visible
const lazyScreenshotLoader = new IntersectionObserver((entries) => {
    for (let entry of entries) {
        const element = entry.target;
        if (entry.isIntersecting) {
            // Stop observing after the first intersection, async load the file data
            lazyScreenshotLoader.unobserve(entry.target);
            element.roFile.load().then((file) => {
                element.roLoadedFile = file;
                element.roEngine.then(() => {
                    // File is loaded and engine is ready. Now rate-limit and serialize the actual rendering
                    screenshot_render_queue.push(entry.target);
                    if (screenshot_render_timer === null) {
                        screenshot_render_timer = setInterval(lazyScreenshotRenderer, 10);
                    }
                });
            });
        }
    }
}, {
    root: document.getElementById('game_area'),
    rootMargin: '0px',
    threshold: 0,
});

export function open(engine, mode, next_state)
{
    files_close_state = next_state;

    const file_elements = [];
    visitElements(modal_files, mode.split(' '), file_elements);

    // The returned promise indicates whether we successfully found
    // any files (resolving on the first success) or if we made it
    // to the end of the transaction without finding anything.

    return new Promise((resolve, reject) => {
        engine.files.listFiles((file) => {
            // One file returned from the database
            for (let element of file_elements) {
                const mode = element.dataset.filemode;
                if (filters[mode](file)) {
                    // Mapped that file to a grid where it appears
                    resolve(true);
                    element.appendChild(fileView(engine, file, mode));
                }
            }
        }).then(() => resolve(false), reject);
    });
}

export function close(optionalStateOverride)
{
    if (files_close_state !== null && GameMenu.getState() === GameMenu.States.MODAL_FILES) {
        GameMenu.setState(optionalStateOverride || files_close_state);
        files_close_state = null;
    }
}

export function setChipId(chip_id)
{
    // Set the click handler to load the correct chip slot and go back to the game
    onclick.chip = (engine, file) => {
        file.load().then((file) => {
            if (engine.setSaveFile(file.data, file.name.includes('z'))
                && engine.loadChip(chip_id)) {
                close(GameMenu.States.EXEC);
            }
        });
    };

    // Update UI to show the correct chip number
    for (let element of Array.from(modal_files.getElementsByClassName('chip_id'))) {
        element.innerText = (1+chip_id).toString();
    }
}

function visitElements(element, mode, file_elements)
{
    const data = element.dataset || {};

    // Control visibility based on the current mode
    if (data.filemode) {
        if (mode.includes(data.filemode)) {
            element.classList.remove('hidden');
        } else {
            element.classList.add('hidden');
        }
    }

    if (data.filemode && element.classList.contains('files')) {
        // File view: replace with a container we populate asynchronously

        var new_element = element.cloneNode(false);
        lazyScreenshotLoader.disconnect();
        if (mode.includes(data.filemode)) {
            file_elements.push(new_element);
        }
        element.parentNode.replaceChild(new_element, element);

    } else {
        // Other: visit children

        for (let child of Array.from(element.children)) {
            visitElements(child, mode, file_elements);
        }
    }
}

function fileView(engine, file, mode)
{
    const item = document.createElement('div');
    item.classList.add('item');
    item.addEventListener('click', () => onclick[mode](engine, file));

    const thumbnail = document.createElement('canvas');
    thumbnail.classList.add('thumbnail');
    thumbnail.width = 640;
    thumbnail.height = 384;
    thumbnail.roFile = file;
    thumbnail.roEngine = engine;
    item.appendChild(thumbnail);
    lazyScreenshotLoader.observe(thumbnail);

    const details = document.createElement('div');
    details.classList.add('details');
    item.appendChild(details);

    const filename = document.createElement('div');
    filename.classList.add('filename', 'rofont');
    filename.innerText = file.name;
    details.appendChild(filename);

    const date = document.createElement('div');
    date.classList.add('date', 'rofont');
    date.innerText = relativeDate(file.date);
    details.appendChild(date);

    return item;
}
