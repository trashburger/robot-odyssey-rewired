import { WasmEngineInstance } from '../io/wasm.js';
import { EnginePromise } from '../index.js';

var cache: WebAssembly.Module | null = null;

async function load(url: string): Promise<WasmEngineInstance> {
    if (cache) {
        const instance = (await WebAssembly.instantiate(cache)) as unknown as WebAssembly.Instance;
        return new WasmEngineInstance(cache, instance);
    } else {
        const wasm = await WebAssembly.instantiateStreaming(fetch(url));
        cache = wasm.module;
        return new WasmEngineInstance(wasm.module, wasm.instance);
    }
}

export class WebEngineLoader extends EnginePromise<WasmEngineInstance> {
    constructor(url: string) {
        super(load(url));
    }
}
