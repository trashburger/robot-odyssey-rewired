#include <algorithm>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "sbt86.h"
#include "roData.h"
#include "input.h"


InputBuffer::InputBuffer()
    : savedPlayerX(-1),
      savedPlayerY(-1),
      savedPlayerRoom(RO_ROOM_NONE),
      mouse_delay_timer(0),
      mouse_virtual_move_timer(0)
{
    clear();
}

void InputBuffer::clear()
{
    key_buffer.clear();
    mouse_buffer.clear();
    clearJoystickAxes();
    js_button_pressed = false;
    js_button_held = false;
    mouse_virtual_move_timer = 0;
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
    js_x = x;
    js_y = y;
}

void InputBuffer::clearJoystickAxes()
{
    js_x = 0.0f;
    js_y = 0.0f;
    js_residual_x = 0.0f;
    js_residual_y = 0.0f;
}

void InputBuffer::setJoystickButton(bool button)
{
    if (!mouse_buffer.empty()) {
        endMouseTracking();
    }
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
    clearJoystickAxes();
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

void InputBuffer::quantizeJoystickAxis(float input, float &residual, uint16_t &output, float axis_scale)
{
    const float total = std::min(1.f, std::max(-1.f, input + residual));
    const float range = axis_scale * float(JOYSTICK_RANGE_MAX - JOYSTICK_RANGE_MIN);
    const float quantized = roundf(total * range);

    residual = total - (1.f / range * quantized);

    const int center = ROJoyfile::DEFAULT_JOYSTICK_CENTER;
    if (quantized > 0.f) {
        output = std::min(center * 2, center + int(JOYSTICK_RANGE_MIN) + int(quantized));
    } else if (quantized < 0.f) {
        output = std::max(0, center - int(JOYSTICK_RANGE_MIN) + int(quantized));
    } else {
        output = center;
    }
}

void InputBuffer::pollJoystick(ROWorld *world, uint16_t &x, uint16_t &y, uint8_t &status)
{
    // Optional mouse tracking will use the joystick input to move the player
    // to a chosen cursor location, without violating game collision detection.

    updateMouse(world);

    // Convert a floating point joystick in the range [-1, +1] to a quantized value subject
    // to the game's supported range, and save the residual. This removes the game's deadzone
    // correction, allowing it to be re-applied as appropriate for specific input devices.

    quantizeJoystickAxis(js_x, js_residual_x, x, 1.f);
    quantizeJoystickAxis(js_y, js_residual_y, y, JOYSTICK_ASPECT_CORRECTION);

    // Button presses must not be missed if they end before the next poll.
    // Clear the button press latch here.

    bool button = js_button_held || js_button_pressed;
    js_button_pressed = false;

    // Port 0x201 style status byte: Low 4 bits are timed based
    // on an RC circuit in each axis. Upper 4 bits are buttons,
    // active low. The byte includes data for two joysticks, and
    // we only emulate one.

    status = 0xFC ^ (button ? 0x10 : 0);
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
            endMouseTracking();
        }
    }

    // Everything else is about handling events
    if (mouse_buffer.empty()) {
        return;
    }

    const struct MouseEvent &evt = mouse_buffer.front();
    switch (evt.type) {

        case EVT_POS:
            // Position events last until the requested position has been reached
            if (virtualMouseToPosition(world, evt.x, evt.y)) {
                mouse_buffer.pop_front();
                clearJoystickAxes();
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

        savedPlayerX = -1;
        savedPlayerY = -1;
        mouse_virtual_move_timer = 0;
        return true;
    }

    int playerX, playerY;
    world->getObjectXY(RO_OBJ_PLAYER, playerX, playerY);

    int xdiff = x - playerX;
    int ydiff = -(y - playerY);
    js_x = MOUSE_GAIN * float(xdiff);
    js_y = (MOUSE_GAIN / JOYSTICK_ASPECT_CORRECTION) * float(ydiff);

    // Are we there yet?
    if (xdiff == 0 && ydiff == 0) {
        // Yes, made it to the exact place we wanted to be

        savedPlayerX = -1;
        savedPlayerY = -1;
        mouse_virtual_move_timer = 0;
        return true;

    } else {
        // Not there yet, make sure the player keeps moving. If it's up against
        // a barrier and stops moving, we'll consider the move complete as well.

        int lastX = savedPlayerX;
        int lastY = savedPlayerY;
        savedPlayerX = playerX;
        savedPlayerY = playerY;
        if (lastX == savedPlayerX && lastY == savedPlayerY) {
            if (++mouse_virtual_move_timer >= MOUSE_VIRTUAL_MOVE_TIMEOUT) {
                mouse_virtual_move_timer = 0;
                return true;
            }
        } else {
            mouse_virtual_move_timer = 0;
        }
    }

    return false;
}
