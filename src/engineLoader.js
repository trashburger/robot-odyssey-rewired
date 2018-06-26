// Compiled engine loader and WASM code/data bundle, via emscripten
import EngineFactory from '../build/engine.js';
import EngineWasm from '../build/engine.wasm';

// These exports are not valid until init()
export var instance = null;
export var complete = null;

export function init()
{
    complete = new Promise((resolve, reject) => {

        // The engine is NOT a promise, but if the real promise implementation
        // thinks we're returning a promise it can cause an infinite loop.
        //
        // We don't want anyone using the fake then() implementation from emscripten,
        // so destroy that here and prevent this resolve() from interpreting the
        // instance value as a promise.

        const done = () => {
            delete instance.then;
            resolve(instance);
        };

        const options = {
            locateFile: () => EngineWasm,
            onAbort: reject,
            onRuntimeInitialized: done,
        };

        try {
            instance = EngineFactory(options);
        } catch (e) {
            reject(e);
        }
    });
}
