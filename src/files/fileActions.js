import downloadjs from 'downloadjs';
import { filenameForSaveData } from '../roData.js';

export function init(engine)
{
    engine.then(function () {
        engineLoaded(engine);
    });
}

function engineLoaded(engine)
{
    // Default handler for files saved by the game
    engine.onSaveFileWrite = function ()
    {
        const saveData = engine.getSaveFile();
        const date = new Date();
        const name = filenameForSaveData(saveData, date);
        engine.files.save(name, saveData, date);
        downloadjs(saveData, name, 'application/octet-stream');
    };

    engine.loadSaveFilePicker = function ()
    {
        let input = document.createElement('input');
        input.type = 'file';
        input.addEventListener('change', () => {
            if (input.files.length == 1 && input.files[0].size <= engine.MAX_FILESIZE) {
                const reader = new FileReader();
                reader.onload = () => engine.loadSaveFile(reader.result);
                reader.readAsArrayBuffer(input.files[0]);
            }
        });
        input.click();
    };

    engine.loadSaveFile = function (array, compressed)
    {
        return engine.setSaveFile(new Uint8Array(array), !!compressed) && engine.loadGame();
    };

    engine.downloadRAMSnapshot = function (filename)
    {
        downloadjs(engine.getMemory(), filename || 'ram-snapshot.bin', 'application/octet-stream');
    };

    engine.downloadCompressionDictionary = function (filename)
    {
        downloadjs(engine.getCompressionDictionary(), filename || 'dictionary.bin', 'application/octet-stream');
    };

    engine.downloadColorTileImage = function(first_slot, num_slots)
    {
        return engine.saveColorTilesToImage(first_slot, num_slots).then(function (blob) {
            downloadjs(blob, 'color-tiles.png', 'image/png');
        });
    };

    engine.downloadArchive = function()
    {
        const date = new Date();
        const datestr = date.toLocaleString().replace(/\//g,'-');
        engine.files.toZip().then((zip) => {
            return zip.generateAsync({
                type: 'blob',
                compression: 'DEFLATE',
                compressionOptions: {level: 9},
            });
        }).then((blob) => {
            downloadjs(blob, `Robot Odyssey Files (${datestr}).zip`);
        });
    };
}
