#pragma once

#include "bs.h"

namespace os
{

enum Keycode {
    KC_Unknown = -1,
    KC_A       = 0,
    KC_B,
    KC_C,
    KC_D,
    KC_E,
    KC_F,
    KC_G,
    KC_H,
    KC_I,
    KC_J,
    KC_K,
    KC_L,
    KC_M,
    KC_N,
    KC_O,
    KC_P,
    KC_Q,
    KC_R,
    KC_S,
    KC_T,
    KC_U,
    KC_V,
    KC_W,
    KC_X,
    KC_Y,
    KC_Z,
    KC_Num0,
    KC_Num1,
    KC_Num2,
    KC_Num3,
    KC_Num4,
    KC_Num5,
    KC_Num6,
    KC_Num7,
    KC_Num8,
    KC_Num9,
    KC_Escape,
    KC_LControl,
    KC_LShift,
    KC_LAlt,
    KC_LSystem,   // The left OS specific key: window (Windows and Linux), apple
                  // (MacOS X), ...
    KC_RControl,  // The right Control key
    KC_RShift,    // The right Shift key
    KC_RAlt,      // The right Alt key
    KC_RSystem,   // The right OS specific key: window (Windows and Linux), apple
                  // (MacOS X), ...
    KC_Menu,      // The Menu key
    KC_LBracket,  // The [ key
    KC_RBracket,  // The ] key
    KC_Semicolon,
    KC_Comma,
    KC_Period,
    KC_Quote,
    KC_Slash,
    KC_Backslash,
    KC_Tilde,
    KC_Equal,
    KC_Hyphen,
    KC_Space,
    KC_Enter,
    KC_Backspace,
    KC_Tab,
    KC_PageUp,
    KC_PageDown,
    KC_End,
    KC_Home,
    KC_Insert,
    KC_Delete,
    KC_Add,
    KC_Subtract,
    KC_Multiply,
    KC_Divide,
    KC_Left,
    KC_Right,
    KC_Up,
    KC_Down,
    KC_Numpad0,
    KC_Numpad1,
    KC_Numpad2,
    KC_Numpad3,
    KC_Numpad4,
    KC_Numpad5,
    KC_Numpad6,
    KC_Numpad7,
    KC_Numpad8,
    KC_Numpad9,
    KC_F1,
    KC_F2,
    KC_F3,
    KC_F4,
    KC_F5,
    KC_F6,
    KC_F7,
    KC_F8,
    KC_F9,
    KC_F10,
    KC_F11,
    KC_F12,
    KC_F13,
    KC_F14,
    KC_F15,
    KC_Pause,

    KC_KeyCount,  // Total count
};

struct KeyModState {
    bool operator==(const KeyModState& b) const { return shift == b.shift && ctrl == b.ctrl && alt == b.alt && sys == b.sys; }
    bool shift = false;
    bool ctrl  = false;
    bool alt   = false;
    bool sys   = false;
};

};  // namespace os
