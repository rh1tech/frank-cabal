cmake_minimum_required(VERSION 3.13)

# Set board to Pico 2 (RP2350)
set(PICO_BOARD pico2 CACHE STRING "Pico board type")

# Board configuration variant (M1, M2) - M1 is default
set(BOARD_VARIANT "M1" CACHE STRING "Board variant: M1 or M2")

# USB HID disabled by default (use USB serial console for debug)
option(USB_HID_ENABLED "Enable USB HID for keyboard/mouse" OFF)

# CPU overclock configuration (252, 378, 504 MHz)
# 252 = no overclock (default), 378 = medium, 504 = high
set(CPU_SPEED "252" CACHE STRING "CPU clock in MHz: 252, 378, 504")

# PSRAM speed configuration (84, 100, 133, 166 MHz target)
set(PSRAM_SPEED "133" CACHE STRING "PSRAM max frequency in MHz: 84, 100, 133, 166")

# CPU voltage selection based on speed
if(CPU_SPEED GREATER_EQUAL 504)
    set(CPU_VOLTAGE "VREG_VOLTAGE_1_65")
elseif(CPU_SPEED GREATER_EQUAL 378)
    set(CPU_VOLTAGE "VREG_VOLTAGE_1_60")
else()
    set(CPU_VOLTAGE "VREG_VOLTAGE_1_50")
endif()

message(STATUS "Cabal Build Configuration:")
message(STATUS "  Board: ${BOARD_VARIANT}")
message(STATUS "  CPU: ${CPU_SPEED} MHz")
message(STATUS "  PSRAM: ${PSRAM_SPEED} MHz")
message(STATUS "  Voltage: ${CPU_VOLTAGE}")
message(STATUS "  USB HID: ${USB_HID_ENABLED}")

# Disable SDK's new/delete overrides - we provide our own for PSRAM
set(PICO_CXX_DISABLE_ALLOCATION_OVERRIDES 1)
add_compile_definitions(PICO_CXX_DISABLE_ALLOCATION_OVERRIDES=1)

include(pico_sdk_import.cmake)

# Import pico-extras for audio_i2s library
set(PICO_EXTRAS_PATH ${CMAKE_CURRENT_LIST_DIR}/pico-extras CACHE PATH "Path to pico-extras")
if(EXISTS ${PICO_EXTRAS_PATH}/external/pico_extras_import.cmake)
    include(${PICO_EXTRAS_PATH}/external/pico_extras_import.cmake)
endif()

