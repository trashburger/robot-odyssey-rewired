import downloadjs from 'downloadjs';
import { getFilesFromDragEvent } from 'html-dir-content';
import { filenameForSaveData } from '../roData.js';
import * as GameMenu from '../gameMenu.js';
import * as FileManager from './fileManager.js';
import * as EngineLoader from '../engineLoader.js';

const FILE_TYPES =
    ' ^ GSV - Game Save\n'+
    ' ^ LSV - Lab Save\n'+
    ' ^ CSV - Chip Save\n\n'+
    ' ^ GSVZ, LSVZ - Compressed Game/Lab Save\n\n'+
    ' ^ ZIP files with any combination of the above\n';

export function init()
{
    const engine = EngineLoader.instance;

    var drag_timer = null;

    document.body.addEventListener('dragover', (e) => {
        e.preventDefault();

        const transfer = e.dataTransfer;
        if (transfer) {
            transfer.dropEffect = 'copy';
            transfer.effectAllowed = 'copy';
        }

        if (transfer && GameMenu.getState() !== GameMenu.States.MODAL_TEXTBOX) {
            GameMenu.modal('Drop files anywhere:\n\n' + FILE_TYPES);
        }

        // Drag may enter/leave many elements as the modal animates,
        // use a timeout to delay the end of this modal by a fraction of a second
        if (drag_timer) {
            clearTimeout(drag_timer);
            drag_timer = setTimeout(() => {
                drag_timer = null;
                GameMenu.exitModal();
            }, 500);
        }
    });

    document.body.addEventListener('drop', async (e) => {
        e.preventDefault();
        if (drag_timer) {
            clearTimeout(drag_timer);
            drag_timer = null;
        }

        const transfer = e.dataTransfer;
        if (!transfer) {
            GameMenu.exitModal();
            return;
        }

        GameMenu.modal('Processing dropped files...');

        const files = await getFilesFromDragEvent(e, { recursive: true });
        const results = await engine.files.saveFiles(files);
        await filesStoredConfirmation(results);
    });

    // Default handler for files saved by the game
    engine.onSaveFileWrite = async () => {
        const saveData = engine.getSaveFile();
        const date = new Date();
        const name = filenameForSaveData(saveData, date);

        const result = await engine.files.save(name, saveData, date);
        if (result) {
            await filesStoredConfirmation([result]);
        } else {
            // Fallback, if we can't put it in storage download it right away
            downloadjs(saveData, name, 'application/octet-stream');
        }
    };

    // When a chip is loaded in-game, we get to pause here and show a file picker
    engine.onLoadChipRequest = (chip_id) => {
        FileManager.setChipId(chip_id);
        FileManager.open('chip');
    };

    // Request a picker for uploading native files
    engine.filePicker = function ()
    {
        return new Promise((resolve) => {
            const input = document.createElement('input');
            input.type = 'file';
            input.multiple = true;
            input.style.display = 'none';
            document.body.appendChild(input);
            input.addEventListener('change', () => {
                if (input.files.length) {
                    GameMenu.modal('Processing selected files...');
                    engine.files.saveFiles(input.files).then((results) => {
                        filesStoredConfirmation(results);
                        resolve(results);
                    });
                }
                input.remove();
            });
            input.click();
        });
    };

    engine.downloadRAMSnapshot = function (filename)
    {
        downloadjs(engine.getMemory(), filename || 'ram-snapshot.bin', 'application/octet-stream');
        return true;
    };

    engine.downloadCompressionDictionary = function (filename)
    {
        downloadjs(engine.getCompressionDictionary(), filename || 'dictionary.bin', 'application/octet-stream');
        return true;
    };

    engine.downloadColorTileImage = async function(first_slot, num_slots)
    {
        const engine = await EngineLoader.complete;
        const blob = await engine.saveColorTilesToImage(first_slot, num_slots);
        downloadjs(blob, 'color-tiles.png', 'image/png');
        return true;
    };

    engine.downloadArchive = async function()
    {
        const date = new Date();
        const datestr = date.toLocaleString().replace(/\//g,'-');

        const promise = engine.files.createZipBlob();
        GameMenu.modalDuringPromise(promise, 'Preparing ZIP file...');

        const blob = await promise;
        downloadjs(blob, `Robot Odyssey Files (${datestr}).zip`);
        return true;
    };
}

async function filesStoredConfirmation(result, onclick)
{
    const files = [];
    const flatten = (l) => {
        for (let i of l) {
            if (Array.isArray(i)) {
                flatten(i);
            } else if (i) {
                files.push(` ^ ${i.name}`);
            }
        }
    };
    flatten(result);

    const msg = files.length ?
        'Ok!\n\nThese files have been stored in your browser, on this device:\n\n' + files.join('\n')
        : 'Didn\'t find any supported files!\n\n' + FILE_TYPES;

    FileManager.close();
    await GameMenu.modal(msg, onclick);
}
