#pragma once

#include <vector>
#include <list>
#include "sbt86.h"
#include "filesystem.h"
#include "output.h"
#include "input.h"

class Hardware
{
 public:
    Hardware();

    uint32_t run();
    void registerProcess(SBTProcess *p);

    uint8_t in(uint16_t port, uint32_t timestamp);
    void out(uint16_t port, uint8_t value, uint32_t timestamp);

    SBTRegs interrupt10(SBTRegs reg, SBTStack *stack);
    SBTRegs interrupt16(SBTRegs reg, SBTStack *stack);
    SBTRegs interrupt21(SBTRegs reg, SBTStack *stack);

    void exec(const char *program, const char *args);

    static const uint32_t MEM_SIZE = 256 * 1024;
    uint8_t mem[MEM_SIZE];

    DOSFilesystem fs;
    OutputQueue output;
    InputBuffer input;
    SBTProcess *process;

 protected:
    std::vector<SBTProcess*> process_vec;
    uint8_t port61;
    uint16_t keycode;
    int js_x, js_y;
    bool js_button_pressed, js_button_held;
    bool mouse_tracking;
    int mouse_x, mouse_y;

    void exit(SBTProcess *exiting_process, uint8_t code);
};
