import { loadingBegin, loadingError, loadingDone } from "./loading.js"
import { filenameForSaveData } from "./roData.js"
import { initGraphics, initGraphicsAfterEngineLoads } from "./graphics.js"
import { initSound } from "./sound.js"
import { initInputEarly, initInputAfterEngineLoads } from "./input.js"
import { initAutoSave } from "./autoSave.js"
import { initFiles } from "./files.js"
import OfflinePluginRuntime from 'offline-plugin/runtime'
import EngineFactory from "../build/engine.js"
import EngineWasm from "../build/engine.wasm"

import './main.css'

// Strong local caching using a Service Worker.
// This starts prefetching all resources.
OfflinePluginRuntime.install();

try {
    const engine = EngineFactory(
    {
        locateFile: function () {
            return EngineWasm;
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
