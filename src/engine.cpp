//parcel: -g4 -I. sbtProcess.cpp hardware.cpp ../build/fspack.cpp ../build/bt_lab.cpp ../build/bt_menu.cpp ../build/bt_game.cpp ../build/bt_play.cpp ../build/bt_renderer.cpp

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

	game = new LabEXE(hw);
	game->exec("30");

	// game = new GameEXE(hw);

	emscripten_set_main_loop(loop, 12, false);
}

extern "C" void EMSCRIPTEN_KEEPALIVE pressKey(uint8_t ascii, uint8_t scancode) 
{
	hw->pressKey(ascii, scancode);
}
