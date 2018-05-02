//parcel: -g4 -I. sbtProcess.cpp hardware.cpp ../build/fspack.cpp ../build/bt_lab.cpp ../build/bt_menu.cpp ../build/bt_game.cpp ../build/bt_play.cpp ../build/bt_renderer.cpp

#include <stdint.h>
#include <emscripten.h>
#include "hardware.h"

static GameEXE *game;
static HwMainInteractive *hw;

void loop() {
	fprintf(stderr, "enter game->run\n");
	game->run();
	fprintf(stderr, "exit game->run\n");
}
	
extern "C" void EMSCRIPTEN_KEEPALIVE start() {
	hw = new HwMainInteractive();
	game = new GameEXE(hw);
	//emscripten_set_main_loop(loop, 15, false);
}

extern "C" void EMSCRIPTEN_KEEPALIVE tick() {	
	loop();
}
