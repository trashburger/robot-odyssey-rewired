import StandaloneAudioBuffer from 'audio-buffer';
import { type AudioBufferLike } from '../index.js';

export function renderSpeakerImpulses(
    clockFrequency: number,
    intervals: number[],
    duration: number,
    context?: AudioContext | null,
): AudioBuffer | AudioBufferLike {
    // Render at least one audio impulse to PCM, using a multi-stage IIR filter.
    // Filter design from notes/sound-filter-design.ipynb

    const filter_hz = 48000;
    const padding_samples = filter_hz / 10;
    const clocks_per_sample = clockFrequency / filter_hz;
    const total_samples = padding_samples + Math.round(duration / clocks_per_sample);
    const buffer = context
        ? context.createBuffer(1, total_samples, filter_hz)
        : new StandaloneAudioBuffer({ numberOfChannels: 1, length: total_samples, sampleRate: filter_hz });
    const data = buffer.getChannelData(0);

    let next_interval = 0;
    let clocks_until_impulse = 0;
    let impulse = 1;

    let s1a = 0,
        s1b = 0,
        s2a = 0,
        s2b = 0,
        s3a = 0,
        s3b = 0,
        s4a = 0,
        s4b = 0,
        s5a = 0,
        s5b = 0,
        s6a = 0,
        s6b = 0,
        s7a = 0,
        s7b = 0,
        s8a = 0,
        s8b = 0,
        s9a = 0,
        s9b = 0,
        s10a = 0,
        s10b = 0,
        s11a = 0,
        s11b = 0,
        s12a = 0,
        s12b = 0,
        s13a = 0,
        s13b = 0,
        s14a = 0,
        s14b = 0,
        s15a = 0,
        s15b = 0,
        s16a = 0,
        s16b = 0,
        s17a = 0,
        s17b = 0,
        s18a = 0,
        s18b = 0,
        s19a = 0,
        s19b = 0,
        s20a = 0,
        s20b = 0,
        s21a = 0,
        s21b = 0,
        s22a = 0,
        s22b = 0,
        s23a = 0,
        s23b = 0,
        s24a = 0,
        s24b = 0,
        s25a = 0,
        s25b = 0,
        s26a = 0,
        s26b = 0,
        s27a = 0,
        s27b = 0,
        s28a = 0,
        s28b = 0;

    for (let sample_num = 0; sample_num < total_samples; sample_num++) {
        let x = 0;

        clocks_until_impulse -= clocks_per_sample;
        if (clocks_until_impulse < 0) {
            // Impulse here
            x = impulse;
            impulse = -impulse;

            if (next_interval < intervals.length) {
                // Set up the next impulse
                clocks_until_impulse += intervals[next_interval++];
            } else {
                // Done, let the padding finish
                clocks_until_impulse = Infinity;
            }
        }

        {
            // Stage 1 of 28
            const y = 0.009053953112749067 * x + s1a;
            s1a = s1b + -0.018107906225498134 * x - -1.9796374795399938 * y;
            s1b = 0.009053953112749067 * x - 0.9798427166181983 * y;
            x = y;
        }
        {
            // Stage 2 of 28
            const y = x + s2a;
            s2a = s2b + -2.0 * x - -1.9816736138512914 * y;
            s2b = x - 0.9818400241394235 * y;
            x = y;
        }
        {
            // Stage 3 of 28
            const y = x + s3a;
            s3a = s3b + -2.0 * x - -1.9835061664508131 * y;
            s3b = x - 0.983641081639581 * y;
            x = y;
        }
        {
            // Stage 4 of 28
            const y = x + s4a;
            s4a = s4b + -2.0 * x - -1.985155486977734 * y;
            s4b = x - 0.9852648579720966 * y;
            x = y;
        }
        {
            // Stage 5 of 28
            const y = x + s5a;
            s5a = s5b + -2.0 * x - -1.9866398923976099 * y;
            s5b = x - 0.9867285483734656 * y;
            x = y;
        }
        {
            // Stage 6 of 28
            const y = x + s6a;
            s6a = s6b + -2.0 * x - -1.9879758696565433 * y;
            s6b = x - 0.9880477287809956 * y;
            x = y;
        }
        {
            // Stage 7 of 28
            const y = x + s7a;
            s7a = s7b + -2.0 * x - -1.9891782582335584 * y;
            s7b = x - 0.9892364989957404 * y;
            x = y;
        }
        {
            // Stage 8 of 28
            const y = x + s8a;
            s8a = s8b + -2.0 * x - -1.9902604145578904 * y;
            s8b = x - 0.9903076150206832 * y;
            x = y;
        }
        {
            // Stage 9 of 28
            const y = x + s9a;
            s9a = s9b + -2.0 * x - -1.9912343600727092 * y;
            s9b = x - 0.9912726110131748 * y;
            x = y;
        }
        {
            // Stage 10 of 28
            const y = x + s10a;
            s10a = s10b + -2.0 * x - -1.9921109145571194 * y;
            s10b = x - 0.9921419113636548 * y;
            x = y;
        }
        {
            // Stage 11 of 28
            const y = x + s11a;
            s11a = s11b + -2.0 * x - -1.992899816163347 * y;
            s11b = x - 0.9929249334576205 * y;
            x = y;
        }
        {
            // Stage 12 of 28
            const y = x + s12a;
            s12a = s12b + -2.0 * x - -1.9936098294848998 * y;
            s12b = x - 0.9936301817009935 * y;
            x = y;
        }
        {
            // Stage 13 of 28
            const y = x + s13a;
            s13a = s13b + -2.0 * x - -1.994248842843328 * y;
            s13b = x - 0.9942653333957537 * y;
            x = y;
        }
        {
            // Stage 14 of 28
            const y = x + s14a;
            s14a = s14b + -2.0 * x - -1.9948239558648986 * y;
            s14b = x - 0.9948373170469392 * y;
            x = y;
        }
        {
            // Stage 15 of 28
            const y = x + s15a;
            s15a = s15b + -2.0 * x - -1.9953415583132041 * y;
            s15b = x - 0.9953523836673354 * y;
            x = y;
        }
        {
            // Stage 16 of 28
            const y = x + s16a;
            s16a = s16b + -2.0 * x - -1.995807401048457 * y;
            s16b = x - 0.9958161716249172 * y;
            x = y;
        }
        {
            // Stage 17 of 28
            const y = x + s17a;
            s17a = s17b + -2.0 * x - -1.9962266598981226 * y;
            s17b = x - 0.9962337655524821 * y;
            x = y;
        }
        {
            // Stage 18 of 28
            const y = x + s18a;
            s18a = s18b + -2.0 * x - -1.9966039931458075 * y;
            s18b = x - 0.9966097498105327 * y;
            x = y;
        }
        {
            // Stage 19 of 28
            const y = x + s19a;
            s19a = s19b + -2.0 * x - -1.9969435932751376 * y;
            s19b = x - 0.9969482569645431 * y;
            x = y;
        }
        {
            // Stage 20 of 28
            const y = x + s20a;
            s20a = s20b + -2.0 * x - -1.997249233542087 * y;
            s20b = x - 0.9972530117072836 * y;
            x = y;
        }
        {
            // Stage 21 of 28
            const y = x + s21a;
            s21a = s21b + -2.0 * x - -1.9975243098921462 * y;
            s21b = x - 0.9975273706265284 * y;
            x = y;
        }
        {
            // Stage 22 of 28
            const y = x + s22a;
            s22a = s22b + -2.0 * x - -1.9977718786872791 * y;
            s22b = x - 0.9977743581887931 * y;
            x = y;
        }
        {
            // Stage 23 of 28
            const y = x + s23a;
            s23a = s23b + -2.0 * x - -1.9979946906612989 * y;
            s23b = x - 0.9979966992811273 * y;
            x = y;
        }
        {
            // Stage 24 of 28
            const y = x + s24a;
            s24a = s24b + -2.0 * x - -1.9981952214805052 * y;
            s24b = x - 0.9981968486256011 * y;
            x = y;
        }
        {
            // Stage 25 of 28
            const y = x + s25a;
            s25a = s25b + -2.0 * x - -1.9983756992488464 * y;
            s25b = x - 0.9983770173552453 * y;
            x = y;
        }
        {
            // Stage 26 of 28
            const y = x + s26a;
            s26a = s26b + -2.0 * x - -1.9985381292629996 * y;
            s26b = x - 0.9985391970158521 * y;
            x = y;
        }
        {
            // Stage 27 of 28
            const y = x + s27a;
            s27a = s27b + 2.0 * x - -1.9603951329819316 * y;
            s27b = x - 0.9623976551531928 * y;
            x = y;
        }
        {
            // Stage 28 of 28
            const y = x + s28a;
            s28a = s28b + -2.0 * x - -1.9859539013699274 * y;
            s28b = x - 0.986219511050537 * y;
            x = y;
        }

        data[sample_num] = x;
    }
    return buffer;
}
