//parcel: -I. -s ASSERTIONS=1 sbtProcess.cpp hardware.cpp ../build/fspack.cpp ../build/bt_game.cpp

#include <stdint.h>
#include <emscripten.h>
#include "hardware.h"

extern "C" void EMSCRIPTEN_KEEPALIVE start() {
	GameEXE *game;
	HwMainInteractive hw;
	game = new GameEXE(&hw);

	while (true) {
		int halt_code = game->run();
		fprintf(stderr, "running, halt reason = %d", halt_code);
	}
}