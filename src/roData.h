#pragma once
#include <stdint.h>
#include "sbt86.h"

/*
 * Colors
 */
enum ROColor {
    RO_COLOR_WIRE_HOT   = 5,
    RO_COLOR_WIRE_COLD  = 7,
};


/*
 * Object grabFlag states. Can the player grab this object?
 */
enum ROGrabFlag {
    RO_CAN_BE_GRABBED = 0x00,
    RO_CANT_BE_GRABBED = 0xFF,
};


/*
 * Room IDs
 */
enum RORoomId {
    RO_ROOM_RENDERER    = 0x00,   // Blank room, patched in bt_renderer.py

    RO_ROOM_ESC_TEXT    = 0x00,   // "To go back to the menu, press ESC."
    RO_ROOM_SPARKY      = 0x09,   // Inside Sparky
    RO_ROOM_CHECKERS    = 0x0A,   // Inside Checkers
    RO_ROOM_SCANNER     = 0x0B,   // Inside Scanner

    RO_ROOM_NONE        = 0x3F,
};


/*
 * Object IDs
 */
enum ROObjectId {
    ///// Game only

    RO_OBJ_ROBOT_MC_L                 = 0x01,
    RO_OBJ_ROBOT_MC_R                 = 0x02,

    RO_OBJ_TOKEN                      = 0x04,

    RO_OBJ_MC_GRABBER_IN              = 0x09,
    RO_OBJ_MC_GRABBER_OUT             = 0x0A,
    RO_OBJ_MC_ANTENNA_IN              = 0x0B,
    RO_OBJ_MC_ANTENNA_OUT             = 0x0C,

    RO_OBJ_DALEK_L                    = 0x22,
    RO_OBJ_DALEK_R                    = 0x23,

    RO_OBJ_VENDING_MACHINE_HANDLE     = 0x24,

    RO_OBJ_MC_THRUSTER_1              = 0x39,
    RO_OBJ_MC_THRUSTER_2              = 0x3A,
    RO_OBJ_MC_THRUSTER_3              = 0x3B,
    RO_OBJ_MC_THRUSTER_4              = 0x3C,
    RO_OBJ_MC_BUMPER_1                = 0x3D,
    RO_OBJ_MC_BUMPER_2                = 0x3E,
    RO_OBJ_MC_BUMPER_3                = 0x3F,
    RO_OBJ_MC_BUMPER_4                = 0x40,

    ///// Innovation Lab only

    RO_OBJ_PROTO_CHIP_OUTSIDE_PIN1    = 0x01,   // Top left
    RO_OBJ_PROTO_CHIP_OUTSIDE_PIN2    = 0x02,
    RO_OBJ_PROTO_CHIP_OUTSIDE_PIN3    = 0x03,
    RO_OBJ_PROTO_CHIP_OUTSIDE_PIN4    = 0x04,   // Bottom left
    RO_OBJ_PROTO_CHIP_OUTSIDE_PIN5    = 0x08,   // Bottom right
    RO_OBJ_PROTO_CHIP_OUTSIDE_PIN6    = 0x07,
    RO_OBJ_PROTO_CHIP_OUTSIDE_PIN7    = 0x06,
    RO_OBJ_PROTO_CHIP_OUTSIDE_PIN8    = 0x05,   // Top right

    RO_OBJ_PROTO_CHIP_TOP_LEFT        = 0x09,
    RO_OBJ_PROTO_CHIP_TOP_RIGHT       = 0x0A,
    RO_OBJ_PROTO_CHIP_BOTTOM_LEFT     = 0x0B,
    RO_OBJ_PROTO_CHIP_BOTTOM_RIGHT    = 0x0C,    

    RO_OBJ_PROTO_CHIP_INSIDE_PIN1     = 0x39,   // Top left
    RO_OBJ_PROTO_CHIP_INSIDE_PIN2     = 0x3a,
    RO_OBJ_PROTO_CHIP_INSIDE_PIN3     = 0x3b,
    RO_OBJ_PROTO_CHIP_INSIDE_PIN4     = 0x3c,   // Bottom left
    RO_OBJ_PROTO_CHIP_INSIDE_PIN5     = 0x40,   // Bottom right
    RO_OBJ_PROTO_CHIP_INSIDE_PIN6     = 0x3f,
    RO_OBJ_PROTO_CHIP_INSIDE_PIN7     = 0x3e,
    RO_OBJ_PROTO_CHIP_INSIDE_PIN8     = 0x3d,   // Top right

    ///// Common to all game binaries

    RO_OBJ_PLAYER                     = 0x00,

