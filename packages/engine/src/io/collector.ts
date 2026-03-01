import {
    CLOCK_HZ,
    CGA_SIZE,
    type ClockCycles,
    type DisplayList,
    type DisplayListItem,
    type DisplayListClasses,
    type EngineOutput,
    type AudioBufferLike,
    type ErrorDisplayItem,
    type LoadingDisplayItem,
} from '../index.js';
import { renderSpeakerImpulses } from './speaker.js';

// Collected engine output for one frame
export type EngineFrame = {
    dl: DisplayList | null;
    sound: AudioBuffer | AudioBufferLike | null;
    error: { output: EngineOutput } | null;
} | null;

class AudioCollector {
    intervals: ClockCycles[];
    duration: ClockCycles;
    nextInterval: ClockCycles;

    // A new AudioCollector starts with a single impulse (no intervals, duration 0)
    constructor() {
        this.intervals = [];
        this.duration = 0;
        this.nextInterval = 0;
    }
}

export class FrameCollector {
    // Pending EngineOutput that's been push()'ed but not fully collected
    pending: EngineOutput[];

    // Number of clock cycles collected so far, including a residual from the last frame.
    cycles: ClockCycles;

    // Current class stack. Must be replaced rather than modified in-place.
    classes: DisplayListClasses;

    // Current display list under construction.
    dl: DisplayList;

    // Current offset into pending[0]
    itemCursor: number;

    // Current offset into a CGAWriteRun
    runCursor: number;

    // Current CGA framebuffer
    cga: Uint8Array;

    // Does the CGA framebuffer have not-yet-presented changes?
    presentCGA: boolean;

    // Display list waiting to be presented next
    presentDL: DisplayList | null;

    // Speaker impulses collected so far
    audio: AudioCollector | null;

    constructor() {
        this.pending = [];
        this.cycles = 0;
        this.classes = [];
        this.dl = [];
        this.itemCursor = 0;
        this.runCursor = 0;
        this.cga = new Uint8Array(CGA_SIZE);
        this.presentCGA = false;
        this.presentDL = null;
        this.audio = null;
    }

    // Discard the in-progress display list
    clearDL() {
        this.dl.length = 0;
        if (this.classes.length !== 0) {
            this.classes = [];
        }
    }

    // Discard all buffered state
    clear() {
        this.pending.length = 0;
        this.cycles = 0;
        this.itemCursor = 0;
        this.runCursor = 0;
        this.clearDL();
        this.cga.fill(0);
        this.presentCGA = false;
        this.presentDL = null;
        this.audio = null;
    }

    // Buffer a batch of engine output
    push(outputs: EngineOutput) {
        this.pending.push(outputs);
    }

    // Buffer an infinite delay. Engine output already buffered will be flushed
    // to collect(), then freeze until clear().
    pushInfiniteDelay() {
        this.pending.push([['clock', Infinity]]);
    }

