#include <string.h>
#include <stdio.h>
#include "sbt86.h"
#include "hardware.h"

static const bool full_stack_trace = false;
static const uint32_t total_calls_threshold = 100000;


static void decompressRLE(uint8_t *dest, uint8_t *limit, const uint8_t *src, uint32_t srcLength)
{
    /*
     * Decompress our very simple RLE format. Runs of 2 or more zeroes
     * are replaced with 2 zeroes plus a 16-bit count of the omitted
     * zeroes. We assume the output buffer has already been
     * zero-filled, so the zero runs are simply skipped.
     */

    int zeroes = 0;

    while (srcLength) {
        uint8_t byte = *(src++);
        assert(dest < limit && "Overflow in decompressRLE");
        *(dest++) = byte;
        srcLength--;

        if (byte) {
            zeroes = 0;
        } else {
            zeroes++;

            if (zeroes == 2) {
                zeroes = 0;
                dest += src[0] + (src[1] << 8);
                src += 2;
                srcLength -= 2;
            }
        }
    }
}

void SBTProcess::exec(const char *cmdLine)
{
    // Initialize registers
    memset(&reg, 0, sizeof reg);

    // Beginning of EXE image, after relocation
    reg.ds = getRelocSegment();
    reg.cs = getEntryCS();
    continue_func = getFunction(SBTADDR_ENTRY_FUNC);

    uint8_t *end_of_mem = hardware->mem + Hardware::MEM_SIZE;
    uint8_t *data_segment = memSeg(reg.ds);

    // Clear low memory including the BIOS data area
    memset(hardware->mem, 0, 0x600);

    // Clear memory at and above the app's data segment.
    // Leave lower memory intact (for play.exe)
    memset(data_segment, 0, end_of_mem - data_segment);

    // Decompress nonzero data
    decompressRLE(data_segment, end_of_mem, getData(), getDataLen());

    /*
     * Program Segment Prefix. Locate it just before the beginning of
     * the EXE, and copy our command line to it.
     */
    reg.es = reg.ds - 0x10;
    uint8_t *psp = memSeg(reg.es);
    memset(psp, 0, 0x80);
    psp[0x80] = strlen(cmdLine);
    memset(&psp[0x81], 0x0D, 0x7f);
    strncpy((char*) &psp[0x81], cmdLine, 0x7e);

    // Capture current state for future re-entry
    default_func = continue_func;
    default_reg = reg;
}

void SBTProcess::run(void)
{
    assert(hardware != NULL && "Hardware environment must be defined before running a process");

    SBTStack stack;
    loadEnvironment(&stack, reg);

    if (!setjmp(jmp_yield)) {
        continue_func();

        // Continuation function returned; back to the default func.
        continue_func = default_func;
        reg = default_reg;
    }
}

bool SBTProcess::isWaitingInMainLoop()
{
    return continue_func == default_func && continue_func != getFunction(SBTADDR_ENTRY_FUNC);
}

bool SBTProcess::hasFunction(SBTAddressId id)
{
    return getFunction(id) != 0;
}

void SBTProcess::call(SBTAddressId id, SBTRegs call_regs)
{
    assert(hardware != NULL && "Hardware environment must be defined before running a process");

    continue_func_t fn = getFunction(id);
    assert(fn);

    SBTStack stack;
    loadEnvironment(&stack, call_regs);

    if (!setjmp(jmp_yield)) {
        fn();
    }
}

static void continue_after_exit()
{
    assert(0 && "Continuing to run an exited SBTProcess");
}

void SBTProcess::exit()
{
    continueFrom(reg, continue_after_exit);
}

void SBTProcess::continueFrom(SBTRegs regs, continue_func_t fn, bool default_entry)
{
    // Does not return

    assert(fn != 0);
    continue_func = fn;
    reg = regs;
    if (default_entry) {
        default_func = fn;
        default_reg = regs;
    }
    longjmp(jmp_yield, 1);
}

void SBTProcess::failedDynamicBranch(uint16_t cs, uint16_t ip, uint32_t value)
{
    fprintf(stderr, "SBT86, failed dynamic branch at %04x:%04x, to %x\n", cs, ip, value);
    assert(0 && "Failed dynamic branch");
}

uint8_t *SBTProcess::memSeg(uint16_t seg)
{
    /*
     * The highest normal segment we can support.  Any segments over
     * MAX_SEGMENT get remapped to MAX_SEGMENT. This serves two
     * purposes:
     *
     *   1. Out of range memory accesses are dumped somewhere safe.
     *   2. It puts the CGA framebuffer in range of the emulated memory.
     *
     * Padding is set to 0x20000 to give us room for two full 16-bit additions,
     * allowing 16-bit offsets to buffers with 16-bit sizes without further checks.
     */

    static const uint32_t SEGMENT_PADDING = 0x20000;
    static const uint32_t MAX_SEGMENT = (Hardware::MEM_SIZE - SEGMENT_PADDING) >> 4;

    if (seg > MAX_SEGMENT) {
        seg = MAX_SEGMENT;
    }
    return hardware->mem + (((uint32_t)seg) << 4);
}

