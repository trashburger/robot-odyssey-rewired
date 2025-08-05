#include "output.h"
#include "hardware.h"
#include "sbt86.h"
#include <algorithm>
#include <emscripten.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <type_traits>

OutputInterface::OutputInterface(ColorTable &colorTable)
    : draw(colorTable), frame_counter(0), reference_timestamp(0) {}

void OutputInterface::clear() { frame_counter = 0; }

void OutputInterface::pushFrameCGA(uint32_t, SBTStack *, uint8_t *) {
    frame_counter++;
}

void OutputInterface::drawFrameRGB(uint32_t) { frame_counter++; }

void OutputInterface::pushDelay(uint32_t, OutputDelayType) {}

void OutputInterface::pushSpeakerTimestamp(uint32_t) {}

OutputQueue::OutputQueue(ColorTable &colorTable)
    : OutputInterface(colorTable), frameskip_value(0), frameskip_counter(0) {
    clear();
}

void OutputQueue::clear() {
    OutputInterface::clear();
    items.clear();
    frames.clear();
}

void OutputQueue::setFrameSkip(uint32_t frameskip) {
    frameskip_value = frameskip;
}

void OutputQueue::pushFrameCGA(uint32_t timestamp, SBTStack *stack,
                               uint8_t *framebuffer) {
    if (frames.full() || items.full()) {
        stack->trace();
        assert(0 && "Frame queue is too deep! Infinite loop likely.");
        return;
    }

    pushDelay(timestamp, OUT_DELAY_FLUSH);

    OutputItem item;
    item.otype = OUT_CGA_FRAME;
    items.push_back(item);

    // CGA frames are copied and queued
    frames.push_back(*(CGAFramebuffer *)framebuffer);
}

void OutputQueue::drawFrameRGB(uint32_t timestamp) {
    pushDelay(timestamp, OUT_DELAY_FLUSH);
    renderFrame();
}

void OutputQueue::renderFrame() {
    // Synchronously render a frame. Handles frame skip, if enabled.
    if (frameskip_counter < frameskip_value) {
        frameskip_counter++;
    } else {
        frameskip_counter = 0;
        EM_ASM_(
            { Module.onRenderFrame(HEAPU8.subarray($0, $1)); }, draw.backbuffer,
            sizeof draw.backbuffer + (uintptr_t)draw.backbuffer);
        frame_counter++;
    }
}

void OutputQueue::pushDelay(uint32_t timestamp, OutputDelayType delay_type) {
    const uint32_t elapsed_msec = clocksToMsec(timestamp - reference_timestamp);
    if (!elapsed_msec) {
        return;
    }

    if (!items.empty()) {
        OutputItem &back = items.back();
        switch (back.otype) {

        case OUT_DELAY:
            // Combine with an existing delay
            back.u.delay += elapsed_msec;
            reference_timestamp += msecToClocks(elapsed_msec);
            return;

        case OUT_CGA_FRAME:
            break;

        case OUT_SPEAKER_TIMESTAMP:
            switch (delay_type) {
            case OUT_DELAY_FLUSH:
                break;
            case OUT_DELAY_MERGE_WITH_SOUND:
                // Don't advance reference_timestamp
                return;
            }
            break;
        }
    }

    reference_timestamp += msecToClocks(elapsed_msec);

    if (!items.full()) {
        OutputItem item;
        item.otype = OUT_DELAY;
        item.u.delay = elapsed_msec;
        if (item.u.delay) {
            items.push_back(item);
        }
    }
}

void OutputQueue::pushSpeakerTimestamp(uint32_t timestamp) {
    if (items.full()) {
        assert(0 && "Speaker queue is too deep! Infinite loop likely.");
        return;
    }

    pushDelay(timestamp, OUT_DELAY_MERGE_WITH_SOUND);

    OutputItem item;
    item.otype = OUT_SPEAKER_TIMESTAMP;
    item.u.timestamp = timestamp;
    items.push_back(item);
}

