#pragma once

#include "sbt86.h"
#include "roData.h"
#include "fspack.h"

SBT_DECL_PROCESS(PlayEXE);
SBT_DECL_PROCESS(MenuEXE);
SBT_DECL_PROCESS(LabEXE);
SBT_DECL_PROCESS(GameEXE);
SBT_DECL_PROCESS(TutorialEXE);
SBT_DECL_PROCESS(RendererEXE);


/*
 * DOSFilesystem --
 *
 *    A utility class for implementing DOS filesystem emulation.
 *    Most files are read-only, and backed by GBFS. One special
 *    "save file" is backed by memory.
 */

class DOSFilesystem
{
public:
    DOSFilesystem();

    int open(const char *name);
    void close(uint16_t fd);
    uint16_t read(uint16_t fd, void *buffer, uint16_t length);

    uint8_t saveFile[sizeof(ROSavedGame)];

private:
    uint16_t allocateFD();

    static const unsigned MAX_OPEN_FILES = 16;
    const FileInfo* openFiles[MAX_OPEN_FILES];
    uint32_t fileOffsets[MAX_OPEN_FILES];
    FileInfo saveFileInfo;
};


/*
 * Hardware --
 *
 *    Subclass of SBTHardware that provides minimal
 *    implementations of all emulated interrupts and hardware.
 */

class Hardware : public SBTHardware
{
 public:
    Hardware();

    virtual void pressKey(uint8_t ascii, uint8_t scancode = 0);

    static const unsigned SCREEN_WIDTH = 320;
    static const unsigned SCREEN_HEIGHT = 200;

    uint32_t rgb_pixels[SCREEN_WIDTH * SCREEN_HEIGHT];
    uint32_t rgb_palette[4];
    
    /*
     * SBT86 Entry points
     */

    virtual uint8_t in(uint16_t port, uint32_t timestamp);
    virtual void out(uint16_t port, uint8_t value, uint32_t timestamp);

    virtual SBTRegs interrupt10(SBTProcess *proc, SBTRegs reg);
    virtual SBTRegs interrupt16(SBTProcess *proc, SBTRegs reg);
    virtual SBTRegs interrupt21(SBTProcess *proc, SBTRegs reg);

    virtual void drawScreen(SBTProcess *proc, uint8_t *framebuffer);

    DOSFilesystem fs;

 protected:
    uint8_t port61;
    uint16_t keycode;

    virtual void writeSpeakerTimestamp(uint32_t timestamp);
};