    RO_OBJ_SENSOR_DIR1_BODY           = 0x0D,
    RO_OBJ_SENSOR_DIR2_BODY           = 0x0E,
    RO_OBJ_SENSOR_DIR3_BODY           = 0x0F,
    RO_OBJ_SENSOR_ROOM1_BODY          = 0x10,
    RO_OBJ_SENSOR_ROOM2_BODY          = 0x11,
    RO_OBJ_SENSOR_ROOM3_BODY          = 0x12,
    RO_OBJ_SENSOR_CONTACT1_BODY       = 0x13,
    RO_OBJ_SENSOR_CONTACT2_BODY       = 0x14,
    RO_OBJ_SENSOR_CONTACT3_BODY       = 0x15,

    RO_OBJ_SENSOR_ROOM1_OUT           = 0x19,
    RO_OBJ_SENSOR_ROOM2_OUT           = 0x1A,
    RO_OBJ_SENSOR_ROOM3_OUT           = 0x1B,

    RO_OBJ_SPARKY_GRABBER_IN          = 0x1C,
    RO_OBJ_CHECKERS_GRABBER_IN        = 0x1D,
    RO_OBJ_SCANNER_GRABBER_IN         = 0x1E,

    RO_OBJ_SENSOR_CONTACT1_OUT        = 0x1F,
    RO_OBJ_SENSOR_CONTACT2_OUT        = 0x20,
    RO_OBJ_SENSOR_CONTACT3_OUT        = 0x21,

    RO_OBJ_FF_1_LEFT                  = 0x25,
    RO_OBJ_FF_1_RIGHT                 = 0x26,
    RO_OBJ_FF_2_LEFT                  = 0x27,
    RO_OBJ_FF_2_RIGHT                 = 0x28,
    RO_OBJ_FF_3_LEFT                  = 0x29,
    RO_OBJ_FF_3_RIGHT                 = 0x2A,
    RO_OBJ_FF_4_LEFT                  = 0x2B,
    RO_OBJ_FF_4_RIGHT                 = 0x2C,
    RO_OBJ_FF_5_LEFT                  = 0x2D,
    RO_OBJ_FF_5_RIGHT                 = 0x2E,
    RO_OBJ_FF_6_LEFT                  = 0x2F,
    RO_OBJ_FF_6_RIGHT                 = 0x30,
    RO_OBJ_FF_7_LEFT                  = 0x31,
    RO_OBJ_FF_7_RIGHT                 = 0x32,
    RO_OBJ_FF_8_LEFT                  = 0x33,
    RO_OBJ_FF_8_RIGHT                 = 0x34,
    RO_OBJ_FF_9_LEFT                  = 0x35,
    RO_OBJ_FF_9_RIGHT                 = 0x36,
    RO_OBJ_FF_10_LEFT                 = 0x37,
    RO_OBJ_FF_10_RIGHT                = 0x38,

    RO_OBJ_SCANNER_THRUSTER_1         = 0x41,
    RO_OBJ_SCANNER_THRUSTER_2         = 0x42,
    RO_OBJ_SCANNER_THRUSTER_3         = 0x43,
    RO_OBJ_SCANNER_THRUSTER_4         = 0x44,
    RO_OBJ_SCANNER_BUMPER_1           = 0x45,
    RO_OBJ_SCANNER_BUMPER_2           = 0x46,
    RO_OBJ_SCANNER_BUMPER_3           = 0x47,
    RO_OBJ_SCANNER_BUMPER_4           = 0x48,

    RO_OBJ_CHECKERS_THRUSTER_1        = 0x49,
    RO_OBJ_CHECKERS_THRUSTER_2        = 0x4A,
    RO_OBJ_CHECKERS_THRUSTER_3        = 0x4B,
    RO_OBJ_CHECKERS_THRUSTER_4        = 0x4C,
    RO_OBJ_CHECKERS_BUMPER_1          = 0x4D,
    RO_OBJ_CHECKERS_BUMPER_2          = 0x4E,
    RO_OBJ_CHECKERS_BUMPER_3          = 0x4F,
    RO_OBJ_CHECKERS_BUMPER_4          = 0x50,

    RO_OBJ_SPARKY_THRUSTER_1          = 0x51,
    RO_OBJ_SPARKY_THRUSTER_2          = 0x52,
    RO_OBJ_SPARKY_THRUSTER_3          = 0x53,
    RO_OBJ_SPARKY_THRUSTER_4          = 0x54,
    RO_OBJ_SPARKY_BUMPER_1            = 0x55,
    RO_OBJ_SPARKY_BUMPER_2            = 0x56,
    RO_OBJ_SPARKY_BUMPER_3            = 0x57,
    RO_OBJ_SPARKY_BUMPER_4            = 0x58,

    RO_OBJ_SPARKY_ANTENNA_IN          = 0x59,

    RO_OBJ_SENSOR_DIR1_OUT1           = 0x5A,
    RO_OBJ_SENSOR_DIR1_OUT2           = 0x5B,
    RO_OBJ_SENSOR_DIR1_OUT3           = 0x5C,
    RO_OBJ_SENSOR_DIR1_OUT4           = 0x5D,

