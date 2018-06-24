#include <emscripten.h>
#include <algorithm>
#include <string.h>
#include <stdio.h>
#include "sbt86.h"
#include "hardware.h"

static const bool verbose_process_info = false;

Hardware::Hardware(OutputInterface &output)
    : output(output)
{
    memset(mem, 0, MEM_SIZE);
    process = 0;
    port61 = 0;
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
            input.clear();
            process->exec(args);
            return;
        }
    }
    assert(0 && "Program not found in exec()");
}

bool Hardware::loadGame()
{
    // If the buffer contains a loadable game, loads it and returns true.
    if (fs.save.isGame()) {
        const char *process = fs.save.asGame().getProcessName();
        if (process) {
            exec(process, "99");
            return true;
        }
    }
    return false;
}

SaveStatus Hardware::saveGame()
{
    if (!process) {
        // Not running at all
        return SaveStatus::NOT_SUPPORTED;
    }

    if (!process->hasFunction(SBTADDR_SAVE_GAME_FUNC)) {
        // No save function in this process
        return SaveStatus::NOT_SUPPORTED;
    }

    if (!process->isWaitingInMainLoop()) {
        // Can't safely interrupt the process
        return SaveStatus::BLOCKED;
    }

    fs.save.file.size = 0;
    process->call(SBTADDR_SAVE_GAME_FUNC, process->reg);

    if (!fs.save.isGame()) {
        // File isn't the right size
        return SaveStatus::NOT_SUPPORTED;
    }

    if (!fs.save.asGame().getProcessName()) {
        // File isn't something we know how to load.
        // (Tutorial 6 runs in LAB.EXE, which knows how to save, but we can't load those files.)
        return SaveStatus::NOT_SUPPORTED;
    }

    return SaveStatus::OK;
}

bool Hardware::loadChipDocumentation()
{
    if (!fs.save.isChip()) {
        return false;
    }

    // Get a fresh lab and run it until it's in the main loop
    exec("lab.exe", "30");
    while (!process->isWaitingInMainLoop()) {
        process->run();
    }

    // Load into the first chip
    if (!loadChip(0)) {
        return false;
    }

    // Switch to the documentation room for that chip
    ROWorld *world = ROWorld::fromProcess(process);
    if (!world) {
        return false;
    }

    world->objects.room[RO_OBJ_PLAYER] = RO_ROOM_CHIP_1;
    return true;
}

void Hardware::requestLoadChip(SBTRegs reg)
{
    EM_ASM_({
        if (Module.onLoadChipRequest) {
            Module.onLoadChipRequest($0);
        }
    }, reg.dl);
}

bool Hardware::loadChip(uint8_t id)
{
    if (process && process->isWaitingInMainLoop() && fs.save.isChip()) {
        SBTRegs r = process->reg;
        r.dl = id;
        process->call(SBTADDR_LOAD_CHIP_FUNC, r);
        return true;
    }
    return false;
}

void Hardware::exit(SBTProcess *exiting_process, uint8_t code)
{
    if (verbose_process_info) {
        printf("EXIT, code %d\n", code);
    }

    // Next state is no process, unless the callback invokes exec(), which it might.
    process = 0;

    EM_ASM_({
        if (Module.onProcessExit) {
            Module.onProcessExit($0);
        }
    }, code);

    // Returns from run() immediately via longjmp/throw
    exiting_process->exit();
}

void Hardware::registerProcess(SBTProcess *p)
{
    process_vec.push_back(p);
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
        reg.ax = input.getKey();
        reg.setZF(reg.ax == 0);
        break;

    case 0x01:                /* Check for keystroke */
        reg.ax = input.checkForKey();
        reg.setZF(reg.ax == 0);
        break;

    default:
        assert(0 && "Unimplemented BIOS Int16");
    }
    return reg;
}

static void set_result_for_fd(SBTRegs &reg, int fd)
{
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
            uint16_t key = input.getKey();
            reg.al = (uint8_t) key;
            reg.setZF(key == 0);
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
        exit(process, reg.al);
        break;

    default:
        stack->trace();
        fprintf(stderr, "int21 ax=%04x\n", reg.ax);
        assert(0 && "Unimplemented DOS Int21");
    }
    return reg;
}
