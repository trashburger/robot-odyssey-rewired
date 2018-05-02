/* -*- Mode: C++; c-basic-offset: 4 -*-
 *
 * Copyright (c) 2009-2018 Micah Elizabeth Scott <micah@scanlime.org>
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

#include <algorithm>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include "sbt86.h"
#include "hardware.h"
#include "fspack.h"


DOSFilesystem::DOSFilesystem()
{
    memset(openFiles, 0, sizeof openFiles);
}

uint16_t DOSFilesystem::open(const char *name)
{
    uint16_t fd = allocateFD();
    const FileInfo *file;

    if (!strcmp(name, SBT_SAVE_FILE_NAME)) {
        /*
         * Save file
         */

        saveFileInfo.data = saveFile;
        saveFileInfo.data_size = sizeof saveFile;
        file = &saveFileInfo;

    } else {
        /*
         * Game file
         */

        file = FileInfo::lookup(name);
        if (!file) {
            fprintf(stderr, "Failed to open file '%s'\n", name);
            assert(0 && "Failed to open file");
        }
    }

    openFiles[fd] = file;
    fileOffsets[fd] = 0;
    return fd;
}

void DOSFilesystem::close(uint16_t fd)
{
    assert(fd < MAX_OPEN_FILES && "Closing an invalid file descriptor");
    assert(openFiles[fd] && "Closing a file which is not open");
    openFiles[fd] = 0;
}

uint16_t DOSFilesystem::read(uint16_t fd, void *buffer, uint16_t length)
{
    const FileInfo *file = openFiles[fd];

    assert(fd < MAX_OPEN_FILES && "Reading an invalid file descriptor");
    assert(file && "Reading a file which is not open");

    uint32_t offset = fileOffsets[fd];
    uint16_t actual_length = std::min<unsigned>(length, file->data_size - offset);

    memcpy(buffer, file->data + offset, actual_length);

    fileOffsets[fd] += actual_length;
    return actual_length;
}

uint16_t DOSFilesystem::allocateFD()
{
    uint16_t fd = 0;

    while (openFiles[fd]) {
        fd++;
        if (fd >= MAX_OPEN_FILES) {
            assert(0 && "Too many open files");
        }
    }

    return fd;
}

HwCommon::HwCommon()
{
    port61 = 0;
}

uint8_t HwCommon::in(uint16_t port, uint32_t timestamp)
{
    switch (port) {

    case 0x61:    /* PC speaker gate */
        return port61;

    default:
        assert(0 && "Unimplemented IO IN");
        return 0;
    }
}

void HwCommon::out(uint16_t port, uint8_t value, uint32_t timestamp)
{
    switch (port) {

    case 0x43:    /* PIT mode bits */
        /*
         * Ignored. We don't emulate the PIT, we just assume the
         * speaker is always being toggled manually.
         */
        break;

    case 0x61:    /* PC speaker gate */
        if ((value ^ port61) & 2) {
            /*
             * PC speaker state toggled. Store a timestamp.
             */
            writeSpeakerTimestamp(timestamp);
        }

        port61 = value;
        break;

    default:
        assert(0 && "Unimplmented IO OUT");
    }
}

SBTRegs HwCommon::interrupt10(SBTProcess *proc, SBTRegs reg)
{
    switch (reg.ah) {

    case 0x00:    /* Set video mode */
        /* Ignore. We're always in CGA mode. */
        break;

    default:
        assert(0 && "Unimplemented BIOS Int10");
    }
    return reg;
}

SBTRegs HwCommon::interrupt16(SBTProcess *proc, SBTRegs reg)
{
    switch (reg.ah) {

    case 0x00:                /* Get keystroke */
        reg.ax = keycode;
        keycode = 0;
        break;

    case 0x01:                /* Check for keystroke */
        if (keycode) {
            reg.clearZF();
            reg.ax = keycode;
        } else {
            reg.setZF();
        }
        break;

    default:
        assert(0 && "Unimplemented BIOS Int16");
    }
    return reg;
}

SBTRegs HwCommon::interrupt21(SBTProcess *proc, SBTRegs reg)
{
    switch (reg.ah) {

    case 0x06:                /* Direct console input/output (Only input supported) */
        if (reg.dl == 0xFF) {
            if (keycode) {
                reg.al = (uint8_t) keycode;
                reg.clearZF();
                keycode = 0;
            } else {
                reg.al = 0;
                reg.setZF();
            }
        }
        break;

    case 0x25:                /* Set interrupt vector */
        /* Ignored. Robot Odyssey uses this to set the INT 24h error handler. */
        break;

    case 0x2C:                /* Get system time */
        /*
         * XXX: Currently a stub. This is used by MENU.EXE
         */
        {
            static time_t unixTime;
            struct tm* timeStruct = gmtime((const time_t *)&unixTime);
            unixTime++;

            reg.ch = timeStruct->tm_hour;
            reg.cl = timeStruct->tm_min;
            reg.dh = timeStruct->tm_sec;
            reg.dl = 0;
        }
        break;

    case 0x3D:                /* Open File */
        reg.ax = fs.open((char*)(proc->memSeg(reg.ds) + reg.dx));
        reg.clearCF();
        break;

    case 0x3E:                /* Close File */
        fs.close(reg.bx);
        break;

    case 0x3F:                /* Read File */
        reg.ax = fs.read(reg.bx, proc->memSeg(reg.ds) + reg.dx, reg.cx);
        reg.clearCF();
        break;

    case 0x4C:                /* Exit with return code */
        // The way we rewrite the main loop means that each frame exits via DOS.
        // Ignore successful exit codes.
        assert(reg.al == 0 && "DOS Exit with error");
        break;

    default:
        assert(0 && "Unimplemented DOS Int21");
    }
    return reg;
}

void HwCommon::pressKey(uint8_t ascii, uint8_t scancode) {
    keycode = (scancode << 8) | ascii;
}

void HwMain::drawScreen(SBTProcess *proc, uint8_t *framebuffer)
{
    // TODO
    fprintf(stderr, "Drawing a frame!\n");

    for (unsigned y=0; y < 200; y++) {
        for (unsigned x=0; x < 320; x++) {
            unsigned field = y % 2;
            unsigned line = y / 2;
            unsigned byte = (x + 320*(line + field*100))/4;
            unsigned bit = x % 4;
            unsigned color = 3 & (framebuffer[byte] >> (bit * 2));
            fprintf(stderr, "%x", color);
        }
        fprintf(stderr, "\n");
    }
}

void HwMainInteractive::writeSpeakerTimestamp(uint32_t timestamp) {
    // TODO
}