    RO_OBJ_SENSOR_DIR2_OUT1           = 0x5E,
    RO_OBJ_SENSOR_DIR2_OUT2           = 0x5F,
    RO_OBJ_SENSOR_DIR2_OUT3           = 0x60,
    RO_OBJ_SENSOR_DIR2_OUT4           = 0x61,

    RO_OBJ_SENSOR_DIR3_OUT1           = 0x62,
    RO_OBJ_SENSOR_DIR3_OUT2           = 0x63,
    RO_OBJ_SENSOR_DIR3_OUT3           = 0x64,
    RO_OBJ_SENSOR_DIR3_OUT4           = 0x65,

    RO_OBJ_SPARKY_ANTENNA_OUT         = 0x66,

    RO_OBJ_GATE_1_BODY                = 0x67,
    RO_OBJ_GATE_1_IN1                 = 0x68,
    RO_OBJ_GATE_1_IN2                 = 0x69,
    RO_OBJ_GATE_2_BODY                = 0x6A,
    RO_OBJ_GATE_2_IN1                 = 0x6B,
    RO_OBJ_GATE_2_IN2                 = 0x6C,
    RO_OBJ_GATE_3_BODY                = 0x6D,
    RO_OBJ_GATE_3_IN1                 = 0x6E,
    RO_OBJ_GATE_3_IN2                 = 0x6F,
    RO_OBJ_GATE_4_BODY                = 0x70,
    RO_OBJ_GATE_4_IN1                 = 0x71,
    RO_OBJ_GATE_4_IN2                 = 0x72,
    RO_OBJ_GATE_5_BODY                = 0x73,
    RO_OBJ_GATE_5_IN1                 = 0x74,
    RO_OBJ_GATE_5_IN2                 = 0x75,
    RO_OBJ_GATE_6_BODY                = 0x76,
    RO_OBJ_GATE_6_IN1                 = 0x77,
    RO_OBJ_GATE_6_IN2                 = 0x78,
    RO_OBJ_GATE_7_BODY                = 0x79,
    RO_OBJ_GATE_7_IN1                 = 0x7A,
    RO_OBJ_GATE_7_IN2                 = 0x7B,
    RO_OBJ_GATE_8_BODY                = 0x7C,
    RO_OBJ_GATE_8_IN1                 = 0x7D,
    RO_OBJ_GATE_8_IN2                 = 0x7E,
    RO_OBJ_GATE_9_BODY                = 0x7F,
    RO_OBJ_GATE_9_IN1                 = 0x80,
    RO_OBJ_GATE_9_IN2                 = 0x81,
    RO_OBJ_GATE_10_BODY               = 0x82,
    RO_OBJ_GATE_10_IN1                = 0x83,
    RO_OBJ_GATE_10_IN2                = 0x84,
    RO_OBJ_GATE_11_BODY               = 0x85,
    RO_OBJ_GATE_11_IN1                = 0x86,
    RO_OBJ_GATE_11_IN2                = 0x87,
    RO_OBJ_GATE_12_BODY               = 0x88,
    RO_OBJ_GATE_12_IN1                = 0x89,
    RO_OBJ_GATE_12_IN2                = 0x8A,
    RO_OBJ_GATE_13_BODY               = 0x8B,
    RO_OBJ_GATE_13_IN1                = 0x8C,
    RO_OBJ_GATE_13_IN2                = 0x8D,
    RO_OBJ_GATE_14_BODY               = 0x8E,
    RO_OBJ_GATE_14_IN1                = 0x8F,
    RO_OBJ_GATE_14_IN2                = 0x90,
    RO_OBJ_GATE_15_BODY               = 0x91,
    RO_OBJ_GATE_15_IN1                = 0x92,
    RO_OBJ_GATE_15_IN2                = 0x93,
    RO_OBJ_GATE_16_BODY               = 0x94,
    RO_OBJ_GATE_16_IN1                = 0x95,
    RO_OBJ_GATE_16_IN2                = 0x96,
    RO_OBJ_GATE_17_BODY               = 0x97,
    RO_OBJ_GATE_17_IN1                = 0x98,
    RO_OBJ_GATE_17_IN2                = 0x99,
    RO_OBJ_GATE_18_BODY               = 0x9A,
    RO_OBJ_GATE_18_IN1                = 0x9B,
    RO_OBJ_GATE_18_IN2                = 0x9C,
    RO_OBJ_GATE_19_BODY               = 0x9D,
    RO_OBJ_GATE_19_IN1                = 0x9E,
    RO_OBJ_GATE_19_IN2                = 0x9F,
    RO_OBJ_GATE_20_BODY               = 0xA0,
    RO_OBJ_GATE_20_IN1                = 0xA1,
    RO_OBJ_GATE_20_IN2                = 0xA2,
    RO_OBJ_GATE_21_BODY               = 0xA3,
    RO_OBJ_GATE_21_IN1                = 0xA4,
    RO_OBJ_GATE_21_IN2                = 0xA5,
    RO_OBJ_GATE_22_BODY               = 0xA6,
    RO_OBJ_GATE_22_IN1                = 0xA7,
    RO_OBJ_GATE_22_IN2                = 0xA8,
    RO_OBJ_GATE_23_BODY               = 0xA9,
    RO_OBJ_GATE_23_IN1                = 0xAA,
    RO_OBJ_GATE_23_IN2                = 0xAB,
    RO_OBJ_GATE_24_BODY               = 0xAC,
    RO_OBJ_GATE_24_IN1                = 0xAD,
    RO_OBJ_GATE_24_IN2                = 0xAE,
    RO_OBJ_GATE_25_BODY               = 0xAF,
    RO_OBJ_GATE_25_IN1                = 0xB0,
    RO_OBJ_GATE_25_IN2                = 0xB1,
    RO_OBJ_GATE_26_BODY               = 0xB2,
    RO_OBJ_GATE_26_IN1                = 0xB3,
    RO_OBJ_GATE_26_IN2                = 0xB4,
    RO_OBJ_GATE_27_BODY               = 0xB5,
    RO_OBJ_GATE_27_IN1                = 0xB6,
    RO_OBJ_GATE_27_IN2                = 0xB7,
    RO_OBJ_GATE_28_BODY               = 0xB8,
    RO_OBJ_GATE_28_IN1                = 0xB9,
    RO_OBJ_GATE_28_IN2                = 0xBA,
    RO_OBJ_GATE_29_BODY               = 0xBB,
    RO_OBJ_GATE_29_IN1                = 0xBC,
    RO_OBJ_GATE_29_IN2                = 0xBD,
    RO_OBJ_GATE_30_BODY               = 0xBE,
    RO_OBJ_GATE_30_IN1                = 0xBF,
    RO_OBJ_GATE_30_IN2                = 0xC0,
    RO_OBJ_GATE_31_BODY               = 0xC1,
    RO_OBJ_GATE_31_IN1                = 0xC2,
    RO_OBJ_GATE_31_IN2                = 0xC3,
    RO_OBJ_GATE_32_BODY               = 0xC4,
    RO_OBJ_GATE_32_IN1                = 0xC5,
    RO_OBJ_GATE_32_IN2                = 0xC6,
    RO_OBJ_GATE_33_BODY               = 0xC7,
    RO_OBJ_GATE_33_IN1                = 0xC8,
    RO_OBJ_GATE_33_IN2                = 0xC9,
    RO_OBJ_GATE_34_BODY               = 0xCA,
    RO_OBJ_GATE_34_IN1                = 0xCB,
    RO_OBJ_GATE_34_IN2                = 0xCC,
    RO_OBJ_GATE_35_BODY               = 0xCD,
    RO_OBJ_GATE_35_IN1                = 0xCE,
    RO_OBJ_GATE_35_IN2                = 0xCF,

