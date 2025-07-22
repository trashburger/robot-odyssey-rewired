#include <emscripten.h>
#include <algorithm>
#include <string.h>
#include <stdio.h>
#include "sbt86.h"
#include "hardware.h"


OutputInterface::OutputInterface(ColorTable &colorTable)
    : draw(colorTable), frame_counter(0), reference_timestamp(0)
{}

void OutputInterface::clear()
{
    frame_counter = 0;
}

void OutputInterface::pushFrameCGA(uint32_t, SBTStack *, uint8_t *)
{
    frame_counter++;
}

void OutputInterface::drawFrameRGB(uint32_t)
{
    frame_counter++;
}

void OutputInterface::pushDelay(uint32_t, uint32_t)
{}

void OutputInterface::pushSpeakerTimestamp(uint32_t)
{}

OutputQueue::OutputQueue(ColorTable &colorTable)
    : OutputInterface(colorTable),
      frameskip_value(0),
      frameskip_counter(0)
{
    clear();
}

void OutputQueue::clear()
{
    OutputInterface::clear();
    items.clear();
    frames.clear();
}

void OutputQueue::setFrameSkip(uint32_t frameskip)
{
    frameskip_value = frameskip;
}

void OutputQueue::pushFrameCGA(uint32_t timestamp, SBTStack *stack, uint8_t *framebuffer)
{
    if (frames.full() || items.full()) {
        stack->trace();
        assert(0 && "Frame queue is too deep! Infinite loop likely.");
        return;
    }

    OutputQueue::pushDelay(timestamp, 0);

    OutputItem item;
    item.otype = OUT_CGA_FRAME;
    items.push_back(item);

    // CGA frames are copied and queued
    frames.push_back(*(CGAFramebuffer*) framebuffer);
}

void OutputQueue::drawFrameRGB(uint32_t timestamp)
{
    pushDelay(timestamp, 0);
    renderFrame();
}

void OutputQueue::renderFrame()
{
    // Synchronously render a frame. Handles frame skip, if enabled.
    if (frameskip_counter < frameskip_value) {
        frameskip_counter++;
    } else {
        frameskip_counter = 0;
        EM_ASM_({
           Module.onRenderFrame(HEAPU8.subarray($0, $1));
        }, draw.backbuffer, sizeof draw.backbuffer + (uintptr_t)draw.backbuffer);
        frame_counter++;
    }
}

void OutputQueue::pushDelay(uint32_t timestamp, uint32_t millis)
{
    constexpr uint32_t clocks_per_msec = (CPU_CLOCK_HZ + 500) / 1000;

    const uint32_t elapsed_clocks = timestamp - reference_timestamp;
    const uint32_t elapsed_msec = (elapsed_clocks + clocks_per_msec / 2) / clocks_per_msec;
    reference_timestamp += elapsed_msec * clocks_per_msec;
    millis += elapsed_msec;
    if (!millis) {
        return;
    }

    if (!items.empty()) {
        OutputItem &back = items.back();
        if (back.otype == OUT_DELAY) {
            // Combine with an existing delay
            back.u.delay += millis;
            return;
        }
    }

    if (!items.full()) {
        OutputItem item;
        item.otype = OUT_DELAY;
        item.u.delay = millis;
        items.push_back(item);
    }
}

void OutputQueue::pushSpeakerTimestamp(uint32_t timestamp)
{
    if (items.full()) {
        assert(0 && "Speaker queue is too deep! Infinite loop likely.");
        return;
    }

    // Generate one OUT_DELAY before the effect begins, then a run of OUT_SPEAKER_TIMESTAMP only.
    if (items.empty() || items.back().otype != OUT_SPEAKER_TIMESTAMP) {
        pushDelay(timestamp, 0);
    }

    OutputItem item;
    item.otype = OUT_SPEAKER_TIMESTAMP;
    item.u.timestamp = timestamp;
    items.push_back(item);
}

void OutputQueue::dequeueCGAFrame()
{
    assert(!frames.empty());
    CGAFramebuffer &frame = frames.front();

    // Expand CGA color to RGBA
    for (unsigned plane = 0; plane < 2; plane++) {
        for (unsigned y=0; y < CGAFramebuffer::HEIGHT/2; y++) {
            uint32_t *rgb_line = draw.backbuffer + (y*2+plane)*RGBDraw::SCREEN_WIDTH*CGAFramebuffer::ZOOM;

            for (unsigned x=0; x < CGAFramebuffer::WIDTH; x++) {
                unsigned byte = 0x2000*plane + (x + CGAFramebuffer::WIDTH*y)/4;
                unsigned bit = 3 - (x % 4);
                unsigned color = 0x3 & (frame.bytes[byte] >> (bit * 2));
                uint32_t rgb = draw.colorTable.cga[color];

                // Zoom each CGA pixel
                for (unsigned zy=0; zy<CGAFramebuffer::ZOOM; zy++) {
                    for (unsigned zx=0; zx<CGAFramebuffer::ZOOM; zx++) {
                        rgb_line[zx + zy*RGBDraw::SCREEN_WIDTH] = rgb;
                    }
                }

                rgb_line += CGAFramebuffer::ZOOM;
            }
        }
    }

    // Free the ring buffer slot we were using
    frames.pop_front();
}

void OutputQueue::renderSoundEffect(uint32_t first_timestamp)
{
    // Starting at the indicated timestamp and from the current output
    // queue position, slurp up all subsequent audio events and generate
    // a single PCM sound effect. Returns the delay, in milliseconds,
    // to insert concurrently with sound playback.

    // The first sample at index zero is always a "1", then the delay
    // we load from the output queue tells us where to put a "0", then
    // where to put the next "1", etc.

    uint32_t previous_timestamp = first_timestamp;
    int8_t next_sample = 1;
    uint32_t sample_count = 0;
    int32_t clocks_remaining = 0;

    while (sample_count < AUDIO_BUFFER_SAMPLES && clocks_remaining >= 0) {

        pcm_samples[sample_count] = next_sample;
        clocks_remaining -= CPU_CLOCKS_PER_SAMPLE;
        sample_count++;

        if (clocks_remaining < 0) {
            if (items.empty() || items.front().otype != OUT_SPEAKER_TIMESTAMP) {
                break;
            }
            uint32_t timestamp = items.front().u.timestamp;
            items.pop_front();
            clocks_remaining += timestamp - previous_timestamp;
            previous_timestamp = timestamp;
            next_sample = !next_sample;
        }
    }

    // Synchronously copy out the buffer and queue it for rendering, in Javascript
    EM_ASM_({
        Module.onRenderSound(HEAP8.subarray($0, $0 + $1), $2);
    }, pcm_samples, sample_count, AUDIO_HZ);
}

uint32_t OutputQueue::run()
{
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
