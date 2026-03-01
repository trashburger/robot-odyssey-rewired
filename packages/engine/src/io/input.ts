import {
    type EngineInputProvider,
    type EngineInput,
    type JoystickItem,
    type KeyboardItem,
    JoystickItemType,
} from '../index.js';

export const enum EventType {
    KeyDown = 'keydown', // KeyboardEventLike
    BeginKeyRepeat = 'beginkeyrepeat', // KeyboardRepeatEvent
    EndKeyRepeat = 'endkeyrepeat', // KeyboardRepeatEvent
    JoystickRaw = 'joystickraw', // RawJoystickEvent
    JoystickRate = 'joystickrate', // RateJoystickEvent
    JoystickRelative = 'joystickrelative', // RelativeJoystickEvent
    JoystickAbsolute = 'joystickabsolute', // AbsoluteJoystickEvent
}

export interface KeyboardEventLike {
    key?: string;
    code?: string;
    shiftKey?: boolean;
    ctrlKey?: boolean;
    altKey?: boolean;
    metaKey?: boolean;
}

export interface KeyboardRepeatEvent extends KeyboardEventLike {
    delay?: number;
}

export interface MouseEventLike {
    buttons: number;
    clientX: number;
    clientY: number;
}

export interface DOMRectLike {
    left: number;
    top: number;
    width: number;
    height: number;
}

// Generic Joystick event: coordinates could be a movement rate, a relative distance, or an absolute location.
export class JoystickEvent extends Event {
    button: boolean;
    x: number;
    y: number;

    constructor(type: string, button: boolean, x: number, y: number) {
        super(type);
        this.button = button;
        this.x = x;
        this.y = y;
    }
}

export class RawJoystickEvent extends JoystickEvent {
    constructor(button: boolean, x: number, y: number) {
        super(EventType.JoystickRaw, button, x, y);
    }
}

export class RateJoystickEvent extends JoystickEvent {
    constructor(button: boolean, x: number, y: number) {
        super(EventType.JoystickRate, button, x, y);
    }
}

export class RelativeJoystickEvent extends JoystickEvent {
    constructor(button: boolean, x: number, y: number) {
        super(EventType.JoystickRelative, button, x, y);
    }
}

export class AbsoluteJoystickEvent extends JoystickEvent {
    constructor(button: boolean, x: number, y: number) {
        super(EventType.JoystickAbsolute, button, x, y);
    }
}

// Joystick event that includes a linearly scaled rate, X and Y each no larger than [-1, +1]
export class LinearRateJoystickEvent extends RateJoystickEvent {
    linearX: number;
    linearY: number;

    constructor(button: boolean, linearX: number, linearY: number) {
        // Usable Y rates are in [-16, 16].
        // Usable X rate is only [-8, 8] but this is superceded by our aspect correction and the Y range.
        super(button, linearX * 25.6, linearY * 16);
        this.linearX = linearX;
        this.linearY = linearY;
    }
}

// Joystick event with an exponentially scaled rate, X and Y each no larger than [-1, +1]
export class ExpRateJoystickEvent extends LinearRateJoystickEvent {
    expX: number;
    expY: number;

    constructor(button: boolean, expX: number, expY: number, curveExponent: number) {
        const angle = Math.atan2(expY, expX);
        const speed = Math.pow(expX * expX + expY * expY, 0.5 * curveExponent);
        super(button, Math.cos(angle) * speed, Math.sin(angle) * speed);
        this.expX = expX;
        this.expY = expY;
    }
}

// Joystick event with an absolute coordinate adjusted from the player hotspot location
export class HotspotJoystickEvent extends AbsoluteJoystickEvent {
    hotspotX: number;
    hotspotY: number;

    constructor(button: boolean, hotspotX: number, hotspotY: number) {
        // Apply hotspot adjustment
        let x = hotspotX - 2;
        let y = hotspotY - 5;

        // Make it easier to get through doorways; if the cursor is near a screen border, push it past the border.
        if (x <= 4) x = -1;
        if (x >= 160 - 8 - 4) x = 160 + 1;
        if (y <= 8) y = -1;
        if (y >= 192 - 16 - 8) y = 192 + 1;

        super(button, x, y);
        this.hotspotX = hotspotX;
        this.hotspotY = hotspotY;
    }
}

