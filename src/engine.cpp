#include <stdint.h>
#include <emscripten.h>
#include <emscripten/bind.h>
#include <algorithm>
#include "hardware.h"
#include "tinySave.h"

using namespace emscripten;

static Hardware hw;
static TinySave tinySave;
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

int main()
{
    hw.registerProcess(&play_exe, true);
    hw.registerProcess(&menu_exe);
    hw.registerProcess(&menu2_exe);
    hw.registerProcess(&lab_exe);
    hw.registerProcess(&game_exe);
    hw.registerProcess(&tut_exe);

    emscripten_set_main_loop(loop, 0, false);

    return 0;
}

static void exec(const std::string &process, const std::string &arg)
{
	hw.output.clear();
	hw.exec(process.c_str(), arg.c_str());
	loop();
}

static void setSpeed(float speed)
{
	if (speed > 0.0f) {
		delay_multiplier = 1.0 / speed;
	}
}

static void pressKey(uint8_t ascii, uint8_t scancode) 
{
    hw.pressKey(ascii, scancode);
}

static void setJoystickAxes(int x, int y)
{
	hw.setJoystickAxes(x, y);
}

static void setJoystickButton(bool button)
{
	hw.setJoystickButton(button);
}

static bool saveGame()
{
	if (hw.process->hasFunction(SBTADDR_SAVE_GAME_FUNC)) {
		hw.process->call(SBTADDR_SAVE_GAME_FUNC, hw.process->reg);
		return true;
	}
	return false;
}

static val getMemory() {
	return val(typed_memory_view(Hardware::MEM_SIZE, hw.mem));
}

static val getJoyFile() {
	return val(typed_memory_view(sizeof hw.fs.joyfile, (uint8_t*) &hw.fs.joyfile));
}

static val getSaveFile() {
	return val(typed_memory_view(hw.fs.save.size, hw.fs.save.buffer));
}

static void setSaveFile(val buffer)
{
	uint32_t size = buffer["length"].as<uint32_t>();
	if (size >= sizeof hw.fs.save.buffer) {
		EM_ASM(throw "Save file too large";);
		return;
	}

	hw.fs.save.size = size;
	uintptr_t addr = (uintptr_t) hw.fs.save.buffer;
	val view = val::global("Uint8Array").new_(val::module_property("buffer"), addr, size);
	view.call<void>("set", buffer);
}

static val packSaveFile()
{
	tinySave.compress(&hw.fs.save.game);
	return val(typed_memory_view(tinySave.size, tinySave.buffer));
}

static void unpackSaveFile(val buffer)
{
	uint32_t size = buffer["length"].as<uint32_t>();
	if (size >= sizeof tinySave.buffer) {
		EM_ASM(throw "Compressed save file too large";);
		return;
	}

	tinySave.size = size;
	uintptr_t addr = (uintptr_t) tinySave.buffer;
	val view = val::global("Uint8Array").new_(val::module_property("buffer"), addr, size);
	view.call<void>("set", buffer);

	if (!tinySave.decompress(&hw.fs.save.game)) {
		EM_ASM(throw "Failed to decompress save file";);		
	}
}

static void setCheatsEnabled(bool enable)
{
	// Set a byte in JOYFILE to enable cheats. Game must restart to take effect.
	// This enables the CTRL-E key (via ASCII \x05) as a toggle to walk through walls in the game.
	// It disables collision detection with walls, but does not disable sentries. It's possible
	// to use this to cheat past most but not all puzzles in the game for debug purposes.
	hw.fs.joyfile.setCheatsEnabled(enable);
}

EMSCRIPTEN_BINDINGS(engine)
{
    function("exec", &exec);
    function("setSpeed", &setSpeed);
    function("pressKey", &pressKey);
    function("setJoystickAxes", &setJoystickAxes);
    function("setJoystickButton", &setJoystickButton);
    function("saveGame", &saveGame);
    function("getMemory", &getMemory);
    function("getJoyFile", &getJoyFile);
    function("getSaveFile", &getSaveFile);
    function("setSaveFile", &setSaveFile);
    function("setCheatsEnabled", &setCheatsEnabled);
    function("packSaveFile", &packSaveFile);
    function("unpackSaveFile", &unpackSaveFile);
}
