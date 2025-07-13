import relativeDate from 'relative-date';
import { isCompressed } from './storage.js';
import * as GameMenu from '../gameMenu.js';
import * as LazyScreenshot from './lazyScreenshot.js';
import * as EngineLoader from '../engineLoader.js';

const modal_files = document.getElementById('modal_files');
const speed_selector = document.getElementById('speed_selector');
let modal_saved = null;

const filters = {
    game: (file) => ['gsv', 'gsvz'].includes(file.extension),
    lab: (file) => ['lsv', 'lsvz'].includes(file.extension),
    chip: (file) => file.extension === 'csv',
};

const onclick = {
    game: (file) => clickedFile(file, (engine) => engine.loadGame()),
    lab: (file) => clickedFile(file, (engine) => engine.loadGame()),
};

async function clickedFile(file, loader)
{
    const engine = await EngineLoader.complete;
    const loadedFile = await file.load();
    if (!engine.setSaveFile(loadedFile.data, isCompressed(loadedFile))) {
        return;
    }
    if (!loader(engine)) {
        return;
    }
    close(GameMenu.States.LOADING);
}

export async function open(mode, next_state)
{
    const file_elements = [];
    const engine = EngineLoader.instance;
    visitElements(modal_files, mode.split(' '), file_elements);

    const result = await new Promise((resolve, reject) => {
        // The returned promise indicates whether we successfully found
        // any files (resolving on the first success) or if we made it
        // to the end of the transaction without finding anything.

        engine.files.listFiles((file) => {
            // One file returned from the database
            for (let element of file_elements) {
                const mode = element.dataset.filemode;
                if (filters[mode](file)) {
                    // Mapped that file to a grid where it appears
                    resolve(true);
                    element.appendChild(fileView(file, mode));
                }
            }
        }).then(() => resolve(false), reject);
    });

    if (result) {
        // Activate the file selector modal
        modal_saved = {
            state: next_state || GameMenu.getState(),
        };

        // Pause
        speed_selector.value = '0';
        speed_selector.dispatchEvent(new Event('change'));

        GameMenu.setState(GameMenu.States.MODAL_FILES);
    }

    return result;
}

export function close(optionalStateOverride)
{
    const saved = modal_saved;
    if (saved !== null && GameMenu.getState() === GameMenu.States.MODAL_FILES) {
        modal_saved = null;

        GameMenu.setState(optionalStateOverride || saved.state);
        speed_selector.value = '1';
        speed_selector.dispatchEvent(new Event('change'));
        return true;
    }
    return false;
}

export function setChipId(chip_id)
{
    // Set the click handler to load the correct chip slot and go back to the game
    onclick.chip = (file) => clickedFile(file, (engine) => engine.loadChip(chip_id));

    // Update UI to show the correct chip number
    for (let element of modal_files.getElementsByClassName('chip_id')) {
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
        LazyScreenshot.disconnect();
        if (mode.includes(data.filemode)) {
            file_elements.push(new_element);
        }
        element.parentNode.replaceChild(new_element, element);

    } else {
        // Other: visit children

        for (let child of element.children) {
            visitElements(child, mode, file_elements);
        }
    }
}

function fileView(file, mode)
{
    const item = document.createElement('div');
    item.classList.add('item');
    item.addEventListener('click', () => onclick[mode](file));

    const thumbnail = document.createElement('img');
    thumbnail.classList.add('thumbnail');
    item.appendChild(thumbnail);
    LazyScreenshot.add(file, thumbnail);

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
