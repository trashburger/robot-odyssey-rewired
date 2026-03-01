import { expect } from 'vitest';
import { type Engine } from '@robot-odyssey-rewired/engine';
import {
    InputEventTarget,
    RawJoystickEvent,
    RelativeJoystickEvent,
} from '@robot-odyssey-rewired/engine/dist/io/input.js';
import { engineOutputSummary, findSpriteSummary } from './summarize.ts';
import { runEngineUntilFrame } from './run_engine.ts';

const PLAYER_SPRITE_COLORS = new Set([5, 7]);
const PLAYER_SPRITE_HASHES = new Set([
    // Rectangle cursor
    '87dcde7fa6df23e15fa7ba9b2a1f31408eac832f4e615ea815ae92024e3d818b',
    // Person with walk cycle facing each direction
    '0ffaef57095828b0e3e01e4e4da83b0777f502928456aa04c9a27ed4f7b9e0b7',
    '553f41bfb7bb16201557b0855bee795b07b7e11758a787115f3c9d5ad3ed874b',
    '5c564243f8cb438d12c8db950a6059c3ca76319c36dd77d4f1f774615973d0ea',
    '7f3dcd7e162748a21306f7ce60d4bffa268ec990f0ce43f8fd156d375029707b',
    '8fc004b2da1965b106977d7339f654cbe630f0a9cb7b6a0b3770d1a5be6111ef',
    '9cd9c9b6f031b3642131953a1f120334c9c6ea1c91f3d5390bd1785b6e8f4966',
    'd83c718ec68191f820bb27aed3807e9a900ed787d84e47b918cb67692007abfe',
    'f7e5fef7605f1cb467e3292948c58c88c023f2221587ac62961c95abf4ecf6b0',
]);

function findPlayerSprite(output: EngineOutputSummary): SpriteSummary | null {
    return findSpriteSummary(output, PLAYER_SPRITE_HASHES, PLAYER_SPRITE_COLORS);
}

type PlayerMovement = [x: number, y: number];

function findPlayerMovement(last: SpriteSummary | null, next: SpriteSummary | null): PlayerMovement | null {
    if (last && next) {
        const [last_key, last_x, last_y, last_color, last_hash] = last;
        const [next_key, next_x, next_y, next_color, next_hash] = next;
        return [next_x - last_x, next_y - last_y];
    }
    return null;
}

class PlayerMovementTester {
    previous: SpriteSummary;
    engine: Engine;
    input: InputEventTarget;

    constructor(engine: Engine, input: InputEventTarget) {
        this.engine = engine;
        this.input = input;

        // Throw away output that may include engine startup
        this.runEngineUntilFrame();

        // First reference for calculating movement
        this.previous = findPlayerSprite(engineOutputSummary(this.runEngineUntilFrame()));
        expect(this.previous).not.toBe(null);
    }

    runEngineUntilFrame(): EngineOutput {
        return runEngineUntilFrame(this.engine, this.input);
    }

    get movement(): PlayerMovement | EngineOutputSummary {
        // Drop frames to let queued input motion finish
        for (let i = 0; i < 8; i++) {
            this.runEngineUntilFrame();
        }

        // Summarize change in player position
        const summary = engineOutputSummary(this.runEngineUntilFrame());
        const player = findPlayerSprite(summary);
        const movement = findPlayerMovement(this.previous, player);
        this.previous = player;

        // If we fail to find the player, returns engine output so that can be debugged
        return movement || summary;
    }
}

export function testArrowKeyMovement(engine: Engine) {
    const t = new PlayerMovementTester(engine, new InputEventTarget());
    for (let shift of [false, true]) {
        for (let [fn, expected] of [
            [
                () =>
                    t.input.dispatchEvent(Object.assign(new Event('keydown'), { code: 'ArrowLeft', shiftKey: shift })),
                shift ? [-1, 0] : [-8, 0],
            ],
            [
                () => t.input.dispatchEvent(Object.assign(new Event('keydown'), { code: 'ArrowUp', shiftKey: shift })),
                shift ? [0, 1] : [0, 16],
            ],
            [
                () =>
                    t.input.dispatchEvent(Object.assign(new Event('keydown'), { code: 'ArrowRight', shiftKey: shift })),
                shift ? [1, 0] : [8, 0],
            ],
            [
                () =>
                    t.input.dispatchEvent(Object.assign(new Event('keydown'), { code: 'ArrowDown', shiftKey: shift })),
                shift ? [0, -1] : [0, -16],
            ],
        ]) {
            const num_steps = shift ? 10 : 4;
            for (let i = 0; i < num_steps; i++) {
                fn();
                expect.soft(t.movement).toStrictEqual(expected);
            }
        }
    }
}

