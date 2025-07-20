#include <cmath>
#include <stdint.h>
#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/html5.h>
#include <algorithm>
#include <vector>
#include <circular_buffer.hpp>
#include "hardware.h"
#include "tinySave.h"

using namespace emscripten;

static bool has_frame_callback = false;
static double engine_speed = 1.0;
static TinySave tinySave;

#define TIMESTAMP_FILTER_MAX_SAMPLES 128
#define TIMESTAMP_FILTER_MIN_SAMPLES 3
#define TIMESTAMP_DISCONTINUITY_LIMIT 290.0
static jm::circular_buffer<double, TIMESTAMP_FILTER_MAX_SAMPLES> timestamp_filter;

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

static int frameCallback(double loop_timestamp, void *)
{
    // Check the speed control, pause callbacks if the engine is paused
    const double speed = engine_speed;
    if (!(speed > 0.0)) {
        has_frame_callback = false;
        return false;
    }

    // If we detect any single discontinuity over the limit, reset the filter.
    if (!timestamp_filter.empty()) {
        const double last_timestamp = timestamp_filter.back();
        if (loop_timestamp < last_timestamp || (loop_timestamp - last_timestamp) > TIMESTAMP_DISCONTINUITY_LIMIT) {
            timestamp_filter.clear();
        }
    }

    // Finite impulse response sliding window filter for per-callback timestamps
    timestamp_filter.push_back(loop_timestamp);
    if (timestamp_filter.size() < TIMESTAMP_FILTER_MIN_SAMPLES) {
        // Wait for more samples
        return true;
    }
    const double filtered_interval = (timestamp_filter.back() - timestamp_filter.front()) / double(timestamp_filter.size());
    if (!(filtered_interval > 0. && filtered_interval < TIMESTAMP_DISCONTINUITY_LIMIT)) {
        // Wait for a realistic filter output
        return true;
    }

    // Accumulator increases when time passes, decreases when we pass delays in the output queue
    static double delay_accumulator = 0.;

    if (delay_accumulator < 0. && hw.input.checkForInputBacklog()) {
        // Speed up for keyboard input backlog
        delay_accumulator = 0.;
    }

    delay_accumulator += filtered_interval * speed;

    uint32_t saved_frame_count = outputQueue.getFrameCount();

    while (delay_accumulator >= 0.) {
        // Run the engine until we hit a delay instruction in its output queue,
        // cancelling into a pause if the process ends or was never set.
        while (true) {
            uint32_t queue_delay = outputQueue.run();
            if (queue_delay == 0) {
                if (hw.process) {
                    hw.process->run();
                    continue;
                } else {
                    has_frame_callback = false;
                    return false;
                }
            }
            delay_accumulator -= double(queue_delay);
            break;
        }
        if (saved_frame_count != outputQueue.getFrameCount()) {
            // Don't call onRenderFrame() more than once per requestAnimationFrame, add an extra delay if we are running fast.
            if (delay_accumulator >= 0.) {
                delay_accumulator = 0.;
            }
            break;
        }
    }

    return true;
}

static void resumeFrameCallbacks(void)
{
    if (!has_frame_callback) {
        has_frame_callback = true;
        timestamp_filter.clear();
        emscripten_request_animation_frame_loop(frameCallback, nullptr);
    }
}

int main()
{
    // Note that it's easy for Javascript to call other functions before main(),
    // for example when loading a game as soon as the engine loads. So this isn't
    // used for initialization, just for setting up the main loop.

    resumeFrameCallbacks();
    return 0;
}

static void exec(const std::string &process, const std::string &arg)
{
    outputQueue.clear();
    hw.exec(process.c_str(), arg.c_str());
    resumeFrameCallbacks();
}

static void setSpeed(double speed)
{
    // Stored speed for delay calculations, used on each main loop iter
    engine_speed = speed;

    // Update frameskip so we can fast-forward without being limited by draw speed
    outputQueue.setFrameSkip((unsigned) std::max(0.0, speed / 8.0));

    // This may restart a paused main loop
    if (speed > 0.0) {
        resumeFrameCallbacks();
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

static SaveStatus saveGame()
{
    return hw.saveGame();
}

static bool loadChip(uint8_t id)
{
    return hw.loadChip(id);
}

static bool loadGame()
{
    if (hw.loadGame()) {
        outputQueue.clear();
        resumeFrameCallbacks();
        return true;
    }
    return false;
}

static val getFile(const FileInfo& file)
{
    return val(typed_memory_view(file.size, file.data));
}

static val getStaticFiles()
{
    val files = val::object();

    for (const FileInfo *info = FileInfo::getAllFiles(); info->name; info++) {
        files.set(info->name, getFile(*info));
    }

    return files;
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
    uint8_t *dest_addr = compressed ? tinySave.buffer : inst.fs.save.buffer;

    if (size > max_size) {
        return false;
    }
    if (size == 0) {
        return false;
    }

    val dest_view = val(typed_memory_view(size, dest_addr));
    dest_view.call<void>("set", buffer);

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

    if (!setSaveFileWithInstance(buffer, hwAux, compressed)) {
        return val::null();
    }

    if (!hwAux.loadGame() && !hwAux.loadChipDocumentation()) {
        return val::null();
    }

    // Run until first frame
    outputAux.clear();
    do {
        assert(hwAux.process);
        hwAux.process->run();
    } while (outputAux.frame_counter == 0);

    uint8_t *image_bytes = reinterpret_cast<uint8_t*>(outputAux.draw.backbuffer);
    size_t image_byte_count = sizeof outputAux.draw.backbuffer;
    unsigned width = RGBDraw::SCREEN_WIDTH;
    unsigned height = RGBDraw::SCREEN_HEIGHT;
    val view = val(typed_memory_view(image_byte_count, image_bytes));

    // The buffer view must be re-wrapped as Uint8ClampedArray for ImageData
    size_t view_offset = view["byteOffset"].as<size_t>();
    size_t view_size = view["byteLength"].as<size_t>();
    val clamped_buffer = val::global("Uint8ClampedArray").new_(view["buffer"]);
    val clamped_slice = clamped_buffer.call<val>("slice", view_offset, view_offset + view_size);
    val image = val::global("ImageData").new_(clamped_slice, width, height);
    return image;
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
    function("loadChip", &loadChip);
    function("getMemory", &getMemory);
    function("getCompressionDictionary", &getCompressionDictionary);
    function("getStaticFiles", &getStaticFiles);
    function("getJoyFile", &getJoyFile);
    function("getSaveFile", &getSaveFile);
    function("setSaveFile", &setSaveFile);
    function("screenshotSaveFile", &screenshotSaveFile);
    function("setCheatsEnabled", &setCheatsEnabled);
    function("packSaveFile", &packSaveFile);
    function("getGameMemory", &getGameMemory);
    function("getColorMemory", &getColorMemory);
}
