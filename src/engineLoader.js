// Compiled engine loader and WASM code/data bundle, via emscripten
import EngineFactory from '../build/engine.js';
import EngineWasm from '../build/engine.wasm';

// These exports are not valid until init()
export var instance = null;
export var complete = null;

export function init()
{
    instance = {
        locateFile: () => EngineWasm,
    };
    complete = EngineFactory(instance);
}
