import engineFactory from "../build/engine.js"
import { loadingBegin, loadingError, loadingDone } from "./loading.js"
import { filenameForSaveData } from "./roData.js"
import { initGraphics } from "./graphics.js"
import { initSound } from "./sound.js"
import { initInputEarly, initInputAfterEngineLoads } from "./input.js"
import { initAutoSave } from "./autosave.js"
import { initFiles } from "./files.js"

initInputEarly();
loadingBegin();

var engine = null;
try {

    engine = engineFactory();
    window.ROEngine = engine;
    engine.onAbort = loadingError;

    initGraphics(engine);
    initSound(engine);

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
