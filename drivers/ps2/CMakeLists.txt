# PS/2 Unified Driver
# Uses PIO for both keyboard and mouse with separate state machines.
# Keyboard wrapper (HID translation) is bundled as a static backing library
# so the INTERFACE target can pull it in for targets that link ps2_driver.

add_library(ps2_driver INTERFACE)

pico_generate_pio_header(ps2_driver ${CMAKE_CURRENT_LIST_DIR}/ps2.pio)

target_sources(ps2_driver INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/ps2.c
)

target_include_directories(ps2_driver INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/kbd
)

target_link_libraries(ps2_driver INTERFACE
    pico_stdlib
    hardware_pio
    hardware_gpio
    hardware_clocks
)

# Keyboard wrapper (Ps2Kbd_Mrmltr + HID translation). Kept as a separate
# static library so the PIO set-2 table selection stays build-time
# configurable per board variant.
add_library(ps2kbd STATIC
    ${CMAKE_CURRENT_LIST_DIR}/kbd/ps2kbd_mrmltr.cpp
    ${CMAKE_CURRENT_LIST_DIR}/kbd/ps2kbd_wrapper.cpp
)

target_link_libraries(ps2kbd PRIVATE hardware_pio hardware_clocks)

if(BOARD_VARIANT STREQUAL "M2")
    target_compile_definitions(ps2kbd PRIVATE BOARD_M2 KBD_CLOCK_PIN=2)
else()
    target_compile_definitions(ps2kbd PRIVATE BOARD_M1 KBD_CLOCK_PIN=0)
endif()

target_include_directories(ps2kbd PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/kbd
)

target_include_directories(ps2kbd PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
)

pico_generate_pio_header(ps2kbd
    ${CMAKE_CURRENT_LIST_DIR}/kbd/ps2kbd_mrmltr.pio
)

pico_generate_pio_header(ps2kbd
    ${CMAKE_CURRENT_LIST_DIR}/kbd/ps2kbd_mrmltr2.pio
)
