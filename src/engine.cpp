#include <stdint.h>
#include <emscripten.h>
#include <emscripten/bind.h>
#include <algorithm>
#include <vector>
#include "hardware.h"
#include "tinySave.h"

using namespace emscripten;

static Hardware hw;
static TinySave tinySave;

SBT_STATIC_PROCESS(&hw, ShowEXE);
SBT_STATIC_PROCESS(&hw, Show2EXE);
SBT_STATIC_PROCESS(&hw, LabEXE);
SBT_STATIC_PROCESS(&hw, GameEXE);
SBT_STATIC_PROCESS(&hw, TutorialEXE);

static float delay_multiplier = 1.0f;

static void loop()
{
    // Runs until the next queued delay
    uint32_t millis = hw.run();

    if (hw.input.checkForInputBacklog()) {
        // Speed up for keyboard input backlog
        emscripten_set_main_loop_timing(EM_TIMING_RAF, 1);
    } else {
        // Millisecond-based timing, with optional modifier
        emscripten_set_main_loop_timing(EM_TIMING_SETTIMEOUT, millis * delay_multiplier);
    }
}

int main()
{
    // Note that it's easy for Javascript to call other functions before main(),
    // for example when loading a game as soon as the engine loads. So this isn't
    // used for initialization, just for setting up the main loop.

    emscripten_set_main_loop(loop, 0, false);
    return 0;
}

static void exec(const std::string &process, const std::string &arg)
{
    hw.output.clear();
    hw.exec(process.c_str(), arg.c_str());
}

static void setSpeed(float speed)
{
    if (speed > 0.0f) {
        delay_multiplier = 1.0 / speed;
    }
}

static void pressKey(uint8_t ascii, uint8_t scancode)
{
    hw.output.skipDelay();
    hw.input.pressKey(ascii, scancode);
}

static void setJoystickAxes(int x, int y)
{
    hw.input.setJoystickAxes(x, y);
}

static void setJoystickButton(bool button)
{
    if (button) {
        hw.output.skipDelay();
    }
    hw.input.setJoystickButton(button);
}

static void setMouseTracking(int x, int y)
{
    hw.input.setMouseTracking(x, y);
}

static void setMouseButton(bool button)
{
    if (button) {
        hw.output.skipDelay();
    }
    hw.input.setMouseButton(button);
}

static void endMouseTracking()
{
    hw.input.endMouseTracking();
}

enum class SaveStatus {
    OK,
    NOT_SUPPORTED,
    BLOCKED,
};

static SaveStatus saveGame()
{
    if (!hw.process) {
        // Not running at all
        return SaveStatus::NOT_SUPPORTED;
    }

    if (!hw.process->hasFunction(SBTADDR_SAVE_GAME_FUNC)) {
        // No save function in this process
        return SaveStatus::NOT_SUPPORTED;
    }

    if (!hw.process->isWaitingInMainLoop()) {
        // Can't safely interrupt the process
        return SaveStatus::BLOCKED;
    }

    hw.fs.save.file.size = 0;
    hw.process->call(SBTADDR_SAVE_GAME_FUNC, hw.process->reg);

    if (!hw.fs.save.isGame()) {
        // File isn't the right size
        return SaveStatus::NOT_SUPPORTED;
    }

    if (!hw.fs.save.asGame().getProcessName()) {
        // File isn't something we know how to load.
        // (Tutorial 6 runs in LAB.EXE, which knows how to save, but we can't load those files.)
        return SaveStatus::NOT_SUPPORTED;
    }

    return SaveStatus::OK;
}

static bool loadGame()
{
    // If the buffer contains a loadable game, loads it and returns true.
    if (hw.fs.save.isGame()) {
        const char *process = hw.fs.save.asGame().getProcessName();
        if (process) {
            exec(process, "99");
            return true;
        }
    }
    return false;
}

static val getFile(const FileInfo& file)
{
    return val(typed_memory_view(file.size, file.data));
}

static val getMemory()
{
    return val(typed_memory_view(Hardware::MEM_SIZE, hw.mem));
}

static val getCompressionDictionary()
{
    const std::vector<uint8_t>& dict = tinySave.getCompressionDictionary();
    return val(typed_memory_view(dict.size(), &dict[0]));
}

static val getJoyFile()
{
    return getFile(hw.fs.config.file);
}

static val getSaveFile()
{
    return getFile(hw.fs.save.file);
}

static void setSaveFile(val buffer)
{
    uint32_t size = buffer["length"].as<uint32_t>();
    if (size >= sizeof hw.fs.save.buffer) {
        EM_ASM(throw "Save file too large";);
        return;
    }

    hw.fs.save.file.size = size;
    if (size > 0) {
        uintptr_t addr = (uintptr_t) hw.fs.save.buffer;
        val view = val::global("Uint8Array").new_(val::module_property("buffer"), addr, size);
        view.call<void>("set", buffer);
    }
}

