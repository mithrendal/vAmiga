// -----------------------------------------------------------------------------
// This file is part of vAmiga
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

// This file must conform to standard ANSI-C to be compatible with Swift.

#ifndef _PORT_T_INC
#define _PORT_T_INC

//
// Enumerations
//

typedef enum : long
{
    SPD_NONE,
    SPD_LOOPBACK
}
SerialPortDevice;

inline bool isSerialPortDevice(long value) {
    return value >= 0 && value <= SPD_LOOPBACK;
}

typedef enum : long
{
    CPD_NONE,
    CPD_MOUSE,
    CPD_JOYSTICK
}
ControlPortDevice;

inline bool isControlPortDevice(long value) {
    return value >= 0 && value <= CPD_JOYSTICK;
}

typedef enum
{
    PULL_UP = 0,   // Pull the joystick up
    PULL_DOWN,     // Pull the joystick down
    PULL_LEFT,     // Pull the joystick left
    PULL_RIGHT,    // Pull the joystick right
    PRESS_FIRE,    // Press the joystick button
    PRESS_LEFT,    // Press the left mouse button
    PRESS_RIGHT,   // Press the right mouse button
    RELEASE_X,     // Move back to neutral horizontally
    RELEASE_Y,     // Move back to neutral vertically
    RELEASE_XY,    // Move back to neutral
    RELEASE_FIRE,  // Release the joystick button
    RELEASE_LEFT,  // Release the left mouse button
    RELEASE_RIGHT  // Release the right mouse button
}
GamePadAction;

inline bool isGamePadAction(long value) {
    return value >= 0 && value <= RELEASE_RIGHT;
}

//
// Structures
//

typedef struct
{
    bool m0v;
    bool m0h;
    bool m1v;
    bool m1h;
    uint16_t joydat;
    uint16_t potgo;
    uint16_t potgor;
    uint16_t potdat;
}
ControlPortInfo;

typedef struct
{
    SerialPortDevice device;
}
SerialPortConfig;

typedef struct
{
    u32 port;

    bool txd;
    bool rxd;
    bool rts;
    bool cts;
    bool dsr;
    bool cd;
    bool dtr;
}
SerialPortInfo;

typedef struct
{
    bool pullUpResistors;
}
MouseConfig;

#endif
