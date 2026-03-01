#include "engine.h"
#include "nostdlib.h"
#include "sbt86.h"
#include "zcontexts.h"
#include <zstd.h>

Engine::Engine() : process(nullptr) {}

bool Engine::exec(const char *program, const char *args) {
    extern SBTProcess GameEXE, LabEXE, ShowEXE, Show2EXE, TutorialEXE;
    static const SBTProcess *procs[] = {&GameEXE, &LabEXE, &ShowEXE, &Show2EXE,
                                        &TutorialEXE};

    // Find the process
    process = nullptr;
    for (unsigned i = 0; i < sizeof procs / sizeof *procs; ++i) {
        if (!strcasecmp(procs[i]->filename, program)) {
            process = procs[i];
            break;
        }
    }
    if (!process) {
        return false;
    }

    // Reset system state
    port61 = 0;
    js.reset();
    fs.reset();

    // Initialize registers
    SBTRegs reg;
    memset(&reg, 0, sizeof reg);
    reg.ds = process->relocSegment;
    reg.cs = process->entryCS;

    uint8_t *end_of_mem = mem.bytes + mem.SIZE;
    uint8_t *data_segment = mem.seg(reg.ds);

    // Clear low memory including the BIOS data area
    memset(mem.bytes, 0, 0x600);

    // Clear memory at and above the app's data segment.
    // Leave lower memory intact (for play.exe)
    memset(data_segment, 0, end_of_mem - data_segment);

    // Decompress nonzero data
    ZSTD_decompressDCtx(ZContexts::get().decompressPlain, data_segment,
                        end_of_mem - data_segment, process->data,
                        process->dataLength);

    /*
     * Program Segment Prefix. Locate it just before the beginning of
     * the EXE, and copy our command line to it.
     */
    reg.es = reg.ds - 0x10;
    uint8_t *psp = mem.seg(reg.es);
    memset(psp, 0, 0x80);
    psp[0x80] = strnlen(args, 0x7f);
    memset(&psp[0x81], 0x0D, 0x7f);
    memcpy(&psp[0x81], args, psp[0x80]);

    // Set up default and next entry
    defaultEntry = entry =
        EntryState{reg, process->getFunction(SBTADDR_ENTRY_FUNC)};

    return true;
}

bool Engine::run(IOBuffer *io) {
    if (!process) {
        return false;
    }

    SBTRuntime rt{
        this,       io,         entry.reg,           mem.allSeg(entry.reg),
        SBTClock{}, SBTStack{}, SBTRuntime::Result{}};

    entry.func(rt);

    switch (rt.result.code) {
    case SBTRuntime::Result::FINISHED:
        entry = defaultEntry;
        break;
    case SBTRuntime::Result::CONTINUE_ONCE:
        entry = EntryState{rt.reg, rt.result.func};
        break;
    case SBTRuntime::Result::CONTINUE_ALWAYS:
        defaultEntry = entry = EntryState{rt.reg, rt.result.func};
        break;
    }

    io->delay(rt.clock.take());
    return true;
}

size_t WASM_EXPORT(Engine_sizeof)() { return sizeof(Engine); }

void WASM_EXPORT(Engine_init)(void *engine) { new (engine) Engine; }

uint8_t *WASM_EXPORT(Engine_memBytes)(Engine *engine) {
    return engine->mem.bytes;
}

size_t WASM_EXPORT(Engine_memSize)() { return sizeof Engine::mem.bytes; }

bool WASM_EXPORT(Engine_exec)(Engine *engine, const char *program,
                              const char *args) {
    return engine->exec(program, args);
}

bool WASM_EXPORT(Engine_run)(Engine *engine, IOBuffer *io) {
    return engine->run(io);
}