static val packSaveFile()
{
    tinySave.compress(hw.fs.save.file);
    return val(typed_memory_view(tinySave.size, tinySave.buffer));
}

static bool unpackSaveFile(val buffer)
{
    uint32_t size = buffer["length"].as<uint32_t>();
    if (size >= sizeof tinySave.buffer) {
        EM_ASM(throw "Compressed save file too large";);
        return false;
    }

    tinySave.size = size;
    if (size > 0) {
        uintptr_t addr = (uintptr_t) tinySave.buffer;
        val view = val::global("Uint8Array").new_(val::module_property("buffer"), addr, size);
        view.call<void>("set", buffer);
    }

    return tinySave.decompress(hw.fs.save.file);
}

static void setCheatsEnabled(bool enable)
{
    // Set a byte in JOYFILE to enable cheats. Game must restart to take effect.
    // This enables the CTRL-E key (via ASCII \x05) as a toggle to walk through walls in the game.
    // It disables collision detection with walls, but does not disable sentries. It's possible
    // to use this to cheat past most but not all puzzles in the game for debug purposes.
    hw.fs.config.joyfile.setCheatsEnabled(enable);
}

static val getGameMemory()
{
    // Get a JS representation of the current ROData, with direct views into memory.
    // Just for exploration/fun currently.

    ROData d;
    if (!hw.process || !d.fromProcess(hw.process)) {
        return val::null();
    }

    val robots = val::array();
    for (unsigned i = 0; i < d.robots.count; i++) {
        val bot = val::object();
        bot.set("state", val(typed_memory_view(sizeof(RORobot), reinterpret_cast<uint8_t*>(&d.robots.state[i]))));
        bot.set("grabbers", val(typed_memory_view(sizeof(RORobotGrabber), reinterpret_cast<uint8_t*>(&d.robots.grabbers[i]))));
        bot.set("batteryAcc", val(typed_memory_view(sizeof(RORobotBatteryAcc), reinterpret_cast<uint8_t*>(&d.robots.batteryAcc[i]))));
        robots.set(i, bot);
    }

    val r = val::object();
    r.set("world", val(typed_memory_view(sizeof(ROWorld), reinterpret_cast<uint8_t*>(d.world))));
    r.set("circuit", val(typed_memory_view(sizeof(ROCircuit), reinterpret_cast<uint8_t*>(d.circuit))));
    r.set("robots", robots);

    return r;
}

static val getColorMemory()
{
    val r = val::object();
    r.set("cga", val(typed_memory_view(sizeof hw.output.cga_palette / sizeof hw.output.cga_palette[0], hw.output.cga_palette)));
    r.set("patterns", val(typed_memory_view(sizeof hw.output.draw.patterns / sizeof hw.output.draw.patterns[0], hw.output.draw.patterns)));
    return r;
}

EMSCRIPTEN_BINDINGS(engine)
{
    constant("MAX_FILESIZE", (unsigned) DOSFilesystem::MAX_FILESIZE);
    constant("MEM_SIZE", (unsigned) Hardware::MEM_SIZE);
    constant("CPU_CLOCK_HZ", (unsigned) OutputQueue::CPU_CLOCK_HZ);
    constant("AUDIO_HZ", (unsigned) OutputQueue::AUDIO_HZ);
    constant("SCREEN_WIDTH", (unsigned) OutputQueue::SCREEN_WIDTH);
    constant("SCREEN_HEIGHT", (unsigned) OutputQueue::SCREEN_HEIGHT);
    constant("SCREEN_TILE_SIZE", (unsigned) RGBDraw::SCREEN_TILE_SIZE);

    enum_<SaveStatus>("SaveStatus")
        .value("OK", SaveStatus::OK)
        .value("NOT_SUPPORTED", SaveStatus::NOT_SUPPORTED)
        .value("BLOCKED", SaveStatus::BLOCKED)
        ;

    function("exec", &exec);
    function("setSpeed", &setSpeed);
    function("pressKey", &pressKey);
    function("setJoystickAxes", &setJoystickAxes);
    function("setJoystickButton", &setJoystickButton);
    function("setMouseTracking", &setMouseTracking);
    function("setMouseButton", &setMouseButton);
    function("endMouseTracking", &endMouseTracking);
    function("saveGame", &saveGame);
    function("loadGame", &loadGame);
    function("getMemory", &getMemory);
    function("getCompressionDictionary", &getCompressionDictionary);
    function("getJoyFile", &getJoyFile);
    function("getSaveFile", &getSaveFile);
    function("setSaveFile", &setSaveFile);
    function("setCheatsEnabled", &setCheatsEnabled);
    function("packSaveFile", &packSaveFile);
    function("unpackSaveFile", &unpackSaveFile);
    function("getGameMemory", &getGameMemory);
    function("getColorMemory", &getColorMemory);
}
