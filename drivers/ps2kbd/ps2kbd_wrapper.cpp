// PS/2 Keyboard Wrapper for Cabal
// Interfaces ps2kbd driver with Cabal's event system
// SPDX-License-Identifier: GPL-2.0-or-later

#include "board_config.h"
#include "ps2kbd_wrapper.h"
#include "ps2kbd_mrmltr.h"
#include <queue>

// Generic key codes for Cabal (matching Common::KeyCode where possible)
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

struct KeyEvent {
    int pressed;
    int keycode;
    int modifier;
};

static std::queue<KeyEvent> event_queue;
static int current_modifiers = 0;

// HID to Cabal keycode mapping
static int hid_to_cabal(uint8_t code) {
    // Letters a-z
    if (code >= 0x04 && code <= 0x1D) return 'a' + (code - 0x04);

    // Numbers 1-9, 0
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
    }

    // Function keys F1-F12
    if (code >= 0x3A && code <= 0x45) {
        return CABAL_KEY_F1 + (code - 0x3A);
    }

    return 0;
}

static void key_handler(hid_keyboard_report_t *curr, hid_keyboard_report_t *prev) {
    // Track modifiers
    current_modifiers = 0;
    if (curr->modifier & (KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT))
        current_modifiers |= 1;  // Shift
    if (curr->modifier & (KEYBOARD_MODIFIER_LEFTCTRL | KEYBOARD_MODIFIER_RIGHTCTRL))
        current_modifiers |= 2;  // Ctrl
    if (curr->modifier & (KEYBOARD_MODIFIER_LEFTALT | KEYBOARD_MODIFIER_RIGHTALT))
        current_modifiers |= 4;  // Alt

    // Check modifier changes
    uint8_t changed_mods = curr->modifier ^ prev->modifier;
    if (changed_mods) {
        if (changed_mods & KEYBOARD_MODIFIER_LEFTSHIFT) {
            int pressed = (curr->modifier & KEYBOARD_MODIFIER_LEFTSHIFT) != 0;
            event_queue.push({pressed, CABAL_KEY_LSHIFT, current_modifiers});
        }
        if (changed_mods & KEYBOARD_MODIFIER_RIGHTSHIFT) {
            int pressed = (curr->modifier & KEYBOARD_MODIFIER_RIGHTSHIFT) != 0;
            event_queue.push({pressed, CABAL_KEY_RSHIFT, current_modifiers});
        }
        if (changed_mods & KEYBOARD_MODIFIER_LEFTCTRL) {
            int pressed = (curr->modifier & KEYBOARD_MODIFIER_LEFTCTRL) != 0;
            event_queue.push({pressed, CABAL_KEY_LCTRL, current_modifiers});
        }
        if (changed_mods & KEYBOARD_MODIFIER_RIGHTCTRL) {
            int pressed = (curr->modifier & KEYBOARD_MODIFIER_RIGHTCTRL) != 0;
            event_queue.push({pressed, CABAL_KEY_RCTRL, current_modifiers});
        }
        if (changed_mods & KEYBOARD_MODIFIER_LEFTALT) {
            int pressed = (curr->modifier & KEYBOARD_MODIFIER_LEFTALT) != 0;
            event_queue.push({pressed, CABAL_KEY_LALT, current_modifiers});
        }
        if (changed_mods & KEYBOARD_MODIFIER_RIGHTALT) {
            int pressed = (curr->modifier & KEYBOARD_MODIFIER_RIGHTALT) != 0;
            event_queue.push({pressed, CABAL_KEY_RALT, current_modifiers});
        }
    }

    // Check for newly pressed keys
    for (int i = 0; i < 6; i++) {
        if (curr->keycode[i] != 0) {
            bool found = false;
            for (int j = 0; j < 6; j++) {
                if (prev->keycode[j] == curr->keycode[i]) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                int k = hid_to_cabal(curr->keycode[i]);
                if (k) event_queue.push({1, k, current_modifiers});
            }
        }
    }

    // Check for released keys
    for (int i = 0; i < 6; i++) {
        if (prev->keycode[i] != 0) {
            bool found = false;
            for (int j = 0; j < 6; j++) {
                if (curr->keycode[j] == prev->keycode[i]) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                int k = hid_to_cabal(prev->keycode[i]);
                if (k) event_queue.push({0, k, current_modifiers});
            }
        }
    }
}

static Ps2Kbd_Mrmltr* kbd = nullptr;

extern "C" void ps2kbd_init(void) {
    kbd = new Ps2Kbd_Mrmltr(pio0, PS2_PIN_CLK, key_handler);
    kbd->init_gpio();
}

extern "C" void ps2kbd_tick(void) {
    if (kbd) kbd->tick();
}

extern "C" int ps2kbd_get_key(int* pressed, int* keycode) {
    if (event_queue.empty()) return 0;
    KeyEvent e = event_queue.front();
    event_queue.pop();
    *pressed = e.pressed;
    *keycode = e.keycode;
    return 1;
}

extern "C" int ps2kbd_get_key_ext(int* pressed, int* keycode, int* modifiers) {
    if (event_queue.empty()) return 0;
    KeyEvent e = event_queue.front();
    event_queue.pop();
    *pressed = e.pressed;
    *keycode = e.keycode;
    *modifiers = e.modifier;
    return 1;
}

extern "C" int ps2kbd_get_modifiers(void) {
    return current_modifiers;
}
