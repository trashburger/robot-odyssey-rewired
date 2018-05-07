#pragma once

#include <vector>
#include <list>
#include "sbt86.h"
#include "roData.h"
#include "fspack.h"

SBT_DECL_PROCESS(PlayEXE);
SBT_DECL_PROCESS(MenuEXE);
SBT_DECL_PROCESS(Menu2EXE);
SBT_DECL_PROCESS(LabEXE);
SBT_DECL_PROCESS(GameEXE);
SBT_DECL_PROCESS(TutorialEXE);
SBT_DECL_PROCESS(RendererEXE);


class DOSFilesystem
{
public:
    DOSFilesystem();
    void reset();

    int open(const char *name);
    int create(const char *name);
    void close(uint16_t fd);
    uint16_t read(uint16_t fd, void *buffer, uint16_t length);
    uint16_t write(uint16_t fd, const void *buffer, uint16_t length);

    ROJoyfile joyfile;

    struct {
        uint32_t size;
        bool writeMode;
        uint8_t buffer[0x10000];
    } save;

private:
    uint16_t allocateFD();

    static const unsigned MAX_OPEN_FILES = 16;
    const FileInfo* openFiles[MAX_OPEN_FILES];
    uint32_t fileOffsets[MAX_OPEN_FILES];
    FileInfo saveFileInfo;
    FileInfo joyFileInfo;
};


struct CGAFramebuffer
{
    uint8_t bytes[0x4000];
};


enum OutputType
{
    OUT_FRAME,
    OUT_SPEAKER_TIMESTAMP,
    OUT_DELAY,
};


struct OutputItem
{
    OutputType otype;
    union {
        uint32_t timestamp;
        uint32_t delay;
        CGAFramebuffer *framebuffer;
    } u;
};


class Hardware : public SBTHardware
{
 public:
    Hardware();
    ~Hardware();

    void clearOutputQueue();
    void pressKey(uint8_t ascii, uint8_t scancode = 0);
    uint32_t run(uint32_t max_delay_per_step = 100);
    void registerProcess(SBTProcess *p, bool is_default = false);

    static const unsigned SCREEN_WIDTH = 320;
    static const unsigned SCREEN_HEIGHT = 200;

    uint32_t rgb_pixels[SCREEN_WIDTH * SCREEN_HEIGHT];
    uint32_t rgb_palette[4];

    virtual uint8_t in(uint16_t port, uint32_t timestamp);
    virtual void out(uint16_t port, uint8_t value, uint32_t timestamp);

    virtual SBTRegs interrupt10(SBTRegs reg, SBTStack *stack);
    virtual SBTRegs interrupt16(SBTRegs reg, SBTStack *stack);
    virtual SBTRegs interrupt21(SBTRegs reg, SBTStack *stack);

    virtual void outputFrame(SBTStack *stack, uint8_t *framebuffer);
    virtual void outputDelay(uint32_t millis);    

    virtual void exec(const char *program, const char *args);
    virtual void clearKeyboardBuffer();

    DOSFilesystem fs;

 protected:
    std::list<OutputItem> output_queue;
    uint32_t output_queue_frame_count;
    uint32_t output_queue_delay_remaining;
    std::vector<SBTProcess*> process_vec;
    SBTProcess *process;
    SBTProcess *default_process;
    uint8_t port61;
    uint16_t keycode;

    void exit(uint8_t code);
    void writeSpeakerTimestamp(uint32_t timestamp);
    void renderFrame(uint8_t *framebuffer);
};
