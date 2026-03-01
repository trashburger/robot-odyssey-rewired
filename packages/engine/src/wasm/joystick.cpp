#include "joystick.h"

static constexpr unsigned MOUSE_DELAY_ON_ROOM_CHANGE = 2;
static constexpr unsigned ABSOLUTE_MOVE_STUCK_TIMEOUT = 3;

JoystickPoller::Result::Result(bool button, int8_t x, int8_t y)
    : x(0x80 + x), y(0x80 + y), status(0xfc ^ (button ? 0x10 : 0)) {}

JoystickPoller::Result::Result(bool button) : Result(button, 0, 0) {}

JoystickPoller::Result::Result() : Result(0, 0, 0) {}

JoystickPoller::Result::Result(IOBuffer::JoystickItem const &item)
    : Result(item.button, item.x, item.y) {}

JoystickPoller::Result::Result(bool button, Motion &motion)
    : Result(button, motion.decodeX(), motion.decodeY()) {}

JoystickPoller::JoystickPoller() { reset(); }

void JoystickPoller::reset() {
    delayTimer = 0;
    stuckTimer = 0;
    savedButton = false;
    savedPlayer = Player{};
    motion = Motion{};
}

// Extract a -1 to +254 value, stored as an int8_t
static int absoluteCoord8(int8_t x) { return x == -1 ? -1 : (uint8_t)x; }

JoystickPoller::Result JoystickPoller::poll(IOBuffer *io, const uint8_t *world,
                                            bool mapActive) {

    IOBuffer::JoystickItem current = io->checkForJoystickItem();
    Player player(world);

    if (current.type == IOBuffer::JoystickItemType::ABSOLUTE) {
        // ABSOLUTE: Continuously update the accumulator based on object
        // positions, wait to consume the event until the accumulator has
        // settled.

        savedButton = current.button;

        // Delay absolute moves for some frames when the player room changes
        if (player.room != savedPlayer.room) {
            delayTimer = MOUSE_DELAY_ON_ROOM_CHANGE;
        } else if (delayTimer) {
            delayTimer--;
        }
        const bool roomChangeDelay = delayTimer != 0;

        // If the player hasn't moved for some frames, consider its movement
        // stuck
        if (player == savedPlayer) {
            if (stuckTimer < 0xFF) {
                stuckTimer++;
            }
        } else {
            stuckTimer = 0;
            savedPlayer = player;
        }
        const bool playerIsStuck = stuckTimer >= ABSOLUTE_MOVE_STUCK_TIMEOUT;

        // Cancel absolute move if player is stuck, world isn't loaded, or map
        // mode is active
        if (playerIsStuck || mapActive || !world) {
            io->takeJoystickItem();
            stuckTimer = 0;
            motion = Motion{};
            return {current.button};
        }

        // On room change delay, pause motion without dequeueing anything
        if (roomChangeDelay) {
            return {current.button};
        }

        // Continuously track the target player location
        motion.absolute(absoluteCoord8(current.x) - player.x,
                        absoluteCoord8(current.y) - player.y);
        motion.step();
        motion.limit();

        JoystickPoller::Result result{current.button, motion};
        if (motion.done()) {
            io->takeJoystickItem();
        }
        return result;

    } else {
        // In all non-ABSOLUTE modes, clear timers and reset the saved player
        // location

        savedPlayer = player;
        stuckTimer = 0;
        delayTimer = 0;

        switch (current.type) {

        // NONE: No dequeue, reset motion
        case IOBuffer::JoystickItemType::NONE: {
            motion = Motion{};
            return {savedButton};
        }

        // RAW: One literal event, dequeue immediately
        case IOBuffer::JoystickItemType::RAW: {
            savedButton = current.button;
            io->takeJoystickItem();
            motion = Motion{};
            return {current};
        }

        // RATE: Update ongoing motion, event is never dequeued
        case IOBuffer::JoystickItemType::RATE: {
            savedButton = current.button;
            motion.rateX = current.x;
            motion.rateY = current.y;
            motion.step();
            motion.limit();
            return {current.button, motion};
        }

        // RELATIVE: Immediately apply all motion to accumulator,
        // but wait to consume the event until the accumulator has finished.
        case IOBuffer::JoystickItemType::RELATIVE: {
            savedButton = current.button;
            motion.relative(current.x, current.y);
            motion.step();
            JoystickPoller::Result result{current.button, motion};
            if (motion.done()) {
                io->takeJoystickItem();
            } else {
                // Plan to revisit this queue item
                motion.relative(-current.x, -current.y);
            }
            return result;
        }

        // ABSOLUTE: Unreachable, handled above.
        case IOBuffer::JoystickItemType::ABSOLUTE:
            abort();
        }
    }
}

