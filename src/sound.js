import * as EngineLoader from './engineLoader.js';

let global_context = null;
let deferred_effect = null;

export function init() {
    EngineLoader.instance.onRenderSound = renderSound;
}

function renderSoundWithContext(context, pcm, rate) {
    if (context) {
        const buffer = context.createBuffer(1, pcm.length, rate);
        buffer.getChannelData(0).set(pcm);

        const source = context.createBufferSource();
        source.buffer = buffer;
        source.connect(context.destination);
        source.start();
    }
}

function renderSound(pcm, rate) {
    if (audioContextSetup()) {
        // We can play it immediately from the engine's PCM output buffer
        renderSoundWithContext(global_context, pcm, rate);
    } else {
        // We might be able to set up the context soon using a different input event.
        // Make a copy of the sound effect and save it. If audioContextSetup() succeeds
        // soon, we will play it then.
        deferred_effect = {
            deadline: performance.now() + 500,
            pcm: pcm.slice(),
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
                deferred_effect.pcm,
                deferred_effect.rate,
            );
        }
        deferred_effect = null;
    }
    return true;
}
