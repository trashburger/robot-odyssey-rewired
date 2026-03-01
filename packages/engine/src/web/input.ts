import { InputEventTarget, JoystickFromMouseEvent, ExpRateJoystickEvent, type KeyboardEventLike } from '../io/input.js';
import { type EngineInput } from '../index.js';
import { WebAudioRenderer } from './audio.js';

export const enum WebEventType {
    KeyDown = 'keydown', // KeyboardEventLike
    BeginKeyRepeat = 'beginkeyrepeat', // KeyboardRepeatEvent
    EndKeyRepeat = 'endkeyrepeat', // KeyboardRepeatEvent
    JoystickRaw = 'joystickraw', // RawJoystickEvent
    JoystickRate = 'joystickrate', // RateJoystickEvent
    JoystickRelative = 'joystickrelative', // RelativeJoystickEvent
    JoystickAbsolute = 'joystickabsolute', // AbsoluteJoystickEvent
    PointerMove = 'pointermove', // PointerEvent
    PointerUp = 'pointerup', // PointerEvent
    PointerDown = 'pointerdown', // PointerEvent
    PointerLeave = 'pointerleave', // PointerEvent
    PointerCancel = 'pointercancel', // PointerEvent
}

const POINTER_EVENTS = [
    WebEventType.PointerMove,
    WebEventType.PointerUp,
    WebEventType.PointerDown,
    WebEventType.PointerLeave,
    WebEventType.PointerCancel,
];

export interface ButtonMapping extends KeyboardEventLike {
    joystick?: boolean;
    repeat?: boolean;
    delay?: number;
}

export class InputSettings {
    suspendDuration: number = 2000;
    longTouchThreshold: number = 160;
    joystickCurveExponent: number = 3;
    gamepadInvertYAxis: boolean = false;
    gamepadDeadzone: number = 0.11;
    gamepadButtons: Map<number, ButtonMapping> = new Map([
        [0, { joystick: true }],
        [1, { key: 'r' }],
        [2, { key: 't' }],
        [4, { key: 'c' }],
        [5, { key: 's' }],
        [6, { key: '?' }],
        [8, { code: 'Escape' }],
        [9, { code: 'Enter' }],
        [12, { code: 'ArrowUp', shiftKey: true, repeat: true, delay: 15 }],
        [13, { code: 'ArrowDown', shiftKey: true, repeat: true, delay: 15 }],
        [14, { code: 'ArrowLeft', shiftKey: true, repeat: true, delay: 15 }],
        [15, { code: 'ArrowRight', shiftKey: true, repeat: true, delay: 15 }],
    ]);
}

class TrackingState {
    readonly active!: boolean;
    readonly begin: () => void;
    readonly cancel: () => void;
    readonly suspend: (settings: InputSettings) => void;
    readonly resume: () => void;

    constructor() {
        let state: number | boolean = false;

        this.begin = () => {
            state = true;
        };
        this.cancel = () => {
            state = false;
        };
        this.suspend = (settings: InputSettings) => {
            if (state === true) {
                state = performance.now() + settings.suspendDuration;
            }
        };
        this.resume = () => {
            if (state !== false) {
                state = true;
            }
        };
        Object.defineProperty(this, 'active', {
            get: function () {
                if (typeof state === 'boolean') {
                    return state;
                } else {
                    const now = performance.now();
                    if (now > state) {
                        state = false;
                        return false;
                    } else {
                        return true;
                    }
                }
            },
        });
    }
}

class PointerState {
    longTouchTimer: number | null = null;
    button: boolean = false;

    cancel() {
        this.button = false;
        if (this.longTouchTimer !== null) {
            window.clearTimeout(this.longTouchTimer);
            this.longTouchTimer = null;
        }
    }

    mousedown() {
        this.cancel();
        this.button = true;
    }

    touchdown(settings: InputSettings, onLongTouch: () => void) {
        this.cancel();
        this.longTouchTimer = window.setTimeout(() => {
            this.button = true;
            onLongTouch();
        }, settings.longTouchThreshold);
    }
}

class GamepadState {
    joystick: ExpRateJoystickEvent = new ExpRateJoystickEvent(false, 0, 0, 1);
    buttons: boolean[] = [];
    axes: number[] = [];

