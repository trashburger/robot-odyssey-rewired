/* -*- Mode: C++; c-basic-offset: 4 -*-
 *
 * Implementation of SBTProcess, the abstract base class for a
 * binary-translated 8086 DOS process.
 *
 * Copyright (c) 2009 Micah Elizabeth Scott <micah@scanlime.org>
 *
 *    Permission is hereby granted, free of charge, to any person
 *    obtaining a copy of this software and associated documentation
 *    files (the "Software"), to deal in the Software without
 *    restriction, including without limitation the rights to use,
 *    copy, modify, merge, publish, distribute, sublicense, and/or sell
 *    copies of the Software, and to permit persons to whom the
 *    Software is furnished to do so, subject to the following
 *    conditions:
 *
 *    The above copyright notice and this permission notice shall be
 *    included in all copies or substantial portions of the Software.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 *    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 *    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *    OTHER DEALINGS IN THE SOFTWARE.
 */

#include <string.h>
#include <stdio.h>
#include "sbt86.h"

static const bool full_stack_trace = false;


static void decompressRLE(uint8_t *dest, uint8_t *limit, uint8_t *src, uint32_t srcLength)
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
    continue_func = getEntry();
    default_func = continue_func;
    default_reg = reg;

    uint8_t *end_of_mem = hardware->mem + SBTHardware::MEM_SIZE;
    uint8_t *data_segment = hardware->memSeg(reg.ds);

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
    uint8_t *psp = hardware->memSeg(reg.es);
    memset(psp, 0, 0x80);
    psp[0x80] = strlen(cmdLine);
    memset(&psp[0x81], 0x0D, 0x7f);
    strncpy((char*) &psp[0x81], cmdLine, 0x7e);
}

void SBTProcess::run(void)
{
    assert(hardware != NULL && "SBTHardware must be defined\nbefore running a process");

    SBTStack stack;
    loadCache(&stack);

    if (!setjmp(jmp_yield)) {
        continue_func();

        // Continuation function returned; back to the default func.
        continue_func = default_func;
        reg = default_reg;
    }
}

static void continue_after_exit() {
    assert(0 && "Continuing to run an exited SBTProcess");
}

void SBTProcess::exit(uint8_t code)
{
    fprintf(stderr, "EXIT, code %d\n", code);
    hardware->resume_default_process(code);
    continue_from(reg, continue_after_exit);
}

void SBTProcess::continue_from(SBTRegs regs, continue_func_t fn, bool default_entry)
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
    assert(0 && "Failed dynamic branch");
}

uint8_t *SBTHardware::memSeg(uint16_t seg)
{
    if (seg > MAX_SEGMENT) {
        seg = MAX_SEGMENT;
    }
    return mem + (((uint32_t)seg) << 4);
}

uint8_t SBTHardware::peek8(uint16_t seg, uint16_t off) {
    return memSeg(seg)[off];
}

void SBTHardware::poke8(uint16_t seg, uint16_t off, uint8_t value) {
    memSeg(seg)[off] = value;
}

int16_t SBTHardware::peek16(uint16_t seg, uint16_t off) {
    return SBTSegmentCache::read16(memSeg(seg) + off);
}

void SBTHardware::poke16(uint16_t seg, uint16_t off, uint16_t value) {
    SBTSegmentCache::write16(memSeg(seg) + off, value);
}

SBTStack::SBTStack() {
    reset();
}

void SBTStack::reset() {
    top = 0;
}

void SBTStack::trace() {
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

void SBTStack::pushw(uint16_t word) {
    assert(top < STACK_SIZE && "SBT86 stack overflow");
    words[top] = word;
    tags[top] = STACK_TAG_WORD;
    top++;
}

void SBTStack::pushf(SBTRegs reg) {
    assert(top < STACK_SIZE && "SBT86 stack overflow");
    flags[top].uresult = reg.uresult;
    flags[top].sresult = reg.sresult;
    tags[top] = STACK_TAG_FLAGS;
    top++;
}

void SBTStack::pushret(uint16_t fn) {
    if (full_stack_trace) {
        fprintf(stderr, "+%04x\n", fn);
    }
    assert(top < STACK_SIZE && "SBT86 stack overflow");
    fn_addrs[top] = fn;
    tags[top] = STACK_TAG_RETADDR;
    top++;
}

uint16_t SBTStack::popw() {
    top--;
    assert(tags[top] == STACK_TAG_WORD && "SBT86 stack tag mismatch");
    return words[top];
}

SBTRegs SBTStack::popf(SBTRegs reg) {
    top--;
    assert(tags[top] == STACK_TAG_FLAGS && "SBT86 stack tag mismatch");
    reg.uresult = flags[top].uresult;
    reg.sresult = flags[top].sresult;
    return reg;
}

void SBTStack::popret(uint16_t fn) {
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

void SBTStack::preSaveRet() {
    assert(tags[top - 1] == STACK_TAG_RETADDR && "SBT86 stack tag mismatch");
    words[top - 1] = RET_VERIFICATION;
    tags[top - 1] = STACK_TAG_WORD;
}

void SBTStack::postRestoreRet() {
    assert(tags[top - 1] == STACK_TAG_WORD && "SBT86 stack tag mismatch");
    assert(words[top - 1] == RET_VERIFICATION && "SBT86 stack retaddr mismatch");
    tags[top - 1] = STACK_TAG_RETADDR;
}
