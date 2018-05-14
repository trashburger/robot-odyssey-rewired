#pragma once

#include <vector>
#include <list>
#include "sbt86.h"
#include "filesystem.h"
#include "output.h"

class Hardware
{
 public:
    Hardware();

    static const uint32_t MEM_SIZE = 256 * 1024;
    uint8_t mem[MEM_SIZE];

    void pressKey(uint8_t ascii, uint8_t scancode = 0);
    void setJoystickAxes(int x, int y);
    void setJoystickButton(bool button);
    void setMouseTracking(int x, int y);
    void endMouseTracking();

    uint32_t run();
    void registerProcess(SBTProcess *p, bool is_default = false);

    static const unsigned SCREEN_WIDTH = 320;
    static const unsigned SCREEN_HEIGHT = 200;

    uint32_t rgb_pixels[SCREEN_WIDTH * SCREEN_HEIGHT];
    uint32_t rgb_palette[4];

    uint8_t in(uint16_t port, uint32_t timestamp);
    void out(uint16_t port, uint8_t value, uint32_t timestamp);

    SBTRegs interrupt10(SBTRegs reg, SBTStack *stack);
    SBTRegs interrupt16(SBTRegs reg, SBTStack *stack);
    SBTRegs interrupt21(SBTRegs reg, SBTStack *stack);

    void exec(const char *program, const char *args);
    void clearInputBuffer();
    void pollJoystick(uint16_t &x, uint16_t &y, uint8_t &status);

    DOSFilesystem fs;
    OutputQueue output;
    SBTProcess *process;

 protected:
    std::vector<SBTProcess*> process_vec;
    SBTProcess *default_process;
    uint8_t port61;
    uint16_t keycode;
    int js_x, js_y;
    bool js_button_pressed, js_button_held;
    bool mouse_tracking;
    int mouse_x, mouse_y;

    void exit(uint8_t code);
};