bool JoystickPoller::Motion::done() const {
    static constexpr int unit = IOBuffer::RATE_UNIT;
    return rateX == 0 && rateY == 0 && accumX > -unit && accumX < unit &&
           accumY > -unit && accumY < unit;
}

void JoystickPoller::Motion::relative(int x, int y) {
    static constexpr int unit = IOBuffer::RATE_UNIT;
    accumX += x * unit;
    accumY += y * unit;
    rateX = 0;
    rateY = 0;
}

void JoystickPoller::Motion::absolute(int x, int y) {
    static constexpr int unit = IOBuffer::RATE_UNIT;
    accumX = x * unit;
    accumY = y * unit;
    rateX = 0;
    rateY = 0;
}

template <typename T>
static constexpr T clamp(T const &v, T const &lo, T const &hi) {
    return v < lo ? lo : v > hi ? hi : v;
}

void JoystickPoller::Motion::step() {
    accumX += rateX;
    accumY += rateY;
}

void JoystickPoller::Motion::limit() {
    // Clamp max accumulator values to limit overshoot at max speed
    static constexpr int unit = IOBuffer::RATE_UNIT;
    static constexpr int x_clamp = 16 * unit;
    static constexpr int y_clamp = 32 * unit;

    accumX = clamp<int>(accumX, -x_clamp, x_clamp);
    accumY = clamp<int>(accumY, -y_clamp, y_clamp);
}

int8_t JoystickPoller::Motion::decodeX() {
    static constexpr int unit = IOBuffer::RATE_UNIT;
    if (accumX < 0) {
        if (accumX <= -8 * unit) {
            accumX += 8 * unit;
            return -8;
        } else if (accumX <= -4 * unit) {
            accumX += 4 * unit;
            return -7;
        } else if (accumX <= -2 * unit) {
            accumX += 2 * unit;
            return -5;
        } else if (accumX <= -1 * unit) {
            accumX += 1 * unit;
            return -3;
        }
    } else {
        if (accumX >= 8 * unit) {
            accumX -= 8 * unit;
            return 8;
        } else if (accumX >= 4 * unit) {
            accumX -= 4 * unit;
            return 7;
        } else if (accumX >= 2 * unit) {
            accumX -= 2 * unit;
            return 5;
        } else if (accumX >= 1 * unit) {
            accumX -= 1 * unit;
            return 3;
        }
    }
    return 0;
}

int8_t JoystickPoller::Motion::decodeY() {
    static constexpr int unit = IOBuffer::RATE_UNIT;
    if (accumY < 0) {
        if (accumY <= -16 * unit) {
            accumY += 16 * unit;
            return 8;
        } else if (accumY <= -8 * unit) {
            accumY += 8 * unit;
            return 7;
        } else if (accumY <= -4 * unit) {
            accumY += 4 * unit;
            return 6;
        } else if (accumY <= -2 * unit) {
            accumY += 2 * unit;
            return 5;
        } else if (accumY <= -1 * unit) {
            accumY += 1 * unit;
            return 3;
        }
    } else {
        if (accumY >= 16 * unit) {
            accumY -= 16 * unit;
            return -8;
        } else if (accumY >= 8 * unit) {
            accumY -= 8 * unit;
            return -7;
        } else if (accumY >= 4 * unit) {
            accumY -= 4 * unit;
            return -6;
        } else if (accumY >= 2 * unit) {
            accumY -= 2 * unit;
            return -5;
        } else if (accumY >= 1 * unit) {
            accumY -= 1 * unit;
            return -3;
        }
    }
    return 0;
}
