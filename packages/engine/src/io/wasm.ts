import { textDecoder, textEncoder, packInput, consumeInput, unpackOutput } from './iobuffer.js';
import { type Engine, type EngineInput, type EngineOutput, type FileInfoMap } from '../index.js';

type EnginePtr = number;
type IOBufferPtr = number;
type FileInfoPtr = number;
type StringPtr = number;
type BytesPtr = number;

interface EngineExports {
    Engine_exec: (engine: EnginePtr, program: StringPtr, args: StringPtr) => boolean;
    Engine_init: (engine: EnginePtr) => void;
    Engine_memBytes: (engine: EnginePtr) => BytesPtr;
    Engine_memSize: () => number;
    Engine_run: (engine: EnginePtr, io: IOBufferPtr) => boolean;
    Engine_sizeof: () => number;
    FileInfo_data: (file: FileInfoPtr) => BytesPtr;
    FileInfo_getAllFiles: () => FileInfoPtr;
    FileInfo_name: (file: FileInfoPtr) => StringPtr;
    FileInfo_size: (file: FileInfoPtr) => number;
    FileInfo_sizeof: () => number;
    memory: WebAssembly.Memory;
    TinySave_compress: (src: FileInfoPtr, dest: BytesPtr) => number;
    TinySave_decompress: (dest: FileInfoPtr, src: BytesPtr, srcLength: number) => number;
    ZSTD_heapUsed: () => number;
}

interface Allocator {
    (size: number): number;
}

function getString(mem: WebAssembly.Memory, ptr: StringPtr, limit: number = 0x10000): string {
    const array = new Uint8Array(mem.buffer, ptr, limit);
    const length = array.findIndex((c) => c === 0);
    return textDecoder.decode(array.subarray(0, length));
}

function putString(mem: WebAssembly.Memory, ptr: StringPtr, limit: number, str: string) {
    const array = new Uint8Array(mem.buffer, ptr, limit);
    array[Math.min(limit - 1, textEncoder.encodeInto(str, array).written)] = 0;
}

function readFileInfoMap(fn: EngineExports, ptr: FileInfoPtr): FileInfoMap {
    const sizeof = fn.FileInfo_sizeof();
    const result: FileInfoMap = {};
    for (; ; ptr += sizeof) {
        const name = fn.FileInfo_name(ptr);
        if (name === 0) {
            return result;
        }
        result[getString(fn.memory, name)] = new Uint8Array(
            fn.memory.buffer,
            fn.FileInfo_data(ptr),
            fn.FileInfo_size(ptr),
        );
    }
}

function align(n: number): number {
    return (n + 0xf) & ~0xf;
}

function pages(n: number): number {
    return (n + 0xffff) / 0x10000;
}

function BumpAlloc(mem: WebAssembly.Memory): Allocator {
    const buffer = mem.buffer;
    var top = align(buffer.byteLength);

    // Note, allocation always calls grow().  All TypedArrays will detach.
    return function (s: number) {
        const r = top;
        top = align(r + (s > 0 ? s : 1));
        mem.grow(pages(top - buffer.byteLength));
        return r;
    };
}

export class EngineInstance implements Engine {
    readonly fn: EngineExports;
    readonly alloc: Allocator;
    readonly exec: (program: string, args?: string | null) => boolean;
    readonly run: (input?: EngineInput | null) => EngineOutput | null;
    readonly ioBuffer!: Uint8Array;
    readonly mem!: Uint8Array;
    readonly staticFiles!: FileInfoMap;

    constructor(fn: EngineExports, alloc: Allocator) {
        this.fn = fn;
        this.alloc = alloc;

        const engineSize = fn.Engine_sizeof();
        const enginePtr = alloc(engineSize);

        fn.Engine_init(enginePtr);

        const programSize = 0x10;
        const programPtr = alloc(programSize);

        const argsSize = 0x10;
        const argsPtr = alloc(argsSize);

        const ioSize = 256 * 1024;
        const ioPtr = alloc(ioSize);
        Object.defineProperty(this, 'ioBuffer', {
            get: function () {
                return new Uint8Array(this.fn.memory.buffer, ioPtr, ioSize);
            },
        });

        const memPtr = fn.Engine_memBytes(enginePtr);
        const memSize = fn.Engine_memSize();
        Object.defineProperty(this, 'mem', {
            get: function () {
                return new Uint8Array(this.fn.memory.buffer, memPtr, memSize);
            },
        });

        const allFilesPtr = fn.FileInfo_getAllFiles();
        Object.defineProperty(this, 'staticFiles', {
            get: function () {
                return readFileInfoMap(this.fn, allFilesPtr);
            },
        });

        this.exec = function (program: string, args?: string | null) {
            putString(this.fn.memory, programPtr, programSize, program);
            putString(this.fn.memory, argsPtr, argsSize, args || '');
            return !!this.fn.Engine_exec(enginePtr, programPtr, argsPtr);
        };

        this.run = function (input?: EngineInput | null): EngineOutput | null {
            var ioBuffer = this.ioBuffer;
            packInput(ioBuffer, input);
            try {
                if (!this.fn.Engine_run(enginePtr, ioBuffer.byteOffset)) {
                    return null;
                }
            } catch (e) {
                if (e instanceof Error) {
                    consumeInput(ioBuffer, input);
                    return unpackOutput(ioBuffer, e);
                } else {
                    throw e;
                }
            }
            consumeInput(ioBuffer, input);
            return unpackOutput(ioBuffer);
        };
    }
}

export class WasmEngineInstance extends EngineInstance {
    module: WebAssembly.Module;

    constructor(module: WebAssembly.Module, wasm: WebAssembly.Instance) {
        const fn = wasm.exports as unknown as EngineExports;
        super(fn, BumpAlloc(fn.memory));
        this.module = module;
    }
}