    update(gamepad: Gamepad, target: WebInput) {
        const buttons = gamepad.connected ? gamepad.buttons.map((button) => button.pressed) : [];
        const axes = gamepad.connected ? Array.from(gamepad.axes) : [];
        const num_buttons = Math.max(buttons.length, this.buttons.length);

        let joystick_button = this.joystick.button;
        let joystick_x = this.joystick.expX;
        let joystick_y = this.joystick.expY;

        const settings = target.settings;
        const deadzone = settings.gamepadDeadzone;
        const button_map = settings.gamepadButtons;

        if (axes[0] * axes[0] + axes[1] * axes[1] <= deadzone * deadzone) {
            joystick_x = 0;
            joystick_y = 0;
        } else {
            joystick_x = axes[0];
            joystick_y = settings.gamepadInvertYAxis ? axes[1] : -axes[1];

            // Joystick movement cancels mouse tracking
            target.tracking.cancel();
        }

        for (let index = 0; index < num_buttons; index++) {
            const pressed = !!buttons[index];
            if (pressed !== !!this.buttons[index]) {
                const mapping = button_map.get(index);
                if (mapping) {
                    // Mapped button was pressed or released

                    if (mapping.joystick) {
                        // Mapped to joystick button
                        joystick_button = pressed;
                    } else if ((mapping.key || mapping.code) && (mapping.repeat || pressed)) {
                        // Mapped to a keyboard event
                        target.dispatchEvent(
                            mapping.repeat
                                ? Object.assign(
                                      new KeyboardEvent(
                                          pressed ? WebEventType.BeginKeyRepeat : WebEventType.EndKeyRepeat,
                                          mapping,
                                      ),
                                      { delay: mapping.delay },
                                  )
                                : new KeyboardEvent(WebEventType.KeyDown, mapping),
                        );
                    }
                }
            }
        }

        this.buttons = buttons;
        this.axes = axes;

        // Avoid sending any events when the joystick state doesn't change
        if (
            joystick_button !== this.joystick.button ||
            joystick_x !== this.joystick.expX ||
            joystick_y !== this.joystick.expY
        ) {
            const event = new ExpRateJoystickEvent(
                joystick_button,
                joystick_x,
                joystick_y,
                settings.joystickCurveExponent,
            );
            this.joystick = event;
            target.dispatchEvent(event);
        }
    }
}

export class WebInput extends InputEventTarget {
    audio?: WebAudioRenderer;
    display?: Element;
    settings: InputSettings = new InputSettings();
    tracking: TrackingState = new TrackingState();
    pointers: Map<number, PointerState> = new Map();
    gamepads: null | GamepadState[] = null;

    constructor() {
        super();
        for (let type of POINTER_EVENTS) {
            this.addEventListener(type, this);
        }
    }

    attachEvent(target: EventTarget, type: string) {
        target.addEventListener(type, (e: Event) => {
            const construct = e.constructor as { new (type: string, options: any): Event };
            if (!this.dispatchEvent(new construct(e.type, e))) {
                e.preventDefault();
            }
        });
    }

    attachKeyboard(target?: Element) {
        this.attachEvent(target || document, WebEventType.KeyDown);
    }

    attachPointer(display: Element, target?: Element | null) {
        this.display = display;
        for (let type of POINTER_EVENTS) {
            this.attachEvent(target || display, type);
        }
    }

    attachGamepad() {
        if ('getGamepads' in navigator && this.gamepads === null) {
            this.gamepads = [];
        }
    }

    provide(): EngineInput {
        this.gamepadPoll();
        return super.provide();
    }

    gamepadPoll() {
        const states = this.gamepads;
        if (states !== null) {
            const gamepads = navigator.getGamepads();
            for (let index = 0; index < gamepads.length; index++) {
                const gamepad = gamepads[index];
                if (gamepad) {
                    if (!states[index]) {
                        states[index] = new GamepadState();
                    }
                    states[index].update(gamepad, this);
                }
            }
        }
    }

    handleEvent(event: Event) {
        switch (event.type) {
            case WebEventType.KeyDown: {
                const e = event as KeyboardEvent;
                switch (e.code) {
                    // Arrow keys cancel mouse tracking and empty the keyboard queue.
                    case 'ArrowUp':
                    case 'ArrowDown':
                    case 'ArrowLeft':
                    case 'ArrowRight':
                        this.keyboardCancel();
                        this.tracking.cancel();
                        break;
                }
                super.handleEvent(e);
                if (!e.ctrlKey && !e.altKey && !e.metaKey) {
                    e.preventDefault();
                }
                break;
            }

            case WebEventType.PointerMove:
            case WebEventType.PointerUp:
            case WebEventType.PointerDown:
            case WebEventType.PointerLeave:
            case WebEventType.PointerCancel: {
                const e = event as PointerEvent;
                let pointer: PointerState | undefined | null = this.pointers.get(e.pointerId);

                if (e.button === 0 && !pointer) {
                    e.preventDefault();
                    pointer = new PointerState();
                    this.pointers.set(e.pointerId, pointer);
                }

                const tracking_was_active = this.tracking.active;

                switch (e.type) {
                    case WebEventType.PointerUp:
                        if (pointer) {
                            this.tracking.begin();
                        }
                        break;
                    case WebEventType.PointerLeave:
                        this.tracking.suspend(this.settings);
                        break;
                    case WebEventType.PointerMove:
                    case WebEventType.PointerDown:
                        this.tracking.resume();
                }

                switch (e.type) {
                    case WebEventType.PointerUp:
                    case WebEventType.PointerLeave:
                    case WebEventType.PointerCancel: {
                        if (pointer) {
                            pointer.cancel();
                            this.pointers.delete(e.pointerId);
                            pointer = null;
                        }
                    }
                }

                if (tracking_was_active && this.display) {
                    e.preventDefault();
                    const js = new JoystickFromMouseEvent(e, this.display.getBoundingClientRect());
                    const dispatch = () => {
                        js.button = pointer ? pointer.button : false;
                        this.dispatchEvent(js);
                    };

                    if (e.type === WebEventType.PointerDown && pointer) {
                        if (e.pointerType === 'mouse') {
                            pointer.mousedown();
                        } else {
                            pointer.touchdown(this.settings, dispatch);
                        }
                    }

                    dispatch();
                }
                break;
            }

            default:
                super.handleEvent(event);
        }
        if (this.audio) {
            this.audio.setup();
        }
    }
}
