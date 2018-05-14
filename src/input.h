#pragma once

#include <stdint.h>
#include <circular_buffer.hpp>
#include "sbt86.h"
#include "roData.h"


class InputBuffer
{
 public:
    InputBuffer();

    void pressKey(uint8_t ascii, uint8_t scancode = 0);
    void setJoystickAxes(int x, int y);
    void setJoystickButton(bool button);
    void setMouseTracking(int x, int y);
    void setMouseButton(bool button);
    void endMouseTracking();

    void clear();

    bool checkForInputBacklog();
    uint16_t checkForKey();
    uint16_t getKey();
    void pollJoystick(ROWorld *world, uint16_t &x, uint16_t &y, uint8_t &status);

 protected:
    static const unsigned KEY_BUFFER_SIZE = 16;
    static const unsigned MOUSE_BUFFER_SIZE = 8;

    enum MouseEventType {
        EVT_POS,
        EVT_BUTTON,
    };

    struct MouseEvent {
        enum MouseEventType type;
        int x, y;
    };

    jm::circular_buffer<uint16_t, KEY_BUFFER_SIZE> key_buffer;
    jm::circular_buffer<MouseEvent, MOUSE_BUFFER_SIZE> mouse_buffer;

    int js_x, js_y;
    bool js_button_pressed, js_button_held;

    int savedPlayerX, savedPlayerY;

    void updateMouse(ROWorld *world);
    bool virtualMouseToPosition(ROWorld *world, int x, int y);
};