void OutputQueue::dequeueCGAFrame() {
    assert(!frames.empty());
    CGAFramebuffer &frame = frames.front();

    // Expand CGA color to RGBA
    for (unsigned plane = 0; plane < 2; plane++) {
        for (unsigned y = 0; y < CGAFramebuffer::HEIGHT / 2; y++) {
            uint32_t *rgb_line = draw.backbuffer + (y * 2 + plane) *
                                                       RGBDraw::SCREEN_WIDTH *
                                                       CGAFramebuffer::ZOOM;

            for (unsigned x = 0; x < CGAFramebuffer::WIDTH; x++) {
                unsigned byte =
                    0x2000 * plane + (x + CGAFramebuffer::WIDTH * y) / 4;
                unsigned bit = 3 - (x % 4);
                unsigned color = 0x3 & (frame.bytes[byte] >> (bit * 2));
                uint32_t rgb = draw.colorTable.cga[color];

                // Zoom each CGA pixel
                for (unsigned zy = 0; zy < CGAFramebuffer::ZOOM; zy++) {
                    for (unsigned zx = 0; zx < CGAFramebuffer::ZOOM; zx++) {
                        rgb_line[zx + zy * RGBDraw::SCREEN_WIDTH] = rgb;
                    }
                }

                rgb_line += CGAFramebuffer::ZOOM;
            }
        }
    }

    // Free the ring buffer slot we were using
    frames.pop_front();
}

