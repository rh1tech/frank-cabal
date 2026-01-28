// PS/2 Mouse Wrapper for Cabal
// Interfaces ps2mouse driver with Cabal's event system
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ps2mouse_wrapper.h"
#include "ps2mouse.h"
#include <stdint.h>

// Mouse state accumulator
static int16_t accumulated_dx = 0;
static int16_t accumulated_dy = 0;
static int8_t accumulated_wheel = 0;
static uint8_t current_buttons = 0;
static uint8_t prev_buttons = 0;

void ps2mouse_wrapper_init(void) {
    ps2mouse_init();
    accumulated_dx = 0;
    accumulated_dy = 0;
    accumulated_wheel = 0;
    current_buttons = 0;
    prev_buttons = 0;
}

void ps2mouse_wrapper_tick(void) {
    int16_t dx, dy;
    int8_t wheel;
    uint8_t buttons;

    // Get mouse state and accumulate movement
    if (ps2mouse_get_state(&dx, &dy, &wheel, &buttons)) {
        accumulated_dx += dx;
        accumulated_dy += dy;
        accumulated_wheel += wheel;
        current_buttons = buttons & 0x07;
    }
}

int ps2mouse_get_motion(int16_t* dx, int16_t* dy) {
    *dx = accumulated_dx;
    *dy = accumulated_dy;

    int has_motion = (accumulated_dx != 0 || accumulated_dy != 0);

    // Clear accumulated motion
    accumulated_dx = 0;
    accumulated_dy = 0;

    return has_motion;
}

int ps2mouse_get_motion_and_buttons(int16_t* dx, int16_t* dy, uint8_t* buttons) {
    *dx = accumulated_dx;
    *dy = accumulated_dy;
    *buttons = current_buttons;

    int has_motion = (accumulated_dx != 0 || accumulated_dy != 0);

    // Clear accumulated motion
    accumulated_dx = 0;
    accumulated_dy = 0;

    return has_motion;
}

int ps2mouse_get_wheel(void) {
    int8_t wheel = accumulated_wheel;
    accumulated_wheel = 0;
    return wheel;
}

uint8_t ps2mouse_get_buttons(void) {
    return current_buttons;
}

int ps2mouse_button_pressed(int button) {
    // button: 0 = left, 1 = right, 2 = middle
    uint8_t mask = 1 << button;
    return (current_buttons & mask) && !(prev_buttons & mask);
}

int ps2mouse_button_released(int button) {
    uint8_t mask = 1 << button;
    return !(current_buttons & mask) && (prev_buttons & mask);
}

void ps2mouse_update_prev_buttons(void) {
    prev_buttons = current_buttons;
}
