# Cabal I2S Audio Driver

# Generate PIO header from assembly
pico_generate_pio_header(frank-cabal ${CMAKE_CURRENT_LIST_DIR}/audio_i2s.pio)

# Add audio source to main executable (no separate library needed)
target_sources(frank-cabal PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/audio.c
)

target_include_directories(frank-cabal PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(frank-cabal
    hardware_pio
    hardware_dma
)
