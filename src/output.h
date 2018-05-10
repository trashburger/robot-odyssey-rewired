#pragma once

#include <vector>
#include <list>
#include "circular_buffer.h"
#include "sbt86.h"


struct CGAFramebuffer
{
    uint8_t bytes[0x4000];
};

enum OutputType
{
    OUT_FRAME,
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
    uint32_t run();

    void pushFrame(SBTStack *stack, uint8_t *framebuffer);
    void pushDelay(uint32_t millis);    
    void pushSpeakerTimestamp(uint32_t timestamp);

    static const unsigned SCREEN_WIDTH = 320;
    static const unsigned SCREEN_HEIGHT = 200;
    uint32_t rgb_pixels[SCREEN_WIDTH * SCREEN_HEIGHT];
    uint32_t rgb_palette[4];

    static const unsigned MAX_BUFFERED_FRAMES = 256;
    static const unsigned MAX_BUFFERED_EVENTS = 16384;

 protected:
    jm::circular_buffer<OutputItem, MAX_BUFFERED_EVENTS> items;
    jm::circular_buffer<CGAFramebuffer, MAX_BUFFERED_FRAMES> frames;
    uint32_t delay_remaining;

    void renderFrame();
};