export function testRawJoystickMovement(engine: Engine) {
    const t = new PlayerMovementTester(engine, new InputEventTarget());
    for (let [[button, x, y], expected] of [
        [
            [0, 0, 0],
            [0, 0],
        ],
        [
            [1, 0, 0],
            [0, 0],
        ],
        [
            [0, -1, 0],
            [0, 0],
        ],
        [
            [0, -2, 0],
            [0, 0],
        ],
        [
            [1, 0, 0],
            [0, 0],
        ],
        [
            [0, 1, 0],
            [0, 0],
        ],
        [
            [0, 2, 0],
            [0, 0],
        ],
        [
            [0, -3, 0],
            [-1, 0],
        ],
        [
            [0, -4, 0],
            [-1, 0],
        ],
        [
            [0, 3, 0],
            [1, 0],
        ],
        [
            [0, 4, 0],
            [1, 0],
        ],
        [
            [0, -5, 0],
            [-2, 0],
        ],
        [
            [0, -6, 0],
            [-2, 0],
        ],
        [
            [0, 5, 0],
            [2, 0],
        ],
        [
            [0, 6, 0],
            [2, 0],
        ],
        [
            [0, -7, 0],
            [-4, 0],
        ],
        [
            [0, 7, 0],
            [4, 0],
        ],
        [
            [0, -8, 0],
            [-8, 0],
        ],
        [
            [0, 8, 0],
            [8, 0],
        ],
        [
            [0, -127, 0],
            [-8, 0],
        ],
        [
            [0, 127, 0],
            [8, 0],
        ],
        [
            [0, 0, 0],
            [0, 0],
        ],
        [
            [1, 0, 0],
            [0, 0],
        ],
        [
            [0, 0, -1],
            [0, 0],
        ],
        [
            [0, 0, -2],
            [0, 0],
        ],
        [
            [1, 0, 0, 0],
            [0, 0],
        ],
        [
            [0, 0, 1],
            [0, 0],
        ],
        [
            [0, 0, 2],
            [0, 0],
        ],
        [
            [0, 0, -3],
            [0, 1],
        ],
        [
            [0, 0, -4],
            [0, 1],
        ],
        [
            [0, 0, 3],
            [0, -1],
        ],
        [
            [0, 0, 4],
            [0, -1],
        ],
        [
            [0, 0, -5],
            [0, 2],
        ],
        [
            [0, 0, 5],
            [0, -2],
        ],
        [
            [0, 0, -6],
            [0, 4],
        ],
        [
            [0, 0, 6],
            [0, -4],
        ],
        [
            [0, 0, -7],
            [0, 8],
        ],
        [
            [0, 0, 7],
            [0, -8],
        ],
        [
            [0, 0, -8],
            [0, 16],
        ],
        [
            [0, 0, 8],
            [0, -16],
        ],
        [
            [0, 0, -127],
            [0, 16],
        ],
        [
            [0, 0, 127],
            [0, -16],
        ],
    ]) {
        for (let i = 0; i < 2; i++) {
            t.input.dispatchEvent(new RawJoystickEvent(button, x, y));
            expect.soft(t.movement).toStrictEqual(expected);
        }
    }
}

export function testRelativePixelJoystickMovement(engine: Engine) {
    const t = new PlayerMovementTester(engine, new InputEventTarget());

    for (let size = 1; size <= 26; size++) {
        t.input.dispatchEvent(new RelativeJoystickEvent(0, -size, 0));
        expect.soft(t.movement).toStrictEqual([-size, 0]);
        t.input.dispatchEvent(new RelativeJoystickEvent(0, size, 0));
        expect.soft(t.movement).toStrictEqual([size, 0]);
        t.input.dispatchEvent(new RelativeJoystickEvent(0, 0, size));
        expect.soft(t.movement).toStrictEqual([0, size]);
        t.input.dispatchEvent(new RelativeJoystickEvent(0, 0, -size));
        expect.soft(t.movement).toStrictEqual([0, -size]);
    }
}
