//parcel: -g4 -I. sbt86.cpp hardware.cpp ../build/fspack.cpp ../build/bt_lab.cpp ../build/bt_menu.cpp ../build/bt_menu2.cpp ../build/bt_game.cpp ../build/bt_tutorial.cpp ../build/bt_play.cpp ../build/bt_renderer.cpp

#include <stdint.h>
#include <emscripten.h>
#include "hardware.h"

static SBTProcess *game;
static Hardware hw;


void loop()
{
	hw.run();
}

extern "C" void EMSCRIPTEN_KEEPALIVE start()
{
	hw.register_process(new GameEXE(&hw));
	hw.register_process(new LabEXE(&hw));
	hw.register_process(new TutorialEXE(&hw));
	hw.register_process(new MenuEXE(&hw));
	hw.register_process(new Menu2EXE(&hw));
	hw.register_process(new PlayEXE(&hw));

	hw.exec("play.exe", "");

	emscripten_set_main_loop(loop, 12, false);
}

extern "C" void EMSCRIPTEN_KEEPALIVE pressKey(uint8_t ascii, uint8_t scancode) 
{
	hw.pressKey(ascii, scancode);
}
