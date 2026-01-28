// PS/2 Mouse Wrapper for Cabal
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef PS2MOUSE_WRAPPER_H
#define PS2MOUSE_WRAPPER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize PS/2 mouse
void ps2mouse_wrapper_init(void);

// Poll mouse for new events (call from main loop)
void ps2mouse_wrapper_tick(void);

// Get accumulated mouse motion since last call
// Returns 1 if there was motion, 0 if stationary
int ps2mouse_get_motion(int16_t* dx, int16_t* dy);

// Get motion and button state
int ps2mouse_get_motion_and_buttons(int16_t* dx, int16_t* dy, uint8_t* buttons);

// Get accumulated wheel movement
int ps2mouse_get_wheel(void);

// Get current button state
// Bit 0 = left, Bit 1 = right, Bit 2 = middle
uint8_t ps2mouse_get_buttons(void);

// Check if button was just pressed (edge detection)
int ps2mouse_button_pressed(int button);

// Check if button was just released (edge detection)
int ps2mouse_button_released(int button);

// Update previous button state (call at end of frame)
void ps2mouse_update_prev_buttons(void);

#ifdef __cplusplus
}
#endif

#endif // PS2MOUSE_WRAPPER_H
