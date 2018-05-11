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
#include <stdio.h>
#include "sbt86.h"
#include "hardware.h"

static const bool verbose_process_info = false;


Hardware::Hardware()
{
    process = 0;
    memset(mem, 0, MEM_SIZE);

    setJoystickAxes(0, 0);
    setJoystickButton(false);

    port61 = 0;
}

void Hardware::clearKeyboardBuffer()
{
    keycode = 0;
}

void Hardware::pollJoystick(uint16_t &x, uint16_t &y, uint8_t &status)
{
    // Button presses must not be missed if they end before the next poll.
    // Clear the button press latch here.

    bool button = js_button_held || js_button_pressed;
    js_button_pressed = false;

    // Port 0x201 style status byte: Low 4 bits are timed based
    // on an RC circuit in each axis. Upper 4 bits are buttons,
    // active low. The byte includes data for two joysticks, and
    // we only emulate one.

    status = 0xFC ^ (button ? 0x10 : 0);
    x = std::max(0, std::min((int)fs.joyfile.x_center * 2, js_x + fs.joyfile.x_center));
    y = std::max(0, std::min((int)fs.joyfile.y_center * 2, js_y + fs.joyfile.y_center));
}

void Hardware::exec(const char *program, const char *args)
{
    if (verbose_process_info) {
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
    if (verbose_process_info) {
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

uint32_t Hardware::run()
{
    while (true) {
        uint32_t delay = output.run();
        if (delay) {
            return delay;
        }

        assert(process);
        process->run();
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
            output.pushSpeakerTimestamp(timestamp);
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
        set_result_for_fd(reg, fs.open((char*)(process->memSeg(reg.ds) + reg.dx)));
        break;

    case 0x3C:                /* Create file */
        set_result_for_fd(reg, fs.create((char*)(process->memSeg(reg.ds) + reg.dx)));
        break;

    case 0x3E:                /* Close File */
        fs.close(reg.bx);
        break;

    case 0x3F:                /* Read File */
        reg.ax = fs.read(reg.bx, process->memSeg(reg.ds) + reg.dx, reg.cx);
        reg.clearCF();
        if (verbose_process_info) {
            printf("FILE, reading %d bytes into %04x:%04x ", reg.ax, reg.ds, reg.dx);
            small_hexdump_and_newline(process->memSeg(reg.ds) + reg.dx, reg.ax);
        }
        break;

    case 0x40:                /* Write file */
        reg.ax = fs.write(reg.bx, process->memSeg(reg.ds) + reg.dx, reg.cx);
        reg.clearCF();
        if (verbose_process_info) {
            printf("FILE, writing %d bytes from %04x:%04x ", reg.ax, reg.ds, reg.dx);
            small_hexdump_and_newline(process->memSeg(reg.ds) + reg.dx, reg.ax);
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
    output.skipDelay();
}

void Hardware::setJoystickAxes(int x, int y)
{
    js_x = x;
    js_y = y;
}

void Hardware::setJoystickButton(bool button)
{
    js_button_held = button;
    js_button_pressed = js_button_pressed || button;
    if (button) {
        output.skipDelay();
    }
}