    RO_OBJ_SPARKY_GRABBER_OUT         = 0xD0,

    RO_OBJ_NODE_1                     = 0xD1,
    RO_OBJ_NODE_2                     = 0xD2,
    RO_OBJ_NODE_3                     = 0xD3,
    RO_OBJ_NODE_4                     = 0xD4,
    RO_OBJ_NODE_5                     = 0xD5,
    RO_OBJ_NODE_6                     = 0xD6,
    RO_OBJ_NODE_7                     = 0xD7,
    RO_OBJ_NODE_8                     = 0xD8,
    RO_OBJ_NODE_9                     = 0xD9,
    RO_OBJ_NODE_10                    = 0xDA,
    RO_OBJ_NODE_11                    = 0xDB,
    RO_OBJ_NODE_12                    = 0xDC,
    RO_OBJ_NODE_13                    = 0xDD,
    RO_OBJ_NODE_14                    = 0xDE,
    RO_OBJ_NODE_15                    = 0xDF,

    RO_OBJ_TOOLBOX                    = 0xE1,

    RO_OBJ_SCANNER_ANTENNA_IN         = 0xE2,
    RO_OBJ_SCANNER_ANTENNA_OUT        = 0xE3,
    RO_OBJ_SCANNER_GRABBER_OUT        = 0xE4,
    RO_OBJ_CHECKERS_ANTENNA_IN        = 0xE5,
    RO_OBJ_CHECKERS_ANTENNA_OUT       = 0xE6,

    RO_OBJ_CHIP_1                     = 0xE7,
    RO_OBJ_CHIP_2                     = 0xE8,
    RO_OBJ_CHIP_3                     = 0xE9,
    RO_OBJ_CHIP_4                     = 0xEA,
    RO_OBJ_CHIP_5                     = 0xEB,
    RO_OBJ_CHIP_6                     = 0xEC,
    RO_OBJ_CHIP_7                     = 0xED,
    RO_OBJ_CHIP_8                     = 0xEE,

