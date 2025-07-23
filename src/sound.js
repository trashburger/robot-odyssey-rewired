import * as EngineLoader from './engineLoader.js';

// TO DO: Volume control
let volume_control = 0.1;

let global_context = null;
let deferred_effect = null;

export function init() {
    EngineLoader.instance.onRenderSound = renderSound;
}

function renderSoundWithContext(context, volume, pcm, rate) {
    if (volume > 0.0 && context) {
        const buffer = context.createBuffer(1, pcm.length, rate);
        const channelData = buffer.getChannelData(0);
        for (let i = 0; i < pcm.length; i++) {
            channelData[i] = pcm[i] * volume;
        }

        const source = context.createBufferSource();
        source.buffer = buffer;
        source.connect(context.destination);
        source.start();
    }
}

function renderSound(pcm, rate) {
    if (audioContextSetup()) {
        // We can play it immediately from the engine's PCM output buffer
        renderSoundWithContext(global_context, volume_control, pcm, rate);
    } else {
        // We might be able to set up the context soon using a different input event.
        // Make a copy of the sound effect and save it. If audioContextSetup() succeeds
        // soon, we will play it then.
        deferred_effect = {
            deadline: performance.now() + 500,
            pcm: new Uint8Array(pcm),
            rate,
        };
    }
}

export function audioContextSetup() {
    if (global_context === null) {
        if (navigator.userActivation && !navigator.userActivation.isActive) {
            return false;
        }
        if (window.AudioContext) {
            global_context = new window.AudioContext();
        }
        if (!global_context) {
            return false;
        }
    }

    if (global_context.state === 'suspended') {
        global_context.resume();
        if (global_context.state === 'suspended') {
            return false;
        }
    }

    if (deferred_effect) {
        if (performance.now() < deferred_effect.deadline) {
            renderSoundWithContext(
                global_context,
                volume_control,
                deferred_effect.pcm,
                deferred_effect.rate,
            );
        }
        deferred_effect = null;
    }
    return true;
}
