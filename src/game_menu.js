import './game_menu.css'

export const States = {
    SPLASH: 0,
    MENU: 1,
    EXEC: 2,
    LOADING: 3,
    ERROR: 4,
};

const splash = document.getElementById('splash');
const loading = document.getElementById('loading');
const framebuffer = document.getElementById('framebuffer');
const error = document.getElementById('error');
const game_menu = document.getElementById('game_menu');
const game_menu_cursor = document.getElementById('game_menu_cursor');
const choices = Array.from(game_menu.getElementsByClassName("choice"));

var current_state = null;
var current_menu_choice = 0;
var menu_joystick_interval = null;
var menu_joystick_y = 0;

export function showError(e)
{
    e = e.toString();
    if (e.includes("no binaryen method succeeded")) {
        // This is obtuse; we only build for wasm, so really this means the device doesn't support wasm
        e = "No WebAssembly?\n\nSorry, this browser might not be supported.";
    } else {
        // Something else went wrong.
        e = "Fail.\n\n" + e;
    }

    error.innerText = e;
    setState(States.ERROR);
}

export function init(engine)
{
    // Splashscreen pointing events
    splash.addEventListener('mousedown', function () {
        if (current_state == States.SPLASH) {
            setState(States.MENU);
        }
    });
    splash.addEventListener('touchstart', function () {
        if (current_state == States.SPLASH) {
            setState(States.MENU);
        }
    });

    // Splash animation end
    getLastSplashImage().addEventListener('animationend', function () {
        setTimeout(function () {
            if (current_state == States.SPLASH) {
                setState(States.MENU);
            }
        }, 1000);
    });

    // Mouse/touch handlers for menu choices
    for (let i = 0; i < choices.length; i++) {
        choices[i].addEventListener('click', function () {
            if (current_state == States.MENU) {
                setMenuChoice(i);
                execMenuChoice(engine);
            }
        })
        choices[i].addEventListener('mouseenter', function () {
            if (current_state == States.MENU) {
                setMenuChoice(i);
            }
        })
    }

    // Back to the menu when a game binary exits
    engine.onProcessExit = function () {
        setState(States.MENU);
    };

    // We're already running the splashscreen via CSS animations
    setState(States.SPLASH);
}

export function pressKey(engine, ascii, scancode)
{
    if (current_state == States.SPLASH) {
        setState(States.MENU);

    } else if (current_state == States.MENU) {

        if (scancode == 0x50 || ascii == 0x20) {
            // Down or Space
            setMenuChoice(current_menu_choice + 1);
        } else if (scancode == 0x48) {
            // Up
            setMenuChoice(current_menu_choice - 1);
        } else if (ascii == 0x0D) {
            // Enter
            execMenuChoice(engine);
        }
    }
}

export function setJoystickAxes(engine, x, y)
{
    if (current_state == States.MENU) {
        // Timed movements, according to Y axis.
        // This installs an interval handler if needed, which stays installed
        // until we change game_menu states.

        const interval = 80;
        const speed = 0.04;

        menu_joystick_y = y;
        if (!menu_joystick_interval) {
            var accumulator = 0;

            menu_joystick_interval = setInterval(function () {
                accumulator += menu_joystick_y * speed;
                if (accumulator > 1) {
                    accumulator = Math.min(accumulator - 1, 1);
                    setMenuChoice(current_menu_choice + 1);
                } else if (accumulator < -1) {
                    accumulator = Math.max(accumulator + 1, -1);
                    setMenuChoice(current_menu_choice - 1);
                }
            }, interval);
        }
    }
}

export function setJoystickButton(engine, b)
{
    if (b && current_state == States.SPLASH) {
        setState(States.MENU);
    } else if (b && current_state == States.MENU) {
        execMenuChoice(engine);
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
    var result = null;
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
    if (current_state == States.ERROR) {
        // Stay stuck in error state
        return;
    }
    current_state = s;

    if (menu_joystick_interval) {
        clearInterval(menu_joystick_interval);
        menu_joystick_interval = null;
    }

    if (s == States.ERROR) {
        error.classList.remove('hidden');
    } else {
        error.classList.add('hidden');
    }

    if (s == States.SPLASH) {
        splash.classList.remove('hidden');
    } else if (s == States.ERROR || s == States.LOADING) {
        splash.classList.add('hidden');
    }

    if (s != States.EXEC && s != States.LOADING) {
        game_menu.classList.remove('fadeout');
    }

    if (s == States.MENU) {
        game_menu.classList.remove('hidden');
    } else if (s == States.ERROR) {
        game_menu.classList.add('hidden');
    }

    if (s == States.LOADING) {
        loading.classList.remove('hidden');
    } else if (s != States.EXEC) {
        loading.classList.add('hidden');
    }

    if (s == States.EXEC) {
        framebuffer.classList.remove('hidden');
    } else {
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

    var element = choices[c];
    var to_percent = 100 / element.offsetParent.offsetHeight;
    var offset_percent = (element.offsetTop - choices[0].offsetTop) * to_percent;
    game_menu_cursor.style.top = offset_percent + '%';
}

function execMenuChoice(engine)
{
    // Sequence of operations:
    //   1. Menu fades out immediately
    //   2. If the WASM needs to load, it does so with the spinner visible
    //   3. The game will take a small amount of time between exec() and the first frame
    //   4. At the first frame, we setState(EXEC) and show the game with an iris transition

    loading.classList.add('hidden');
    splash.classList.add('hidden');
    game_menu.classList.add('fadeout');

    const args = choices[current_menu_choice].dataset.exec.split(" ");

    afterLoading(engine, function () {
        engine.exec(args[0], args[1] || "");
    });
}
