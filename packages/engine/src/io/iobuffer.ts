import { type EngineOutput, type ClockItem, type SpeakerItem, type CGAWriteItem, type EngineInput } from '../index.js';

export const textDecoder = new TextDecoder();
export const textEncoder = new TextEncoder();

const unpackByOutputTag = [
    // EXIT, Process has exited. 1 byte: exit code
    function (src: Uint8Array, dest: EngineOutput) {
        dest.push(['exit', src[0]]);
        return 1;
    },
    // LOG, Log message. Variable length: length16, string bytes
    function (src: Uint8Array, dest: EngineOutput) {
        const len16 = src[0] | (src[1] << 8);
        dest.push(['log', textDecoder.decode(src.subarray(2, 2 + len16))]);
        return 2 + len16;
    },
    // LOG_BINARY, Binary debug log. Variable length: length16, data bytes
    function (src: Uint8Array, dest: EngineOutput) {
        const len16 = src[0] | (src[1] << 8);
        dest.push(['log_binary', src.subarray(2, 2 + len16)]);
        return 2 + len16;
    },
    // STACK, Return address stack. Variable length: count16, words
    function (src: Uint8Array, dest: EngineOutput) {
        const count16 = src[0] | (src[1] << 8);
        const stack = [];
        for (var i = 0; i < count16; i++) {
            stack.push('cs:' + (src[2 * i + 2] | (src[2 * i + 3] << 8)).toString(16));
        }
        dest.push(['stack', stack]);
        return 2 + 2 * count16;
    },
    // CLOCK, Full-size clock difference. 4 bytes: cycles32
    // Combines with previous items of 'cga_wr' or 'speaker' type.
    function (src: Uint8Array, dest: EngineOutput) {
        const item: ClockItem = ['clock', src[0] | (src[1] << 8) | (src[2] << 16) | (src[3] << 24)];
        if (dest.length !== 0) {
            const last = dest[dest.length - 1];
            const last_key = last[0];
            if (last_key === 'cga_wr' || last_key === 'speaker') {
                last[1].push(item);
                return 4;
            }
        }
        dest.push(item);
        return 4;
    },
    // SPEAKER, Speaker impulse after short clock difference. 2 bytes: cycles16
    // Combines with previous items of 'speaker' type.
    function (src: Uint8Array, dest: EngineOutput) {
        const item: SpeakerItem = [src[0] | (src[1] << 8)];
        if (dest.length !== 0) {
            const last = dest[dest.length - 1];
            if (last[0] === 'speaker') {
                last[1].push(item);
                return 2;
            }
        }
        dest.push(['speaker', [item]]);
        return 2;
    },
    // CGA_WR, framebuffer write after short clock difference. 6 bytes: cycles16, address16, data16
    // Combines with previous items of 'cga_wr' type.
    function (src: Uint8Array, dest: EngineOutput) {
        const item: CGAWriteItem = [src[0] | (src[1] << 8), src[2] | (src[3] << 8), src[4] | (src[5] << 8)];
        if (dest.length !== 0) {
            const last = dest[dest.length - 1];
            if (last[0] === 'cga_wr') {
                last[1].push(item);
                return 6;
            }
        }
        dest.push(['cga_wr', [item]]);
        return 6;
    },
    // CGA_CLEAR, clear the CGA framebuffer. 0 bytes
    function (src: Uint8Array, dest: EngineOutput) {
        dest.push(['cga_clear']);
        return 0;
    },
    // CHECK_KEYBOARD, checkForKeyboardItem called. 0 bytes
    function (src: Uint8Array, dest: EngineOutput) {
        dest.push(['check_keyboard']);
        return 0;
    },
    // TAKE_KEYBOARD, takeKeyboardItem called. 0 bytes
    function (src: Uint8Array, dest: EngineOutput) {
        dest.push(['take_keyboard']);
        return 0;
    },
    // CHECK_JOYSTICK, checkForJoystickItem called. 0 bytes
    function (src: Uint8Array, dest: EngineOutput) {
        dest.push(['check_joystick']);
        return 0;
    },
    // TAKE_JOYSTICK, takeJoystickItem called. 0 bytes
    function (src: Uint8Array, dest: EngineOutput) {
        dest.push(['take_joystick']);
        return 0;
    },
    // DL_CLEAR, Clear the display list and its style stack. 0 bytes
    function (src: Uint8Array, dest: EngineOutput) {
        dest.push(['dl_clear']);
        return 0;
    },
    // DL_PRESENT, Present the current display list. 0 bytes
    function (src: Uint8Array, dest: EngineOutput) {
        dest.push(['dl_present']);
        return 0;
    },
    // DL_CLASS_PUSH, Push a CSS class onto the style stack. Variable length string beginning with a length byte.
    // Combines with previous items of 'dl_class_push' type.
    function (src: Uint8Array, dest: EngineOutput) {
        const item = textDecoder.decode(src.subarray(1, 1 + src[0]));
        const result = 1 + src[0];
        if (dest.length !== 0) {
            const last = dest[dest.length - 1];
            if (last[0] === 'dl_class_push') {
                last[1].push(item);
                return result;
            }
        }
        dest.push(['dl_class_push', [item]]);
        return result;
    },
    // DL_CLASS_POP, Pop from the CSS style stack. 0 bytes
    // Combines with previous items of 'dl_class_pop' type.
    function (src: Uint8Array, dest: EngineOutput) {
        if (dest.length !== 0) {
            const last = dest[dest.length - 1];
            if (last[0] === 'dl_class_pop') {
                last[1]++;
                return 0;
            }
        }
        dest.push(['dl_class_pop', 1]);
        return 0;
    },
    // DL_ROOM, Add a room to the display list. 32 bytes: foreground8, background8, 30 data bytes
    function (src: Uint8Array, dest: EngineOutput) {
        dest.push(['dl_item', ['room', src[0], src[1], src.slice(2, 32)]]);
        return 32;
    },
    // DL_SPRITE, Add a sprite to the display list. 19 bytes: x, y, color, 16 data bytes
    function (src: Uint8Array, dest: EngineOutput) {
        dest.push(['dl_item', ['sprite', src[0], src[1], src[2], src.slice(3, 19)]]);
        return 19;
    },
    // DL_TEXT, Add text to the display list. Variable length: x, y, color, font_id, style, length16, string bytes
    function (src: Uint8Array, dest: EngineOutput) {
        const len16 = src[5] | (src[6] << 8);
        dest.push([
            'dl_item',
            ['text', src[0], src[1], src[2], src[3], src[4], textDecoder.decode(src.subarray(7, 7 + len16))],
        ]);
        return 7 + len16;
    },
    // DL_HLINE, Add a horizontal line to the display list. 4 bytes: x1, x2, y, color
    function (src: Uint8Array, dest: EngineOutput) {
        dest.push(['dl_item', ['hline', src[0], src[1], src[2], src[3]]]);
        return 4;
    },
    // DL_VLINE, Add a vertical line to the display list. 4 bytes: x, y1, y2, color
    function (src: Uint8Array, dest: EngineOutput) {
        dest.push(['dl_item', ['vline', src[0], src[1], src[2], src[3]]]);
        return 4;
    },
    // SAVE_FILE_CLOSED, The save file, open for writing, was just closed. 0 bytes
    function (src: Uint8Array, dest: EngineOutput) {
        dest.push(['save_file_closed']);
        return 0;
    },
];

