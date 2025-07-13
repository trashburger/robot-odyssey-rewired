import * as FileManager from './files/fileManager.js';
import * as EngineLoader from './engineLoader.js';
import { mouseTrackingEnd } from './input/mouse.js';

export const States = {
    SPLASH:             'SPLASH',
    MENU_TRANSITION:    'MENU_TRANSITION',
    MENU_ACTIVE:        'MENU_ACTIVE',
    EXEC_LAUNCHING:     'EXEC_LAUNCHING',
    EXEC:               'EXEC',
    LOADING:            'LOADING',
    MODAL_TEXTBOX:      'MODAL_TEXTBOX',
    MODAL_FILES:        'MODAL_FILES',
    ERROR_HALT:         'ERROR_HALT',
};

const engine_controls = document.getElementById('engine_controls');
const speed_selector = document.getElementById('speed_selector');
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
let modal_saved = null;

export function showError(e)
{
    // This error handler should be callable at any time, even before init()

    e = e.toString();
    if (e.includes('no binaryen method succeeded')) {
        // This is obtuse; we only build for wasm, so really this means the device doesn't support wasm
        e = 'No WebAssembly?\n\nSorry, this browser might not be supported.';
    } else {
        // Something else went wrong.
        e = 'Fail.\n\n' + e;
    }

    modal(e);
    setState(States.ERROR_HALT);

    throw e;    // In case we have a debugger attached
}

export function modal(message, onclick)
{
    return new Promise((resolve) => {

        if (modal_saved === null) {
            modal_saved = {
                state: current_state,
            };
        }

        if (modal_saved.resolve) {
            // Resolve and replace previous modal
            modal_saved.resolve();
        }
        modal_saved.resolve = resolve;

        modal_textbox.firstElementChild.innerText = message;
        modal_textbox.onclick = onclick || exitModal;

        speed_selector.value = '0';
        speed_selector.dispatchEvent(new Event('change'));

        setState(States.MODAL_TEXTBOX);
    });
}

export function exitModal()
{
    const saved = modal_saved;
    modal_saved = null;

    if (current_state === States.MODAL_TEXTBOX && saved) {
        speed_selector.value = '1';
        speed_selector.dispatchEvent(new Event('change'));
        setState(saved.state);
    }

    // Resolve promise from modal()
    saved.resolve();
}

export function modalDuringPromise(promise, message)
{
    var end = exitModal;
    const nop = () => {};
    const finish = () => { end = nop; };
    modal(message, nop).then(finish);

    const minimumDuration = 750;
    setTimeout(() => { promise.then(() => end(), () => end()); }, minimumDuration);
}

export function getState()
{
    return current_state;
}

export function init()
{
    // Handle asynchronous errors in loading the engine
    EngineLoader.complete.then(null, showError);

    // Splashscreen pointing events
    splash.onclick = () => {
        if (current_state === States.SPLASH) {
            setState(States.MENU_TRANSITION);
        }
    };

    // Splash animation end
    getLastSplashImage().addEventListener('animationend', function () {
        setTimeout(function () {
            if (current_state === States.SPLASH) {
                setState(States.MENU_TRANSITION);
            }
        }, 1000);
    });

    // Mouse/touch handlers for menu choices
    for (let i = 0; i < choices.length; i++) {
        choices[i].addEventListener('click', function () {
            if (current_state === States.MENU_ACTIVE) {
                setMenuChoice(i);
                invokeMenuChoice();
            }
        });
        choices[i].addEventListener('mouseenter', function () {
            if (current_state === States.MENU_ACTIVE && menu_joystick_y === 0) {
                setMenuChoice(i);
            }
        });
    }

    // Back to the menu when a game binary exits
    EngineLoader.instance.onProcessExit = function () {
        if (current_state === States.EXEC || current_state === States.EXEC_LAUNCHING) {
            setState(States.MENU_TRANSITION);
        }
    };

    // We're already running the splashscreen via CSS animations
    setState(States.SPLASH);
}

export function pressKey(ascii, scancode)
{
    const engine = EngineLoader.instance;

    if (current_state === States.SPLASH) {
        if (ascii === 0x0D || ascii === 0x20) {
            // Enter or space
            setState(States.MENU_TRANSITION);
        }

    } else if (current_state === States.MENU_ACTIVE) {

        if (scancode === 0x50 || ascii === 0x20) {
            // Down or Space
            setMenuChoice(current_menu_choice + 1);
        } else if (scancode === 0x48) {
            // Up
            setMenuChoice(current_menu_choice - 1);
        } else if (ascii === 0x0D) {
            // Enter
            invokeMenuChoice();
        }

    } else if (current_state === States.MODAL_TEXTBOX) {
        if (ascii === 0x0D || ascii === 0x20 || ascii === 0x1B) {
            // Enter, space, escape
            modal_textbox.onclick();
        }

    } else if (current_state === States.MODAL_FILES) {
        if (ascii === 0x1B) {
            // Escape
            FileManager.close();
        }

    } else if (current_state === States.EXEC && engine.calledRun) {
        // Engine gets keys only when it's in front
        engine.pressKey(ascii, scancode);
        engine.autoSave();
    }
}

