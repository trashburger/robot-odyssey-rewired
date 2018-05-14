import engineFactory from "../build/engine.js"
import { loadingBegin, loadingError, loadingDone } from "./loading.js"
import { filenameForSaveData } from "./roData.js"
import { initGraphics } from "./graphics.js"
import { initSound } from "./sound.js"
import { initInputEarly, initInputAfterEngineLoads } from "./input.js"
import { initAutoSave } from "./autoSave.js"
import { initFiles } from "./files.js"

try {
    const engine = engineFactory();
    window.ROEngine = engine;

    initGraphics(engine);
    loadingBegin();
    initInputEarly();
    initSound(engine);

    engine.onAbort = loadingError;
    engine.then(function ()
    {
        initFiles(engine);
        loadingDone();
        initInputAfterEngineLoads(engine);
        initAutoSave(engine);
    });

} catch (e) {
    loadingError(e);
    throw e;
}
