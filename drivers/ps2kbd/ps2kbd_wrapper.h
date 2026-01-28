// PS/2 Keyboard Wrapper for Cabal
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef PS2KBD_WRAPPER_H
#define PS2KBD_WRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

// Initialize PS/2 keyboard
void ps2kbd_init(void);

// Poll keyboard for new events (call from main loop)
void ps2kbd_tick(void);

// Get next key event
// Returns 1 if event available, 0 if queue empty
// pressed: 1 = key down, 0 = key up
// keycode: Cabal keycode (ASCII for printable, special codes for others)
int ps2kbd_get_key(int* pressed, int* keycode);

// Get next key event with modifier state
// modifiers: bit 0 = shift, bit 1 = ctrl, bit 2 = alt
int ps2kbd_get_key_ext(int* pressed, int* keycode, int* modifiers);

// Get current modifier state
int ps2kbd_get_modifiers(void);

#ifdef __cplusplus
}
#endif

#endif // PS2KBD_WRAPPER_H
