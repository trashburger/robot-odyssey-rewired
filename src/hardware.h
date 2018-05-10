#pragma once

#include <vector>
#include <list>
#include "sbt86.h"
#include "filesystem.h"


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


class Hardware
{
 public:
    Hardware();
    ~Hardware();

    static const int CLOCK_HZ = 4770000;

    static const uint32_t MEM_SIZE = 256 * 1024;
    uint8_t mem[MEM_SIZE];

    void clearOutputQueue();
    void pressKey(uint8_t ascii, uint8_t scancode = 0);
    void setJoystickAxes(int x, int y);
    void setJoystickButton(bool button);
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
    virtual void pollJoystick(uint16_t &x, uint16_t &y, uint8_t &status);

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
    int js_x, js_y;
    bool js_button_pressed, js_button_held;

    void exit(uint8_t code);
    void writeSpeakerTimestamp(uint32_t timestamp);
    void renderFrame(uint8_t *framebuffer);
};
