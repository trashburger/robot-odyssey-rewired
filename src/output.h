#pragma once

#include <vector>
#include <list>
#include <circular_buffer.hpp>
#include "sbt86.h"
#include "draw.h"

enum OutputType
{
    OUT_CGA_FRAME,
    OUT_RGB_FRAME,
    OUT_SPEAKER_TIMESTAMP,
    OUT_DELAY,
};

struct OutputItem
{
    OutputType otype;
    union {
        uint32_t timestamp;
        uint32_t delay;
    } u;
};

class OutputQueue
{
 public:
    OutputQueue();

    void clear();
    void skipDelay();
    uint32_t run();

    void pushFrameCGA(SBTStack *stack, uint8_t *framebuffer);
    void pushFrameRGB(SBTStack *stack, uint32_t *framebuffer);
    void pushDelay(uint32_t millis);
    void pushSpeakerTimestamp(uint32_t timestamp);

    static const unsigned SCREEN_WIDTH = CGAFramebuffer::WIDTH * CGAFramebuffer::ZOOM;
    static const unsigned SCREEN_HEIGHT = CGAFramebuffer::HEIGHT * CGAFramebuffer::ZOOM;

    uint32_t rgb_pixels[SCREEN_WIDTH * SCREEN_HEIGHT];
    uint32_t cga_palette[4];
    RGBDraw draw;

    static const int CPU_CLOCK_HZ = 4770000;
    static const unsigned CPU_CLOCKS_PER_SAMPLE = 200;
    static const unsigned AUDIO_HZ = CPU_CLOCK_HZ / CPU_CLOCKS_PER_SAMPLE;
    static const unsigned AUDIO_BUFFER_SECONDS = 10;
    static const unsigned AUDIO_BUFFER_SAMPLES = AUDIO_HZ * AUDIO_BUFFER_SECONDS;

    int8_t pcm_samples[AUDIO_BUFFER_SAMPLES];

    static const unsigned MAX_BUFFERED_FRAMES = 256;
    static const unsigned MAX_BUFFERED_EVENTS = 16384;

 protected:
    jm::circular_buffer<OutputItem, MAX_BUFFERED_EVENTS> items;
    jm::circular_buffer<CGAFramebuffer, MAX_BUFFERED_FRAMES> frames;
    uint32_t delay_remaining;

    void dequeueCGAFrame();
    void renderFrame();
    uint32_t renderSoundEffect(uint32_t first_timestamp);
};
