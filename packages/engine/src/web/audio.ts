import { type AudioRenderer, type AudioBufferLike } from '../index.js';

type DeferredSound = [buffer: AudioBufferLike, deadline: number];

export class WebAudioRenderer implements AudioRenderer {
    context: AudioContext | null;
    deferred: DeferredSound | null;

    constructor() {
        this.context = null;
        this.deferred = null;
    }

    setup(): boolean {
        if (!this.context) {
            if (navigator.userActivation && !navigator.userActivation.isActive) {
                return false;
            }
            if (window.AudioContext) {
                this.context = new window.AudioContext();
            }
            if (!this.context) {
                return false;
            }
        }
        if (this.context.state === 'suspended') {
            this.context.resume();
            if (this.context.state === 'suspended') {
                return false;
            }
        }
        if (this.deferred) {
            const [buffer, deadline] = this.deferred;
            this.deferred = null;
            if (performance.now() < deadline) {
                this.present(buffer);
            }
        }
        return true;
    }

    present(buffer: AudioBuffer | AudioBufferLike) {
        this.setup();
        const context = this.context;
        if (!context) {
            // If we can't create the context yet, save this sound for the near future
            const deadline = performance.now() + 500;
            this.deferred = [buffer, deadline];
            return;
        }

        const source = context.createBufferSource();

        if (buffer instanceof AudioBuffer) {
            // This is already a real WebAudio AudioBuffer (it was created after we acquired a context)
            source.buffer = buffer;
        } else {
            // This was a standalone AudioBuffer created without a context. Copy it into a real AudioBuffer.
            const new_buffer = context.createBuffer(1, buffer.length, buffer.sampleRate);
            new_buffer.getChannelData(0).set(buffer.getChannelData(0));
        }

        source.connect(context.destination);
        source.start();
    }
}
