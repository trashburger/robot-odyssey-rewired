#pragma once
#include "filesystem.h"
#include "iobuffer.h"
#include "joystick.h"
#include "nostdlib.h"
#include "sbt86.h"

enum class SaveStatus {
    OK,
    NOT_SUPPORTED,
    BLOCKED,
};

/*
 * Engine --
 *
 *    Contains the entire state of the virtual machine, capable of running an
 * SBTProcess with input and output connections.
 */

class Engine {
  public:
    Engine();

    bool exec(const char *program, const char *args = "");
    bool run(IOBuffer *io);

    SaveStatus saveGame();

    SBT_INLINE uint8_t in(SBTRuntime &rt, uint16_t port) {
        switch (port) {

        case 0x61: /* PC speaker gate */
            return port61;

        default:
            rt.io->error("Unhandled IN");
        }
    }

    SBT_INLINE void out(SBTRuntime &rt, uint16_t port, uint8_t value) {
        switch (port) {

        case 0x43: /* PIT mode bits */
            /*
             * Ignored. We don't emulate the PIT, we just assume the
             * speaker is always being toggled manually.
             */
            break;

        case 0x61: /* PC speaker gate */
            if ((value ^ port61) & 2) {
                rt.io->speakerImpulse(rt.clock.take());
            }
            port61 = value;
            break;

        default:
            rt.io->error("Unhandled OUT");
        }
    }

    SBT_INLINE bool interrupt10(SBTRuntime &rt) {
        switch (rt.reg.ah) {

        case 0x00: /* Set video mode */
            /* We're always in CGA mode. Use this as an opportunity to clear the
             * output CGA buffer on exec. */
            rt.io->cgaClear();
            return false;

        default:
            rt.io->error("Unhandled INT10");
        }
    }

    SBT_INLINE bool interrupt16(SBTRuntime &rt) {
        switch (rt.reg.ah) {

        case 0x00: { /* Get keystroke */
            auto key = rt.io->takeKeyboardItem();
            rt.reg.ax = (key.scancode << 8) | key.ascii;
            rt.reg.setZF(rt.reg.ax == 0);
            return false;
        }

        case 0x01: { /* Check for keystroke */
            auto key = rt.io->checkForKeyboardItem();
            rt.reg.ax = (key.scancode << 8) | key.ascii;
            rt.reg.setZF(rt.reg.ax == 0);
            return false;
        }

        default:
            rt.io->error("Unhandled INT16");
        }
    }

    // Handle DOS interrupt 21h. Returns true to break control flow.
    SBT_INLINE bool interrupt21(SBTRuntime &rt) {
        uint8_t *dx_ptr = rt.seg.ds + rt.reg.dx;
        switch (rt.reg.ah) {

        case 0x06: /* Direct console input/output (Only input supported) */
            if (rt.reg.dl == 0xFF) {
                uint16_t key = 0;
                rt.reg.al = (uint8_t)key;
                rt.reg.setZF(key == 0);
            }
            return false;

        case 0x25: /* Set interrupt vector */
            /* Ignored. Robot Odyssey uses this to set the INT 24h error
             * handler. */
            return false;

        case 0x3D: /* Open File */
            returnFd(rt, fs.open(rt.io, (char *)dx_ptr));
            return false;

        case 0x3C: /* Create file */
            returnFd(rt, fs.create(rt.io, (char *)dx_ptr));
            return false;

        case 0x3E: /* Close File */
            fs.close(rt.io, rt.reg.bx);
            return false;

        case 0x3F: /* Read File */
            rt.reg.ax = fs.read(rt.io, rt.reg.bx, dx_ptr, rt.reg.cx);
            rt.reg.clearCF();
            return false;

        case 0x40: /* Write file */
            rt.reg.ax = fs.write(rt.io, rt.reg.bx, dx_ptr, rt.reg.cx);
            rt.reg.clearCF();
            return false;

        case 0x4A: /* Reserve memory */
            return false;

        case 0x4C: /* Exit with return code */
            rt.stack.trace(rt.io);
            return exit(rt.io, rt.reg.al);

        default:
            rt.io->error("Unhandled INT21");
        }
    }

    SBT_INLINE bool exit(IOBuffer *io, uint8_t code) {
        io->exit(code);
        process = nullptr;
        entry.func = nullptr;
        return true;
    }

    const SBTProcess *process;
    SBTMemory mem;
    DOSFilesystem fs;
    JoystickPoller js;

  private:
    struct EntryState {
        SBTRegs reg;
        SBTRuntime::func_t func;
    };

    EntryState defaultEntry;
    EntryState entry;
    uint8_t port61;

    bool isWaitingInMainLoop() {
        return process != nullptr && defaultEntry.func == entry.func &&
               entry.func != process->getFunction(SBTADDR_ENTRY_FUNC);
    }

    static void returnFd(SBTRuntime &rt, int fd) {
        if (fd < 0) {
            rt.reg.setCF();
        } else {
            rt.reg.ax = (uint16_t)fd;
            rt.reg.clearCF();
        }
    }
};
