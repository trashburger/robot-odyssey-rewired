import nipplejs from 'nipplejs';
import { audioContextSetup } from "./sound.js"
import * as GameMenu from "./game_menu.js"
import './input.css'

let joystick = null;

const canvas = document.getElementById('framebuffer');
const game_area = document.getElementById('game_area');
const gamepad_button_mappings = [];

function controlCode(key)
{
    return String.fromCharCode(key.toUpperCase().charCodeAt(0) - 'A'.charCodeAt(0) + 1)
}

export function init(engine)
{
    let mouse_tracking_unlocked = false;

    joystick = nipplejs.create({
        zone: document.getElementById('joystick_xy'),
        mode: 'static',
        size: 100,
        threshold: 0.01,
        position: { left: '50%', top: '50%' }
    });

    // Fade in the controls as soon as this Javascript is ready
    document.getElementById('engine_controls').classList.remove('hidden');

    // Gamepads must be polled; currently this isn't hooked into the game's own event polling
    // cycle at all, since that happens on the C++ side without any JS callbacks.
    function gamepadPoll(gamepad, last_axes, last_pressed)
    {
        if (gamepad.connected) {
            const axes = gamepad.axes;
            const pressed = gamepad.buttons.map(b => b.pressed);

            window.requestAnimationFrame( function () {
                // Must call getGamepads() to refresh the state snapshot
                gamepadPoll(navigator.getGamepads()[gamepad.index], axes, pressed);
            });

            // If any axes changed, send an XY event
            if (axes[0] !== last_axes[0] || axes[1] !== last_axes[1]) {
                gamepadAxesChanged(axes[0], axes[1]);
            }

            // Dispatch individual button events to mapping handlers
            for (let i = 0; i < pressed.length; i++) {
                if (pressed[i] !== last_pressed[i]) {
                    const handler = gamepad_button_mappings[i];
                    if (handler) {
                        handler(pressed[i], i);
                    }
                }
            }
        }
    }

    function gamepadAxesChanged(x, y)
    {
        // Pass it on to the engine and menu
        const scale = 10;
        joystickAxes(x * scale, y * scale);

        // Animate the on-screen joystick
        const js = joystick[0];
        const size = js.options.size * 0.25;
        const threshold = 0.1;
        js.ui.front.style.left = (x * size) + 'px';
        js.ui.front.style.top = (y * size) + 'px';
        if (x*x + y*y > threshold*threshold) {
            js.show();
        } else {
            js.hide();
        }
    }

    // Begin polling gamepads if a new one appears
    window.addEventListener('gamepadconnected', function (e)
    {
        gamepadPoll(e.gamepad, [], []);
    });

    function mouseTrackingEnd()
    {
        if (mouse_tracking_unlocked && engine.calledRun) {
            // Avoid calling engine.endMouseTracking unless we were using it, since it will
            // reset the joystick state to the detriment of multitouch support.
            engine.endMouseTracking();
        }
        mouse_tracking_unlocked = false;
    }

    function mouseLocationForEvent(e)
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

        const canvasX = (e.clientX - canvasRect.x) * canvas_width / canvasRect.width;
        const canvasY = (e.clientY - canvasRect.y) * canvas_height / canvasRect.height;
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
        if (e.button == 0 && engine.calledRun) {
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
    });

    game_area.addEventListener('mouseup', function (e)
    {
        if (e.button == 0 && engine.calledRun) {
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

    canvas.addEventListener('touchstart', function (e)
    {
        if (engine.calledRun) {
            const loc = mouseLocationForEvent(e.targetTouches[0]);
            engine.setMouseTracking(loc.x, loc.y);
            engine.setMouseButton(true);
            engine.autoSave();
        }
        audioContextSetup();
        e.preventDefault();
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
        if (engine.calledRun) {
            engine.setMouseButton(false);
            engine.autoSave();
        }
        e.preventDefault();
    });

    canvas.addEventListener('touchcancel', function (e)
    {
        if (engine.calledRun) {
            engine.setMouseButton(false);
            engine.autoSave();
        }
        e.preventDefault();
    });

    function addButtonEvents(button_element, down, up, click)
    {
        const down_wrapper = function (e)
        {
            mouseTrackingEnd();
            audioContextSetup();
            if (!click) {
                e.preventDefault();
            }
            if (down) {
                down(e);
            }
        };

        const up_wrapper = function (e)
        {
            if (!click) {
                e.preventDefault();
            }
            if (up) {
                up(e);
            }
            button_element.blur();
            canvas.focus();
        };

        const options = {
            passive: !!click
        };

        button_element.addEventListener('mousedown', down_wrapper, options);
        button_element.addEventListener('mouseup', up_wrapper, options);
        button_element.addEventListener('mouseleave', up_wrapper, options);

        button_element.addEventListener('touchstart', down_wrapper, options);
        button_element.addEventListener('touchend', up_wrapper, options);
        button_element.addEventListener('touchcancel', up_wrapper, options);

        if (click) {
            button_element.addEventListener('click', click);
        }

        const gamepad_button = parseInt(button_element.dataset.gamepad);
        if (gamepad_button >= 0) {
            gamepad_button_mappings[gamepad_button] = function (pressed)
            {
                if (pressed) {
                    down();
                } else {
                    if (click) {
                        click();
                    }
                    up();
                }
            }
        }
    }

    function keycode(ascii, scancode)
    {
        if (typeof(ascii) != typeof(0)) {
            ascii = ascii.length == 1 ? ascii.charCodeAt(0) : parseInt(ascii, 0);
        }
        if (engine.calledRun) {
            engine.pressKey(ascii, scancode);
            engine.autoSave();
        }
        GameMenu.pressKey(engine, ascii, scancode);
        audioContextSetup();
    }

    function joystickAxes(x, y)
    {
        // This limit is much larger than the usable range, it's for preventing overflow
        const limit = 127;
        x = Math.min(limit, Math.max(-limit, x));
        y = Math.min(limit, Math.max(-limit, y));

        if (engine.calledRun) {
            engine.setJoystickAxes(x, y);
            engine.autoSave();
        }
        GameMenu.setJoystickAxes(engine, x, y);
    }

    function joystickButton(b)
    {
        if (engine.calledRun) {
            engine.setJoystickButton(b);
            engine.autoSave();
        }
        GameMenu.setJoystickButton(engine, b);
    }

    document.body.addEventListener('keydown', function (e)
    {
        const code = e.code || e.key || '';
        const key = e.key || '';
        const shift = e.shiftKey;
        const ctrl = e.ctrlKey;
        const alt = e.altKey;
        const meta = e.metaKey;
        const plain = !(ctrl || alt || meta);

        if (code.includes("Arrow")) {
            // Most keys can coexist with mouse tracking, but arrow keys will take over.
            mouseTrackingEnd();
        }

        if (code == "ArrowUp" && !shift)         keycode(0, 0x48);
        else if (code == "ArrowUp" && shift)     keycode('8', 0x48);
        else if (code == "ArrowDown" && !shift)  keycode(0, 0x50);
        else if (code == "ArrowDown" && shift)   keycode('2', 0x50);
        else if (code == "ArrowLeft" && !shift)  keycode(0, 0x4B);
        else if (code == "ArrowLeft" && shift)   keycode('4', 0x4B);
        else if (code == "ArrowRight" && !shift) keycode(0, 0x4D);
        else if (code == "ArrowRight" && shift)  keycode('6', 0x4D);
        else if (code == "Backspace" && plain)   keycode('\x08', 0);
        else if (code == "Enter" && plain)       keycode('\x0D', 0x1C);
        else if (code == "Escape" && plain)      keycode('\x1b', 0x01);

        else if (key.length == 1 && plain) {
            // Letter keys
            keycode(key.toUpperCase(), 0);
        } else if (key.length == 1 && ctrl && !alt && !meta) {
            // CTRL keys, useful for sound on-off and for cheats
            keycode(controlCode(key), 0);
        } else {
            // Unrecognized special key, let the browser keep it.
            return;
        }

        // Eat events for any recognized keys
        e.preventDefault();
    });

    joystick.on('move', function (e, data)
    {
        const scale = 8.0;
        mouseTrackingEnd();
        joystickAxes(scale * data.force * Math.cos(data.angle.radian),
                    -scale * data.force * Math.sin(data.angle.radian));
    });

    joystick.on('end', function (e)
    {
        joystickAxes(0, 0);
    });

    for (let button of Array.from(document.getElementsByClassName('joystick_btn'))) {
        addButtonEvents(button, () => {
            button.classList.add('active_btn');
            joystickButton(true);
        }, () => {
            button.classList.remove('active_btn');
            joystickButton(false);
        });
    }

    let delay = null;
    let repeater = null;
    const stop_repeat = () => {
        if (delay !== null) {
            clearTimeout(delay);
            delay = null;
        }
        if (repeater !== null) {
            clearInterval(repeater);
            repeater = null;
        }
    }

    for (let button of Array.from(document.getElementsByClassName('keyboard_btn'))) {
        const press = () => {
            delay = null;
            keycode(button.dataset.ascii || '0x00', parseInt(button.dataset.scancode || '0', 0));
        };

        addButtonEvents(button, () => {
            button.classList.add('active_btn');
            press();
            stop_repeat();
            if (button.dataset.rdelay && button.dataset.rrate) {
                delay = setTimeout(() => {
                    stop_repeat();
                    repeater = setInterval(press, parseInt(button.dataset.rrate));
                }, parseInt(button.dataset.rdelay));
            }
        }, () => {
            button.classList.remove('active_btn');
            stop_repeat();
        });
    }

    for (let button of Array.from(document.getElementsByClassName('setspeed_btn'))) {
        addButtonEvents(button, () => {
            if (engine.calledRun) {
                for (let sibling of button.parentNode.children) {
                    sibling.classList.remove('active_btn');
                }
                button.classList.add('active_btn');
                engine.setSpeed(parseFloat(button.dataset.speed));
            }
        });
    }

    for (let button of Array.from(document.getElementsByClassName('loadgame_btn'))) {
        addButtonEvents(button, () => {
            button.classList.add('active_btn');
        }, () => {
            button.classList.remove('active_btn');
        }, () => {
            if (engine.calledRun) {
                engine.loadGame();
            }
        });
    }

    for (let button of Array.from(document.getElementsByClassName('savegame_btn'))) {
        addButtonEvents(button, () => {
            button.classList.add('active_btn');
        }, () => {
            button.classList.remove('active_btn');
        }, () => {
            if (engine.calledRun) {
                engine.saveGame();
            }
        });
    }

    for (let button of Array.from(document.getElementsByClassName('palette_btn'))) {
        addButtonEvents(button, (e) => {
            for (let sibling of button.parentNode.children) {
                sibling.classList.remove('active_btn');
            }
            button.classList.add('active_btn');

            engine.then(function () {
                if (button.dataset.name == "hgr") {
                    engine.setHGRColors();
                } else if (button.dataset.name == "cga") {
                    engine.setCGAColors();
                }
                if (button.dataset.src) {
                    engine.setColorTilesFromImage(button.dataset.src);
                }
            });
        });
    }
}