// Joystick event generated from a MouseEvent
export class JoystickFromMouseEvent extends HotspotJoystickEvent {
    mouseEvent: MouseEvent | MouseEventLike;
    displayRect: DOMRect | DOMRectLike;

    constructor(event: MouseEvent | MouseEventLike, displayRect: DOMRect | DOMRectLike) {
        super(
            (event.buttons & 1) !== 0,
            Math.round(((event.clientX - displayRect.left) * 160) / displayRect.width),
            Math.round(191 - ((event.clientY - displayRect.top) * 192) / displayRect.height),
        );
        this.mouseEvent = event;
        this.displayRect = displayRect;
    }
}

// Encode button value as 1/0
function button8(x: { button: boolean }): number {
    return x.button ? 1 : 0;
}

// Encode clamped signed 8-bit int (-128 to +127)
function clampInt8(x: number): number {
    return Math.max(-128, Math.min(127, Math.round(x))) & 0xff;
}

// Encode clamped absolute coordinate with 8-bit value from -1 to 254
function clampAbsoluteCoord8(x: number): number {
    return Math.max(-1, Math.min(254, Math.round(x))) & 0xff;
}

// Encode clamped fixed-point joystick rate
function clampJoystickRate(x: number): number {
    return clampInt8(Math.round(x * 8)); // RATE_UNIT = 8
}

function cancelJoystickRate(buffer: JoystickItem[]) {
    // If the queue has reached a RATE item, we need to manually clear it
    if (buffer.length >= 1 && buffer[0][0] == JoystickItemType.RATE) {
        buffer.length = 0;
    }
}

class RepeatingKey {
    id: string;
    event: KeyboardRepeatEvent;
    delay: number;

    constructor(event: KeyboardRepeatEvent) {
        this.id = [event.code, event.key].toString();
        this.event = event;
        this.delay = event.delay || 0;
    }

    dispatchTo(target: InputEventTarget) {
        target.dispatchEvent(new KeyboardEvent(EventType.KeyDown, this.event));
    }

    provide(target: InputEventTarget) {
        if (this.delay > 0) {
            this.delay--;
        } else {
            this.dispatchTo(target);
        }
    }
}

export class InputEventTarget extends EventTarget implements EngineInputProvider {
    buffer: { joystick: JoystickItem[]; keyboard: KeyboardItem[] } = { keyboard: [], joystick: [] };
    repeating: Map<string, RepeatingKey> = new Map();

    constructor() {
        super();
        for (let type of [
            EventType.KeyDown,
            EventType.BeginKeyRepeat,
            EventType.EndKeyRepeat,
            EventType.JoystickRaw,
            EventType.JoystickRate,
            EventType.JoystickRelative,
            EventType.JoystickAbsolute,
        ]) {
            this.addEventListener(type, this);
        }
    }

    // Provide EngineInput (This is an EngineInputProvider)
    provide(): EngineInput {
        for (let key of this.repeating.values()) {
            key.provide(this);
        }
        return this.buffer;
    }

    // Cancel queued joystick events
    joystickCancel() {
        this.buffer.joystick.length = 0;
    }

    // Cancel queued keyboard events
    keyboardCancel() {
        this.buffer.keyboard.length = 0;
    }

