import { createHash } from 'node:crypto';
import {
    type EngineFrame,
    type EngineOutput,
    type EngineOutputItem,
    type FileInfoMap,
    type DisplayListItem,
    type DisplayListClasses,
} from '@robot-odyssey-rewired/engine';

export type CGASummary = ['cga_summary', length: number, hash: string];
export type SpriteSummary = ['sprite_summary', x: number, y: number, color: number, data_hash: string];
export type RoomSummary = ['room_summary', fg_color: number, bg_color: number, data_hash: string];
export type TextSummary = [
    'text_summary',
    x: number,
    y: number,
    color: number,
    font: number,
    style: number,
    text_length: number,
    text_hash: string,
];

export type DisplayListItemUnionSummary = CGASummary | SpriteSummary | RoomSummary | TextSummary;

function displayListItemUnionSummary(item: DisplayListItemUnion): DisplayListItemUnionSummary | null {
    switch (item[0]) {
        case 'sprite': {
            const hash = createHash('sha256');
            const [key, x, y, color, data] = item;
            hash.update(data);
            return ['sprite_summary', x, y, color, hash.digest('hex')];
        }

        case 'room': {
            const hash = createHash('sha256');
            const [key, fg, bg, data] = item;
            hash.update(data);
            return ['room_summary', fg, bg, hash.digest('hex')];
        }

        case 'text': {
            const hash = createHash('sha256');
            const [key, x, y, color, font, style, text] = item;
            hash.update(text);
            return ['text_summary', x, y, color, font, style, text.length, hash.digest('hex')];
        }

        case 'cga': {
            const hash = createHash('sha256');
            const [key, data] = item;
            hash.update(data);
            return ['cga_summary', data.length, hash.digest('hex')];
        }

        default:
            return null;
    }
}

export type DisplayListItemSummary = [
    union: DisplayListItemUnion | DisplayListItemUnionSummary,
    classes: DisplayListClasses,
];
export type DisplayListSummary = DisplayListItem[];

function displayListSummary(dl: DisplayList): DisplayListSummary {
    const result: DisplayListSummary = [];
    for (var item of dl) {
        const [union, classes] = item;
        const summary = displayListItemUnionSummary(union);
        result.push([summary ? summary : union, classes]);
    }
    return result;
}

export type EngineOutputSummaryItem =
    | EngineOutputItem
    | DisplayListSummaryItemUnion
    | ['return_from_run']
    | ['cga_wr_summary', count: number, duration: number, data_hash: string]
    | ['speaker_summary', count: number, duration: number, data_hash: string];

export type EngineOutputSummary = EngineOutputSummaryItem[];

export function engineOutputSummary(outputs: EngineOutput[]): EngineOutputSummary {
    const result: EngineOutputSummary = [];
    for (var output of outputs) {
        for (var item of output) {
            switch (item[0]) {
                case 'cga_wr': {
                    const hash = createHash('sha256');
                    const list = item[1];
                    let duration = 0;
                    for (var item of list) {
                        duration += item[0] === 'clock' ? item[1] : item[0];
                        hash.update(item.toString());
                    }
                    result.push(['cga_wr_summary', list.length, duration, hash.digest('hex')]);
                    break;
                }

                case 'speaker': {
                    const hash = createHash('sha256');
                    const list = item[1];
                    let duration = 0;
                    for (var item of list) {
                        duration += item[0] === 'clock' ? item[1] : item[0];
                        hash.update(item.toString());
                    }
                    result.push(['speaker_summary', list.length, duration, hash.digest('hex')]);
                    break;
                }

                case 'dl_item': {
                    const summary = displayListItemUnionSummary(item[1]);
                    result.push(summary === null ? item : summary);
                    break;
                }

                default: {
                    result.push(item);
                    break;
                }
            }
        }
        result.push(['return_from_run']);
    }
    return result;
}

export function findSpriteSummary(
    output: EngineOutputSummary,
    hashes: Set<string>,
    colors: Set<number>,
): SpriteSummary | null {
    let num_matches = 0;
    let result = null;
    for (let item of output) {
        if (item[0] === 'sprite_summary') {
            const [key, x, y, color, hash] = item;
            if (hashes.has(hash) && colors.has(color)) {
                result = item;
                num_matches++;
            }
        }
    }
    return num_matches === 1 ? result : null;
}

export interface FileInfoSummary {
    [name: string]: [length: number, hash: string];
}

export function fileInfoSummary(map: FileInfoMap): FileInfoSummary {
    const result: FileInfoSummary = {};
    for (var name in map) {
        const hash = createHash('sha256');
        const data = map[name];
        hash.update(data);
        result[name] = [data.length, hash.digest('hex')];
    }
    return result;
}

export type AudioBufferSummary = [sampleRate: number, numberOfChannels: number, length: number, hash: string];

export function audioBufferSummary(buffer: AudioBuffer): AudioBufferSummary {
    const hash = createHash('sha256');
    for (let channel = 0; channel < buffer.numberOfChannels; channel++) {
        hash.update(buffer.getChannelData(channel));
    }
    return [buffer.sampleRate, buffer.numberOfChannels, buffer.length, hash.digest('hex')];
}

export type EngineFrameSummary =
    | null
    | { error: { output: EngineOutputSummary } }
    | { dl: DisplayListSummary | null; sound: AudioBufferSummary | null };

export function engineFrameSummary(frame: EngineFrame): EngineFrameSummary {
    if (frame === null) {
        return null;
    } else if (frame.error) {
        return { error: { output: engineOutputSummary([frame.error.output]) } };
    } else {
        return {
            dl: frame.dl === null ? null : displayListSummary(frame.dl),
            sound_summary: frame.sound === null ? null : audioBufferSummary(frame.sound),
        };
    }
}
