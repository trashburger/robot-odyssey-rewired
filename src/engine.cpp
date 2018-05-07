//parcel: -Oz -I. -s ASSERTIONS=2 sbt86.cpp hardware.cpp ../build/fspack.cpp ../build/bt_lab.cpp ../build/bt_menu.cpp ../build/bt_menu2.cpp ../build/bt_game.cpp ../build/bt_tutorial.cpp ../build/bt_play.cpp

#include <stdint.h>
#include <emscripten.h>
#include <algorithm>
#include "hardware.h"

static Hardware hw;

static void loop()
{
    uint32_t millis = hw.run();
    emscripten_set_main_loop_timing(EM_TIMING_SETTIMEOUT, millis);
}

extern "C" void EMSCRIPTEN_KEEPALIVE start()
{
    hw.registerProcess(new PlayEXE(&hw), true);
    hw.registerProcess(new MenuEXE(&hw));
    hw.registerProcess(new Menu2EXE(&hw));
    hw.registerProcess(new GameEXE(&hw));
    hw.registerProcess(new LabEXE(&hw));
    hw.registerProcess(new TutorialEXE(&hw));

    emscripten_set_main_loop(loop, 0, false);
}

extern "C" void EMSCRIPTEN_KEEPALIVE exec(const char *process, const char *arg)
{
	hw.clearOutputQueue();
	hw.exec(process, arg);
	loop();
}

extern "C" void EMSCRIPTEN_KEEPALIVE pressKey(uint8_t ascii, uint8_t scancode) 
{
    hw.pressKey(ascii, scancode);

	// Skip cutscenes
	hw.clearOutputQueue();
}

extern "C" uint8_t* EMSCRIPTEN_KEEPALIVE memPointer()
{
	return hw.mem;
}

extern "C" uint32_t EMSCRIPTEN_KEEPALIVE memSize()
{
	return Hardware::MEM_SIZE;
}