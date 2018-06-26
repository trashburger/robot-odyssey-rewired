import 'intersection-observer';
import OfflinePluginRuntime from 'offline-plugin/runtime';

import * as EngineLoader from './engineLoader.js';
import * as GameMenu from './gameMenu.js';
import * as Storage from './files/storage.js';
import * as Graphics from './graphics.js';
import * as Sound from './sound.js';
import * as Buttons from './input/buttons.js';
import * as Mouse from './input/mouse.js';
import * as Joystick from './input/joystick.js';
import * as FileActions from './files/fileActions.js';
import * as AutoSave from './files/autoSave.js';

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
    // Engine loader must be first
    EngineLoader.init();

    GameMenu.init();
    Storage.init();
    Graphics.init();
    Sound.init();
    Buttons.init();
    Mouse.init();
    Joystick.init();
    FileActions.init();

    // Autosave should be last, it may try to launch a saved game
    // and the other subsystems should be installed by then.
    AutoSave.init();

} catch (e) {
    GameMenu.showError(e);
}

// Publish the engine to the JS console, to make it easy to tinker with! This is
// only for debugging/experimenting, no code in this repository should rely on this.
window.ROEngine = EngineLoader.instance;
