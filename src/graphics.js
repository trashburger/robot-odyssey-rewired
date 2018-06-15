import * as GameMenu from './game_menu.js'
import './graphics.css'

export function init(engine)
{
    const width = 640;
    const height = 400;
    const visible_height = 384;
    const border = 2;

    const canvas = document.getElementById('framebuffer');
    canvas.width = width + border*2;
    canvas.height = visible_height + border*2;

    const context = canvas.getContext('2d');
    const image = context.createImageData(width, height);

    // The first putImageData can take a while on Chrome? Do this at init
    // rather than causing a delay later on when the game is opening.
    context.putImageData(image, border, border);

    engine.onRenderFrame = function (rgb)
    {
        image.data.set(rgb);
        context.putImageData(image, border, border);
        GameMenu.setState(GameMenu.States.EXEC);
    };

    engine.screenshotSavedGame = function (save_file)
    {
        engine.worker.onmessage = function (e) {
            console.log(e);
        }
        engine.worker.postMessage({
            type: 'game_screenshot_request',
            save_file,
        });
    }

    canvas.focus();
}