    // Generic event listener
    handleEvent(event: Event) {
        const buffer = this.buffer;
        switch (event.type) {
            // Begin repeating a key event. Sends one KeyDown now, and more during provide().
            case EventType.BeginKeyRepeat: {
                const e = event as KeyboardEvent as KeyboardRepeatEvent;
                const key = new RepeatingKey(e);
                this.repeating.set(key.id, key);
                key.dispatchTo(this);
                break;
            }

            // End repetition for the indicated key
            case EventType.EndKeyRepeat: {
                const e = event as KeyboardEvent as KeyboardRepeatEvent;
                const key = new RepeatingKey(e);
                this.repeating.delete(key.id);
                break;
            }

            // Enqueue a keypress (The game engine does not use up/down states)
            case EventType.KeyDown: {
                const e = event as KeyboardEvent as KeyboardEventLike;

                if (!e.altKey && !e.metaKey) {
                    const shift = e.shiftKey;
                    switch (e.code) {
                        case 'ArrowUp':
                            buffer.keyboard.push([shift ? 0x38 : 0, 0x48]);
                            break;
                        case 'ArrowDown':
                            buffer.keyboard.push([shift ? 0x32 : 0, 0x50]);
                            break;
                        case 'ArrowLeft':
                            buffer.keyboard.push([shift ? 0x34 : 0, 0x4b]);
                            break;
                        case 'ArrowRight':
                            buffer.keyboard.push([shift ? 0x36 : 0, 0x4d]);
                            break;
                        case 'Backspace':
                            buffer.keyboard.push([0x08, 0x00]);
                            break;
                        case 'Enter':
                            buffer.keyboard.push([0x0d, 0x1c]);
                            break;
                        case 'Escape':
                            buffer.keyboard.push([0x1b, 0x01]);
                            break;
                        default:
                            if (e.key && e.key.length === 1) {
                                if (e.ctrlKey) {
                                    const n = e.key.toUpperCase().charCodeAt(0);
                                    if (n >= 65 && n <= 90) {
                                        this.buffer.keyboard.push([n - 64, 0]);
                                    }
                                } else {
                                    const n = e.key.charCodeAt(0);
                                    if (n < 0x80) {
                                        this.buffer.keyboard.push([n, 0]);
                                    }
                                }
                            }
                    }
                }
                break;
            }

            // Queue a single raw joystick movement.
            case EventType.JoystickRaw: {
                const e = event as RawJoystickEvent;
                const jsbuf = buffer.joystick;

                cancelJoystickRate(jsbuf);

                const item: JoystickItem = [JoystickItemType.RAW, button8(e), clampInt8(e.x), clampInt8(e.y)];
                jsbuf.push(item);
                break;
            }

            // Queue a joystick movement that continues until replaced, in pixels per frame.
            case EventType.JoystickRate: {
                const e = event as RateJoystickEvent;
                const jsbuf = buffer.joystick;

                cancelJoystickRate(jsbuf);
                const last = jsbuf[jsbuf.length - 1];

                const item: JoystickItem = [
                    JoystickItemType.RATE,
                    button8(e),
                    clampJoystickRate(e.x),
                    clampJoystickRate(e.y),
                ];
                if (last && last[0] === item[0]) {
                    // Replace any existing RATE at the end of the queue
                    jsbuf[jsbuf.length - 1] = item;
                } else {
                    jsbuf.push(item);
                }
                break;
            }

            // Queue a relative joystick movement in pixels.
            case EventType.JoystickRelative: {
                const e = event as RelativeJoystickEvent;
                const jsbuf = buffer.joystick;

                cancelJoystickRate(jsbuf);
                const last = jsbuf[jsbuf.length - 1];

                const item: JoystickItem = [JoystickItemType.RELATIVE, button8(e), clampInt8(e.x), clampInt8(e.y)];
                if (last && last[0] === item[0] && last[1] === item[1]) {
                    // Add to an existing RELATIVE event with the same button state
                    last[2] = clampInt8(last[2] + item[2]);
                    last[3] = clampInt8(last[3] + item[3]);
                } else {
                    jsbuf.push(item);
                }
                break;
            }

            // Queue an absolute joystick movement to the indicated player location in pixels.
            case EventType.JoystickAbsolute: {
                const e = event as AbsoluteJoystickEvent;
                const jsbuf = buffer.joystick;

                cancelJoystickRate(jsbuf);
                const last = jsbuf[jsbuf.length - 1];

                const item: JoystickItem = [
                    JoystickItemType.ABSOLUTE,
                    button8(e),
                    clampAbsoluteCoord8(e.x),
                    clampAbsoluteCoord8(e.y),
                ];
                if (last && last[0] === item[0] && last[1] === item[1]) {
                    // Replace an existing ABSOLUTE event with the same button state
                    jsbuf[jsbuf.length - 1] = item;
                } else {
                    jsbuf.push(item);
                }
                break;
            }

            // Press the joystick button without making any other changes
        }
    }
}
