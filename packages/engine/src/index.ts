export interface FileInfoMap {
    [name: string]: Uint8Array;
}

export const enum JoystickItemType {
    NONE = 0, // No item, queue is empty
    RAW, // A single raw joystick polling result, -128 to +127
    RATE, // Continue to move the player at a rate of N/16 pixels per frame until the next event, -128 to +127
    RELATIVE, // Try to move the indicated number of pixels before moving to the next event, -128 to +127
    ABSOLUTE, // Try to move the player to the indicated position before moving to the next event, -1 to +254
}

export type KeyboardItem = [ascii: number, scancode: number];
export type JoystickItem = [type: JoystickItemType, button: number, x: number, y: number];

export interface EngineInput {
    joystick?: JoystickItem[];
    keyboard?: KeyboardItem[];
}

export interface EngineInputProvider {
    provide: () => EngineInput | null;
    complete?: (input: EngineInput) => void;
}

export type CGADisplayItem = ['cga', data: Uint8Array];
export type RoomDisplayItem = ['room', fg_color: number, bg_color: number, data: Uint8Array];
export type SpriteDisplayItem = ['sprite', x: number, y: number, color: number, data: Uint8Array];
export type HLineDisplayItem = ['hline', x1: number, x2: number, y: number, color: number];
export type VLineDisplayItem = ['vline', x: number, y1: number, y2: number, color: number];
export type TextDisplayItem = ['text', x: number, y: number, color: number, font: number, style: number, text: string];
export type LoadingDisplayItem = ['loading'];
export type ErrorDisplayItem = ['error', name: string, message: string];

// Union of basic display list items.
// Most of these corresponding to one single OutputTag from the Engine.
// 'cga' items are generated from framebuffer snapshots by FrameCollector.
export type DisplayListItemUnion =
    | CGADisplayItem
    | RoomDisplayItem
    | SpriteDisplayItem
    | HLineDisplayItem
    | VLineDisplayItem
    | TextDisplayItem
    | LoadingDisplayItem
    | ErrorDisplayItem;

// Classes may be shared between DisplayListItems. DisplayListClasses must be immutable.
export type DisplayListClasses = string[];

// Complete display list items include data from all OutputTag::DL_* including Classes.
// Each DisplayListItem gets a reference to the immutable class list that was active during its DisplayListItemUnion OutputTag.
export type DisplayListItem = [union: DisplayListItemUnion, classes: DisplayListClasses];
export type DisplayList = DisplayListItem[];

// CPU clock timers
export type ClockCycles = number;
export const CLOCK_KHZ: ClockCycles = 4770;
export const CLOCK_HZ: ClockCycles = CLOCK_KHZ * 1000;

// Full size clock diffs can appear alone or in a cga/speaker list
export type ClockItem = ['clock', cycles: ClockCycles];

// Speaker items each represent one audio impulse after a clock-cycle delay
export type SpeakerItem = [cycles: ClockCycles];

// CGAWriteItems represent an unaligned 16-bit write to an emulated framebuffer after a clock-cycle delay
export type CGAWriteItem = [cycles: ClockCycles, address: number, data: number];
export const CGA_SIZE: number = 0x4000;
export const CGA_WIDTH: number = 320;
export const CGA_HEIGHT: number = 200;

// CGA and Speaker items are grouped together as early as possible.
// Runs may have ClockItems as needed to represent longer delays.
export type CGAWriteRun = ['cga_wr', (ClockItem | CGAWriteItem)[]];
export type SpeakerRun = ['speaker', (ClockItem | SpeakerItem)[]];

// Everything produced by Engine::run()
export type EngineOutputItem =
    | ['error', error: Error]
    | ['loading', promise: Promise<Engine>]
    | ['exit', code: number]
    | ['log', message: string]
    | ['log_binary', data: Uint8Array]
    | ['stack', frames: string[]]
    | ['check_keyboard']
    | ['take_keyboard']
    | ['check_joystick']
    | ['take_joystick']
    | ['save_file_closed']
    | ['dl_clear']
    | ['dl_present']
    | ['dl_class_push', classes: string[]]
    | ['dl_class_pop', num_classes: number]
    | ['dl_item', union: DisplayListItemUnion]
    | ['cga_clear']
    | ClockItem
    | CGAWriteRun
    | SpeakerRun;

export interface DisplayListRenderer {
    present: (dl: DisplayListItem[]) => void;
}

export interface AudioBufferLike {
    duration: number;
    length: number;
    sampleRate: number;
    numberOfChannels: number;
    getChannelData(channel: number): Float32Array<ArrayBuffer>;
}

export interface AudioRenderer {
    context: AudioContext | null;
    present: (buffer: AudioBuffer | AudioBufferLike) => void;
}

export type EngineOutput = EngineOutputItem[];

export interface Engine {
    exec: (program: string, args?: string | null) => boolean;
    run: (input?: EngineInput | null) => EngineOutput | null;
}

export interface EngineWrapper<InnerType extends Engine> extends Engine {
    inner: InnerType | null;
}

export class EnginePromise<InnerType extends Engine> implements EngineWrapper<InnerType> {
    exec: (program: string, args?: string | null) => boolean;
    run: (input?: EngineInput | null) => EngineOutput | null;
    inner: InnerType | null = null;
    promise: Promise<InnerType>;

    constructor(promise: Promise<InnerType>) {
        this.promise = promise;

        this.exec = function (program: string, args?: string | null): boolean {
            promise.then((engine) => {
                engine.exec(program, args);
            });
            return true;
        };

        this.run = function (input?: EngineInput | null): EngineOutput | null {
            return [['loading', promise]];
        };

        promise.then(
            (engine: InnerType) => {
                this.inner = engine;
                this.exec = function (program: string, args?: string | null): boolean {
                    return engine.exec(program, args);
                };
                this.run = function (input?: EngineInput | null): EngineOutput | null {
                    return engine.run(input);
                };
            },
            (error: Error) => {
                this.exec = function (program: string, args?: string | null): boolean {
                    return false;
                };
                this.run = function (input?: EngineInput | null): EngineOutput | null {
                    return [['error', error]];
                };
            },
        );
    }
}

export interface EngineLooper<InnerType extends Engine> {
    engine: InnerType;
    speed: number;
    running: boolean;
}
