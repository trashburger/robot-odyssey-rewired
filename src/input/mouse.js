import { audioContextSetup } from '../sound.js';

let mouse_tracking_unlocked = false;
let end_tracking_func = null;

const canvas = document.getElementById('framebuffer');
const game_area = document.getElementById('game_area');

export function mouseTrackingEnd()
{
    if (mouse_tracking_unlocked && end_tracking_func) {
        end_tracking_func();
    }
    mouse_tracking_unlocked = false;
}

export function mouseLocationForEvent(e)
{
    const canvas_width = canvas.width;
    const canvas_height = canvas.height;

    const canvasRect = canvas.getBoundingClientRect();
    const border = 2;

    const hotspot_x = 2;
    const hotspot_y = 4;
    const sprite_width = 7;
    const sprite_height = 16;

    const game_width = 160;
    const game_height = 192;
    const cga_zoom = 2;

    const doorway_border = 8;

    const canvasX = (e.clientX - canvasRect.left) * canvas_width / canvasRect.width;
    const canvasY = (e.clientY - canvasRect.top) * canvas_height / canvasRect.height;
    const framebufferX = canvasX - border;
    const framebufferY = canvasY - border;

    // Adjust for cursor hotspot and game coordinate system
    let x = framebufferX / cga_zoom / 2 - hotspot_x;
    let y = game_height - framebufferY / cga_zoom - hotspot_y;

    // Make it easier to get through doorways; if the cursor
    // is near a screen border, push it past the border.
    if (x <= doorway_border) x = -1;
    if (x >= game_width - sprite_width - doorway_border) x = game_width+1;
    if (y <= doorway_border) y = -1;
    if (y >= game_height - sprite_height - doorway_border) y = game_height+1;

    return { x, y };
}

export function init(engine)
{
    engine.then(() => {
        end_tracking_func = () => engine.endMouseTracking();
    });

    game_area.addEventListener('mousemove', function (e)
    {
        if (mouse_tracking_unlocked && engine.calledRun) {
            const loc = mouseLocationForEvent(e);
            engine.setMouseTracking(loc.x, loc.y);
            engine.autoSave();
        }
    });

    game_area.addEventListener('mousedown', function (e)
    {
        if (e.button === 0 && engine.calledRun) {
            e.preventDefault();
            if (mouse_tracking_unlocked) {
                // Already unlocked, this is a click
                engine.setMouseButton(true);
            } else {
                // Move to this location, and unlock tracking on mouseup
                const loc = mouseLocationForEvent(e);
                engine.setMouseTracking(loc.x, loc.y);
            }
            engine.autoSave();
        }
        audioContextSetup();
        canvas.focus();
    });

    game_area.addEventListener('mouseup', function (e)
    {
        if (e.button === 0 && engine.calledRun) {
            e.preventDefault();
            if (mouse_tracking_unlocked) {
                // Already unlocked
                engine.setMouseButton(false);
                engine.autoSave();
            } else {
                // Unlock now; already moved to the location on mousedown
                mouse_tracking_unlocked = true;
            }
        }
        audioContextSetup();
    });

    game_area.addEventListener('mouseleave', mouseTrackingEnd);

    let touch_timer = null;
    const stopTouchTimer = () => {
        if (touch_timer !== null) {
            clearTimeout(touch_timer);
            touch_timer = null;
        }
    };

    canvas.addEventListener('touchstart', function (e)
    {
        stopTouchTimer();
        if (engine.calledRun) {
            const loc = mouseLocationForEvent(e.targetTouches[0]);
            engine.setMouseTracking(loc.x, loc.y);
            engine.autoSave();

            // Short touches only move, slightly longer touches also engage the button
            const long_touch_threshold = 160;
            touch_timer = setInterval(function () {
                engine.setMouseButton(true);
            }, long_touch_threshold);
        }
        audioContextSetup();
        e.preventDefault();
        canvas.focus();
    });

    canvas.addEventListener('touchmove', function (e)
    {
        if (engine.calledRun) {
            const loc = mouseLocationForEvent(e.targetTouches[0]);
            engine.setMouseTracking(loc.x, loc.y);
        }
        e.preventDefault();
    });

    canvas.addEventListener('touchend', function (e)
    {
        stopTouchTimer();
        if (engine.calledRun) {
            engine.setMouseButton(false);
            engine.autoSave();
        }
        e.preventDefault();
    });

    canvas.addEventListener('touchcancel', function (e)
    {
        stopTouchTimer();
        mouseTrackingEnd();
        e.preventDefault();
    });
}
