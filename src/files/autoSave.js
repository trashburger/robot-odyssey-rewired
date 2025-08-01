import * as GameMenu from '../gameMenu.js';
import * as EngineLoader from '../engineLoader.js';
import { filenameForAutosave } from '../roData.js';
import base64 from 'base64-arraybuffer';

let last_set_window_hash = null;
let autosave_timer = null;

// Autosave takes place if the game has been modified
// and then left alone for this many milliseconds.
// The intent is to capture game state changes that
// the user may forget to save, but we don't want to
// create so many saves that it's just clutter.
const autosave_delay = 10000;

export async function doAutoSave() {
    const engine = await EngineLoader.complete;
    return await new Promise(function (resolve) {
        // this is hacky... autosaves have a different
        // completion action, and they shouldn't clobber
        // the user's save buffer (it's especially annoying
        // when loading chips), so I'm saving and restoring
        // the callback and contents, for now. This could
        // be improved by using a modified filename for the
        // autosave, or otherwise asking the engine to use
        // a different buffer than usual.
        // fix me!

        const saved_contents = engine.getSaveFile().slice();
        const saved_callback = engine.onSaveFileWrite;
        engine.onSaveFileWrite = function () {};

        try {
            const save_status = engine.saveGame();
            const storage_status =
                save_status === engine.SaveStatus.OK
                    ? storeAutoSave(engine)
                    : save_status;
            if (storage_status === engine.SaveStatus.NOT_SUPPORTED) {
                last_set_window_hash = '';
                window.location.hash = '';
            }
            resolve(storage_status);
        } finally {
            engine.onSaveFileWrite = saved_callback;
            engine.setSaveFile(saved_contents, false);
        }
    });
}

async function checkHashForAutoSave() {
    const hash = window.location.hash;
    if (hash && hash[0] === '#') {
        let s = hash.slice(1);
        if (s !== last_set_window_hash) {
            const packed = new Uint8Array(base64.decode(s));

            GameMenu.setState(GameMenu.States.LOADING);
            const engine = await GameMenu.afterLoadingState();
            if (engine.setSaveFile(packed, true) && engine.loadGame()) {
                return true;
            }

            GameMenu.modal('FAILED to load packed saved game');
        }
    }
    return false;
}

export function init() {
    EngineLoader.instance.autoSave = function () {
        if (autosave_timer) {
            clearTimeout(autosave_timer);
        }
        autosave_timer = setTimeout(function () {
            doAutoSave();
        }, autosave_delay);
    };

    window.addEventListener('hashchange', checkHashForAutoSave);
    checkHashForAutoSave();
}

function storeAutoSave(engine) {
    const saveData = engine.getSaveFile();
    const packed = engine.packSaveFile();

    const date = new Date();
    const name = filenameForAutosave(saveData, date);
    if (!name) {
        return engine.SaveStatus.NOT_SUPPORTED;
    }

    engine.files.save(name, packed, date);

    const hash = base64.encode(packed);
    last_set_window_hash = hash;
    window.location.hash = hash;

    return engine.SaveStatus.OK;
}