    RO_OBJ_CHECKERS_GRABBER_OUT       = 0xE4,

    RO_OBJ_ROBOT_SPARKY_L             = 0xF0,
    RO_OBJ_ROBOT_SPARKY_R             = 0xF1,
    RO_OBJ_ROBOT_CHECKERS_L           = 0xF2,
    RO_OBJ_ROBOT_CHECKERS_R           = 0xF3,
    RO_OBJ_ROBOT_SCANNER_L            = 0xF4,
    RO_OBJ_ROBOT_SCANNER_R            = 0xF5,

    RO_OBJ_SHAPE_1                    = 0xF6,
    RO_OBJ_SHAPE_2                    = 0xF7,
    RO_OBJ_SHAPE_3                    = 0xF8,
    RO_OBJ_SHAPE_4                    = 0xF9,
    RO_OBJ_SHAPE_5                    = 0xFA,       // Blue key in game
    RO_OBJ_SHAPE_6                    = 0xFB,

    RO_OBJ_ANTENNA                    = 0xFC,
    RO_OBJ_SPECIAL_CURSOR             = 0xFD,       // Debug? Level editor? What is this?
    RO_OBJ_CURSOR                     = 0xFE,

    RO_OBJ_NONE                       = 0xFF,
};


/*
 * Sprite IDs
 */
enum ROSpriteId {
    RO_SPR_CURSOR = 0,              // 0x00
    RO_SPR_PIN_INOUT_LEFT,          // 0x01
    RO_SPR_PIN_INOUT_RIGHT,         // 0x02
    RO_SPR_SOLIDBLACK_1,            // 0x03
    RO_SPR_SOLIDBLACK_2,            // 0x04
    RO_SPR_SOLIDBLACK_3,            // 0x05
    RO_SPR_PIN_INPUT_DOWN,          // 0x06
    RO_SPR_ANDGATE_UP,              // 0x07
    RO_SPR_ANDGATE_DOWN,            // 0x08
    RO_SPR_ORGATE_UP,               // 0x09
    RO_SPR_ORGATE_DOWN,             // 0x0A
    RO_SPR_XORGATE_UP,              // 0x0B
    RO_SPR_XORGATE_DOWN,            // 0x0C
    RO_SPR_NOTGATE_UP,              // 0x0D
    RO_SPR_NOTGATE_DOWN,            // 0x0E
    RO_SPR_PIN_INPUT2_DOWN,         // 0x0F
    RO_SPR_PIN_INPUT2_UP,           // 0x10
    RO_SPR_PIN_INPUT3_DOWN,         // 0x11
    RO_SPR_PIN_INPUT3_UP,           // 0x12
    RO_SPR_FLIPFLOP_RIGHT,          // 0x13
    RO_SPR_FLIPFLOP_LEFT,           // 0x14
    RO_SPR_PIN_INPUT_LEFT,          // 0x15
    RO_SPR_PIN_INPUT_RIGHT,         // 0x16
    RO_SPR_PIN_OUTPUT_LEFT,         // 0x17
    RO_SPR_PIN_OUTPUT_RIGHT,        // 0x18
    RO_SPR_NODE,                    // 0x19
    RO_SPR_BLANK,                   // 0x1A
    RO_SPR_TOOLBOX,                 // 0x1B
    RO_SPR_SOLDER_IRON,             // 0x1C
    RO_SPR_SOLDER_TIP,              // 0x1D
    RO_SPR_BATTERY_TOP,             // 0x1E
    RO_SPR_PAINTBRUSH,              // 0x1F
    RO_SPR_CRYSTAL_CHARGER,         // 0x20
    RO_SPR_CHIP_1,                  // 0x21
    RO_SPR_CHIP_2,                  // 0x22
    RO_SPR_CHIP_3,                  // 0x23
    RO_SPR_CHIP_4,                  // 0x24
    RO_SPR_CHIP_5,                  // 0x25
    RO_SPR_CHIP_6,                  // 0x26
    RO_SPR_CHIP_7,                  // 0x27
    RO_SPR_CHIP_8,                  // 0x28
    RO_SPR_CHIP_BLANK,              // 0x29
    RO_SPR_THRUSTER_RIGHT,          // 0x2A
    RO_SPR_THRUSTER_LEFT,           // 0x2B
    RO_SPR_THRUSTER_ANIM1W,         // 0x2C
    RO_SPR_THRUSTER_ANIM2W,         // 0x2D
    RO_SPR_THRUSTER_ANIM3W,         // 0x2E
    RO_SPR_THRUSTER_ANIM4W,         // 0x2F
    RO_SPR_THRUSTER_SWITCH_CLOSED,  // 0x30
    RO_SPR_THRUSTER_SWITCH_OPEN,    // 0x31
    RO_SPR_SENTRY_BODY,             // 0x32
    RO_SPR_KEYHOLE,                 // 0x33
    RO_SPR_PIN_OUTPUT_UP,           // 0x34
    RO_SPR_PIN_OUTPUT_DOWN,         // 0x35
    RO_SPR_REMOTE_CONTROL,          // 0x36
    RO_SPR_BUTTON,                  // 0x37
    RO_SPR_KEY,                     // 0x38
    RO_SPR_CRYSTAL,                 // 0x39

