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
    void setJoystickAxes(float x, float y);
    void setJoystickButton(bool button);
    void setMouseTracking(int x, int y);
    void setMouseButton(bool button);
    void endMouseTracking();

    void clear();

    bool checkForInputBacklog();
    uint16_t checkForKey();
    uint16_t getKey();
    void pollJoystick(ROWorld *world, uint16_t &x, uint16_t &y, uint8_t &status);

 private:
    static constexpr unsigned KEY_BUFFER_SIZE = 32;
    static constexpr unsigned MOUSE_BUFFER_SIZE = 8;
    static constexpr unsigned MOUSE_DELAY_ON_ROOM_CHANGE = 4;
    static constexpr unsigned MOUSE_VIRTUAL_MOVE_TIMEOUT = 4;
    static constexpr float MOUSE_GAIN = 0.07f;
    static constexpr unsigned JOYSTICK_RANGE_MIN = 3;
    static constexpr unsigned JOYSTICK_RANGE_MAX = 10;
    static constexpr float JOYSTICK_ASPECT_CORRECTION = 0.8f;

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

    float js_x, js_y;
    float js_residual_x, js_residual_y;
    bool js_button_pressed, js_button_held;

    int savedPlayerX, savedPlayerY;
    RORoomId savedPlayerRoom;
    unsigned mouse_delay_timer;
    unsigned mouse_virtual_move_timer;

    void updateMouse(ROWorld *world);
    void clearJoystickAxes();
    bool virtualMouseToPosition(ROWorld *world, int x, int y);
    void quantizeJoystickAxis(float input, float &residual, uint16_t &output, float axis_scale);
};
