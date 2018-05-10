#pragma once

#include <vector>
#include <list>
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
        CGAFramebuffer *framebuffer;
    } u;
};


class OutputQueue
{
 public:
    OutputQueue();
    ~OutputQueue();

    void clear();

    static const unsigned SCREEN_WIDTH = 320;
    static const unsigned SCREEN_HEIGHT = 200;

    uint32_t rgb_pixels[SCREEN_WIDTH * SCREEN_HEIGHT];
    uint32_t rgb_palette[4];

    uint32_t run();

    void pushFrame(SBTStack *stack, uint8_t *framebuffer);
    void pushDelay(uint32_t millis);    
    void pushSpeakerTimestamp(uint32_t timestamp);

 protected:
    std::list<OutputItem> output_queue;
    uint32_t output_queue_frame_count;
    uint32_t output_queue_delay_remaining;

    void renderFrame(uint8_t *framebuffer);
};