    RO_SPR_GRABBER_UP,              // 0x3A
    RO_SPR_GRABBER_RIGHT,           // 0x3B
    RO_SPR_GRABBER_LEFT,            // 0x3C
    RO_SPR_GRABBER_DOWN,            // 0x3D
    RO_SPR_UNUSED_1,                // 0x3E
    RO_SPR_UNUSED_2,                // 0x3F

    // Note: Some of the grabber sprites are different in GAME.EXE
    RO_SPR_GAME_GRABBER_UP = RO_SPR_GRABBER_RIGHT,
    RO_SPR_GAME_GRABBER_RIGHT = RO_SPR_GRABBER_LEFT,
    RO_SPR_GAME_GRABBER_LEFT = RO_SPR_UNUSED_1,
};


/*
 * Robot IDs.
 *
 * Often a robot is referred to by its room ID or object ID,
 * but these numbers are used in the RORobot tables.
 */
enum RORobotId {
    RO_ROBOT_SPARKY = 0,
    RO_ROBOT_CHECKERS,
    RO_ROBOT_SCANNER,
    RO_ROBOT_MC,
};

/*
 * Robot sides. These are used as indices into the bumper, thruster,
 * and grabber tables.
 */
enum ROSide {
    RO_SIDE_TOP = 0,
    RO_SIDE_RIGHT,
    RO_SIDE_BOTTOM,
    RO_SIDE_LEFT,
};

/*
 * World IDs.
 *
 * These are numbers that identify each world uniquely.  They are used
 * as command line parameters to the game binaries, and they are
 * stored in the saved game files to identify which world the save
 * goes with.
 */
enum ROWorldId {
    RO_WORLD_SEWER = 0,
    RO_WORLD_SUBWAY = 1,
    RO_WORLD_TOWN = 2,
    RO_WORLD_COMP = 3,
    RO_WORLD_STREET = 4,

    RO_WORLD_TUT1 = 21,
    RO_WORLD_TUT2 = 22,
    RO_WORLD_TUT3 = 23,
    RO_WORLD_TUT4 = 24,
    RO_WORLD_TUT5 = 25,
    RO_WORLD_TUT6 = 26,
    RO_WORLD_TUT7 = 27,

    RO_WORLD_LAB = 30,
    RO_WORLD_DEMO = 40,

    RO_WORLD_SAVED = 99,   // As a command line option, opens the load menu
};


/*
 * Sprites are a 16x8 bitmap
 */
typedef uint8_t ROSprite[16];

/*
 * Rooms are a 30-byte bitmap, in which each byte is a 2x4 grid of tiles.
 */
typedef uint8_t RORoomTiles[30];


/*
 * Robot Odyssey's world file data. This is in memory at the address
 * SBTADDR_WORLD_DATA, and it is the first structure in the saved game
 * and level data files.
 *
 * Length: 0x3500
 */
class ROWorld {
 public:
    struct {
        uint8_t nextInRoom[0x100];          // 0x0000
        uint8_t spriteId[0x100];            // 0x0100
        uint8_t color[0x100];               // 0x0200
        uint8_t room[0x100];                // 0x0300
        uint8_t x[0x100];                   // 0x0400
        uint8_t y[0x100];                   // 0x0500
        struct {
            uint8_t object[0x100];          // 0x0600
            struct {
                uint8_t x[0x100];           // 0x0700
                uint8_t y[0x100];           // 0x0800
            } offset;
        } movedBy;
        uint8_t grabFlag[0x100];            // 0x0900
    } objects;

    ROSprite sprites[64];                   // 0x0A00

    struct {
        uint8_t objectListHead[0x40];       // 0x0E00
        uint8_t bgColor[0x40];              // 0x0E40
        uint8_t fgColor[0x40];              // 0x0E80
        struct {
            uint8_t up[0x40];               // 0x0EC0
            uint8_t down[0x40];             // 0x0F00
            uint8_t right[0x40];            // 0x0F40
            uint8_t left[0x40];             // 0x0F80
        } links;
        uint8_t reserved[0x40];             // 0x0FC0   Always zero?
        RORoomTiles tiles[0x40];            // 0x1000
    } rooms;

    uint8_t unknown[0x100];                 // 0x1780

