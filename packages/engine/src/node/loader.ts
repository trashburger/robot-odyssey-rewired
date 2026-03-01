import * as fs from 'node:fs/promises';
import { WasmEngineInstance } from '../io/wasm.js';
import { EnginePromise } from '../index.js';

var cache: WebAssembly.Module | null = null;

async function load(): Promise<WasmEngineInstance> {
    if (cache) {
        const instance = (await WebAssembly.instantiate(cache)) as unknown as WebAssembly.Instance;
        return new WasmEngineInstance(cache, instance);
    } else {
        const path = require.resolve('../wasm');
        const wasm = await WebAssembly.instantiate((await fs.readFile(path)) as BufferSource);
        cache = wasm.module;
        return new WasmEngineInstance(wasm.module, wasm.instance);
    }
}

export class NodeEngineLoader extends EnginePromise<WasmEngineInstance> {
    constructor() {
        super(load());
    }
}