function joystickIntervalFunc()
{
    var rate = 0.25;  // Max menu ticks per interval

    // Make rollover at the edges slower
    if ((current_menu_choice === 0 && menu_joystick_y < 0) ||
        (current_menu_choice === choices.length-1 && menu_joystick_y > 0)) {
        rate *= 0.3;
    }

    menu_joystick_accum += rate * menu_joystick_y;
    let intpart = menu_joystick_accum|0;
    if (intpart !== 0) {
        menu_joystick_accum -= intpart;
        setMenuChoice(current_menu_choice + intpart);
    }
}

export function setJoystickAxes(x, y)
{
    if (current_state === States.MENU_ACTIVE) {
        menu_joystick_y = Math.max(-1, Math.min(1, y));
        if (y === 0) {
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

export function setJoystickButton(b)
{
    if (b && current_state === States.SPLASH) {
        setState(States.MENU_TRANSITION);
    } else if (b && current_state === States.MODAL_TEXTBOX) {
        modal_textbox.onclick();
    } else if (b && current_state === States.MENU_ACTIVE) {
        invokeMenuChoice();
    }
}

function getLastSplashImage()
{
    let result = null;
    for (let child of splash.children) {
        if (child.nodeName === 'IMG') {
            result = child;
        }
    }
    return result;
}

export async function afterLoadingState()
{
    const engine = EngineLoader.instance;
    if (engine.calledRun) {
        // Already loaded
        return engine;
    }

    setState(States.LOADING);
    return await EngineLoader.complete;
}

export function setState(s)
{
    if (s === current_state) {
        return;
    }
    if (current_state === States.ERROR_HALT) {
        // Stay stuck in error halt
        return;
    }
    current_state = s;

    // At this point, a state change is definitely happening.
    // Reset per-state context, and make UI visibility changes.

    if (menu_joystick_interval) {
        clearInterval(menu_joystick_interval);
        menu_joystick_interval = null;
    }

    // Interrupt mouse tracking on any state change
    mouseTrackingEnd();

    if (s === States.MENU_TRANSITION) {
        // Brief timed state to lock out input when menu is fading in
        setTimeout(function () {
            if (current_state === States.MENU_TRANSITION) {
                setState(States.MENU_ACTIVE);
            }
        }, 300);
    }

    if (s === States.MODAL_TEXTBOX) {
        modal_textbox.classList.remove('hidden');
        requestAnimationFrame(() => modal_textbox.classList.remove('fadeout'));
    } else if (s !== States.ERROR_HALT) {
        modal_textbox.classList.add('fadeout', 'hidden');
    }

    if (s === States.MODAL_FILES) {
        // Fade-in
        modal_files.classList.remove('hidden');
        requestAnimationFrame(() => modal_files.classList.remove('fadeout'));
    } else {
        // Hide and pass events immediately
        modal_files.classList.add('fadeout', 'hidden');
    }

    if (s === States.MODAL_FILES || s === States.MODAL_TEXTBOX) {
        // Completely hide engine controls so their blank space can't be scrolled into
        engine_controls.classList.add('hidden', 'fadeout');
    } else if (s !== States.SPLASH) {
        // Fade-in in states other than modals and the splash
        engine_controls.classList.remove('hidden');
        setTimeout(() => engine_controls.classList.remove('fadeout'), 50);
    }

    if (s === States.SPLASH) {
        splash.classList.remove('hidden');
    } else if (s === States.LOADING || s === States.EXEC_LAUNCHING) {
        splash.classList.add('hidden');
    }

    if (s === States.EXEC_LAUNCHING || s === States.LOADING) {
        game_menu.classList.add('fadeout');
    } else if (s !== States.EXEC && s !== States.LOADING && s !== States.MODAL_FILES && s !== States.MODAL_TEXTBOX) {
        game_menu.classList.remove('fadeout');
    }

    if (s === States.MENU_TRANSITION || s === States.MENU_ACTIVE) {
        game_menu.classList.remove('hidden');
    }

    if (s === States.LOADING) {
        loading.classList.remove('hidden');
    } else if (s !== States.EXEC) {
        loading.classList.add('hidden');
    }

    if (s === States.EXEC) {
        framebuffer.classList.remove('hidden');
    } else if (s === States.MENU_TRANSITION || s === States.MENU_ACTIVE || s === States.SPLASH) {
        framebuffer.classList.add('hidden');
    }
}

function setMenuChoice(c)
{
    c %= choices.length;
    if (c < 0) c += choices.length;

    if (c !== current_menu_choice) {
        current_menu_choice = c;
    }

    // The cursor's default positioning is aligned with the first choice.
    // Adjust the cursor offset based on the choice position.

    let element = choices[c];
    let to_percent = 100 / element.offsetParent.offsetHeight;
    let offset_percent = (element.offsetTop - choices[0].offsetTop) * to_percent;
    game_menu_cursor.style.top = offset_percent + '%';
}

async function invokeMenuChoice()
{
    const choice = choices[current_menu_choice].dataset;
    const engine = await afterLoadingState();

    const try_exec = () => {
        if (choice.exec) {
            setState(States.EXEC_LAUNCHING);
            const args = choice.exec.split(' ');
            engine.exec(args[0], args[1] || '');
        }
    };

    if (!choice.files) {
        // No file manager is associated with this choice
        return try_exec();
    }

    // Asynchronously load the file manager, but it may
    // report that there are no files to manage, in which
    // case we should also try exec.

    setState(States.LOADING);
    const result = await FileManager.open(choice.files, States.MENU_TRANSITION);
    if (result) {
        setState(States.MODAL_FILES);
    } else {
        // If no files are available, go right to exec
        try_exec();
    }
}