project(cabal C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Version
set(CABAL_VERSION "0.1.0")

pico_sdk_init()

# Initialize pico-extras if available
if(COMMAND pico_extras_init)
    pico_extras_init()
endif()

# Add FatFS library
add_subdirectory(src/fatfs)

# Add driver subdirectories
add_subdirectory(drivers/sdcard)
add_subdirectory(drivers/ps2kbd)
add_subdirectory(drivers/ps2mouse)

# Create a library for core drivers
add_library(drivers
    drivers/HDMI.c
    drivers/psram_init.c
    drivers/psram_allocator.c
)
target_include_directories(drivers PUBLIC
    drivers
    src
)
target_compile_definitions(drivers PRIVATE
    BOARD_${BOARD_VARIANT}
    PSRAM_MAX_FREQ_MHZ=${PSRAM_SPEED}
    CABAL_PSRAM_SIZE_BYTES=8388608
)
target_link_libraries(drivers pico_stdlib hardware_dma hardware_pio hardware_spi)

# ScummVM Common library
set(COMMON_SOURCES
    src/common/archive.cpp
    src/common/config-manager.cpp
    src/common/debug.cpp
    src/common/error.cpp
    src/common/EventDispatcher.cpp
    src/common/EventMapper.cpp
    src/common/fs.cpp
    src/common/file.cpp
    src/common/hashmap.cpp
    src/common/language.cpp
    src/common/memorypool.cpp
    src/common/mutex.cpp
    src/common/platform.cpp
    src/common/random.cpp
    src/common/rational.cpp
    src/common/rendermode.cpp
    src/common/str.cpp
    src/common/stream.cpp
    src/common/system.cpp
    src/common/textconsole.cpp
    src/common/tokenizer.cpp
    src/common/util.cpp
    src/common/md5.cpp
    src/common/gui_options.cpp
    src/common/macresman.cpp
    src/common/timestamp.cpp
)

# ScummVM Graphics library
set(GRAPHICS_SOURCES
    src/graphics/pixelformat.cpp
    src/graphics/surface.cpp
    src/graphics/primitives.cpp
    src/graphics/cursorman.cpp
    src/graphics/thumbnail.cpp
    src/graphics/scaler/thumbnail_stub.cpp
)

# ScummVM Audio library (minimal for AGI)
set(AUDIO_SOURCES
    src/audio/audiostream.cpp
    src/audio/mixer.cpp
    src/audio/rate.cpp
    src/audio/mididrv.cpp
    src/audio/midiparser.cpp
    src/audio/midiparser_smf.cpp
    src/audio/midiplayer.cpp
    src/audio/mpu401.cpp
    src/audio/musicplugin.cpp
    src/audio/null.cpp
    src/audio/softsynth/emu_pcspk.cpp
)

# ScummVM Base library
set(BASE_SOURCES
    src/base/plugins.cpp
    src/base/version.cpp
)

# ScummVM Engine infrastructure
set(ENGINE_SOURCES
    src/engines/engine.cpp
    src/engines/advancedDetector.cpp
    src/engines/game.cpp
    src/engines/savestate.cpp
)

# AGI Engine sources
set(AGI_ENGINE_SOURCES
    src/engines/agi/agi.cpp
    src/engines/agi/checks.cpp
    src/engines/agi/console.cpp
    src/engines/agi/cycle.cpp
    src/engines/agi/detection.cpp
    src/engines/agi/global.cpp
    src/engines/agi/graphics.cpp
    src/engines/agi/id.cpp
    src/engines/agi/inv.cpp
    src/engines/agi/keyboard.cpp
    src/engines/agi/loader_v1.cpp
    src/engines/agi/loader_v2.cpp
    src/engines/agi/loader_v3.cpp
    src/engines/agi/logic.cpp
    src/engines/agi/lzw.cpp
    src/engines/agi/menu.cpp
    src/engines/agi/motion.cpp
    src/engines/agi/objects.cpp
    src/engines/agi/op_cmd.cpp
    src/engines/agi/op_dbg.cpp
    src/engines/agi/op_test.cpp
    src/engines/agi/opcodes.cpp
    src/engines/agi/picture.cpp
    src/engines/agi/preagi.cpp
    src/engines/agi/preagi_mickey.cpp
    src/engines/agi/preagi_troll.cpp
    src/engines/agi/preagi_winnie.cpp
    src/engines/agi/saveload.cpp
    src/engines/agi/sound.cpp
    src/engines/agi/sound_2gs.cpp
    src/engines/agi/sound_coco3.cpp
    src/engines/agi/sound_midi.cpp
    src/engines/agi/sound_pcjr.cpp
    src/engines/agi/sound_sarien.cpp
    src/engines/agi/sprite.cpp
    src/engines/agi/text.cpp
    src/engines/agi/view.cpp
    src/engines/agi/wagparser.cpp
    src/engines/agi/words.cpp
)

# ScummVM Backends
set(BACKEND_SOURCES
    src/backends/base-backend.cpp
    src/backends/fs/abstract-fs.cpp
    src/backends/fs/rp2350/rp2350-fs.cpp
    src/backends/fs/rp2350/rp2350-fs-factory.cpp
    src/backends/events/default/default-events.cpp
    src/backends/timer/default/default-timer.cpp
    src/backends/platform/rp2350/rp2350-minimal.cpp
    src/backends/platform/rp2350/rp2350-system.cpp
)

# Main executable
add_executable(cabal
    src/main.c
    src/cabal_main.cpp
    src/cabal_new.cpp
    src/cabal_fs.c
    src/cabal_sbrk.c
    ${COMMON_SOURCES}
    ${GRAPHICS_SOURCES}
    ${AUDIO_SOURCES}
    ${BASE_SOURCES}
    ${ENGINE_SOURCES}
    ${AGI_ENGINE_SOURCES}
    ${BACKEND_SOURCES}
)

target_include_directories(cabal PRIVATE
    src
    src/engines
    src/fatfs
    drivers
    drivers/sdcard
    drivers/ps2kbd
    drivers/ps2mouse
)

# ScummVM include paths - src is the root for all ScummVM headers
# Headers use paths like "common/system.h", "backends/base-backend.h", etc.

target_compile_definitions(cabal PRIVATE
    BOARD_${BOARD_VARIANT}
    CPU_CLOCK_MHZ=${CPU_SPEED}
    CPU_VOLTAGE=${CPU_VOLTAGE}
    PSRAM_MAX_FREQ_MHZ=${PSRAM_SPEED}
    CABAL_PSRAM_SIZE_BYTES=8388608
    CABAL_VERSION="${CABAL_VERSION}"

    # Screen configuration
    CABAL_SCREEN_WIDTH=320
    CABAL_SCREEN_HEIGHT=200
    CABAL_HDMI_HEIGHT=240

    # I2S Audio configuration
    PICO_AUDIO_I2S_PIO=1
    PICO_AUDIO_I2S_STATE_MACHINE=2
    PICO_AUDIO_I2S_DMA_IRQ=1

    # Game engines to enable
    ENABLE_AGI=1      # Sierra AGI games
    ENABLE_GOB=1      # Gobliins games
    ENABLE_SCI=1      # Sierra SCI games (early versions)

    # Memory optimization flags
    CABAL_LOW_MEMORY=1

    # Platform identification
    PICO_ON_DEVICE=1
    __RP2350__=1

    # Use standard stdint.h types
    CABAL_HAVE_STDINT_H=1

    # ScummVM feature flags for embedded
    DISABLE_GUI=1
    DISABLE_DEFAULT_AUDIOCD_MANAGER=1

    # Use minimal audio for embedded (only PC speaker/PCjr)
    CABAL_MINIMAL_AUDIO=1
)

# Set peripheral pins based on board variant
if(BOARD_VARIANT STREQUAL "M1")
    target_compile_definitions(cabal PRIVATE
        # I2S Audio
        PICO_AUDIO_I2S_DATA_PIN=26
        PICO_AUDIO_I2S_CLOCK_PIN_BASE=27
        # SD Card
        SDCARD_PIN_SPI0_CS=5
        SDCARD_PIN_SPI0_SCK=2
        SDCARD_PIN_SPI0_MOSI=3
        SDCARD_PIN_SPI0_MISO=4
    )
elseif(BOARD_VARIANT STREQUAL "M2")
    target_compile_definitions(cabal PRIVATE
        # I2S Audio
        PICO_AUDIO_I2S_DATA_PIN=9
        PICO_AUDIO_I2S_CLOCK_PIN_BASE=10
        # SD Card
        SDCARD_PIN_SPI0_CS=5
        SDCARD_PIN_SPI0_SCK=6
        SDCARD_PIN_SPI0_MOSI=7
        SDCARD_PIN_SPI0_MISO=4
    )
endif()


target_link_libraries(cabal
    pico_stdlib
    pico_multicore
    hardware_spi
    hardware_dma
    hardware_pio
    hardware_pwm
    hardware_irq
    hardware_flash
    fatfs
    sdcard
    ps2kbd
    ps2mouse
    drivers
)

# Add pico_audio_i2s if available
if(TARGET pico_audio_i2s)
    target_link_libraries(cabal pico_audio_i2s)
    target_compile_definitions(cabal PRIVATE USE_I2S_AUDIO=1)
endif()

# USB stdio configuration
# USB HID disabled by default - use USB serial console for debug
if(USB_HID_ENABLED)
    message(STATUS "USB HID enabled: Using UART for stdio")
    pico_enable_stdio_usb(cabal 0)
    pico_enable_stdio_uart(cabal 1)
else()
    message(STATUS "USB Serial Console enabled for debug")
    pico_enable_stdio_usb(cabal 1)
    pico_enable_stdio_uart(cabal 0)
endif()

pico_add_extra_outputs(cabal)

# Print memory usage estimate
message(STATUS "")
message(STATUS "Memory Budget:")
message(STATUS "  SRAM: ~520KB total")
message(STATUS "    - Double framebuffer: ~154KB (320x240x2)")
message(STATUS "    - Stack/DMA/Code: ~366KB")
message(STATUS "  PSRAM: 8MB total")
message(STATUS "    - Scratch buffers: 512KB")
message(STATUS "    - Game zone: 3.5MB")
message(STATUS "    - Temp/Audio: 4MB")
message(STATUS "")
