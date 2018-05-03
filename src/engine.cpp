//parcel: -g4 -I. sbt86.cpp hardware.cpp ../build/fspack.cpp ../build/bt_lab.cpp ../build/bt_menu.cpp ../build/bt_menu2.cpp ../build/bt_game.cpp ../build/bt_tutorial.cpp ../build/bt_play.cpp ../build/bt_renderer.cpp

#include <stdint.h>
#include <emscripten.h>
#include "hardware.h"

static SBTProcess *game;
static Hardware hw;

void loop()
{
	// fprintf(stderr, "entering run\n");
	game->run();
	// fprintf(stderr, "left run\n");
}

extern "C" void EMSCRIPTEN_KEEPALIVE start()
{
	// works, first level, sewer.wor. Crashes on modal dialog or at end of level.
	game = new GameEXE(&hw);

	// works, normal innovation lab
	// game = new LabEXE(&hw);
	// game->exec("30");

	// First 6 tutorial levels are in tut.exe
	// game = new TutorialEXE(&hw);
	// game->exec("21");

	// game = new TutorialEXE(&hw);
	// game->exec("26");

	// Last tutorial is actually part of lab.exe
	// game = new LabEXE(&hw);
	// game->exec("27");

	// fixme, Main entry point. Needs patches to break up main loop at each exec_wrapper call site
	// game = new PlayEXE(&hw);

	// menu, kinda works, return code good, cut scenes still not drawing/running really
	// game = new MenuEXE(&hw);

	emscripten_set_main_loop(loop, 12, false);
}

extern "C" void EMSCRIPTEN_KEEPALIVE pressKey(uint8_t ascii, uint8_t scancode) 
{
	hw.pressKey(ascii, scancode);
}
