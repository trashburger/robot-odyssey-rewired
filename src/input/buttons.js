import { audioContextSetup } from '../sound.js';
import { mouseTrackingEnd } from './mouse.js';
import * as GameMenu from '../gameMenu.js';

const canvas = document.getElementById('framebuffer');
const speed_selector = document.getElementById('speed_selector');
const gamepad_button_mappings = [];

export function updateMappedGamepadButton(pressed, index)
{
    const handler = gamepad_button_mappings[index];
    if (handler) {
        handler(pressed, index);
    }
}

export function addButtonClick(button_element, click)
{
    addButtonEvents(button_element, () => {
        button_element.classList.add('active_btn');
    }, () => {
        button_element.classList.remove('active_btn');
    }, click);
}

export function addButtonEvents(button_element, down, up, click)
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
        gamepad_button_mappings[gamepad_button] = (pressed) => {
            if (pressed) {
                down();
            } else {
                if (click) {
                    click();
                }
                up();
            }
        };
    }
}

function controlCode(key)
{
    return String.fromCharCode(key.toUpperCase().charCodeAt(0) - 'A'.charCodeAt(0) + 1);
}

export function init(engine)
{
    // Fade in the controls about a frame after the JS is ready, before the engine loads
    setTimeout(() => {
        document.getElementById('engine_controls').classList.remove('hidden');
    }, 10);

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

    document.body.addEventListener('keydown', function (e)
    {
        const code = e.code || e.key || '';
        const key = e.key || '';
        const shift = e.shiftKey;
        const ctrl = e.ctrlKey;
        const alt = e.altKey;
        const meta = e.metaKey;
        const plain = !(ctrl || alt || meta);

        if (code.includes('Arrow')) {
            // Most keys can coexist with mouse tracking, but arrow keys will take over.
            mouseTrackingEnd();
        }

        if (code == 'ArrowUp' && !shift)         keycode(0, 0x48);
        else if (code == 'ArrowUp' && shift)     keycode('8', 0x48);
        else if (code == 'ArrowDown' && !shift)  keycode(0, 0x50);
        else if (code == 'ArrowDown' && shift)   keycode('2', 0x50);
        else if (code == 'ArrowLeft' && !shift)  keycode(0, 0x4B);
        else if (code == 'ArrowLeft' && shift)   keycode('4', 0x4B);
        else if (code == 'ArrowRight' && !shift) keycode(0, 0x4D);
        else if (code == 'ArrowRight' && shift)  keycode('6', 0x4D);
        else if (code == 'Backspace' && plain)   keycode('\x08', 0);
        else if (code == 'Enter' && plain)       keycode('\x0D', 0x1C);
        else if (code == 'Escape' && plain)      keycode('\x1b', 0x01);

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

    let delay = null;
    let repeater = null;
    function stopRepeat()
    {
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
            stopRepeat();
            if (button.dataset.rdelay && button.dataset.rrate) {
                delay = setTimeout(() => {
                    stopRepeat();
                    repeater = setInterval(press, parseInt(button.dataset.rrate));
                }, parseInt(button.dataset.rdelay));
            }
        }, () => {
            button.classList.remove('active_btn');
            stopRepeat();
        });
    }

    speed_selector.addEventListener('change', (e) => {
        engine.then(() => engine.setSpeed(parseFloat(speed_selector.value)));
        if (e && e.target) {
            e.target.blur();
            canvas.focus();
        }
    });

    for (let button of Array.from(document.getElementsByClassName('speed_adjust_btn'))) {
        addButtonEvents(button, () => {
            let i = speed_selector.selectedIndex + parseInt(button.dataset.value);
            if (i >= 0 && i < speed_selector.length) {
                speed_selector.selectedIndex = i;
                engine.then(() => engine.setSpeed(parseFloat(speed_selector.value)));
                button.classList.add('active_btn');
            }
        }, () => {
            button.classList.remove('active_btn');
        });
    }

    for (let button of Array.from(document.getElementsByClassName('savezip_btn'))) {
        addButtonClick(button, () => {
            engine.then(() => engine.downloadArchive());
        });
    }

    for (let button of Array.from(document.getElementsByClassName('filepicker_btn'))) {
        addButtonClick(button, () => {
            engine.then(() => engine.filePicker());
        });
    }

    for (let button of Array.from(document.getElementsByClassName('exec_btn'))) {
        addButtonClick(button, () => {
            const args = button.dataset.exec.split(' ');
            engine.then(() => {
                GameMenu.setState(GameMenu.States.EXEC_LAUNCHING);
                engine.exec(args[0], args[1] || '');
            });
        });
    }
}