uint8_t SBTProcess::peek8(uint16_t seg, uint16_t off)
{
    return memSeg(seg)[off];
}

void SBTProcess::poke8(uint16_t seg, uint16_t off, uint8_t value)
{
    memSeg(seg)[off] = value;
}

uint16_t SBTProcess::peek16(uint16_t seg, uint16_t off)
{
    return read16(memSeg(seg) + off);
}

void SBTProcess::poke16(uint16_t seg, uint16_t off, uint16_t value)
{
    write16(memSeg(seg) + off, value);
}

SBTStack::SBTStack()
{
    reset();
}

void SBTStack::reset()
{
    top = 0;
    total_calls_made = 0;
}

void SBTStack::trace()
{
    fprintf(stderr, "--- Stack trace:\n");
    for (unsigned addr = 0; addr < top; addr++) {
        fprintf(stderr, "[%d] ", addr);
        switch (tags[addr]) {
            case STACK_TAG_INVALID:
                fprintf(stderr, "INVALID\n");
                break;
            case STACK_TAG_WORD:
                fprintf(stderr, "word %04x\n", words[addr]);
                break;
            case STACK_TAG_FLAGS:
                fprintf(stderr, "flags u=%08x s=%08x\n", flags[addr].uresult, flags[addr].sresult);
                break;
            case STACK_TAG_RETADDR:
                fprintf(stderr, "ret fn=%04x\n", fn_addrs[addr]);
                break;
            default:
                fprintf(stderr, "BAD TAG %x\n", tags[addr]);
                break;
        }
    }
    fprintf(stderr, "---\n");
}

void SBTStack::pushw(uint16_t word)
{
    assert(top < STACK_SIZE && "SBT86 stack overflow");
    words[top] = word;
    tags[top] = STACK_TAG_WORD;
    top++;
}

void SBTStack::pushf(SBTRegs reg)
{
    assert(top < STACK_SIZE && "SBT86 stack overflow");
    flags[top].uresult = reg.uresult;
    flags[top].sresult = reg.sresult;
    tags[top] = STACK_TAG_FLAGS;
    top++;
}

void SBTStack::pushret(uint16_t fn)
{
    if (full_stack_trace) {
        fprintf(stderr, "+%04x\n", fn);
    }

    total_calls_made++;
    if (total_calls_made > total_calls_threshold) {
        fprintf(stderr, "SBT86, over %d calls since entry, infinite loop?\n", total_calls_threshold);
        trace();
        assert(0 && "loop detected");
    }

    assert(top < STACK_SIZE && "SBT86 stack overflow");
    fn_addrs[top] = fn;
    tags[top] = STACK_TAG_RETADDR;
    top++;
}

uint16_t SBTStack::popw()
{
    top--;
    assert(tags[top] == STACK_TAG_WORD && "SBT86 stack tag mismatch");
    return words[top];
}

SBTRegs SBTStack::popf(SBTRegs reg)
{
    top--;
    assert(tags[top] == STACK_TAG_FLAGS && "SBT86 stack tag mismatch");
    reg.uresult = flags[top].uresult;
    reg.sresult = flags[top].sresult;
    return reg;
}

void SBTStack::popret(uint16_t fn)
{
    top--;
    assert(tags[top] == STACK_TAG_RETADDR && "SBT86 stack tag mismatch");
    if (full_stack_trace) {
        fprintf(stderr, "-%04x\n", fn);
        if (fn_addrs[top] != fn) {
            fprintf(stderr, "STACK MISMATCH, expected 0x%04x\n", fn_addrs[top]);
        }
    }
}

/*
 * These are special stack accessors for use in hook routines.
 * Some routines in Robot Odyssey save the return value off
 * the stack, manipulate the caller's stack, then restore
 * the return value.
 *
 * preSaveRet() should be called before the return value is
 * saved at the beginning of such a function. It converts the
 * top of the stack from a RETADDR to a WORD, and stores a
 * verification value in that word.
 *
 * postRestoreRet() should be called after the return value is
 * restored. It verifies the value saved by preSaveRet, and
 * converts the top of stack back to a RETADDR.
 */

void SBTStack::preSaveRet()
{
    assert(tags[top - 1] == STACK_TAG_RETADDR && "SBT86 stack tag mismatch");
    words[top - 1] = RET_VERIFICATION;
    tags[top - 1] = STACK_TAG_WORD;
}

void SBTStack::postRestoreRet()
{
    assert(tags[top - 1] == STACK_TAG_WORD && "SBT86 stack tag mismatch");
    assert(words[top - 1] == RET_VERIFICATION && "SBT86 stack retaddr mismatch");
    tags[top - 1] = STACK_TAG_RETADDR;
}
