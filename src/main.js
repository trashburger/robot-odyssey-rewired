import engineFactory from "../build/engine.js"
import engineWasm from "../build/engine.wasm"

import { loadingBegin, loadingError, loadingDone } from "./loading.js"
import { filenameForSaveData } from "./roData.js"
import { initGraphics, initGraphicsAfterEngineLoads } from "./graphics.js"
import { initSound } from "./sound.js"
import { initInputEarly, initInputAfterEngineLoads } from "./input.js"
import { initAutoSave } from "./autoSave.js"
import { initFiles } from "./files.js"
import './main.css'

try {
    const engine = engineFactory(
    {
        locateFile: function () {
            return engineWasm;
        },

        onAbort: loadingError,
    });

    window.ROEngine = engine;

    initGraphics(engine);
    loadingBegin();
    initInputEarly();
    initSound(engine);

    engine.then(function ()
    {
        initFiles(engine);
        initGraphicsAfterEngineLoads(engine);
        loadingDone();
        initInputAfterEngineLoads(engine);
        initAutoSave(engine);
    });

} catch (e) {
    loadingError(e);
    throw e;
}
