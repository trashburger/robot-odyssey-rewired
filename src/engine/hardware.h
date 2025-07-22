#pragma once

#include "filesystem.h"
#include "input.h"
#include "output.h"
#include "sbt86.h"
#include <list>
#include <vector>

enum class SaveStatus {
    OK,
    NOT_SUPPORTED,
    BLOCKED,
};

class Hardware {
  public:
    Hardware(OutputInterface &output);

    void registerProcess(SBTProcess *p);

    uint8_t in(uint16_t port, uint32_t timestamp);
    void out(uint16_t port, uint8_t value, uint32_t timestamp);

    SBTRegs interrupt10(SBTRegs reg, SBTStack *stack);
    SBTRegs interrupt16(SBTRegs reg, SBTStack *stack);
    SBTRegs interrupt21(SBTRegs reg, SBTStack *stack);

    void requestLoadChip(SBTRegs reg);

    void exec(const char *program, const char *args = "");

    SaveStatus saveGame();
    bool loadGame();
    bool loadChip(uint8_t id);
    bool loadChipDocumentation();

    static const uint32_t MEM_SIZE = 256 * 1024;
    uint8_t mem[MEM_SIZE];

    DOSFilesystem fs;
    InputBuffer input;
    OutputInterface &output;
    SBTProcess *process;

  private:
    std::vector<SBTProcess *> process_vec;
    uint8_t port61;
    uint16_t keycode;
    int js_x, js_y;
    bool js_button_pressed, js_button_held;
    bool mouse_tracking;
    int mouse_x, mouse_y;

    void exit(SBTProcess *exiting_process, uint8_t code);
};