function packInputItems(ioBuffer: Uint8Array, offset: number, items?: number[][] | null): number {
    let current = offset;
    if (items) {
        for (let item of items) {
            ioBuffer.set(item, current);
            current += item.length;
        }
    }
    return current - offset;
}

function consumeItems(byteCount: number, bytesPerItem: number, items?: number[][] | null) {
    if (items && byteCount) {
        items.splice(0, byteCount / bytesPerItem);
    }
}

export function packInput(ioBuffer: Uint8Array, input?: EngineInput | null) {
    const queues = new Uint32Array(ioBuffer.buffer, ioBuffer.byteOffset, 9);
    queues[2] = queues[5] = queues[8] = 0;
    const keyboardOffset = (queues[3] = queues.byteLength);
    const keyboardSize = (queues[4] = packInputItems(ioBuffer, keyboardOffset, input ? input.keyboard : null));
    const joystickOffset = (queues[6] = keyboardOffset + keyboardSize);
    const joystickSize = (queues[7] = packInputItems(ioBuffer, joystickOffset, input ? input.joystick : null));
    queues[0] = joystickOffset + joystickSize;
    queues[1] = ioBuffer.byteLength - queues[0];
}

export function consumeInput(ioBuffer: Uint8Array, input?: EngineInput | null) {
    const queues = new Uint32Array(ioBuffer.buffer, ioBuffer.byteOffset, 9);
    consumeItems(queues[5], 2, input ? input.keyboard : null);
    consumeItems(queues[8], 3, input ? input.joystick : null);
}

export function unpackOutput(ioBuffer: Uint8Array, exc: Error | null = null): EngineOutput {
    const result: EngineOutput = [];
    const queue = new Uint32Array(ioBuffer.buffer, ioBuffer.byteOffset, 3);
    var remaining = ioBuffer.subarray(queue[0], queue[0] + queue[2]);
    while (remaining.length > 0) {
        remaining = remaining.subarray(1 + unpackByOutputTag[remaining[0]](remaining.subarray(1), result));
    }
    if (exc) {
        result.push(['error', exc]);
    }
    return result;
}
