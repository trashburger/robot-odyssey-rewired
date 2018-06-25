import 'intersection-observer';
import OfflinePluginRuntime from 'offline-plugin/runtime';

// Game frontend modules
import * as Graphics from './graphics.js';
import * as Sound from './sound.js';
import * as Buttons from './input/buttons.js';
import * as Mouse from './input/mouse.js';
import * as Joystick from './input/joystick.js';
import * as AutoSave from './files/autoSave.js';
import * as FileActions from './files/fileActions.js';
import * as Storage from './files/storage.js';
import * as GameMenu from './gameMenu.js';

// Compiled engine loader and WASM code/data bundle, via emscripten
import EngineFactory from '../build/engine.js';
import EngineWasm from '../build/engine.wasm';

import './style/main.css';
import './style/buttons.css';
import './style/joystick.css';
import './style/graphics.css';
import './style/gameMenu.css';
import './style/fileManager.css';
import '../build/font/font.template.css';

// Strong local caching using a Service Worker.
// This starts prefetching all resources.
OfflinePluginRuntime.install();

try {
    const engine = EngineFactory({
        locateFile: function () { return EngineWasm; },
        onAbort: GameMenu.showError,
    });

    window.ROEngine = engine;

    Storage.init(engine);
    Graphics.init(engine);
    Sound.init(engine);
    Buttons.init(engine);
    Mouse.init(engine);
    Joystick.init(engine);
    FileActions.init(engine);
    GameMenu.init(engine);

    // Autosave should be last, it may try to launch a saved game
    // and the other subsystems should be installed by then.
    AutoSave.init(engine);

} catch (e) {
    GameMenu.showError(e);
}
