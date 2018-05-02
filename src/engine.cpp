//parcel: -g4 -I. sbtProcess.cpp hardware.cpp ../build/fspack.cpp ../build/bt_lab.cpp ../build/bt_menu.cpp ../build/bt_game.cpp ../build/bt_tutorial.cpp ../build/bt_play.cpp ../build/bt_renderer.cpp

#include <stdint.h>
#include <emscripten.h>
#include "hardware.h"

static SBTProcess *game;
static Hardware *hw;

void loop()
{
	game->run();
}

extern "C" void EMSCRIPTEN_KEEPALIVE start()
{
	hw = new Hardware();

	// works, first level, sewer.wor. Crashes on modal dialog or at end of level.
	game = new GameEXE(hw);

	// works, normal innovation lab
	// game = new LabEXE(hw);
	// game->exec("30");

	// First 6 tutorial levels are in tut.exe
	// game = new TutorialEXE(hw);
	// game->exec("21");

	// game = new TutorialEXE(hw);
	// game->exec("26");

	// Last tutorial is actually part of lab.exe
	// game = new LabEXE(hw);
	// game->exec("27");

	// fixme, Main entry point; hits unimplemented int21 ax=4a60
	// game = new PlayEXE(hw);

	// menu, stuck in int21 char output loop
	// game = new MenuEXE(hw);

	emscripten_set_main_loop(loop, 12, false);
}

extern "C" void EMSCRIPTEN_KEEPALIVE pressKey(uint8_t ascii, uint8_t scancode) 
{
	hw->pressKey(ascii, scancode);
}
