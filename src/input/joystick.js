import { mouseTrackingEnd } from './mouse.js';
import { audioContextSetup } from '../sound.js';
import * as Buttons from './buttons.js';
import * as GameMenu from '../gameMenu.js';
import * as EngineLoader from '../engineLoader.js';

const zone = document.getElementById('joystick_zone');
const center = document.getElementById('joystick_center');
const thumb = document.getElementById('joystick_thumb');

function joystickAxes(x, y)
{
    const engine = EngineLoader.instance;

    if (zone && thumb) {
        const zone_rect = zone.getBoundingClientRect();
        const scale = 0.5 * zone_rect.height;
        thumb.style.left = scale * x + 'px';
        thumb.style.top = scale * y + 'px';
    }

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

function onMove(e)
{
    if (center && zone) {
        const zone_rect = zone.getBoundingClientRect();
        const center_rect = center.getBoundingClientRect();
        const scale = 2.0 / zone_rect.height;
        const x = (e.clientX - center_rect.x) * scale;
        const y = (e.clientY - center_rect.y) * scale;
        joystickAxes(Math.max(-1, Math.min(1, x)),
            Math.max(-1, Math.min(1, y)));
        e.preventDefault();
    }
}

function onBegin(e)
{
    mouseTrackingEnd();
    audioContextSetup();
    if (thumb && zone) {
        thumb.classList.add('active_btn');
        zone.onpointermove = onMove;
        zone.setPointerCapture(e.pointerId);
    }
    onMove(e);
}

function onEnd(e)
{
    joystickAxes(0, 0);
    if (thumb && zone) {
        thumb.classList.remove('active_btn');
        zone.onpointermove = null;
        zone.releasePointerCapture(e.pointerId);
    }
    e.preventDefault();
}

export function init()
{
    const engine = EngineLoader.instance;

    if (thumb && zone) {
        zone.addEventListener('pointerdown', onBegin);
        zone.addEventListener('pointerup', onEnd);
        zone.addEventListener('pointercancel', onEnd);
    }

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
