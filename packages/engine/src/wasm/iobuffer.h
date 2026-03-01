#pragma once
#include "nostdlib.h"

struct IOBuffer {

    template <typename T> struct Queue {
        uint32_t offset;
        uint32_t sizeProvided;
        uint32_t sizeUsed;

        bool hasItem(size_t count = 1) const {
            return sizeProvided >= sizeUsed &&
                   (sizeProvided - sizeUsed) >= (count * sizeof(T));
        }

        T &item(IOBuffer *buffer) const {
            return *reinterpret_cast<T *>(reinterpret_cast<uint8_t *>(buffer) +
                                          offset + sizeUsed);
        }

        void next(size_t count = 1) { sizeUsed += count * sizeof(T); }
    };

    struct KeyboardItem {
        uint8_t ascii;
        uint8_t scancode;

        KeyboardItem() : ascii(0), scancode(0) {}
    };

    // Joystick rates are in units of 1/RATE_UNIT pixels per frame
    static constexpr int RATE_UNIT = 8;

    enum class JoystickItemType : uint8_t {
        NONE = 0, // No item, queue is empty
        RAW,      // A single raw joystick polling result, -128 to +127
        RATE, // Continue to move the player at a rate of N/RATE_UNIT pixels per
              // frame until the next event, -128 to +127
        RELATIVE, // Try to move the indicated number of pixels before moving to
                  // the next event, -128 to +127
        ABSOLUTE, // Try to move the player to the indicated position
                  // before moving to the next event, -1 to +254
    };

    struct JoystickItem {
        JoystickItemType type;
        bool button;
        int8_t x;
        int8_t y;

        JoystickItem()
            : type(JoystickItemType::NONE), button(false), x(0), y(0) {}
    };

    enum class OutputTag : uint8_t {
        EXIT,       // Process has exited. 1 byte: exit code
        LOG,        // Log message. Variable length: length16, string bytes
        LOG_BINARY, // Binary debug log. Variable length: length16, data bytes
        STACK,      // Return address stack. Variable length: count16, words
        CLOCK,      // Full-size clock difference. 4 bytes: cycles32
        SPEAKER,    // Speaker impulse after short clock difference. 2 bytes:
                    // cycles16
        CGA_WR,    // CGA write after short clock difference. 6 bytes: cycles16,
                   // address16, data16
        CGA_CLEAR, // Clear the CGA framebuffer. 0 bytes
        CHECK_KEYBOARD, // checkForKeyboardItem called. 0 bytes
        TAKE_KEYBOARD,  // takeKeyboardItem called. 0 bytes
        CHECK_JOYSTICK, // checkForJoystickItem called. 0 bytes
        TAKE_JOYSTICK,  // takeJoystickItem called. 0 bytes
        DL_CLEAR,       // Clear the display list and its style stack. 0 bytes
        DL_PRESENT,     // Present the current display list. 0 bytes
        DL_CLASS_PUSH,  // Push a CSS class onto the style stack. Variable
                        // length string beginning with a length byte.
        DL_CLASS_POP,   // Pop from the CSS style stack. 0 bytes
        DL_ROOM,        // Add a room to the display list. 32 bytes:
                        // foreground8, background8, 30 data bytes
        DL_SPRITE,      // Add a sprite to the display list. 19 bytes: x, y,
                        // color, 16 data bytes
        DL_TEXT,        // Add text to the display list. Variable length: x, y,
                        // color, font_id, style, length16, string bytes
        DL_HLINE, // Add a horizontal line to the display list. 4 bytes: x1,
                  // x2, y, color
        DL_VLINE, // Add a vertical line to the display list. 4 bytes: x,
                  // y1, y2, color
        SAVE_FILE_CLOSED, // The save file, open for writing, was just closed. 0
                          // bytes
    };

    Queue<uint8_t> output;
    Queue<KeyboardItem> keyboard;
    Queue<JoystickItem> joystick;

    KeyboardItem checkForKeyboardItem() {
        write(OutputTag::CHECK_KEYBOARD, 0);
        if (keyboard.hasItem()) {
            return keyboard.item(this);
        } else {
            return {};
        }
    }

    KeyboardItem takeKeyboardItem() {
        write(OutputTag::TAKE_KEYBOARD, 0);
        if (keyboard.hasItem()) {
            KeyboardItem result = keyboard.item(this);
            keyboard.next();
            return result;
        } else {
            return {};
        }
    }

