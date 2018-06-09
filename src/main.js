import * as Graphics from "./graphics.js"
import * as Sound from "./sound.js"
import * as Input from "./input.js"
import * as AutoSave from "./autoSave.js"
import * as Files from "./files.js"
import * as Loading from "./loading.js"
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

        onAbort: Loading.error,

        onProcessExit: function (code) {
            console.log("Process exited, code:", code);
        },
    });

    window.ROEngine = engine;

    Loading.init(engine);
    Graphics.init(engine);
    Input.init(engine);
    Sound.init(engine);

    engine.then(function ()
    {
        try {

            Loading.engineLoaded(engine);
            Files.engineLoaded(engine);
            Graphics.engineLoaded(engine);
            Input.engineLoaded(engine);
            AutoSave.engineLoaded(engine);

            // TO DO: menu in js. For now, test game exec directly.
            engine.exec("game.exe", "");

        } catch (e) {
            Loading.error(e);
        }
    });

} catch (e) {
    Loading.error(e);
}
