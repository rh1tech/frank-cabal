/*
 * Cabal - ScummVM port for RP2350 with 8MB QSPI PSRAM
 *
 * Main entry point with hardware initialization
 */

#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/clocks.h"
#include "hardware/vreg.h"
#include "hardware/flash.h"

#include "board_config.h"
#include "psram_init.h"
#include "psram_allocator.h"
#include "HDMI.h"

// Version string
#ifndef CABAL_VERSION
#define CABAL_VERSION "0.1.0"
#endif

// Startup delay for USB serial console (5 seconds)
#define STARTUP_DELAY_MS 5000

// Forward declarations
void cabal_init(void);
int cabal_main(void);
void cabal_enable_psram_heap(void);

// Double-buffered framebuffer
// Note: Framebuffers are allocated from PSRAM at runtime since RP2350
// scratch sections are only 4KB each (too small for 76KB framebuffers)
static uint8_t *framebuffer_0 = NULL;
static uint8_t *framebuffer_1 = NULL;
static uint8_t *current_framebuffer = NULL;
static int current_buffer_index = 0;

uint8_t *cabal_get_framebuffer(void) {
    return current_framebuffer;
}

// Get the back buffer (not currently being displayed) for drawing
uint8_t *cabal_get_back_buffer(void) {
    // Return the buffer that's NOT currently being displayed
    return current_buffer_index ? framebuffer_0 : framebuffer_1;
}

void cabal_swap_buffers(void) {
    current_buffer_index = 1 - current_buffer_index;
    current_framebuffer = current_buffer_index ? framebuffer_1 : framebuffer_0;
    graphics_set_buffer(current_framebuffer);
}

// Flash timing configuration for overclocking
// Flash max safe frequency without additional wait states
#define FLASH_MAX_FREQ_MHZ 88

#if CPU_CLOCK_MHZ > 252
#include "hardware/structs/qmi.h"

static void __no_inline_not_in_flash_func(set_flash_timings)(int cpu_mhz) {
    // Calculate flash clock divisor and receive delay for stable operation
    const int clock_hz = cpu_mhz * 1000000;
    const int max_flash_freq = FLASH_MAX_FREQ_MHZ * 1000000;

    // Calculate minimum divisor to keep flash below max frequency
    int divisor = (clock_hz + max_flash_freq - (max_flash_freq >> 4) - 1) / max_flash_freq;
    if (divisor == 1 && clock_hz >= 166000000) {
        divisor = 2;
    }

    // Calculate receive delay - needs extra cycle at high speeds
    int rxdelay = divisor;
    if (clock_hz / divisor > 100000000 && clock_hz >= 166000000) {
        rxdelay += 1;
    }

    // Configure QMI flash timing (M0 = flash on CS0)
    qmi_hw->m[0].timing = 0x60007000 |
                        rxdelay << QMI_M0_TIMING_RXDELAY_LSB |
                        divisor << QMI_M0_TIMING_CLKDIV_LSB;
}
#endif

int main(void) {
    // Voltage and clock setup MUST happen before stdio_init_all()
    // USB needs to initialize at the target clock speed

    // Always set voltage explicitly for stable HDMI output
    // Match quakegeneric: 1.6V with 100ms stabilization delay
    vreg_disable_voltage_limit();
    vreg_set_voltage(VREG_VOLTAGE_1_60);
#if CPU_CLOCK_MHZ > 252
    set_flash_timings(CPU_CLOCK_MHZ);
#endif
    sleep_ms(100);  // Let voltage stabilize (100ms like quakegeneric)

    // Set system clock before USB init
    set_sys_clock_khz(CPU_CLOCK_MHZ * 1000, false);

    // Initialize stdio for USB serial console (at target clock speed)
    stdio_init_all();

    // Wait for USB serial connection with countdown
    printf("\n\n");
    printf("================================\n");
    printf("  Cabal v%s\n", CABAL_VERSION);
    printf("  ScummVM for RP2350\n");
    printf("================================\n");
    printf("\n");

#ifdef BOARD_M1
    printf("Board: M1\n");
#else
    printf("Board: M2\n");
#endif
    printf("CPU: %d MHz\n", CPU_CLOCK_MHZ);
    printf("PSRAM: %d MHz max\n", PSRAM_MAX_FREQ_MHZ);
    printf("System clock: %lu Hz\n", clock_get_hz(clk_sys));
    printf("\n");

    // Startup delay for USB serial console
    printf("Starting in ");
    for (int i = 5; i > 0; i--) {
        printf("%d...", i);
        fflush(stdout);
        sleep_ms(1000);
    }
    printf("Go!\n\n");

    // Initialize PSRAM
    printf("Initializing PSRAM...\n");
    uint psram_pin = get_psram_pin();
    printf("  PSRAM CS pin: GPIO%d\n", psram_pin);
    psram_init(psram_pin);
    psram_set_sram_mode(0); // Use PSRAM for allocations
    cabal_enable_psram_heap(); // Enable PSRAM for malloc/new heap
    printf("  PSRAM: 8MB at 0x11000000\n");

    // Initialize framebuffers from PSRAM
    printf("Initializing framebuffers...\n");
    framebuffer_0 = (uint8_t *)psram_malloc(CABAL_FRAMEBUFFER_SIZE);
    framebuffer_1 = (uint8_t *)psram_malloc(CABAL_FRAMEBUFFER_SIZE);
    if (!framebuffer_0 || !framebuffer_1) {
        printf("ERROR: Failed to allocate framebuffers!\n");
        while (1) tight_loop_contents();
    }
    memset(framebuffer_0, 0, CABAL_FRAMEBUFFER_SIZE);
    memset(framebuffer_1, 0, CABAL_FRAMEBUFFER_SIZE);
    current_framebuffer = framebuffer_0;
    current_buffer_index = 0;
    printf("  FB0: %p (%d bytes)\n", framebuffer_0, CABAL_FRAMEBUFFER_SIZE);
    printf("  FB1: %p (%d bytes)\n", framebuffer_1, CABAL_FRAMEBUFFER_SIZE);

    // Initialize HDMI
    printf("Initializing HDMI...\n");
    graphics_set_res(CABAL_SCREEN_WIDTH, CABAL_HDMI_HEIGHT);
    graphics_set_buffer(current_framebuffer);
    graphics_init(g_out_HDMI);
    printf("  Resolution: %dx%d\n", CABAL_SCREEN_WIDTH, CABAL_HDMI_HEIGHT);

    // Set initial palette to grayscale
    for (int i = 0; i < 256; i++) {
        uint32_t gray = (i << 16) | (i << 8) | i;
        graphics_set_palette(i, gray);
    }

    printf("\nCabal initialized successfully!\n");
    printf("Game directory: /cabal/\n\n");

    // Initialize Cabal engine
    cabal_init();

    // Run main loop
    return cabal_main();
}

// cabal_init() and cabal_main() are implemented in cabal_main.cpp
