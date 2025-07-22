#pragma once

#include "draw.h"
#include "sbt86.h"
#include <circular_buffer.hpp>
#include <list>
#include <vector>

enum OutputType {
    OUT_CGA_FRAME,
    OUT_SPEAKER_TIMESTAMP,
    OUT_DELAY,
};

enum OutputDelayType {
    OUT_DELAY_MERGE_WITH_SOUND,
    OUT_DELAY_FLUSH,
};

struct OutputItem {
    OutputType otype;
    union {
        uint32_t timestamp;
        uint32_t delay;
    } u;
};

class OutputInterface {
  public:
    static const uint32_t CPU_CLOCK_KHZ = 4770;
    static const uint32_t CPU_CLOCK_HZ = CPU_CLOCK_KHZ * 1000;
    static const uint32_t CPU_CLOCKS_PER_SAMPLE = 200;
    static const uint32_t AUDIO_HZ = CPU_CLOCK_HZ / CPU_CLOCKS_PER_SAMPLE;

    OutputInterface(ColorTable &colorTable);

    static constexpr uint32_t msecToClocks(uint32_t millis) {
        return millis * CPU_CLOCK_KHZ;
    }

    static constexpr uint32_t clocksToMsec(uint32_t clocks) {
        return clocks / CPU_CLOCK_KHZ;
    }

    void setTimeReference(uint32_t timestamp) {
        reference_timestamp = timestamp;
    }

    uint32_t getFrameCount() { return frame_counter; }

    virtual void clear();
    virtual void pushFrameCGA(uint32_t timestamp, SBTStack *stack,
                              uint8_t *framebuffer);
    virtual void drawFrameRGB(uint32_t timestamp);
    virtual void pushDelay(uint32_t timestamp, OutputDelayType delayType);
    virtual void pushSpeakerTimestamp(uint32_t timestamp);

    RGBDraw draw;

  protected:
    uint32_t frame_counter;
    uint32_t reference_timestamp;
};

class OutputQueue final : public OutputInterface {
  public:
    OutputQueue(ColorTable &colorTable);

    void setFrameSkip(uint32_t frameskip);
    uint32_t run();

    virtual void clear();
    virtual void pushFrameCGA(uint32_t timestamp, SBTStack *stack,
                              uint8_t *framebuffer);
    virtual void drawFrameRGB(uint32_t timestamp);
    virtual void pushDelay(uint32_t timestamp, OutputDelayType delayType);
    virtual void pushSpeakerTimestamp(uint32_t timestamp);

    static const unsigned AUDIO_BUFFER_SECONDS = 10;
    static const unsigned AUDIO_BUFFER_SAMPLES =
        AUDIO_HZ * AUDIO_BUFFER_SECONDS;

    int8_t pcm_samples[AUDIO_BUFFER_SAMPLES];

    static const unsigned MAX_BUFFERED_FRAMES = 128;
    static const unsigned MAX_BUFFERED_EVENTS = 16384;

  private:
    jm::circular_buffer<OutputItem, MAX_BUFFERED_EVENTS> items;
    jm::circular_buffer<CGAFramebuffer, MAX_BUFFERED_FRAMES> frames;

    uint32_t frameskip_value;
    uint32_t frameskip_counter;

    void dequeueCGAFrame();
    void renderSoundEffect(uint32_t first_timestamp);
    void renderFrame();
};
