#pragma once

#include "sbt86.h"
#include "roData.h"
#include "fspack.h"

SBT_DECL_PROCESS(MenuEXE);
SBT_DECL_PROCESS(LabEXE);
SBT_DECL_PROCESS(GameEXE);
SBT_DECL_PROCESS(TutorialEXE);


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

    uint16_t open(const char *name);
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
 * HwCommon --
 *
 *    Concrete subclass of SBTHardware that provides minimal
 *    implementations of all emulated interrupts and hardware.
 *    This class doesn't know how to do anything user-visible,
 *    but it can handle DOS interrupts and load files.
 */

class HwCommon : public SBTHardware
{
 public:
    HwCommon();

    /*
     * SBT86 Entry points
     */

    virtual uint8_t in(uint16_t port, uint32_t timestamp);
    virtual void out(uint16_t port, uint8_t value, uint32_t timestamp);

    virtual SBTRegs interrupt10(SBTProcess *proc, SBTRegs reg);
    virtual SBTRegs interrupt16(SBTProcess *proc, SBTRegs reg);
    virtual SBTRegs interrupt21(SBTProcess *proc, SBTRegs reg);

    virtual void drawScreen(SBTProcess *proc, uint8_t *framebuffer);

    /*
     * Entry points useful to the main program
     */

    virtual void pressKey(uint8_t ascii, uint8_t scancode = 0);

    DOSFilesystem fs;

 protected:
    uint8_t port61;
    uint16_t keycode;

    virtual void writeSpeakerTimestamp(uint32_t timestamp);
    virtual void pollKeys(SBTProcess *proc);
};


/*
 * Basic double-buffered video using the main screen.
 */
class HwMain : public HwCommon
{
 public:

    virtual void drawScreen(SBTProcess *proc, uint8_t *framebuffer);

protected:
    int bg;
    uint16_t *backbuffer;
};


/*
 * Video on the main screen, plus sound and input.
 */
class HwMainInteractive : public HwMain
{
protected:
    virtual void writeSpeakerTimestamp(uint32_t timestamp);
    virtual void pollKeys(SBTProcess *proc);
};
