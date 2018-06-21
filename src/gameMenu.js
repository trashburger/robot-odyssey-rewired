import * as FileManager from './files/fileManager.js';

export const States = {
    SPLASH: 0,
    MENU_TRANSITION: 1,
    MENU_ACTIVE: 2,
    EXEC_LAUNCHING: 3,
    EXEC: 4,
    LOADING: 5,
    MODAL_TEXTBOX: 6,
    MODAL_FILES: 7,
    ERROR_HALT: 8,
};

const splash = document.getElementById('splash');
const loading = document.getElementById('loading');
const framebuffer = document.getElementById('framebuffer');
const modal_textbox = document.getElementById('modal_textbox');
const modal_files = document.getElementById('modal_files');
const game_menu = document.getElementById('game_menu');
const game_menu_cursor = document.getElementById('game_menu_cursor');
const choices = Array.from(game_menu.getElementsByClassName('choice'));

let current_state = null;
let current_menu_choice = 0;
let menu_joystick_interval = null;
let menu_joystick_y = 0;
let menu_joystick_accum = 0;

export function showError(e)
{
    e = e.toString();
    if (e.includes('no binaryen method succeeded')) {
        // This is obtuse; we only build for wasm, so really this means the device doesn't support wasm
        e = 'No WebAssembly?\n\nSorry, this browser might not be supported.';
    } else {
        // Something else went wrong.
        e = 'Fail.\n\n' + e;
    }

    modal_textbox.firstElementChild.innerText = e;
    setState(States.MODAL_TEXTBOX);
    setState(States.ERROR_HALT);
}

export function init(engine)
{
    // Splashscreen pointing events
    splash.addEventListener('mousedown', function () {
        if (current_state == States.SPLASH) {
            setState(States.MENU_TRANSITION);
        }
    });
    splash.addEventListener('touchstart', function () {
        if (current_state == States.SPLASH) {
            setState(States.MENU_TRANSITION);
        }
    });

    // Splash animation end
    getLastSplashImage().addEventListener('animationend', function () {
        setTimeout(function () {
            if (current_state == States.SPLASH) {
                setState(States.MENU_TRANSITION);
            }
        }, 1000);
    });

    // Mouse/touch handlers for menu choices
    for (let i = 0; i < choices.length; i++) {
        choices[i].addEventListener('click', function () {
            if (current_state == States.MENU_ACTIVE) {
                setMenuChoice(i);
                invokeMenuChoice(engine);
            }
        });
        choices[i].addEventListener('mouseenter', function () {
            if (current_state == States.MENU_ACTIVE && menu_joystick_y == 0) {
                setMenuChoice(i);
            }
        });
    }

    // Back to the menu when a game binary exits
    engine.onProcessExit = function () {
        setState(States.MENU_TRANSITION);
    };

    // We're already running the splashscreen via CSS animations
    setState(States.SPLASH);
}

export function pressKey(engine, ascii, scancode)
{
    if (current_state == States.SPLASH) {
        setState(States.MENU_TRANSITION);

    } else if (current_state == States.MENU_ACTIVE) {

        if (scancode == 0x50 || ascii == 0x20) {
            // Down or Space
            setMenuChoice(current_menu_choice + 1);
        } else if (scancode == 0x48) {
            // Up
            setMenuChoice(current_menu_choice - 1);
        } else if (ascii == 0x0D) {
            // Enter
            invokeMenuChoice(engine);
        }
    }
}

function joystickIntervalFunc()
{
    var rate = 0.25;  // Max menu ticks per interval

    // Make rollover at the edges slower
    if ((current_menu_choice == 0 && menu_joystick_y < 0) ||
        (current_menu_choice == choices.length-1 && menu_joystick_y > 0)) {
        rate *= 0.3;
    }

    menu_joystick_accum += rate * menu_joystick_y;
    let intpart = menu_joystick_accum|0;
    if (intpart != 0) {
        menu_joystick_accum -= intpart;
        setMenuChoice(current_menu_choice + intpart);
    }
}

