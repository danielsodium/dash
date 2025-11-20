#ifndef _COMMON_H_
#define _COMMON_H_

typedef struct Window Window;

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
