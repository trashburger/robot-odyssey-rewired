import * as Graphics from "./graphics.js"
import * as Sound from "./sound.js"
import * as Input from "./input.js"
import * as AutoSave from "./autoSave.js"
import * as Files from "./files.js"
import * as GameMenu from "./game_menu.js"

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

        onAbort: GameMenu.showError,
        onRenderSound: Sound.onRenderSound,
    });

    window.ROEngine = engine;

    Graphics.init(engine);
    Input.init(engine);
    GameMenu.init(engine);

    engine.then(function ()
    {
        try {

            Files.engineLoaded(engine);
            Graphics.engineLoaded(engine);
            Input.engineLoaded(engine);
            AutoSave.engineLoaded(engine);
            GameMenu.engineLoaded(engine);

        } catch (e) {
            GameMenu.showError(e);
        }
    });

} catch (e) {
    GameMenu.showError(e);
}
