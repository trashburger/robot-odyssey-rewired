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

static const bool verbose_filesystem_info = false;


DOSFilesystem::DOSFilesystem()
{
    reset();
}

void DOSFilesystem::reset()
{
    memset(openFiles, 0, sizeof openFiles);
}

int DOSFilesystem::open(const char *name)
{
    int fd = allocateFD();
    const FileInfo *file;

    if (verbose_filesystem_info) {
        printf("FILE, opening '%s'\n", name);
    }

    if (!strcmp(name, SBT_SAVE_FILE_NAME)) {
        /*
         * Save file
         */

        saveFileInfo.data = save.buffer;
        saveFileInfo.data_size = save.size;
        save.writeMode = false;
        file = &saveFileInfo;

    } else if (!strcmp(name, SBT_JOYFILE)) {
        /*
         * Joystick/configuration file
         */

        joyFileInfo.data = (const uint8_t*) &joyfile;
        joyFileInfo.data_size = sizeof joyfile;
        file = &joyFileInfo;

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

int DOSFilesystem::create(const char *name)
{
    int fd = allocateFD();
    const FileInfo *file;

    if (verbose_filesystem_info) {
        printf("FILE, creating '%s'\n", name);
    }

    if (!strcmp(name, SBT_SAVE_FILE_NAME)) {

        saveFileInfo.data = save.buffer;
        saveFileInfo.data_size = 0;
        save.size = 0;
        save.writeMode = true;
        file = &saveFileInfo;

    } else {
        fprintf(stderr, "FILE, failed to open '%s' for writing\n", name);
        return -1;
    }

    openFiles[fd] = file;
    fileOffsets[fd] = 0;
    return fd;
}

void DOSFilesystem::close(uint16_t fd)
{
    assert(fd < MAX_OPEN_FILES && "Closing an invalid file descriptor");
    assert(openFiles[fd] && "Closing a file which is not open");

    if (openFiles[fd] == &saveFileInfo && save.writeMode) {
        EM_ASM_({
            Module.onSaveFileWrite(HEAPU8.subarray($0, $0 + $1));
        }, save.buffer, save.size);
    }

    openFiles[fd] = 0;
}

uint16_t DOSFilesystem::read(uint16_t fd, void *buffer, uint16_t length)
{
    const FileInfo *file = openFiles[fd];

    assert(fd < MAX_OPEN_FILES && "Reading an invalid file descriptor");
    assert(file && "Reading a file which is not open");

    uint32_t offset = fileOffsets[fd];
    uint16_t actual_length = std::min<unsigned>(length, file->data_size - offset);

    if (verbose_filesystem_info) {
        printf("FILE, read %d(%d) bytes at %d\n", length, actual_length, offset);
    }
    memcpy(buffer, file->data + offset, actual_length);

    fileOffsets[fd] += actual_length;
    return actual_length;
}

uint16_t DOSFilesystem::write(uint16_t fd, const void *buffer, uint16_t length)
{
    const FileInfo *file = openFiles[fd];

    assert(fd < MAX_OPEN_FILES && "Writing an invalid file descriptor");
    assert(file && "Writing a file which is not open");
    assert(file == &saveFileInfo && "Writing a file that isn't the saved game file");

    uint32_t offset = fileOffsets[fd];
    uint16_t actual_length = std::min<unsigned>(length, sizeof save.buffer - offset);

    if (verbose_filesystem_info) {
        printf("FILE, write %d(%d) bytes at %d\n", length, actual_length, offset);
    }
    memcpy(save.buffer + offset, buffer, actual_length);

    fileOffsets[fd] += actual_length;
    save.size = fileOffsets[fd];
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

    setJoystickAxes(0, 0);
    setJoystickButton(false);

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
    output_queue_frame_count = 0;
    output_queue_delay_remaining = 0;
}

void Hardware::clearKeyboardBuffer()
{
    keycode = 0;
}

void Hardware::pollJoystick(uint16_t &x, uint16_t &y, uint8_t &status)
{
    // Port 0x201 style status byte: Low 4 bits are timed based
    // on an RC circuit in each axis. Upper 4 bits are buttons,
    // active low. The byte includes data for two joysticks, and
    // we only emulate one.

    status = 0xFC ^ (js_button ? 0x10 : 0);
    x = std::max(0, std::min((int)fs.joyfile.x_center * 2, js_x + fs.joyfile.x_center));
    y = std::max(0, std::min((int)fs.joyfile.y_center * 2, js_y + fs.joyfile.y_center));
}

void Hardware::exec(const char *program, const char *args)
{
    if (verbose_filesystem_info) {
        printf("EXEC, '%s' '%s'\n", program, args);
    }
    for (std::vector<SBTProcess*>::iterator i = process_vec.begin(); i != process_vec.end(); i++) {
        const char *filename = (*i)->getFilename();
        if (!strcasecmp(program, filename)) {
            process = *i;
            fs.reset();
            process->exec(args);
            return;
        }
    }
    assert(0 && "Program not found in exec()");
}

void Hardware::exit(uint8_t code)
{
    if (verbose_filesystem_info) {
        printf("EXIT, code %d\n", code);
    }
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

            switch (item.otype) {

                case OUT_FRAME:
                    renderFrame(item.u.framebuffer->bytes);
                    output_queue_frame_count--;
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

static void set_result_for_fd(SBTRegs &reg, int fd) {
    if (fd < 0) {
        reg.setCF();
    } else {
        reg.ax = fd;
        reg.clearCF();
    }    
}

static void small_hexdump_and_newline(const uint8_t *bytes, uint16_t count)
{
    if (count <= 32) {
        for (unsigned i = 0; i < count; i++) {
            printf(" %02x", bytes[i]);
        }
    }
    printf("\n");
}

SBTRegs Hardware::interrupt21(SBTRegs reg, SBTStack *stack)
{
    switch (reg.ah) {

    case 0x06:                /* Direct console input/output (Only input supported) */
        if (reg.dl == 0xFF) {
            reg.al = (uint8_t) keycode;
            if (keycode) {
                reg.clearZF();
            } else {
                reg.setZF();
            }
            keycode = 0;
        }
        break;

    case 0x25:                /* Set interrupt vector */
        /* Ignored. Robot Odyssey uses this to set the INT 24h error handler. */
        break;

    case 0x3D:                /* Open File */
        set_result_for_fd(reg, fs.open((char*)(memSeg(reg.ds) + reg.dx)));
        break;

    case 0x3C:                /* Create file */
        set_result_for_fd(reg, fs.create((char*)(memSeg(reg.ds) + reg.dx)));
        break;

    case 0x3E:                /* Close File */
        fs.close(reg.bx);
        break;

    case 0x3F:                /* Read File */
        reg.ax = fs.read(reg.bx, memSeg(reg.ds) + reg.dx, reg.cx);
        reg.clearCF();
        if (verbose_filesystem_info) {
            printf("FILE, reading %d bytes into %04x:%04x ", reg.ax, reg.ds, reg.dx);
            small_hexdump_and_newline(memSeg(reg.ds) + reg.dx, reg.ax);
        }
        break;

    case 0x40:                /* Write file */
        reg.ax = fs.write(reg.bx, memSeg(reg.ds) + reg.dx, reg.cx);
        reg.clearCF();
        if (verbose_filesystem_info) {
            printf("FILE, writing %d bytes from %04x:%04x ", reg.ax, reg.ds, reg.dx);
            small_hexdump_and_newline(memSeg(reg.ds) + reg.dx, reg.ax);
        }
        break;

    case 0x4A:                /* Reserve memory */
        break;

    case 0x4C:                /* Exit with return code */
        exit(reg.al);
        break;

    default:
        stack->trace();
        fprintf(stderr, "int21 ax=%04x\n", reg.ax);
        assert(0 && "Unimplemented DOS Int21");
    }
    return reg;
}

void Hardware::pressKey(uint8_t ascii, uint8_t scancode)
{
    keycode = (scancode << 8) | ascii;
}

void Hardware::setJoystickAxes(int x, int y)
{
    js_x = x;
    js_y = y;
}

void Hardware::setJoystickButton(bool button)
{
    js_button = button;
}

void Hardware::outputFrame(SBTStack *stack, uint8_t *framebuffer)
{
    if (output_queue_frame_count > 500) {
        stack->trace();
        assert(0 && "Frame queue is too deep! Infinite loop likely.");
    }

    OutputItem item;
    item.otype = OUT_FRAME;
    item.u.framebuffer = new CGAFramebuffer;
    memcpy(item.u.framebuffer, framebuffer, sizeof *item.u.framebuffer);
    output_queue.push_back(item);
    output_queue_frame_count++;
}

void Hardware::outputDelay(uint32_t millis)
{
    OutputItem item;
    item.otype = OUT_DELAY;
    item.u.delay = millis;
    output_queue.push_back(item);
}

void Hardware::writeSpeakerTimestamp(uint32_t timestamp)
{
    OutputItem item;
    item.otype = OUT_SPEAKER_TIMESTAMP;
    item.u.timestamp = timestamp;
    output_queue.push_back(item);
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
        Module.onRenderFrame(HEAPU8.subarray($0, $0 + 320*200*4))
    }, rgb_pixels);
}
