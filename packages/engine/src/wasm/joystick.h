#pragma once
#include "iobuffer.h"
#include "nostdlib.h"

class JoystickPoller {
    struct Motion {
        // Pending movement in 1/RATE_UNIT pixel units
        int accumX = 0, accumY = 0;
        int8_t rateX = 0, rateY = 0;

        bool done() const;
        void relative(int x, int y);
        void absolute(int x, int y);
        void step();
        void limit();
        int8_t decodeX();
        int8_t decodeY();
    };

    struct Player {
        uint8_t room, x, y;

        Player() : room(0xFF), x(0xFF), y(0xFF) {}
        Player(const uint8_t *world)
            : room(world ? world[0x300] : 0xFF), x(world ? world[0x400] : 0xFF),
              y(world ? world[0x500] : 0xFF) {}

        bool operator==(Player const &other) {
            return room == other.room && x == other.x && y == other.y;
        }
    };

  public:
    struct Result {
        uint8_t x;
        uint8_t y;
        uint8_t status;

        Result(bool button, int8_t x, int8_t y);
        Result(bool button);
        Result(IOBuffer::JoystickItem const &item);
        Result(bool button, Motion &motion);
        Result();
    };

    JoystickPoller();
    void reset();
    Result poll(IOBuffer *io, const uint8_t *world, bool mapActive);

  private:
    uint8_t delayTimer; // Counts down while delaying joystick queue
    uint8_t stuckTimer; // Counts up while absolute motion is stuck
    bool savedButton;   // Saved button state to use when the queue is empty

    Player savedPlayer;
    Motion motion;
};