    struct {
        uint8_t room[0x80];                 // 0x1880
        uint8_t x[0x80];                    // 0x1900
        uint8_t y[0x80];                    // 0x1980
        uint8_t style[0x80];                // 0x1A00
        uint8_t font[0x80];                 // 0x1A80
        uint8_t color[0x80];                // 0x1B00
        uint8_t ptrLow[0x80];               // 0x1B80
        uint8_t ptrHigh[0x80];              // 0x1C00

        uint8_t stringHeap[0x1880];         // 0x1C80
    } text;

    void clear();
    static ROWorld *fromProcess(SBTProcess *proc);

    RORoomId getObjectRoom(ROObjectId obj);
    void getObjectXY(ROObjectId obj, int &x, int &y);

    void setObjectRoom(ROObjectId obj, RORoomId room);
    void setObjectXY(ROObjectId obj, int x, int y);

    void setRobotRoom(ROObjectId obj, RORoomId room);
    void setRobotXY(ROObjectId obj, int x, int y);

 private:
    void removeObjectFromRoom(ROObjectId obj, RORoomId room);
    void addObjectToRoom(ROObjectId obj, RORoomId room);
} __attribute__ ((packed));


/*
 * Gate types
 */
enum ROGate {
    RO_GATE_NONE = 0,
    RO_GATE_AND  = 1,
    RO_GATE_OR   = 2,
    RO_GATE_XOR  = 3,
    RO_GATE_NOT  = 4,
};


/*
 * Robot Odyssey's circuit data. This is in memory at the address
 * SBTADDR_CIRCUIT_DATA, and it is also saved on disk as the .CIR file
 * or as part of a saved game file.
 */
class ROCircuit {
 public:
    struct {
        // Wires attached to normal sprite objects
        struct {
            uint8_t output_obj[0x100];  // 0x0000   What object does this wire drive?
            uint8_t x1[0x100];          // 0x0100
            uint8_t x2[0x100];          // 0x0200
            uint8_t y1[0x100];          // 0x0300
            uint8_t y2[0x100];          // 0x0400
        } obj_wires;
        
        // Flip flop state
        struct {
            uint8_t state[20];          // 0x0500   One byte per half, 0 or 1
            uint8_t inputs[20];         // 0x0514   Signal applied to input pins
        } ff;

        // Additional wires for nodes
        struct {
            uint8_t input_obj[15];      // 0x0528   What object drives this node?
            uint8_t output2_obj[15];    // 0x0537   Second output for nodes
            uint8_t x1[15];             // 0x0546
            uint8_t x2[15];             // 0x0555
            uint8_t y1[15];             // 0x0564
            uint8_t y2[15];             // 0x0573
        } node_wires;

        // Wires attached to small chips
        struct {
            uint8_t y1[64];             // 0x0582
            uint8_t y2[64];             // 0x05c2
            uint8_t x1[64];             // 0x0602
            uint8_t x2[64];             // 0x0642
            uint8_t output_obj[64];     // 0x0682   What object does this wire drive?
            uint8_t output_pin[64];     // 0x06c2   FF if obj isn't a chip, or 0-7
        } chip_wires;
        
        // Allocating free gates
        struct {
            uint8_t gates[105];         // 0x0702   ROGate type, every 3rd slot used
            uint8_t nodes[15];          // 0x076b
            uint8_t ff[20];             // 0x077a   By sprite ID, only even slots used
        } allocation;

        uint8_t special_cursor_obj;     // 0x078e   Normally zero, special: 0xFD, nonzero
        uint8_t remote_is_on;           // 0x078f

        // Toolbox status
        struct {
            uint8_t ff_count;           // 0x0790
            uint8_t node_count;         // 0x0791
            uint8_t gate_count;         // 0x0792
            uint8_t is_closed;          // 0x0793
        } toolbox;
    };

    static ROCircuit *fromProcess(SBTProcess *proc);
} __attribute__ ((packed));


/*
 * Robot Odyssey's main table of per-robot internal data. This
 * contains thruster state, bumper state, thruster switch state, and
 * grabber state. It includes state that can't be recovered from just
 * a world file.
 */
class RORobot {
 public:
    uint8_t objLeft;           // Left half of robot
    uint8_t objLeft2;          // (duplicate)
    uint8_t objRight;          // Right half of robot
    uint8_t objRight2;         // (duplicate)
    uint8_t objThrusters[4];
    uint8_t objBumpers[4];

    uint8_t thrusterState[4];  // Animation frame #, or 0 if off
    uint8_t bumperState[4];    // Color ID
    uint8_t grabberState[4];

    uint8_t batteryLevel;      // 0 through 15
    uint8_t thrusterSwitch;    // 0 or 1

    ROObjectId getObjectId() {
        return (ROObjectId) objLeft;
    }

    void thrusterEnable(ROSide side, bool on) {
        if (on) {
            if (thrusterState[side] == 0) {
                thrusterState[side] = 1;
            }
        } else {
            thrusterState[side] = 0;
        }
    }

