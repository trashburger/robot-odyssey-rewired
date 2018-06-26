import * as EngineLoader from '../engineLoader.js';
import { audioContextSetup } from '../sound.js';

let mouse_tracking_unlocked = false;
let end_tracking_timer = null;

const canvas = document.getElementById('framebuffer');
const game_area = document.getElementById('game_area');

export function mouseTrackingEnd(afterDelay)
{
    // After an optional delay, prevent automatic mouse tracking
    // without an explicit unlock click.

    if (end_tracking_timer) {
        clearTimeout(end_tracking_timer);
        end_tracking_timer = null;
    }
    end_tracking_timer = setTimeout(async () => {
        if (mouse_tracking_unlocked) {
            mouse_tracking_unlocked = false;
            const engine = await EngineLoader.complete;
            engine.endMouseTracking();
        }
    }, afterDelay || 0);
}

function mouseTrackingContinue()
{
    // Don't start tracking if we aren't already, but prevent
    // tracking from being stopped if an end is scheduled.

    if (end_tracking_timer) {
        clearTimeout(end_tracking_timer);
        end_tracking_timer = null;
    }
}

function mouseTrackingBegin()
{
    // Start tracking the mouse because of an explicit click.

    mouseTrackingContinue();
    mouse_tracking_unlocked = true;
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

export function init()
{
    const engine = EngineLoader.instance;

    game_area.addEventListener('mousemove', function (e)
    {
        if (mouse_tracking_unlocked && engine.calledRun) {
            const loc = mouseLocationForEvent(e);
            engine.setMouseTracking(loc.x, loc.y);
            engine.autoSave();
            mouseTrackingContinue();
        }
    });

    game_area.addEventListener('mousedown', function (e)
    {
        if (e.button === 0 && engine.calledRun) {
            e.preventDefault();
            if (mouse_tracking_unlocked) {
                // Already unlocked, this is a click
                engine.setMouseButton(true);
                mouseTrackingContinue();
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
            }
            // Unlock and/or prevent re-locking
            mouseTrackingBegin();
        }
        audioContextSetup();
    });

    game_area.addEventListener('mouseleave', () => mouseTrackingEnd(2000));

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