export function setJoystickAxes(engine, x, y)
{
    if (current_state == States.MENU_ACTIVE) {
        menu_joystick_y = Math.max(-1, Math.min(1, y));
        if (y == 0) {
            // Reset on idle
            if (menu_joystick_interval) {
                clearInterval(menu_joystick_interval);
                menu_joystick_interval = null;
            }

        } else {
            // Immediate response + variable repeat rate
            if (!menu_joystick_interval) {
                menu_joystick_accum = Math.sign(menu_joystick_y);
                menu_joystick_interval = setInterval(joystickIntervalFunc, 50);
                joystickIntervalFunc();
            }
        }
    }
}

export function setJoystickButton(engine, b)
{
    if (b && current_state == States.SPLASH) {
        setState(States.MENU_TRANSITION);
    } else if (b && current_state == States.MENU_ACTIVE) {
        invokeMenuChoice(engine);
    }
}

export function afterLoading(engine, func)
{
    if (engine.calledRun) {
        // Already loaded
        func();
    } else {
        setState(States.LOADING);
        engine.then(func);
    }
}

function getLastSplashImage()
{
    let result = null;
    for (let child of Array.from(splash.children)) {
        if (child.nodeName == 'IMG') {
            result = child;
        }
    }
    return result;
}

export function setState(s)
{
    if (s == current_state) {
        return;
    }
    if (current_state == States.ERROR_HALT) {
        return;
    }
    current_state = s;

    if (menu_joystick_interval) {
        clearInterval(menu_joystick_interval);
        menu_joystick_interval = null;
    }

    if (s == States.MENU_TRANSITION) {
        // Brief timed state to lock out input when menu is fading in
        setTimeout(function () {
            if (current_state == States.MENU_TRANSITION) {
                setState(States.MENU_ACTIVE);
            }
        }, 300);
    }

    if (s == States.MODAL_TEXTBOX) {
        modal_textbox.classList.remove('hidden');
    } else if (s != States.ERROR_HALT) {
        modal_textbox.classList.add('hidden');
    }

    if (s == States.MODAL_FILES) {
        modal_files.classList.remove('hidden');
    } else {
        modal_files.classList.add('hidden');
    }

    if (s == States.SPLASH) {
        splash.classList.remove('hidden');
    } else if (s == States.LOADING || s == States.EXEC_LAUNCHING) {
        splash.classList.add('hidden');
    }

    if (s == States.EXEC_LAUNCHING) {
        game_menu.classList.add('fadeout');
    } else if (s != States.EXEC && s != States.LOADING) {
        game_menu.classList.remove('fadeout');
    }

    if (s == States.MENU_TRANSITION || s == States.MENU_ACTIVE) {
        game_menu.classList.remove('hidden');
    }

    if (s == States.LOADING) {
        loading.classList.remove('hidden');
    } else if (s != States.EXEC) {
        loading.classList.add('hidden');
    }

    if (s == States.EXEC) {
        framebuffer.classList.remove('hidden');
    } else if (s == States.MENU_TRANSITION || s == States.MENU_ACTIVE || s == States.SPLASH) {
        framebuffer.classList.add('hidden');
    }
}

function setMenuChoice(c)
{
    c %= choices.length;
    if (c < 0) c += choices.length;

    if (c != current_menu_choice) {
        current_menu_choice = c;
    }

    // The cursor's default positioning is aligned with the first choice.
    // Adjust the cursor offset based on the choice position.

    let element = choices[c];
    let to_percent = 100 / element.offsetParent.offsetHeight;
    let offset_percent = (element.offsetTop - choices[0].offsetTop) * to_percent;
    game_menu_cursor.style.top = offset_percent + '%';
}

function invokeMenuChoice(engine)
{
    const choice = choices[current_menu_choice].dataset;
    afterLoading(engine, () => {

        if (choice.files && FileManager.open(choice.files)) {
            // New or saved game/lab
            setState(States.MODAL_FILES);

        } else if (choice.exec) {
            // Tutorials, new game/lab with no saves available
            const args = choice.exec.split(' ');
            setState(States.EXEC_LAUNCHING);
            engine.exec(args[0], args[1] || '');
        }

    });
}
