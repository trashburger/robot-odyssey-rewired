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

    // Initialize memory
    memset(mem, 0, MEM_SIZE);
    decompressRLE(memSeg(reg.ds), mem + MEM_SIZE, getData(), getDataLen());

    // Initialize emulated stack
    stack.reset();

    /*
     * Program Segment Prefix. Locate it just before the beginning of
     * the EXE, and copy our command line to it.
     */
    reg.es = reg.ds - 0x10;
    poke8(reg.es, 0x80, strlen(cmdLine));
    strcpy((char*) (memSeg(reg.es) + 0x81), cmdLine);

    // Use the entry point instead of jmpHalt
    jmpHaltIsSet = false;
}

int SBTProcess::run(void)
{
    assert(hardware != NULL && "SBTHardware must be defined\nbefore running a process");

    /*
     * Give us a way to exit from run mode. The halt() routine will
     * bounce back here with a return code.
     */
    int result = setjmp(jmpRun);
    if (result) {
        return result;
    }

    /*
     * Go back to executing this process.
     */
    loadCache();
    if (jmpHaltIsSet) {
        longjmp(jmpHalt, 1);
    } else {
        invokeEntry();
        jmpHaltIsSet = false;
    }
    return 0;
}

void SBTProcess::halt(int code)
{
    /*
     * Save our current state, and exit from run().
     */
    if (!setjmp(jmpHalt)) {
        jmpHaltIsSet = true;
        saveCache();
        longjmp(jmpRun, code);
    }
}

uint8_t *SBTProcess::memSeg(uint16_t seg)
{
    if (seg > MAX_SEGMENT) {
        seg = MAX_SEGMENT;
    }
    return mem + (((uint32_t)seg) << 4);
}

void SBTProcess::failedDynamicBranch(uint16_t cs, uint16_t ip, uint32_t value)
{
    assert(0 && "Failed dynamic branch");
}

uint8_t SBTProcess::peek8(uint16_t seg, uint16_t off) {
    return memSeg(seg)[off];
}

void SBTProcess::poke8(uint16_t seg, uint16_t off, uint8_t value) {
    memSeg(seg)[off] = value;
}

int16_t SBTProcess::peek16(uint16_t seg, uint16_t off) {
    return SBTSegmentCache::read16(memSeg(seg) + off);
}

void SBTProcess::poke16(uint16_t seg, uint16_t off, uint16_t value) {
    SBTSegmentCache::write16(memSeg(seg) + off, value);
}
