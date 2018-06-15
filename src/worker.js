import EngineFactory from "../build/engine.js"
import EngineWasm from "../build/engine.wasm"

// Make an engine here too. In the future when structured
// cloning of Module objects is widely supported, we can
// compile the module once and pass a copy to the UI thread.

const engine = EngineFactory({
    locateFile: function () { return EngineWasm },
});

// Listen for requests from the main thread. All messages
// require the engine to be loaded.

const handlers = {};
self.addEventListener('message', (event) => {
    engine.then(() => {
        handlers[event.data.type](engine, event.data);
    });
});

handlers['palette'] = function (engine, data)
{
    const colorMem = engine.getColorMemory();
    colorMem.cga.set(data.cga);
    colorMem.patterns.set(data.patterns);
}

handlers['game_screenshot_request'] = function (engine, data)
{
    engine.onRenderFrame = (rgb) =>
    {
        engine.pauseMainLoop();
        self.postMessage({
            type: 'game_screenshot_reply',
            image: new ImageData(new Uint8ClampedArray(rgb), 640, 400),
        });
    };

    engine.setSaveFile(data.save_file);
    engine.loadGame();
    engine.resumeMainLoop();
}
