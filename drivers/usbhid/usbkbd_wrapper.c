/**
 * USB Keyboard wrapper for Cabal
 * Converts HID keycodes to Cabal keycodes (same format as PS/2 wrapper)
 */

#include "usbkbd_wrapper.h"
#include "usbhid.h"

#ifdef USB_HID_ENABLED

// Cabal key codes (same as Common::KeyCode values used by ScummVM)
#define CABAL_KEY_BACKSPACE  8
#define CABAL_KEY_TAB        9
#define CABAL_KEY_RETURN     13
#define CABAL_KEY_ESCAPE     27
#define CABAL_KEY_SPACE      32

#define CABAL_KEY_F1         282
#define CABAL_KEY_F2         283
#define CABAL_KEY_F3         284
#define CABAL_KEY_F4         285
#define CABAL_KEY_F5         286
#define CABAL_KEY_F6         287
#define CABAL_KEY_F7         288
#define CABAL_KEY_F8         289
#define CABAL_KEY_F9         290
#define CABAL_KEY_F10        291
#define CABAL_KEY_F11        292
#define CABAL_KEY_F12        293

#define CABAL_KEY_UP         273
#define CABAL_KEY_DOWN       274
#define CABAL_KEY_RIGHT      275
#define CABAL_KEY_LEFT       276
#define CABAL_KEY_INSERT     277
#define CABAL_KEY_HOME       278
#define CABAL_KEY_END        279
#define CABAL_KEY_PAGEUP     280
#define CABAL_KEY_PAGEDOWN   281
#define CABAL_KEY_DELETE     127

#define CABAL_KEY_LSHIFT     304
#define CABAL_KEY_RSHIFT     303
#define CABAL_KEY_LCTRL      306
#define CABAL_KEY_RCTRL      305
#define CABAL_KEY_LALT       308
#define CABAL_KEY_RALT       307

// HID keycode to Cabal keycode mapping (same as PS/2 wrapper)
static int hid_to_cabal(uint8_t code) {
    // Letters a-z (HID 0x04-0x1D)
    if (code >= 0x04 && code <= 0x1D) {
        return 'a' + (code - 0x04);
    }

    // Numbers 1-9, 0 (HID 0x1E-0x27)
    if (code >= 0x1E && code <= 0x27) {
        if (code == 0x27) return '0';
        return '1' + (code - 0x1E);
    }

    // Special keys
    switch (code) {
        case 0x28: return CABAL_KEY_RETURN;
        case 0x29: return CABAL_KEY_ESCAPE;
        case 0x2A: return CABAL_KEY_BACKSPACE;
        case 0x2B: return CABAL_KEY_TAB;
        case 0x2C: return CABAL_KEY_SPACE;

        case 0x2D: return '-';
        case 0x2E: return '=';
        case 0x2F: return '[';
        case 0x30: return ']';
        case 0x31: return '\\';
        case 0x33: return ';';
        case 0x34: return '\'';
        case 0x35: return '`';
        case 0x36: return ',';
        case 0x37: return '.';
        case 0x38: return '/';

        // Arrow keys
        case 0x4F: return CABAL_KEY_RIGHT;
        case 0x50: return CABAL_KEY_LEFT;
        case 0x51: return CABAL_KEY_DOWN;
        case 0x52: return CABAL_KEY_UP;

        // Navigation keys
        case 0x49: return CABAL_KEY_INSERT;
        case 0x4A: return CABAL_KEY_HOME;
        case 0x4B: return CABAL_KEY_PAGEUP;
        case 0x4C: return CABAL_KEY_DELETE;
        case 0x4D: return CABAL_KEY_END;
        case 0x4E: return CABAL_KEY_PAGEDOWN;

        // Modifiers (0xE0-0xE7 pseudo-keycodes from hid_app.c)
        case 0xE0: return CABAL_KEY_LCTRL;
        case 0xE1: return CABAL_KEY_LSHIFT;
        case 0xE2: return CABAL_KEY_LALT;
        case 0xE4: return CABAL_KEY_RCTRL;
        case 0xE5: return CABAL_KEY_RSHIFT;
        case 0xE6: return CABAL_KEY_RALT;
    }

    // Function keys F1-F12 (HID 0x3A-0x45)
    if (code >= 0x3A && code <= 0x45) {
        return CABAL_KEY_F1 + (code - 0x3A);
    }

    return 0;  // Unknown key
}

void usbkbd_init(void) {
    usbhid_init();
}

void usbkbd_tick(void) {
    usbhid_task();
}

int usbkbd_get_key(int *is_down, int *keycode) {
    uint8_t hid_keycode;
    int down;

    if (!usbhid_get_key_action(&hid_keycode, &down)) {
        return 0;
    }

    int cabal_code = hid_to_cabal(hid_keycode);
    if (cabal_code == 0) {
        return 0; // Unknown key, skip
    }

    *is_down = down;
    *keycode = cabal_code;
    return 1;
}

int usbkbd_connected(void) {
    return usbhid_keyboard_connected();
}

#else // !USB_HID_ENABLED - stub implementations

void usbkbd_init(void) {}
void usbkbd_tick(void) {}
int usbkbd_get_key(int *is_down, int *keycode) { (void)is_down; (void)keycode; return 0; }
int usbkbd_connected(void) { return 0; }

#endif // USB_HID_ENABLED
