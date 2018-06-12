let context = null;

export function init(engine)
{
    engine.onRenderSound = onRenderSound;
}

function onRenderSound(pcmData, rate)
{
    // TO DO: Volume control
    const volume = 0.1;

    if (volume > 0.0 && audioContextSetup()) {
        const buffer = context.createBuffer(1, pcmData.length, rate);
        const channelData = buffer.getChannelData(0);
        for (let i = 0; i < pcmData.length; i++) {
            channelData[i] = pcmData[i] * volume;
        }

        const source = context.createBufferSource();
        source.buffer = buffer;
        source.connect(context.destination);
        source.start();
    }
}

export function audioContextSetup()
{
    if (context === null) {
        const AudioContext = window.AudioContext || window.webkitAudioContext;
        context = new AudioContext();
        if (!context) {
            return false;
        }
    }

    if (context.state == 'suspended') {
        context.resume();
        if (context.state == 'suspended') {
            return false;
        }
    }

    return true;
}
