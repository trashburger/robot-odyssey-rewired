#include <stdio.h>
#include <stdint.h>
#include "sbt86.h"
#include "roData.h"
#include "input.h"


InputBuffer::InputBuffer()
    : savedPlayerX(-1),
      savedPlayerY(-1),
      savedPlayerRoom(RO_ROOM_NONE),
      mouse_delay_timer(0)
{
    clear();
}

void InputBuffer::clear()
{
    key_buffer.clear();
    mouse_buffer.clear();
    js_x = 0.0f;
    js_y = 0.0f;
    js_residual_x = 0.0f;
    js_residual_y = 0.0f;
    js_button_pressed = false;
    js_button_held = false;
}

bool InputBuffer::checkForInputBacklog()
{
    return key_buffer.size() > 1;
}

void InputBuffer::pressKey(uint8_t ascii, uint8_t scancode)
{
    if (!key_buffer.full()) {
        key_buffer.push_back((scancode << 8) | ascii);
    }
}

void InputBuffer::setJoystickAxes(float x, float y)
{
    mouse_buffer.clear();
    js_x = std::max(-1.0f, std::min(1.0f, x)) * JOYSTICK_RANGE_MAX;
    js_y = std::max(-1.0f, std::min(1.0f, y)) * JOYSTICK_RANGE_MAX;
}

void InputBuffer::setJoystickButton(bool button)
{
    mouse_buffer.clear();
    js_button_held = button;
    js_button_pressed = js_button_pressed || button;
}

void InputBuffer::setMouseTracking(int x, int y)
{
    if (!mouse_buffer.empty() && mouse_buffer.back().type == EVT_POS) {
        // Combine with an existing position event
        MouseEvent &evt = mouse_buffer.back();
        evt.x = x;
        evt.y = y;

    } else {
        // Make a new position event

        if (mouse_buffer.full()) {
            // If the buffer overflows, assume something is wrong/stuck. Clear it.
            mouse_buffer.clear();
        }

        MouseEvent evt = { EVT_POS, x, y };
        mouse_buffer.push_back(evt);
    }
}

void InputBuffer::setMouseButton(bool button)
{
    if (!mouse_buffer.full()) {
        MouseEvent evt = { EVT_BUTTON, button };
        mouse_buffer.push_back(evt);
    }
}

void InputBuffer::endMouseTracking()
{
    mouse_buffer.clear();
    js_x = 0.0f;
    js_y = 0.0f;
    js_button_pressed = false;
    js_button_held = false;
}

uint16_t InputBuffer::checkForKey()
{
    return key_buffer.empty() ? 0 : key_buffer.front();
}

uint16_t InputBuffer::getKey()
{
    if (key_buffer.empty()) {
        return 0;
    } else {
        uint16_t key = key_buffer.front();
        key_buffer.pop_front();
        return key;
    }
}

void InputBuffer::pollJoystick(ROWorld *world, uint16_t &x, uint16_t &y, uint8_t &status)
{
    // Optional mouse tracking will use the joystick input to move the player
    // to a chosen cursor location, without violating game collision detection.

    updateMouse(world);

    // Dither joystick motion so we can go slower than the game normally provides for

    float total_x = js_x + js_residual_x;
    float total_y = js_y + js_residual_y;

    int quantized_x = total_x;
    int quantized_y = total_y;
    if (quantized_x < JOYSTICK_RANGE_MIN && quantized_x > -JOYSTICK_RANGE_MIN) quantized_x = 0;
    if (quantized_y < JOYSTICK_RANGE_MIN && quantized_y > -JOYSTICK_RANGE_MIN) quantized_y = 0;

    js_residual_x = total_x - quantized_x;
    js_residual_y = total_y - quantized_y;

    // Button presses must not be missed if they end before the next poll.
    // Clear the button press latch here.

    bool button = js_button_held || js_button_pressed;
    js_button_pressed = false;

    // Port 0x201 style status byte: Low 4 bits are timed based
    // on an RC circuit in each axis. Upper 4 bits are buttons,
    // active low. The byte includes data for two joysticks, and
    // we only emulate one.

    int center = ROJoyfile::DEFAULT_JOYSTICK_CENTER;
    status = 0xFC ^ (button ? 0x10 : 0);
    x = std::max(0, std::min(center * 2, quantized_x + center));
    y = std::max(0, std::min(center * 2, quantized_y + center));
}

void InputBuffer::updateMouse(ROWorld *world)
{
    // If the player has moved to a different room, clear buffered mouse input
    if (world) {
        RORoomId room = world->getObjectRoom(RO_OBJ_PLAYER);
        if (room != savedPlayerRoom) {
            mouse_delay_timer = MOUSE_DELAY_ON_ROOM_CHANGE;
        }
        savedPlayerRoom = room;
    }

    // Keep the mouse buffer empty while the delay timer is in effect
    if (mouse_delay_timer) {
        mouse_delay_timer--;
        if (!mouse_buffer.empty()) {
            setJoystickAxes(0, 0);
        }
    }

    // Everything else is about handling events
    if (mouse_buffer.empty()) {
        return;
    }

    struct MouseEvent &evt = mouse_buffer.front();
    switch (evt.type) {

        case EVT_POS:
            // Position events last until the requested position has been reached
            if (virtualMouseToPosition(world, evt.x, evt.y)) {
                mouse_buffer.pop_front();
            }
            break;

        case EVT_BUTTON:
            // Button events set the state immediately and last one frame
            js_button_held = evt.x;
            mouse_buffer.pop_front();
            break;

        default:
            assert(0);
    }
}

bool InputBuffer::virtualMouseToPosition(ROWorld *world, int x, int y)
{
    if (!world) {
        // This must be some part of the game we don't have sprite data
        // for, like the main menu. For now, this means position events
        // resolve immediately and do nothing, but button events work.

        return true;
    }

    int playerX, playerY;
    world->getObjectXY(RO_OBJ_PLAYER, playerX, playerY);

    int xdiff = x - playerX;
    int ydiff = -(y - playerY);

    if (xdiff > 0) {
        js_x = std::min<int>(JOYSTICK_RANGE_MAX, JOYSTICK_RANGE_MIN + MOUSE_GAIN * (xdiff - 1));
    } else if (xdiff < 0) {
        js_x = -std::min<int>(JOYSTICK_RANGE_MAX, JOYSTICK_RANGE_MIN - MOUSE_GAIN * (xdiff + 1));
    } else {
        js_x = 0;
    }

    if (ydiff > 0) {
        js_y = std::min<int>(JOYSTICK_RANGE_MAX, JOYSTICK_RANGE_MIN + MOUSE_GAIN * (ydiff - 1));
    } else if (ydiff < 0) {
        js_y = -std::min<int>(JOYSTICK_RANGE_MAX, JOYSTICK_RANGE_MIN - MOUSE_GAIN * (ydiff + 1));
    } else {
        js_y = 0;
    }

    // Are we there yet?
    if (xdiff == 0 && ydiff == 0) {
        // Yes, made it to the exact place we wanted to be

        savedPlayerX = -1;
        savedPlayerY = -1;
        return true;

    } else {
        // Not there yet, make sure the player keeps moving. If it's up against
        // a barrier and stops moving, we'll consider the move complete as well.

        int lastX = savedPlayerX;
        int lastY = savedPlayerY;
        savedPlayerX = playerX;
        savedPlayerY = playerY;
        return lastX == savedPlayerX && lastY == savedPlayerY;
    }
}
