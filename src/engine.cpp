//parcel: -I. sbtProcess.cpp hardware.cpp ../build/bt_game.cpp

#include <stdint.h>
#include <emscripten.h>
#include "hardware.h"

extern "C" void EMSCRIPTEN_KEEPALIVE start() {
	GameEXE *game;
	HwMainInteractive hw;
	game = new GameEXE(&hw);
	game->run();
}