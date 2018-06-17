import * as Graphics from "./graphics.js"
import * as Sound from "./sound.js"
import * as Buttons from "./input/buttons.js"
import * as Mouse from "./input/mouse.js"
import * as Joystick from "./input/joystick.js"
import * as AutoSave from "./autoSave.js"
import * as Files from "./files.js"
import * as GameMenu from "./gameMenu.js"

import OfflinePluginRuntime from 'offline-plugin/runtime'
import EngineFactory from "../build/engine.js"
import EngineWasm from "../build/engine.wasm"

import './style/main.css'
import './style/buttons.css'
import './style/joystick.css'
import './style/graphics.css'
import './style/gameMenu.css'
import '../build/font/font.template.css'

// Strong local caching using a Service Worker.
// This starts prefetching all resources.
OfflinePluginRuntime.install();

try {
    const engine = EngineFactory({
        locateFile: function () { return EngineWasm },
        onAbort: GameMenu.showError,
    });

    window.ROEngine = engine;

    Graphics.init(engine);
    Sound.init(engine);
    Buttons.init(engine);
    Mouse.init(engine);
    Joystick.init(engine);
    Files.init(engine);
    GameMenu.init(engine);

    // Autosave should be last, it may try to launch a saved game
    // and the other subsystems should be installed by then.
    AutoSave.init(engine);

} catch (e) {
    GameMenu.showError(e);
}
