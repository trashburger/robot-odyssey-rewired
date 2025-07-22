#pragma once

#include <vector>
#include <list>
#include <circular_buffer.hpp>
#include "sbt86.h"
#include "draw.h"

enum OutputType
{
    OUT_CGA_FRAME,
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

class OutputInterface
{
 public:
    static const int CPU_CLOCK_HZ = 4770000;
    static const unsigned CPU_CLOCKS_PER_SAMPLE = 200;
    static const unsigned AUDIO_HZ = CPU_CLOCK_HZ / CPU_CLOCKS_PER_SAMPLE;

    OutputInterface(ColorTable &colorTable);

    void setTimeReference(uint32_t timestamp)
    {
        reference_timestamp = timestamp;
    }

    uint32_t getFrameCount()
    {
        return frame_counter;
    }

    virtual void clear();
    virtual void pushFrameCGA(uint32_t timestamp, SBTStack *stack, uint8_t *framebuffer);
    virtual void drawFrameRGB(uint32_t timestamp);
    virtual void pushDelay(uint32_t timestamp, uint32_t millis);
    virtual void pushSpeakerTimestamp(uint32_t timestamp);

    RGBDraw draw;

 protected:
    uint32_t frame_counter;
    uint32_t reference_timestamp;
};

class OutputQueue final : public OutputInterface
{
 public:
    OutputQueue(ColorTable &colorTable);

    void setFrameSkip(uint32_t frameskip);
    uint32_t run();

    virtual void clear();
    virtual void pushFrameCGA(uint32_t timestamp, SBTStack *stack, uint8_t *framebuffer);
    virtual void drawFrameRGB(uint32_t timestamp);
    virtual void pushDelay(uint32_t timestamp, uint32_t millis);
    virtual void pushSpeakerTimestamp(uint32_t timestamp);

    static const unsigned AUDIO_BUFFER_SECONDS = 10;
    static const unsigned AUDIO_BUFFER_SAMPLES = AUDIO_HZ * AUDIO_BUFFER_SECONDS;

    int8_t pcm_samples[AUDIO_BUFFER_SAMPLES];

    static const unsigned MAX_BUFFERED_FRAMES = 128;
    static const unsigned MAX_BUFFERED_EVENTS = 16384;

 private:
    jm::circular_buffer<OutputItem, MAX_BUFFERED_EVENTS> items;
    jm::circular_buffer<CGAFramebuffer, MAX_BUFFERED_FRAMES> frames;

    uint32_t frameskip_value;
    uint32_t frameskip_counter;
    uint32_t frame_counter;

    void pushDelayInternal(uint32_t timestamp, uint32_t millis, bool combine_with_sound);
    void dequeueCGAFrame();
    void renderSoundEffect(uint32_t first_timestamp);
    void renderFrame();
};
