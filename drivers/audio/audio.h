/**
 * Cabal - I2S Audio Driver for RP2350
 *
 * DMA-based I2S audio output using PIO for ScummVM audio mixer.
 */

#ifndef CABAL_AUDIO_H
#define CABAL_AUDIO_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <hardware/pio.h>
#include <hardware/clocks.h>
#include <hardware/dma.h>

#ifdef __cplusplus
extern "C" {
#endif

// Audio sample rate - standard CD quality
#define AUDIO_SAMPLE_RATE 44100

// Audio buffer size - enough samples for ~16ms at 44.1kHz
// 44100 / 60 = ~735 samples per frame at 60Hz
// Use 1024 for headroom
#define AUDIO_BUFFER_SAMPLES 1024

// I2S configuration structure
typedef struct {
    uint32_t sample_freq;
    uint16_t channel_count;
    uint8_t  data_pin;
    uint8_t  clock_pin_base;
    PIO      pio;
    uint8_t  sm;
    uint8_t  dma_channel;
    uint16_t dma_trans_count;
    uint16_t *dma_buf;
    int8_t   volume;  // >0 = attenuation (right shift), <0 = amplification (left shift)
} i2s_config_t;

// Get default I2S configuration
i2s_config_t i2s_get_default_config(void);

// Initialize I2S with the given configuration
void i2s_init(i2s_config_t *config);

// Write samples to I2S via DMA (non-blocking after first call)
// samples: pointer to stereo samples (interleaved L/R as 32-bit words)
void i2s_dma_write(i2s_config_t *config, const int16_t *samples);

// Write specific number of samples to I2S via DMA
void i2s_dma_write_count(i2s_config_t *config, const int16_t *samples, uint32_t sample_count);

// Adjust volume (0 = loudest if attenuation only, or specific shifts)
void i2s_volume(i2s_config_t *config, int8_t volume);

//=============================================================================
// High-level audio API for Cabal
//=============================================================================

// Initialize audio system
bool cabal_audio_init(void);

// Shutdown audio system
void cabal_audio_shutdown(void);

// Check if audio is initialized
bool cabal_audio_is_initialized(void);

// Process audio for one frame - call from main loop
// This invokes the ScummVM mixer and sends samples to I2S
void cabal_audio_process_frame(void);

// Set master volume (0-255)
void cabal_audio_set_volume(int volume);

// Get current master volume
int cabal_audio_get_volume(void);

// Enable/disable audio
void cabal_audio_set_enabled(bool enabled);

// Check if audio is enabled
bool cabal_audio_is_enabled(void);

// Get the I2S config for direct access
i2s_config_t* cabal_audio_get_i2s_config(void);

// Check if audio system needs more samples (buffer free)
bool cabal_audio_needs_samples(void);

#ifdef __cplusplus
}
#endif

#endif // CABAL_AUDIO_H