    // Collect one EngineFrame of the indicated duration.
    // When null is returned, push() more EngineOutput and try again.
    // Sound effects will be rendered to PCM if an audio context is provided.
    collect(duration: ClockCycles, audioContext?: AudioContext | null): EngineFrame {
        const pending = this.pending;
        const pending_len = pending.length;
        const cga = this.cga;

        let classes = this.classes; // Local cache of class stack
        let cycles = this.cycles; // Local cycle count; done when >= duration
        let audio = this.audio; // AudioCollector when at least one impulse is buffered, null when quiet
        let dl = this.dl; // Local cache of display list under construction

        let present_cga = this.presentCGA; // True if any applied CGA writes need to be presented
        let present_dl = this.presentDL; // Display list ready to present, if any

        let audio_intervals = audio?.intervals || []; // Cached array of collected audio intervals
        let audio_duration = audio?.duration || 0; // Cached collected audio duration
        let audio_next_interval = audio?.nextInterval || 0; // Cached clock cycles for the next audio interval

        let pending_cursor = 0; // Always start with pending[0] but maybe continue past that
        let item_cursor = this.itemCursor; // Cached index of item in pending[0]
        let run_cursor = this.runCursor; // Cached index of item in a CGA run

        while (pending_cursor < pending_len) {
            const output_list = pending[pending_cursor];
            const output_len = output_list.length;

            while (cycles < duration && item_cursor < output_len) {
                const item = output_list[item_cursor];
                switch (item[0]) {
                    case 'error': {
                        // On error, save all EngineOutput for diagnostic purposes and clear the collector's state.
                        // Local cached state is discarded. The full engine output is availble in the EngineFrame.
                        // Message and name go into the display list so we can render the error visually if needed.
                        const error = item[1];
                        const classes: DisplayListClasses = [];
                        const err_display: ErrorDisplayItem = ['error', error.name, error.message];
                        const dli: DisplayListItem = [err_display, classes];
                        const result = {
                            dl: [dli],
                            sound: null,
                            error: { output: pending.flat() },
                        };
                        this.clear();
                        return result;
                    }

                    case 'loading': {
                        // Engine is still loading. EngineOutput has the raw promise, but the display list omits it.
                        const classes: DisplayListClasses = [];
                        const loading_display: LoadingDisplayItem = ['loading'];
                        const dli: DisplayListItem = [loading_display, classes];
                        const result = { dl: [dli], sound: null, error: null };
                        this.clear();
                        return result;
                    }

                    case 'dl_clear': {
                        // Clear the display list under construction, and reset the class stack
                        this.clearDL();
                        item_cursor++;
                        break;
                    }

                    case 'dl_present': {
                        // The latest completed display list per EngineFrame will be saved
                        present_dl = dl;
                        present_cga = false;
                        this.dl = dl = [];
                        item_cursor++;
                        break;
                    }

                    case 'dl_item': {
                        // Annotate items with a reference to the immutable class stack
                        dl.push([item[1], classes]);
                        item_cursor++;
                        break;
                    }

                    case 'dl_class_push': {
                        // New class stack with appended items
                        classes = classes.concat(item[1]);
                        item_cursor++;
                        break;
                    }

                    case 'dl_class_pop': {
                        // New class stack with deleted items
                        classes = classes.slice(0, -item[1]);
                        item_cursor++;
                        break;
                    }

                    case 'clock': {
                        // Elapsed time in CPU cycles
                        cycles += item[1];
                        audio_next_interval += item[1];
                        item_cursor++;
                        break;
                    }

                    case 'speaker': {
                        // A run of speaker impulses and clock updates.
                        // These are never split across frames.

                        for (let speaker_item of item[1]) {
                            if (speaker_item[0] === 'clock') {
                                // Wide clock update
                                const clock_item = speaker_item[1];
                                cycles += clock_item;
                                audio_next_interval += clock_item;
                            } else {
                                // Narrow clock update and then a speaker impulse
                                const clock_item = speaker_item[0];
                                cycles += clock_item;
                                audio_next_interval += clock_item;
                                if (audio) {
                                    // Add as an interval on an existing AudioCollector
                                    audio_intervals.push(audio_next_interval);
                                    audio_duration += audio_next_interval;
                                } else {
                                    // Start a new AudioCollector with one impulse
                                    audio = new AudioCollector();
                                    audio_intervals = audio.intervals;
                                    audio_duration = 0;
                                }
                                audio_next_interval = 0;
                            }
                        }
                        item_cursor++;
                        break;
                    }

                    case 'cga_wr': {
                        // A run of CGA writes and clock updates.
                        // We may split a single CGA write run across multiple output frames.

                        const cga_list = item[1];
                        const cga_len = cga_list.length;

                        present_cga = true;
                        present_dl = null;

                        do {
                            const cga_item = cga_list[run_cursor];
                            if (cga_item[0] === 'clock') {
                                // Wide clock update
                                const clock_item = cga_item[1];
                                cycles += clock_item;
                                audio_next_interval += clock_item;
                            } else {
                                // Narrow clock update and then a CGA write
                                const [clock_item, address, data] = cga_item;
                                cycles += clock_item;
                                audio_next_interval += clock_item;
                                cga[address] = data & 0xff;
                                cga[address + 1] = data >> 8;
                            }
                            run_cursor++;
                            if (run_cursor >= cga_len) {
                                item_cursor++;
                                run_cursor = 0;
                                break;
                            }
                        } while (cycles < duration);
                        break;
                    }

                    case 'cga_clear': {
                        // Clear the CGA framebuffer to zero

                        present_cga = true;
                        present_dl = null;
                        cga.fill(0);
                        item_cursor++;
                        break;
                    }

                    default: {
                        item_cursor++;
                        break;
                    }
                }
            }

            if (cycles >= duration) {
                break;
            }
            pending_cursor++;
            item_cursor = 0;
        }

        // Decide on a result. Errors were reported immediately.
        // If insufficient time has passed, plan to return null.
        let result = null;
        if (cycles >= duration) {
            // If there were any CGA writes, take a snapshot of the framebuffer as a display list item
            if (present_cga && !present_dl) {
                present_dl = [[['cga', cga.slice()], []]];
                present_cga = false;
            }

            result = {
                dl: present_dl,
                sound: audio ? renderSpeakerImpulses(CLOCK_HZ, audio_intervals, audio_duration, audioContext) : null,
                error: null,
            };

            cycles -= duration;
            audio = null;
            present_dl = null;
        }

        // If pending_cursor advanced, we can remove finished arrays from this.pending
        if (pending_cursor !== 0) {
            this.pending.splice(0, pending_cursor);
        }

        // Write back cached locals
        if (audio) {
            audio.duration = audio_duration;
            audio.nextInterval = audio_next_interval;
        }
        this.classes = classes;
        this.cycles = cycles;
        this.audio = audio;
        this.dl = dl;
        this.itemCursor = item_cursor;
        this.runCursor = run_cursor;
        this.presentCGA = present_cga;
        this.presentDL = present_dl;

        return result;
    }
}
