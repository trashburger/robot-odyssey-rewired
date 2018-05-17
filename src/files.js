import downloadjs from 'downloadjs';
import { filenameForSaveData } from './roData.js';

export function initFiles(engine)
{
    // Default handler for files saved by the game
    engine.onSaveFileWrite = function () {
        const saveData = engine.getSaveFile();
        downloadjs(saveData, filenameForSaveData(saveData), 'application/octet-stream');
    };

    // Loader for arbitrary saved files
    document.getElementById('savefile_btn').addEventListener('click', (e) => {
        document.getElementById('savefile_input').click();
    });

    document.getElementById('savefile_input').addEventListener('change', (e) => {
        const files = e.target.files;
        if (files.length == 1 && files[0].size <= engine.MAX_FILESIZE) {
            const reader = new FileReader();
            reader.onload = (e) => engine.loadSaveFile(reader.result);
            reader.readAsArrayBuffer(files[0]);
        }
    });

    engine.loadSaveFile = function (array)
    {
        engine.setSaveFile(new Uint8Array(array));
        engine.loadGame();
    };

    engine.downloadRAMSnapshot = function (filename)
    {
        downloadjs(engine.getMemory(), filename || 'ram-snapshot.bin', 'application/octet-stream');
    };

    engine.downloadCompressionDictionary = function (filename)
    {
        downloadjs(engine.getCompressionDictionary(), filename || 'dictionary.bin', 'application/octet-stream');
    };
}