    void animateThrusters() {
        static const uint8_t nextState[4] = {0, 2, 3, 1};

        for (unsigned int i = 0; i < sizeof thrusterState; i++) {
            thrusterState[i] = nextState[thrusterState[i]];
        }
    }

    static RORobot *fromProcess(SBTProcess *proc);
} __attribute__ ((packed));


/*
 * Robot grabber directions. This table specifies which direction a
 * robot's grabber is pointing. It has four entries, one for each
 * direction. Each entry is zero (no grabber) or a sprite index.
 */
class RORobotGrabber {
 public:
    uint8_t state[4];

    static RORobotGrabber *fromProcess(SBTProcess *proc);
} __attribute__ ((packed));


/*
 * Robot battery discharge accumulators. The RORobot::batteryLevel
 * byte keeps track of the visible battery guage status, but this is
 * the internal accumulator that is used to track the actual use of
 * power in a robot. As the robot moves, this number is
 * incremented. When it overflows, one bar of battery is removed from
 * RORobot::batteryLevel.
 *
 * This is in an array which comes almost immediately after RORobot
 * in memory. (There is one intervening 0xFF termination byte.)
 */
class RORobotBatteryAcc {
 public:
    uint8_t high;
    uint8_t low;

    inline uint16_t get() {
        return (high << 8) | low;
    }
} __attribute__ ((packed));


/*
 * Data for compiled chips. The format is described in detail by:
 * http://scanwidget.livejournal.com/38373.html
 */
struct ROChipBytecode
{
    uint8_t bytes[1024];
};

struct ROChipPins
{
    uint8_t pins[8];
};

/*
 * The on-disk file format for a saved game. These are the .GSV/.LSV
 * files saved by GAME.EXE or LAB.EXE. There doesn't seem to be any
 * pre- or post-processing at all. The saved game files are just dumps
 * of the in-memory structures for the world and circuit data.
 *
 * Warning: This is only correct for GAME.EXE and LAB.EXE. The tutorial
 *          uses a slightly different circuit format.
 */
class ROSavedGame
{
public:
    ROWorld world;                    // Length: 0x3500
    union {
        ROCircuit circuit;            // Length: 0x0A00
        uint8_t circuitPadding[0xA00];
    };
    ROChipBytecode chipBytecode[8];   // Length: 8 * 1024
    ROChipPins chipPins[8];           // Length: 8 * 8

    // 5 extra bytes which are saved individually
    uint8_t unk_objectId_1;
    uint8_t unk_objectId_2;
    uint8_t unk_offset_x;
    uint8_t unk_offset_y;
    uint8_t worldId;

    const char *getWorldName();
    const char *getProcessName();
};


/*
 * Top level class for all data we know how to poke at in a Robot Odyssey binary.
 */
class ROData
{
 public:
    bool fromProcess(SBTProcess *proc);

    ROWorld *world;
    ROCircuit *circuit;

    struct {
        int count;
        RORobot *state;
        RORobotGrabber *grabbers;
        RORobotBatteryAcc *batteryAcc;
    } robots;

    void copyFrom(ROData *source);
};


/*
 * Saved joystick (and other) configuration data, the 16-byte joyfile.dat
 */
class ROJoyfile
{
 public:
    ROJoyfile();

    // When enabled, CTRL-E toggles collision detection on/off
    void setCheatsEnabled(bool enable);

    uint8_t joystick_enabled;

    uint16_t joystick_io_port;
    static const uint16_t DEFAULT_JOYSTICK_PORT = 0x201;

    uint8_t x_center;
    uint8_t y_center;
    static const uint8_t DEFAULT_JOYSTICK_CENTER = 0x80;

    uint8_t xplus_divisor;
    uint8_t yplus_divisor;
    uint8_t xminus_divisor;
    uint8_t yminus_divisor;
    static const uint8_t DEFAULT_JOYSTICK_DIVISOR = 0x01;

    uint8_t cheat_control;
    static const uint8_t CHEATS_ENABLED = 0x5e;

    // Not sure what this is (joyfile+A), its use in GAME.EXE triggers
    // a delay somewhere in the Street world's main loop, and MENU.EXE
    // chooses different sound routines depending on whether this is
    // under 0x200, between 0x200 and 0x500, or over 0x500. Sound volume?
    // Hardware related? Debug related?
    uint16_t debug_control;
    static const uint16_t DEBUG_NORMAL = 0x238;
    static const uint16_t DEBUG_DELAY_IN_STREET_LOOP_AT_C1FD = 0x500;

    uint8_t disk_drive_id;
    static const uint8_t DRIVE_A = 0x01;
    static const uint8_t DRIVE_B = 0x02;

    uint8_t joyfile_D;
    uint8_t joyfile_E;
    uint8_t joyfile_F;

} __attribute__ ((packed));
