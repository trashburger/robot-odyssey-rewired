#include <stdint.h>
#include <emscripten.h>
#include <algorithm>
#include "hardware.h"

static Hardware hw;
static float delay_multiplier = 1.0f;

SBT_DECL_PROCESS(PlayEXE);
static PlayEXE play_exe(&hw);

SBT_DECL_PROCESS(MenuEXE);
static MenuEXE menu_exe(&hw);

SBT_DECL_PROCESS(Menu2EXE);
static Menu2EXE menu2_exe(&hw);

SBT_DECL_PROCESS(LabEXE);
static LabEXE lab_exe(&hw);

SBT_DECL_PROCESS(GameEXE);
static GameEXE game_exe(&hw);

SBT_DECL_PROCESS(TutorialEXE);
static TutorialEXE tut_exe(&hw);

static void loop()
{
    uint32_t millis = hw.run();
    emscripten_set_main_loop_timing(EM_TIMING_SETTIMEOUT, millis * delay_multiplier);
}

extern "C" void EMSCRIPTEN_KEEPALIVE start()
{
    hw.registerProcess(&play_exe, true);
    hw.registerProcess(&menu_exe);
    hw.registerProcess(&menu2_exe);
    hw.registerProcess(&lab_exe);
    hw.registerProcess(&game_exe);
    hw.registerProcess(&tut_exe);

    emscripten_set_main_loop(loop, 0, false);
}

extern "C" void EMSCRIPTEN_KEEPALIVE exec(const char *process, const char *arg)
{
	hw.output.clear();
	hw.exec(process, arg);
	loop();
}

extern "C" void EMSCRIPTEN_KEEPALIVE setSpeed(float speed)
{
	if (speed > 0.0f) {
		delay_multiplier = 1.0 / speed;
	}
}

extern "C" void EMSCRIPTEN_KEEPALIVE pressKey(uint8_t ascii, uint8_t scancode) 
{
    hw.pressKey(ascii, scancode);
}

extern "C" void EMSCRIPTEN_KEEPALIVE setJoystickAxes(int x, int y)
{
	hw.setJoystickAxes(x, y);
}

extern "C" void EMSCRIPTEN_KEEPALIVE setJoystickButton(bool button)
{
	hw.setJoystickButton(button);
}

extern "C" void EMSCRIPTEN_KEEPALIVE saveGame()
{
	if (hw.process->hasFunction(SBTADDR_SAVE_GAME_FUNC)) {
		hw.process->call(SBTADDR_SAVE_GAME_FUNC, hw.process->reg);
	}
}


extern "C" uint8_t* EMSCRIPTEN_KEEPALIVE memPointer()
{
	return hw.mem;
}

extern "C" uint32_t EMSCRIPTEN_KEEPALIVE memSize()
{
	return Hardware::MEM_SIZE;
}

extern "C" uint8_t* EMSCRIPTEN_KEEPALIVE joyFilePointer()
{
	return (uint8_t*) &hw.fs.joyfile;
}

extern "C" uint8_t* EMSCRIPTEN_KEEPALIVE saveFilePointer()
{
	return hw.fs.save.buffer;
}

extern "C" uint32_t EMSCRIPTEN_KEEPALIVE getSaveFileSize()
{
	return hw.fs.save.size;
}

extern "C" void EMSCRIPTEN_KEEPALIVE setSaveFileSize(uint32_t size)
{
	hw.fs.save.size = size;
}

extern "C" void EMSCRIPTEN_KEEPALIVE setCheatsEnabled(bool enable)
{
	// Set a byte in JOYFILE to enable cheats. Game must restart to take effect.
	// This enables the CTRL-E key (via ASCII \x05) as a toggle to walk through walls in the game.
	// It disables collision detection with walls, but does not disable sentries. It's possible
	// to use this to cheat past most but not all puzzles in the game for debug purposes.
	hw.fs.joyfile.setCheatsEnabled(enable);
}
