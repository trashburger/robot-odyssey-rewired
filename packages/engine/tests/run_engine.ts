import {
    CLOCK_HZ,
    type Engine,
    type EngineInputProvider,
    type EngineOutput,
    type EngineOutputItem,
    type EngineFrame,
} from '@robot-odyssey-rewired/engine';
import { FrameCollector } from '@robot-odyssey-rewired/engine/dist/io/collector.js';

export function runEngineUntil(
    engine: Engine,
    match: (item: EngineOutputItem) => boolean,
    inputProvider?: EngineInputProvider,
    limit: number = 1000,
): EngineOutput[] {
    const result: EngineOutput[] = [];
    var done = false;
    var input = inputProvider ? inputProvider.provide() : null;
    do {
        const output = engine.run(input);
        if (output === null) {
            break;
        }
        for (var item of output) {
            if (!done && match(item)) done = true;
        }
        result.push(output);
        if (result.length > limit) {
            throw new Error('limit reached');
        }
    } while (!done);
    if (inputProvider && inputProvider.complete) {
        inputProvider.complete(input);
    }
    return result;
}

export function runEngineUntilDone(engine: Engine, input?: EngineInputProvider): EngineOutput[] {
    return runEngineUntil(
        engine,
        (item) => {
            switch (item[0]) {
                case 'exit':
                    return true;
                case 'error':
                    throw new Error('engine error while expecting exit', { cause: item[1] });
                default:
                    return false;
            }
        },
        input,
    );
}

export function runEngineUntilFrame(engine: Engine, input?: EngineInputProvider): EngineOutput[] {
    return runEngineUntil(
        engine,
        (item) => {
            switch (item[0]) {
                case 'dl_present':
                    return true;
                case 'exit':
                    throw new Error('engine exit while expecting frame', { cause: item[1] });
                case 'error':
                    throw new Error('engine error while expecting frame', { cause: item[1] });
                default:
                    return false;
            }
        },
        input,
    );
}

export function collectSingleFrame(engine: Engine, seconds: number, limit: number = 1000): EngineFrame {
    const collector = new FrameCollector();
    const clocks = seconds * CLOCK_HZ;
    while (limit > 0) {
        const frame = collector.collect(clocks);
        if (frame) {
            return frame;
        }

        const output = engine.run();
        if (output === null) {
            collector.pushInfiniteDelay();
        } else {
            collector.push(output);
        }
        limit -= 1;
    }
    throw new Error('limit reached');
}
