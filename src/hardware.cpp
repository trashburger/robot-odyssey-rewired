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

#include <emscripten.h>
#include <algorithm>
#include <string.h>
#include <stdio.h>
#include "sbt86.h"
#include "hardware.h"
#include "fspack.h"


DOSFilesystem::DOSFilesystem()
{
    memset(openFiles, 0, sizeof openFiles);
}

int DOSFilesystem::open(const char *name)
{
    int fd = allocateFD();
    const FileInfo *file;

    fprintf(stderr, "FILE, opening '%s'\n", name);

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
            return -1;
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

    fprintf(stderr, "FILE, read %d(%d) bytes at %d\n", length, actual_length, offset);
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

Hardware::Hardware()
{
    clearOutputQueue();
    process = 0;
    memset(mem, 0, MEM_SIZE);

    port61 = 0;

    rgb_palette[0] = 0xff000000;
    rgb_palette[1] = 0xffffff55;
    rgb_palette[2] = 0xffff55ff;
    rgb_palette[3] = 0xffffffff;
}

Hardware::~Hardware()
{
    clearOutputQueue();
}

void Hardware::clearOutputQueue()
{
    while (!output_queue.empty()) {
        OutputItem item = output_queue.front();
        output_queue.pop_front();
        if (item.otype == OUT_FRAME) {
            delete item.u.framebuffer;
        }
    }
    output_queue_depth = 0;
    output_queue_delay_remaining = 0;
}

void Hardware::exec(const char *program, const char *args)
{
    fprintf(stderr, "EXEC, '%s' '%s'\n", program, args);
    for (std::vector<SBTProcess*>::iterator i = process_vec.begin(); i != process_vec.end(); i++) {
        const char *filename = (*i)->getFilename();
        if (!strcasecmp(program, filename)) {
            process = *i;
            process->exec(args);
            return;
        }
    }
    assert(0 && "Program not found in exec()");
}

void Hardware::exit(uint8_t code)
{
    fprintf(stderr, "EXIT, code %d\n", code);
    SBTProcess *exiting_process = process;
    process = default_process;
    process->reg.ax = code;
    exiting_process->exit();
}

void Hardware::registerProcess(SBTProcess *p, bool is_default)
{
    process_vec.push_back(p);
    if (is_default) {
        default_process = p;
        exec(p->getFilename(), "");
    }
}

uint32_t Hardware::run(uint32_t max_delay_per_step)
{
    // Generate output until we hit the next delay, and run() when the output queue is empty.
    // Returns the number of milliseconds to wait until the next call.

    while (true) {
        if (output_queue_delay_remaining > 0) {
            uint32_t delay = std::min(output_queue_delay_remaining, max_delay_per_step);
            output_queue_delay_remaining -= delay;
            return delay;

        } else if (output_queue.empty()) {
            assert(process);
            process->run();

        } else {
            OutputItem item = output_queue.front();
            output_queue.pop_front();
            output_queue_depth--;

            switch (item.otype) {

                case OUT_FRAME:
                    renderFrame(item.u.framebuffer->bytes);
                    delete item.u.framebuffer;
                    break;

                case OUT_DELAY:
                    output_queue_delay_remaining += item.u.delay;
                    break;

                case OUT_SPEAKER_TIMESTAMP:
                    // fprintf(stderr, "TODO, sound at %d\n", item.u.timestamp);
                    break;
            }
        }
    }
}


uint8_t Hardware::in(uint16_t port, uint32_t timestamp)
{
    switch (port) {

    case 0x61:    /* PC speaker gate */
        return port61;

    default:
        assert(0 && "Unimplemented IO IN");
        return 0;
    }
}

void Hardware::out(uint16_t port, uint8_t value, uint32_t timestamp)
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

SBTRegs Hardware::interrupt10(SBTRegs reg, SBTStack *stack)
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

SBTRegs Hardware::interrupt16(SBTRegs reg, SBTStack *stack)
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

SBTRegs Hardware::interrupt21(SBTRegs reg, SBTStack *stack)
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

    case 0x3D: {              /* Open File */
        int fd = fs.open((char*)(memSeg(reg.ds) + reg.dx));
        if (fd < 0) {
            reg.setCF();
        } else {
            reg.ax = fd;
            reg.clearCF();
        }
        break;
    }

    case 0x3E:                /* Close File */
        fs.close(reg.bx);
        break;

    case 0x3F:                /* Read File */
        reg.ax = fs.read(reg.bx, memSeg(reg.ds) + reg.dx, reg.cx);
        reg.clearCF();
        break;

    case 0x4A:                /* Reserve memory */
        break;

    case 0x4C:                /* Exit with return code */
        exit(reg.al);
        break;

    default:
        fprintf(stderr, "int21 ax=%04x\n", reg.ax);
        assert(0 && "Unimplemented DOS Int21");
    }
    return reg;
}

void Hardware::pressKey(uint8_t ascii, uint8_t scancode) {
    keycode = (scancode << 8) | ascii;
}

void Hardware::outputFrame(uint8_t *framebuffer)
{
    if (output_queue_depth > 500) {
        assert(0 && "Frame queue is too deep! Infinite loop likely.");
    }
    OutputItem item;
    item.otype = OUT_FRAME;
    item.u.framebuffer = new CGAFramebuffer;
    memcpy(item.u.framebuffer, framebuffer, sizeof *item.u.framebuffer);
    output_queue.push_back(item);
    output_queue_depth++;
}

void Hardware::outputDelay(uint32_t millis)
{
    OutputItem item;
    item.otype = OUT_DELAY;
    item.u.delay = millis;
    output_queue.push_back(item);
    output_queue_depth++;
}

void Hardware::writeSpeakerTimestamp(uint32_t timestamp)
{
    OutputItem item;
    item.otype = OUT_SPEAKER_TIMESTAMP;
    item.u.timestamp = timestamp;
    output_queue.push_back(item);
    output_queue_depth++;
}

void Hardware::renderFrame(uint8_t *framebuffer)
{
    for (unsigned plane = 0; plane < 2; plane++) {
        for (unsigned y=0; y < SCREEN_HEIGHT/2; y++) {
            for (unsigned x=0; x < SCREEN_WIDTH; x++) {
                unsigned byte = 0x2000*plane + (x + SCREEN_WIDTH*y)/4;
                unsigned bit = 3 - (x % 4);
                unsigned color = 0x3 & (framebuffer[byte] >> (bit * 2));
                uint32_t rgb = rgb_palette[color];
                rgb_pixels[x + (y*2+plane)*SCREEN_WIDTH] = rgb;
            }
        }
    }

    EM_ASM_({
        var canvas = window.document.getElementById('framebuffer');
        var ctx = canvas.getContext('2d');
        var img = ctx.createImageData(320, 200);
        img.data.set(HEAPU8.subarray($0, $0 + 320*200*4));
        ctx.putImageData(img, 1, 1);
    }, rgb_pixels);
}
