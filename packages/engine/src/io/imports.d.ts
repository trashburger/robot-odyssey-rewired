declare module 'region2d' {
    type Rect =
        | { left: number; top: number; right: number; bottom: number }
        | [left: number, top: number, right: number, bottom: number]
        | { x: number; y: number; width: number; height: number };

    type Point = { x: number; y: number };

    export class Region2D {
        constructor(rect: Rect);

        static fromRects(rects: Rect[]): Region2D;
        getPath(): Point[][];
    }
}

declare module 'audio-buffer' {
    export default class AudioBuffer {
        duration: number;
        length: number;
        sampleRate: number;
        numberOfChannels: number;

        constructor(options: { length: number; numberOfChannels?: number; sampleRate?: number });

        getChannelData(channel: number): Float32Array<ArrayBuffer>;
        copyFromChannel(destination: Float32Array, channelNumber: number, startInChannel?: number): void;
        copyToChannel(source: Float32Array, channelNumber: number, startInChannel?: number): void;
    }
}
