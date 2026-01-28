# Cabal I2S Audio Driver

# Generate PIO header from assembly
pico_generate_pio_header(cabal ${CMAKE_CURRENT_LIST_DIR}/audio_i2s.pio)

# Add audio source to main executable (no separate library needed)
target_sources(cabal PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/audio.c
)

target_include_directories(cabal PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(cabal
    hardware_pio
    hardware_dma
)
