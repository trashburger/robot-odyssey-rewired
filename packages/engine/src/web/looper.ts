import {
    type Engine,
    type DisplayListRenderer,
    type AudioRenderer,
    type EngineOutput,
    type EngineInputProvider,
    type EngineLooper,
    CLOCK_KHZ,
} from '../index.js';
import { FrameCollector } from '../io/collector.js';

export interface ErrorHandler {
    engineError: (error: { output: EngineOutput }) => void;
}

export class WebEngineLooper<InnerType extends Engine> implements EngineLooper<InnerType> {
    engine: InnerType;
    collector: FrameCollector;
    input: EngineInputProvider | null = null;
    display: DisplayListRenderer | null = null;
    audio: AudioRenderer | null = null;
    error: ErrorHandler | null = null;
    speed: number = 1;
    running!: boolean;

    constructor(engine: InnerType) {
        this.engine = engine;
        this.collector = new FrameCollector();

        let is_running = false;
        let has_callback = false;
        let frame_timestamp: number | null = null;

        const looper = (now: number) => {
            has_callback = false;

            if (frame_timestamp && now > frame_timestamp) {
                const clocks = (now - frame_timestamp) * this.speed * CLOCK_KHZ;
                const audio_context = this.audio ? this.audio.context : null;
                const input = this.input ? this.input.provide() : null;

                let frame = null;
                while (!(frame = this.collector.collect(clocks, audio_context))) {
                    const outputs = this.engine.run(input);
                    if (outputs === null) {
                        // Engine can't run any more
                        break;
                    } else {
                        this.collector.push(outputs);
                    }
                }

                if (input && this.input && this.input.complete) {
                    this.input.complete(input);
                }

                if (frame !== null) {
                    if (frame.error && this.error) {
                        this.error.engineError(frame.error);
                    }
                    if (frame.sound && this.audio) {
                        this.audio.present(frame.sound);
                    }
                    if (frame.dl && this.display) {
                        this.display.present(frame.dl);
                    }
                }
            }
            frame_timestamp = now;

            if (is_running && !has_callback) {
                window.requestAnimationFrame(looper);
                has_callback = true;
            }
        };

        Object.defineProperty(this, 'running', {
            get: function () {
                return is_running;
            },
            set: function (next_running: boolean) {
                if (next_running) {
                    if (!is_running) {
                        frame_timestamp = null;
                    }
                    if (!has_callback) {
                        window.requestAnimationFrame(looper);
                        has_callback = true;
                    }
                }
                is_running = next_running;
            },
        });
    }
}