void OutputQueue::renderSoundEffect(uint32_t first_timestamp) {
    // Starting at the indicated timestamp and from the current output
    // queue position, slurp up all subsequent audio events and generate
    // a single PCM sound effect.

    // Filter design from notes/sound-filter-design.ipynb
    constexpr uint32_t padding_samples = AUDIO_HZ / 10;
    float filter_state[28][2] = {{0}};
    constexpr float second_order_stages[28][6] = {
        {0.009053953112749067, -0.018107906225498134, 0.009053953112749067, 1.0,
         -1.9796374795399938, 0.9798427166181983}, // 0 of 28
        {1.0, -2.0, 1.0, 1.0, -1.9816736138512914,
         0.9818400241394235}, // 1 of 28
        {1.0, -2.0, 1.0, 1.0, -1.9835061664508131,
         0.983641081639581}, // 2 of 28
        {1.0, -2.0, 1.0, 1.0, -1.985155486977734,
         0.9852648579720966}, // 3 of 28
        {1.0, -2.0, 1.0, 1.0, -1.9866398923976099,
         0.9867285483734656}, // 4 of 28
        {1.0, -2.0, 1.0, 1.0, -1.9879758696565433,
         0.9880477287809956}, // 5 of 28
        {1.0, -2.0, 1.0, 1.0, -1.9891782582335584,
         0.9892364989957404}, // 6 of 28
        {1.0, -2.0, 1.0, 1.0, -1.9902604145578904,
         0.9903076150206832}, // 7 of 28
        {1.0, -2.0, 1.0, 1.0, -1.9912343600727092,
         0.9912726110131748}, // 8 of 28
        {1.0, -2.0, 1.0, 1.0, -1.9921109145571194,
         0.9921419113636548}, // 9 of 28
        {1.0, -2.0, 1.0, 1.0, -1.992899816163347,
         0.9929249334576205}, // 10 of 28
        {1.0, -2.0, 1.0, 1.0, -1.9936098294848998,
         0.9936301817009935}, // 11 of 28
        {1.0, -2.0, 1.0, 1.0, -1.994248842843328,
         0.9942653333957537}, // 12 of 28
        {1.0, -2.0, 1.0, 1.0, -1.9948239558648986,
         0.9948373170469392}, // 13 of 28
        {1.0, -2.0, 1.0, 1.0, -1.9953415583132041,
         0.9953523836673354}, // 14 of 28
        {1.0, -2.0, 1.0, 1.0, -1.995807401048457,
         0.9958161716249172}, // 15 of 28
        {1.0, -2.0, 1.0, 1.0, -1.9962266598981226,
         0.9962337655524821}, // 16 of 28
        {1.0, -2.0, 1.0, 1.0, -1.9966039931458075,
         0.9966097498105327}, // 17 of 28
        {1.0, -2.0, 1.0, 1.0, -1.9969435932751376,
         0.9969482569645431}, // 18 of 28
        {1.0, -2.0, 1.0, 1.0, -1.997249233542087,
         0.9972530117072836}, // 19 of 28
        {1.0, -2.0, 1.0, 1.0, -1.9975243098921462,
         0.9975273706265284}, // 20 of 28
        {1.0, -2.0, 1.0, 1.0, -1.9977718786872791,
         0.9977743581887931}, // 21 of 28
        {1.0, -2.0, 1.0, 1.0, -1.9979946906612989,
         0.9979966992811273}, // 22 of 28
        {1.0, -2.0, 1.0, 1.0, -1.9981952214805052,
         0.9981968486256011}, // 23 of 28
        {1.0, -2.0, 1.0, 1.0, -1.9983756992488464,
         0.9983770173552453}, // 24 of 28
        {1.0, -2.0, 1.0, 1.0, -1.9985381292629996,
         0.9985391970158521}, // 25 of 28
        {1.0, 2.0, 1.0, 1.0, -1.9603951329819316,
         0.9623976551531928}, // 26 of 28
        {1.0, -2.0, 1.0, 1.0, -1.9859539013699274,
         0.986219511050537}, // 27 of 28
    };

    static_assert(sizeof filter_state / sizeof filter_state[0] ==
                      sizeof second_order_stages /
                          sizeof second_order_stages[0],
                  "filter stages and state array length must match");
    constexpr size_t num_filter_stages =
        sizeof filter_state / sizeof filter_state[0];

    constexpr int32_t cpu_clocks_per_sample =
        OutputQueue::CPU_CLOCK_HZ / AUDIO_HZ;

    uint32_t sample_count = 0;
    uint32_t sample_limit = AUDIO_BUFFER_SAMPLES;
    uint32_t ref_timestamp = first_timestamp;
    int32_t clocks_until_impulse = 0;
    float impulse = 1.f;

    while (sample_count < sample_limit) {
        float signal = 0.f;

        clocks_until_impulse -= cpu_clocks_per_sample;
        if (clocks_until_impulse < 0) {
            // Impulse here, and set up for the next one
            signal = impulse;
            impulse = -impulse;

            if (items.empty() || items.front().otype != OUT_SPEAKER_TIMESTAMP) {
                // No more timestamps; apply the padding and finish.
                sample_limit =
                    std::min(sample_limit, sample_count + padding_samples);
                clocks_until_impulse = INT_MAX;
            } else {
                uint32_t timestamp = items.front().u.timestamp;
                items.pop_front();
                clocks_until_impulse += int(timestamp - ref_timestamp);
                ref_timestamp = timestamp;
            }
        }

// IIR filter implemented as second-order stages
#pragma unroll
        for (size_t stage = 0; stage < num_filter_stages; stage++) {
            const float B0 = second_order_stages[stage][0];
            const float B1 = second_order_stages[stage][1];
            const float B2 = second_order_stages[stage][2];
            const float A0 = second_order_stages[stage][3];
            const float A1 = second_order_stages[stage][4];
            const float A2 = second_order_stages[stage][5];
            assert(A0 == 1.f); // wants to be static_assert but the loop
                               // isn't constexpr

            float &s1 = filter_state[stage][0];
            float &s2 = filter_state[stage][1];

            float x = signal;
            float y = B0 * x + s1;
            s1 = s2 + B1 * x - A1 * y;
            s2 = B2 * x - A2 * y;
            signal = y;
        }

        pcm_samples[sample_count++] = signal;
    }

    // Synchronously copy out the buffer and queue it for rendering, in
    // Javascript.
    EM_ASM_(
        { Module.onRenderSound(HEAPF32.subarray($0 / 4, $0 / 4 + $1), $2); },
        pcm_samples, sample_count, AUDIO_HZ);
}

uint32_t OutputQueue::run() {
    // Generate output until the queue is empty (returning zero) or
    // a delay (returning a nonzero number of milliseconds)

    while (!items.empty()) {
        OutputItem item = items.front();
        items.pop_front();

        switch (item.otype) {

        case OUT_CGA_FRAME:
            dequeueCGAFrame();
            renderFrame();
            break;

        case OUT_DELAY:
            assert(item.u.delay > 0);
            return item.u.delay;

        case OUT_SPEAKER_TIMESTAMP:
            renderSoundEffect(item.u.timestamp);
            break;
        }
    }
    return 0;
}
