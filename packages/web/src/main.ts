import 'intersection-observer';
import * as OfflinePluginRuntime from 'offline-plugin/runtime';

import { WebInput } from '@robot-odyssey-rewired/engine/dist/web/input.js';
import { SVGDisplayRenderer } from '@robot-odyssey-rewired/engine/dist/web/display.js';
import { WebAudioRenderer } from '@robot-odyssey-rewired/engine/dist/web/audio.js';
import { WebEngineLoader } from '@robot-odyssey-rewired/engine/dist/web/loader.js';
import { WebEngineLooper } from '@robot-odyssey-rewired/engine/dist/web/looper.js';
import engineWasm from '@robot-odyssey-rewired/engine/dist/wasm';

import '@robot-odyssey-rewired/engine/dist/images/font.css';
import '@robot-odyssey-rewired/engine/dist/web/display.css';

// import * as GameMenu from './gameMenu';
// import * as Storage from './files/storage';
// import * as Graphics from './graphics';
// import * as Sound from './sound';
// import * as Buttons from './input/buttons';
// import * as Mouse from './input/mouse';
// import * as Joystick from './input/joystick';
// import * as FileActions from './files/fileActions';
// import * as AutoSave from './files/autoSave';

import './style/main.css';
import './style/buttons.css';
import './style/joystick.css';
import './style/graphics.css';
import './style/gameMenu.css';
import './style/fileManager.css';

// Strong local caching using a Service Worker.
// This starts prefetching all resources.
OfflinePluginRuntime.install();

const looper = new WebEngineLooper(new WebEngineLoader(engineWasm));

// Publish for interactive use in the console
(window as unknown as any).RO = looper;

const display = new SVGDisplayRenderer(document.getElementById('playfield') as unknown as SVGSVGElement);
looper.display = display;

const audio = new WebAudioRenderer();
looper.audio = audio;

const input = new WebInput();
looper.input = input;
input.audio = audio;
input.attachGamepad();
input.attachKeyboard();
input.attachPointer(display.svg, document.getElementById('game_area'));

looper.engine.exec('lab.exe', '30');
looper.running = true;

// Autosave should be last, it may try to launch a saved game
// and the other subsystems should be installed by then.
// AutoSave.init();
