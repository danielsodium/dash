#ifndef _COMMON_H_
#define _COMMON_H_

#include <xkbcommon/xkbcommon.h>

typedef struct Overlord Overlord;
typedef struct Window Window;
typedef struct Widget Widget;

enum Surfaces {
    SURFACE_OVERLAY,
    SURFACE_TOP,
    SURFACE_BOTTOM,
    SURFACE_BACKGROUND
};

// on_key lines up with the enum for key_state and KeyboardEvent
enum KeyboardEvent {
    KEYBOARD_EVENT_KEY_RELEASE,
    KEYBOARD_EVENT_KEY_PRESS,
    KEYBOARD_EVENT_KEY_REPEAT,
    KEYBOARD_EVENT_KEYMAP,
    KEYBOARD_EVENT_LEAVE,
    KEYBOARD_EVENT_ENTER,
    KEYBOARD_EVENT_MODIFIER,
};

struct KeyboardData {
    enum KeyboardEvent event;
    xkb_keysym_t* key;
};

typedef struct Keyboard Keyboard;
typedef struct KeyboardData KeyboardData;

#endif
