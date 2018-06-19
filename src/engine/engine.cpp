#include <stdint.h>
#include <emscripten.h>
#include <emscripten/bind.h>
#include <algorithm>
#include <vector>
#include "hardware.h"
#include "tinySave.h"

using namespace emscripten;

static bool has_main_loop = false;
static float engine_speed = 1.0f;
static TinySave tinySave;

SBT_DECL_PROCESS(ShowEXE);
SBT_DECL_PROCESS(Show2EXE);
SBT_DECL_PROCESS(LabEXE);
SBT_DECL_PROCESS(GameEXE);
SBT_DECL_PROCESS(TutorialEXE);

// Main hardware instance, for running the game
static ColorTable colorTable;
static OutputQueue outputQueue(colorTable);
static Hardware hw(outputQueue);
SBT_STATIC_PROCESS(hw, ShowEXE);
SBT_STATIC_PROCESS(hw, Show2EXE);
SBT_STATIC_PROCESS(hw, LabEXE);
SBT_STATIC_PROCESS(hw, GameEXE);
SBT_STATIC_PROCESS(hw, TutorialEXE);

// Auxiliary hardware instance, for screenshots. Shares main color table.
static OutputMinimal outputAux(colorTable);
static Hardware hwAux(outputAux);
SBT_STATIC_PROCESS(hwAux, LabEXE);
SBT_STATIC_PROCESS(hwAux, GameEXE);


static void loop()
{
    // Run the main hardware instance until the next queued delay

    unsigned delay_accum = 0;
    const float speed = engine_speed;
    const unsigned minimum_delay_milliseconds = 10;

    if (!(speed > 0.0f)) {
        // Engine paused via speed control
        emscripten_pause_main_loop();
        return;
    }

    while (true) {
        unsigned queue_delay = outputQueue.run();

        if (queue_delay == 0) {
            if (hw.process) {
                hw.process->run();
            } else {
                // Engine paused until exec
                emscripten_pause_main_loop();
                return;
            }
        }

        delay_accum += queue_delay;
        unsigned adjusted_delay = delay_accum / engine_speed;

        if (adjusted_delay >= minimum_delay_milliseconds) {
            if (hw.input.checkForInputBacklog()) {
                // Speed up for keyboard input backlog
                emscripten_set_main_loop_timing(EM_TIMING_RAF, 1);
            } else {
                // Millisecond-based timing, with optional modifier
                emscripten_set_main_loop_timing(EM_TIMING_SETTIMEOUT, adjusted_delay);
            }
            return;
        }
    }
}

int main()
{
    // Note that it's easy for Javascript to call other functions before main(),
    // for example when loading a game as soon as the engine loads. So this isn't
    // used for initialization, just for setting up the main loop.

    emscripten_set_main_loop(loop, 0, false);
    has_main_loop = true;
    return 0;
}

static void exec(const std::string &process, const std::string &arg)
{
    outputQueue.clear();
    hw.exec(process.c_str(), arg.c_str());
    if (has_main_loop) {
        emscripten_resume_main_loop();
    }
}

static void setSpeed(float speed)
{
    // Stored speed for delay calculations, used on each main loop iter
    engine_speed = speed;

    // Update frameskip so we can fast-forward without being limited by draw speed
    outputQueue.setFrameSkip((unsigned) std::max(0.0f, speed / 8.0f));

    // This may restart a paused main loop
    if (has_main_loop && speed > 0.0f) {
        emscripten_resume_main_loop();
    }
}

static void pressKey(uint8_t ascii, uint8_t scancode)
{
    outputQueue.skipDelay();
    hw.input.pressKey(ascii, scancode);
}

static void setJoystickAxes(float x, float y)
{
    hw.input.setJoystickAxes(x, y);
}

static void setJoystickButton(bool button)
{
    if (button) {
        outputQueue.skipDelay();
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
        outputQueue.skipDelay();
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
    if (hw.loadGame()) {
        outputQueue.clear();
        if (has_main_loop) {
            emscripten_resume_main_loop();
        }
        return true;
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

static bool setSaveFileWithInstance(val buffer, Hardware &inst, bool compressed)
{
    uint32_t size = buffer["length"].as<uint32_t>();
    uint32_t max_size = compressed ? sizeof tinySave.buffer : sizeof inst.fs.save.buffer;
    uintptr_t addr = compressed ? (uintptr_t) tinySave.buffer : (uintptr_t) inst.fs.save.buffer;

    if (size > max_size) {
        return false;
    }
    if (size == 0) {
        return false;
    }

    val view = val::global("Uint8Array").new_(val::module_property("buffer"), addr, size);
    view.call<void>("set", buffer);

    if (compressed) {
        tinySave.size = size;
        return tinySave.decompress(inst.fs.save.file);
    } else {
        inst.fs.save.file.size = size;
        return true;
    }
}

static bool setSaveFile(val buffer, bool compressed)
{
    return setSaveFileWithInstance(buffer, hw, compressed);
}

static val screenshotSaveFile(val buffer, bool compressed)
{
    // Load the save file within our auxiliary hardware instance, and run until the first frame.
    // Has no effect on the main game instance. Returns null if the save file can't be loaded.

    if (setSaveFileWithInstance(buffer, hwAux, compressed) && hwAux.loadGame()) {

        // Run until first frame
        outputAux.clear();
        do {
            assert(hwAux.process);
            hwAux.process->run();
        } while (outputAux.frame_counter == 0);

        uintptr_t addr = (uintptr_t) outputAux.draw.backbuffer;
        size_t size = sizeof outputAux.draw.backbuffer;
        unsigned width = RGBDraw::SCREEN_WIDTH;
        unsigned height = RGBDraw::SCREEN_HEIGHT;
        val view = val::global("Uint8ClampedArray").new_(val::module_property("buffer"), addr, size);
        val image = val::global("ImageData").new_(view, width, height);
        return image;

    } else {
        return val::null();
    }
}

static val packSaveFile()
{
    tinySave.compress(hw.fs.save.file);
    return val(typed_memory_view(tinySave.size, tinySave.buffer));
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
    r.set("cga", val(typed_memory_view(sizeof colorTable.cga / sizeof colorTable.cga[0], colorTable.cga)));
    r.set("patterns", val(typed_memory_view(sizeof colorTable.patterns / sizeof colorTable.patterns[0], colorTable.patterns)));
    return r;
}

EMSCRIPTEN_BINDINGS(engine)
{
    constant("MAX_FILESIZE", (unsigned) DOSFilesystem::MAX_FILESIZE);
    constant("MEM_SIZE", (unsigned) Hardware::MEM_SIZE);
    constant("CPU_CLOCK_HZ", (unsigned) OutputQueue::CPU_CLOCK_HZ);
    constant("AUDIO_HZ", (unsigned) OutputQueue::AUDIO_HZ);
    constant("SCREEN_WIDTH", (unsigned) RGBDraw::SCREEN_WIDTH);
    constant("SCREEN_HEIGHT", (unsigned) RGBDraw::SCREEN_HEIGHT);
    constant("SCREEN_TILE_SIZE", (unsigned) ColorTable::SCREEN_TILE_SIZE);

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
    function("screenshotSaveFile", &screenshotSaveFile);
    function("setCheatsEnabled", &setCheatsEnabled);
    function("packSaveFile", &packSaveFile);
    function("getGameMemory", &getGameMemory);
    function("getColorMemory", &getColorMemory);
}
