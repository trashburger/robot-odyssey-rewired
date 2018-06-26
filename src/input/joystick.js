import nipplejs from 'nipplejs';
import { mouseTrackingEnd } from './mouse.js';
import * as Buttons from './buttons.js';
import * as GameMenu from '../gameMenu.js';
import * as EngineLoader from '../engineLoader.js';


function joystickAxes(x, y)
{
    const engine = EngineLoader.instance;
    if (engine.calledRun) {
        engine.setJoystickAxes(x, y);
        engine.autoSave();
    }
    GameMenu.setJoystickAxes(x, y);
}

function joystickButton(b)
{
    const engine = EngineLoader.instance;
    if (engine.calledRun) {
        engine.setJoystickButton(b);
        engine.autoSave();
    }
    GameMenu.setJoystickButton(b);
}


export function init()
{
    const engine = EngineLoader.instance;
    let joystick = null;

    // Create the on-screen joystick after the UI has become visible,
    // when it's starting to fade in. In our initial state, the whole
    // UI is invisible and nipplejs will capture the wrong position
    // for its default joystick.

    const zone = document.getElementById('joystick_xy');
    const observer = new IntersectionObserver((entries) => {
        if (!entries.length || !entries[0].isIntersecting) {
            return;
        }
        observer.disconnect();

        joystick = nipplejs.create({
            zone,
            mode: 'static',
            size: 100,
            threshold: 0.01,
            position: { left: '50%', top: '50%' }
        });

        joystick.on('move', (e, data) => {
            mouseTrackingEnd();
            joystickAxes(data.force * Math.cos(data.angle.radian),
                -data.force * Math.sin(data.angle.radian));
        });

        joystick.on('end', () => {
            joystickAxes(0, 0);
        });

    }, {
        root: null,
        rootMargin: '0px',
        threshold: 0,
    });
    observer.observe(zone);

    for (let button of document.getElementsByClassName('joystick_btn')) {
        Buttons.addButtonEvents(button, () => {
            button.classList.add('active_btn');
            joystickButton(true);
        }, () => {
            button.classList.remove('active_btn');
            joystickButton(false);
        });
    }

    // to do: preferences system, configurable joystick mapping
    engine.gamepadInvertYAxis = false;
    engine.gamepadDeadzone = 0.11;

    function gamepadPoll(gamepad, last_axes, last_pressed)
    {
        if (gamepad && gamepad.connected) {
            const pressed = gamepad.buttons.map(b => b.pressed);
            var axes = gamepad.axes;

            // If the controller is nearly centered, assume it's centered.
            // This is needed for some wired controllers that will constantly
            // spam positions near zero when idle, overriding any mouse or
            // virtual joystick input.

            const deadzone = engine.gamepadDeadzone;
            if (axes[0]*axes[0] + axes[1]*axes[1] <= deadzone*deadzone) {
                axes = [0,0];
                if (last_axes[0] !== 0 || last_axes[1] !== 0) {
                    if (joystick) {
                        joystick[0].hide();
                    }
                }
            }

            // Poll gamepads on every frame, to animate the virtual joystick
            window.requestAnimationFrame(() => {

                // Must call getGamepads() to refresh the state snapshot.
                // Game pads can disappear unpredictably, and Firefox
                // seems to be going so far as making 'gamepad' from the
                // scope above unexpectedly become undefined.
                const g = gamepad;
                if (g && g.index >= 0) {
                    const next_state = navigator.getGamepads()[g.index];
                    gamepadPoll(next_state, axes, pressed);
                }
            });

            // If any axes changed, send an XY event
            if (axes[0] !== last_axes[0] || axes[1] !== last_axes[1]) {
                const yinv = engine.gamepadInvertYAxis ? -1 : 1;
                joystickAxes(axes[0], yinv * axes[1]);

                // Animate the on-screen joystick
                if (joystick) {
                    const size = joystick[0].options.size * 0.25;
                    joystick[0].ui.front.style.left = (axes[0] * size) + 'px';
                    joystick[0].ui.front.style.top = (yinv * axes[1] * size) + 'px';
                    if (last_axes[0] === 0 && last_axes[1] === 0) {
                        joystick[0].show();
                    }
                }
            }

            // Dispatch individual button events to mapping handlers
            for (let i = 0; i < pressed.length; i++) {
                if (pressed[i] !== last_pressed[i]) {
                    Buttons.updateMappedGamepadButton(pressed[i], i);
                }
            }
        }
    }

    // Begin polling gamepads if a new one appears
    window.addEventListener('gamepadconnected', (e) => {
        gamepadPoll(e.gamepad, [0,0], []);

        // This is a hack for a specific gamepad that needs Y axis inversion really.
        // Replace this with more explicit preferences.
        if (e.gamepad.mapping !== 'standard' && e.gamepad.id.includes('Nimbus')) {
            engine.gamepadInvertYAxis = true;
        }
    });
}