    JoystickItem checkForJoystickItem() {
        write(OutputTag::CHECK_JOYSTICK, 0);
        if (joystick.hasItem()) {
            return joystick.item(this);
        } else {
            return {};
        }
    }

    JoystickItem takeJoystickItem() {
        write(OutputTag::TAKE_JOYSTICK, 0);
        if (joystick.hasItem()) {
            JoystickItem result = joystick.item(this);
            joystick.next();
            return result;
        } else {
            return {};
        }
    }

    uint8_t *write(OutputTag tag, uint32_t dataSize = 0) {
        if (!output.hasItem(dataSize + 1)) {
            abort();
        }
        uint8_t *item = &output.item(this);
        output.next(dataSize + 1);
        item[0] = (uint8_t)tag;
        return item + 1;
    }

    void exit(uint8_t code) { *write(OutputTag::EXIT, 1) = code; }

    void log(const char *str) {
        uint16_t len = (uint16_t)strnlen(str, 0xffff);
        uint8_t *item = write(OutputTag::LOG, 2 + len);
        memcpy(item, &len, 2);
        memcpy(item + 2, str, len);
    }

    void logBinary(const void *data, uint16_t len) {
        uint8_t *item = write(OutputTag::LOG_BINARY, 2 + len);
        memcpy(item, &len, 2);
        memcpy(item + 2, data, len);
    }

    template <typename T> void logBinary(T const &item) {
        logBinary(&item, sizeof item);
    }

    [[noreturn]] void error(const char *message) {
        log(message);
        abort();
    }

    void saveFileClosed() { write(OutputTag::SAVE_FILE_CLOSED, 0); }

    void delay(uint32_t cycles) {
        if (cycles > 0) {
            memcpy(write(OutputTag::CLOCK, 4), &cycles, 4);
        }
    }

    void speakerImpulse(uint32_t cycles) {
        if (cycles > 0xffff) {
            delay(cycles);
            cycles = 0;
        }
        memcpy(write(OutputTag::SPEAKER, 2), &cycles, 2);
    }

    void cgaWrite(uint32_t cycles, uint16_t address, uint16_t data) {
        if (cycles > 0xffff) {
            delay(cycles);
            cycles = 0;
        }
        uint8_t *item = write(OutputTag::CGA_WR, 6);
        memcpy(item, &cycles, 2);
        memcpy(item + 2, &address, 2);
        memcpy(item + 4, &data, 2);
    }

    void cgaClear() { write(OutputTag::CGA_CLEAR, 0); }

    void displayListClear() { write(OutputTag::DL_CLEAR, 0); }

    void displayListPresent() { write(OutputTag::DL_PRESENT, 0); }

    void classPush(const char *cls) {
        uint8_t len = strnlen(cls, 0xff);
        uint8_t *item = write(OutputTag::DL_CLASS_PUSH, 1 + len);
        item[0] = len;
        memcpy(item + 1, cls, len);
    }

    void classPop() { write(OutputTag::DL_CLASS_POP, 0); }

    void room(uint8_t fg, uint8_t bg, const uint8_t *data) {
        uint8_t *item = write(OutputTag::DL_ROOM, 32);
        item[0] = fg;
        item[1] = bg;
        memcpy(item + 2, data, 30);
    }

    void sprite(uint8_t x, uint8_t y, uint8_t color, const uint8_t *data) {
        uint8_t *item = write(OutputTag::DL_SPRITE, 19);
        item[0] = x;
        item[1] = y;
        item[2] = color;
        memcpy(item + 3, data, 16);
    }

    void text(uint8_t x, uint8_t y, uint8_t color, uint8_t font_id,
              uint8_t style, const char *str) {
        uint16_t len = (uint16_t)strnlen(str, 0xffff);
        uint8_t *item = write(OutputTag::DL_TEXT, 7 + len);
        item[0] = x;
        item[1] = y;
        item[2] = color;
        item[3] = font_id;
        item[4] = style;
        memcpy(item + 5, &len, 2);
        memcpy(item + 7, str, len);
    }

    void hline(uint8_t x1, uint8_t x2, uint8_t y, uint8_t color) {
        uint8_t *item = write(OutputTag::DL_HLINE, 4);
        item[0] = x1;
        item[1] = x2;
        item[2] = y;
        item[3] = color;
    }

    void vline(uint8_t x, uint8_t y1, uint8_t y2, uint8_t color) {
        uint8_t *item = write(OutputTag::DL_VLINE, 4);
        item[0] = x;
        item[1] = y1;
        item[2] = y2;
        item[3] = color;
    }
};
