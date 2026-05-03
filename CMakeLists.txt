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

project(frank-cabal C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Version
set(CABAL_VERSION "0.1.0")

# Override pico_malloc BEFORE SDK init: We provide our own __wrap_malloc/calloc/realloc/free
# that route to PSRAM. The SDK's pico_malloc is guarded by "if (NOT TARGET pico_malloc)".
add_library(pico_malloc INTERFACE)
add_library(pico_malloc_headers INTERFACE)
target_include_directories(pico_malloc_headers SYSTEM INTERFACE
    ${PICO_SDK_PATH}/src/rp2_common/pico_malloc/include
)
target_link_options(pico_malloc INTERFACE
    -Wl,--wrap=malloc
    -Wl,--wrap=calloc
    -Wl,--wrap=realloc
    -Wl,--wrap=free
)

pico_sdk_init()

# Initialize pico-extras if available
if(COMMAND pico_extras_init)
    pico_extras_init()
endif()

# Add FatFS library
add_subdirectory(src/fatfs)

# Add driver subdirectories
add_subdirectory(drivers/sdcard)
add_subdirectory(drivers/ps2)

# USB HID drivers (optional - when enabled, USB serial is disabled)
if(USB_HID_ENABLED)
    add_subdirectory(drivers/usbhid)
endif()

# Create a library for core drivers
add_library(drivers
    drivers/HDMI.c
    drivers/psram_init.c
    drivers/psram_allocator.c
    drivers/dlmalloc.c
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
# dlmalloc compiled in mspace-only mode — real malloc/free inside PSRAM,
# isolated from newlib's SRAM heap.
set_source_files_properties(drivers/dlmalloc.c PROPERTIES COMPILE_DEFINITIONS
    "ONLY_MSPACES=1;HAVE_MMAP=0;HAVE_MORECORE=0;NO_MALLOC_STATS=1;USE_LOCKS=0;ABORT_ON_ASSERT_FAILURE=0;MALLOC_FAILURE_ACTION=;LACKS_SYS_MMAN_H=1;LACKS_UNISTD_H=1;LACKS_SYS_PARAM_H=1")
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
    src/common/iff_container.cpp
    src/common/ini-file.cpp
    src/common/winexe.cpp
    src/common/winexe_pe.cpp
    src/common/localization.cpp
    src/common/dcl.cpp
)

# ScummVM Graphics library
set(GRAPHICS_SOURCES
    src/graphics/pixelformat.cpp
    src/graphics/surface.cpp
    src/graphics/primitives.cpp
    src/graphics/cursorman.cpp
    src/graphics/thumbnail.cpp
    # thumbnail_stub.cpp replaced by thumbnail_intern.cpp (for Kyrandia save thumbnails)
    src/graphics/yuv_to_rgb.cpp
    src/graphics/wincursor.cpp
    src/graphics/maccursor.cpp
)

# GUI stubs for embedded
set(GUI_SOURCES
    src/gui/gui-manager.cpp
)

# Video decoder library (for GOB engine)
set(VIDEO_SOURCES
    src/video/video_decoder.cpp
    src/video/coktel_decoder.cpp
)

# Image decoder library (for GOB engine)
set(IMAGE_SOURCES
    src/image/iff.cpp
    src/image/codecs/codec.cpp
    src/image/codecs/indeo3.cpp
)

# ScummVM Audio library
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
    # Additional audio for GOB engine
    src/audio/decoders/wave.cpp
    src/audio/decoders/adpcm.cpp
    src/audio/decoders/pcm.cpp
    src/audio/mods/infogrames.cpp
    src/audio/mods/module.cpp
    src/audio/mods/paula.cpp
    src/audio/mods/protracker.cpp
    src/audio/softsynth/opl/dbopl.cpp
    src/audio/softsynth/opl/dosbox.cpp
    src/audio/softsynth/opl/mame.cpp
    src/audio/audiodev/audiodev.cpp
    src/audio/audiodev/opl.cpp
    src/audio/audiodev/pcspk.cpp
    src/audio/adlib.cpp
    src/audio/softsynth/cms.cpp
    src/audio/softsynth/sid.cpp
    src/audio/softsynth/wave6581.cpp
    src/audio/mods/tfmx.cpp
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
    src/engines/obsolete.cpp
    src/engines/dialogs.cpp
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

# Gob (Gobliins) Engine sources
set(GOB_ENGINE_SOURCES
    src/engines/gob/anifile.cpp
    src/engines/gob/aniobject.cpp
    src/engines/gob/backbuffer.cpp
    src/engines/gob/cheater.cpp
    src/engines/gob/cheater_geisha.cpp
    src/engines/gob/cmpfile.cpp
    src/engines/gob/console.cpp
    src/engines/gob/databases.cpp
    src/engines/gob/dataio.cpp
    src/engines/gob/dbase.cpp
    src/engines/gob/decfile.cpp
    src/engines/gob/demos/batplayer.cpp
    src/engines/gob/demos/demoplayer.cpp
    src/engines/gob/demos/scnplayer.cpp
    src/engines/gob/detection/detection.cpp
    src/engines/gob/draw.cpp
    src/engines/gob/draw_bargon.cpp
    src/engines/gob/draw_fascin.cpp
    src/engines/gob/draw_playtoons.cpp
    src/engines/gob/draw_v1.cpp
    src/engines/gob/draw_v2.cpp
    src/engines/gob/expression.cpp
    src/engines/gob/game.cpp
    src/engines/gob/global.cpp
    src/engines/gob/gob.cpp
    src/engines/gob/goblin.cpp
    src/engines/gob/goblin_v1.cpp
    src/engines/gob/goblin_v2.cpp
    src/engines/gob/goblin_v3.cpp
    src/engines/gob/goblin_v4.cpp
    src/engines/gob/hotspots.cpp
    src/engines/gob/iniconfig.cpp
    src/engines/gob/init.cpp
    src/engines/gob/init_fascin.cpp
    src/engines/gob/init_geisha.cpp
    src/engines/gob/init_v1.cpp
    src/engines/gob/init_v2.cpp
    src/engines/gob/init_v3.cpp
    src/engines/gob/init_v4.cpp
    src/engines/gob/init_v6.cpp
    src/engines/gob/init_v7.cpp
    src/engines/gob/inter.cpp
    src/engines/gob/inter_bargon.cpp
    src/engines/gob/inter_fascin.cpp
    src/engines/gob/inter_geisha.cpp
    src/engines/gob/inter_inca2.cpp
    src/engines/gob/inter_littlered.cpp
    src/engines/gob/inter_playtoons.cpp
    src/engines/gob/inter_v1.cpp
    src/engines/gob/inter_v2.cpp
    src/engines/gob/inter_v3.cpp
    src/engines/gob/inter_v4.cpp
    src/engines/gob/inter_v5.cpp
    src/engines/gob/inter_v6.cpp
    src/engines/gob/inter_v7.cpp
    src/engines/gob/map.cpp
    src/engines/gob/map_v1.cpp
    src/engines/gob/map_v2.cpp
    src/engines/gob/minigames/geisha/diving.cpp
    src/engines/gob/minigames/geisha/evilfish.cpp
    src/engines/gob/minigames/geisha/meter.cpp
    src/engines/gob/minigames/geisha/mouth.cpp
    src/engines/gob/minigames/geisha/oko.cpp
    src/engines/gob/minigames/geisha/penetration.cpp
    src/engines/gob/minigames/geisha/submarine.cpp
    src/engines/gob/mult.cpp
    src/engines/gob/mult_v1.cpp
    src/engines/gob/mult_v2.cpp
    src/engines/gob/palanim.cpp
    src/engines/gob/pregob/gctfile.cpp
    src/engines/gob/pregob/onceupon/abracadabra.cpp
    src/engines/gob/pregob/onceupon/babayaga.cpp
    src/engines/gob/pregob/onceupon/chargenchild.cpp
    src/engines/gob/pregob/onceupon/onceupon.cpp
    src/engines/gob/pregob/onceupon/parents.cpp
    src/engines/gob/pregob/onceupon/stork.cpp
    src/engines/gob/pregob/onceupon/title.cpp
    src/engines/gob/pregob/pregob.cpp
    src/engines/gob/pregob/seqfile.cpp
    src/engines/gob/pregob/txtfile.cpp
    src/engines/gob/resources.cpp
    src/engines/gob/rxyfile.cpp
    src/engines/gob/save/saveconverter.cpp
    src/engines/gob/save/saveconverter_v2.cpp
    src/engines/gob/save/saveconverter_v3.cpp
    src/engines/gob/save/saveconverter_v4.cpp
    src/engines/gob/save/savefile.cpp
    src/engines/gob/save/savehandler.cpp
    src/engines/gob/save/saveload.cpp
    src/engines/gob/save/saveload_ajworld.cpp
    src/engines/gob/save/saveload_fascin.cpp
    src/engines/gob/save/saveload_geisha.cpp
    src/engines/gob/save/saveload_inca2.cpp
    src/engines/gob/save/saveload_playtoons.cpp
    src/engines/gob/save/saveload_v2.cpp
    src/engines/gob/save/saveload_v3.cpp
    src/engines/gob/save/saveload_v4.cpp
    src/engines/gob/save/saveload_v6.cpp
    src/engines/gob/save/saveload_v7.cpp
    src/engines/gob/scenery.cpp
    src/engines/gob/scenery_v1.cpp
    src/engines/gob/scenery_v2.cpp
    src/engines/gob/script.cpp
    src/engines/gob/sound/adlib.cpp
    src/engines/gob/sound/adlplayer.cpp
    src/engines/gob/sound/bgatmosphere.cpp
    src/engines/gob/sound/cdrom.cpp
    src/engines/gob/sound/infogrames.cpp
    src/engines/gob/sound/musplayer.cpp
    src/engines/gob/sound/pcspeaker.cpp
    src/engines/gob/sound/protracker.cpp
    src/engines/gob/sound/sound.cpp
    src/engines/gob/sound/soundblaster.cpp
    src/engines/gob/sound/sounddesc.cpp
    src/engines/gob/sound/soundmixer.cpp
    src/engines/gob/surface.cpp
    src/engines/gob/totfile.cpp
    src/engines/gob/util.cpp
    src/engines/gob/variables.cpp
    src/engines/gob/video.cpp
    src/engines/gob/video_v1.cpp
    src/engines/gob/video_v2.cpp
    src/engines/gob/video_v6.cpp
    src/engines/gob/videoplayer.cpp
)

# Kyrandia Engine sources
set(KYRA_ENGINE_SOURCES
    src/engines/kyra/detection.cpp
    src/engines/kyra/debugger.cpp
    src/engines/kyra/gui.cpp
    src/engines/kyra/resource.cpp
    src/engines/kyra/resource_intern.cpp
    src/engines/kyra/saveload.cpp
    src/engines/kyra/screen.cpp
    src/engines/kyra/script.cpp
    src/engines/kyra/sound.cpp
    src/engines/kyra/sound_adlib.cpp
    src/engines/kyra/sound_digital.cpp
    src/engines/kyra/sound_midi.cpp
    src/engines/kyra/sound_pcspk.cpp
    src/engines/kyra/sprites.cpp
    src/engines/kyra/staticres.cpp
    src/engines/kyra/text.cpp
    src/engines/kyra/timer.cpp
    src/engines/kyra/util.cpp
    src/engines/kyra/wsamovie.cpp
    src/engines/kyra/vqa.cpp
    src/engines/kyra/seqplayer.cpp
    src/engines/kyra/animator_tim.cpp
    src/engines/kyra/script_tim.cpp
    # Kyrandia 1
    src/engines/kyra/kyra_v1.cpp
    src/engines/kyra/kyra_lok.cpp
    src/engines/kyra/animator_lok.cpp
    src/engines/kyra/gui_v1.cpp
    src/engines/kyra/gui_lok.cpp
    src/engines/kyra/items_lok.cpp
    src/engines/kyra/saveload_lok.cpp
    src/engines/kyra/scene_lok.cpp
    src/engines/kyra/scene_v1.cpp
    src/engines/kyra/screen_lok.cpp
    src/engines/kyra/script_lok.cpp
    src/engines/kyra/script_v1.cpp
    src/engines/kyra/sequences_lok.cpp
    src/engines/kyra/sound_lok.cpp
    src/engines/kyra/text_lok.cpp
    src/engines/kyra/timer_lok.cpp
    # Kyrandia 2
    src/engines/kyra/kyra_v2.cpp
    src/engines/kyra/kyra_hof.cpp
    src/engines/kyra/animator_hof.cpp
    src/engines/kyra/animator_v2.cpp
    src/engines/kyra/gui_hof.cpp
    src/engines/kyra/gui_v2.cpp
    src/engines/kyra/items_hof.cpp
    src/engines/kyra/items_v2.cpp
    src/engines/kyra/saveload_hof.cpp
    src/engines/kyra/scene_hof.cpp
    src/engines/kyra/scene_v2.cpp
    src/engines/kyra/screen_hof.cpp
    src/engines/kyra/screen_v2.cpp
    src/engines/kyra/script_hof.cpp
    src/engines/kyra/script_v2.cpp
    src/engines/kyra/sequences_hof.cpp
    src/engines/kyra/sequences_v2.cpp
    src/engines/kyra/text_hof.cpp
    src/engines/kyra/timer_hof.cpp
    # Kyrandia 3
    src/engines/kyra/kyra_mr.cpp
    src/engines/kyra/animator_mr.cpp
    src/engines/kyra/gui_mr.cpp
    src/engines/kyra/items_mr.cpp
    src/engines/kyra/saveload_mr.cpp
    src/engines/kyra/scene_mr.cpp
    src/engines/kyra/screen_mr.cpp
    src/engines/kyra/script_mr.cpp
    src/engines/kyra/sequences_mr.cpp
    src/engines/kyra/text_mr.cpp
    src/engines/kyra/timer_mr.cpp
    src/engines/kyra/sound_amiga.cpp
    src/engines/kyra/sound_towns.cpp
)

# SCI Engine sources (Sierra SCI0 / SCI1 / SCI1.1 — no SCI32, no video)
set(SCI_ENGINE_SOURCES
    src/engines/sci/console.cpp
    src/engines/sci/decompressor.cpp
    src/engines/sci/detection.cpp
    src/engines/sci/event.cpp
    src/engines/sci/resource.cpp
    src/engines/sci/resource_audio.cpp
    src/engines/sci/sci.cpp
    src/engines/sci/util.cpp
    # engine/*
    src/engines/sci/engine/features.cpp
    src/engines/sci/engine/file.cpp
    src/engines/sci/engine/gc.cpp
    src/engines/sci/engine/kernel.cpp
    src/engines/sci/engine/kevent.cpp
    src/engines/sci/engine/kfile.cpp
    src/engines/sci/engine/kgraphics.cpp
    src/engines/sci/engine/klists.cpp
    src/engines/sci/engine/kmath.cpp
    src/engines/sci/engine/kmenu.cpp
    src/engines/sci/engine/kmisc.cpp
    src/engines/sci/engine/kmovement.cpp
    src/engines/sci/engine/kparse.cpp
    src/engines/sci/engine/kpathing.cpp
    src/engines/sci/engine/kscripts.cpp
    src/engines/sci/engine/ksound.cpp
    src/engines/sci/engine/kstring.cpp
    src/engines/sci/engine/kvideo.cpp
    src/engines/sci/engine/message.cpp
    src/engines/sci/engine/object.cpp
    src/engines/sci/engine/savegame.cpp
    src/engines/sci/engine/script.cpp
    src/engines/sci/engine/scriptdebug.cpp
    src/engines/sci/engine/script_patches.cpp
    src/engines/sci/engine/selector.cpp
    src/engines/sci/engine/seg_manager.cpp
    src/engines/sci/engine/segment.cpp
    src/engines/sci/engine/state.cpp
    src/engines/sci/engine/static_selectors.cpp
    src/engines/sci/engine/vm.cpp
    src/engines/sci/engine/vm_types.cpp
    src/engines/sci/engine/workarounds.cpp
    # graphics/* (SCI16 only)
    src/engines/sci/graphics/animate.cpp
    src/engines/sci/graphics/cache.cpp
    src/engines/sci/graphics/compare.cpp
    src/engines/sci/graphics/controls16.cpp
    src/engines/sci/graphics/coordadjuster.cpp
    src/engines/sci/graphics/cursor.cpp
    src/engines/sci/graphics/font.cpp
    src/engines/sci/graphics/fontsjis.cpp
    src/engines/sci/graphics/maciconbar.cpp
    src/engines/sci/graphics/menu.cpp
    src/engines/sci/graphics/paint.cpp
    src/engines/sci/graphics/paint16.cpp
    src/engines/sci/graphics/palette.cpp
    src/engines/sci/graphics/picture.cpp
    src/engines/sci/graphics/portrait.cpp
    src/engines/sci/graphics/ports.cpp
    src/engines/sci/graphics/screen.cpp
    src/engines/sci/graphics/text16.cpp
    src/engines/sci/graphics/transitions.cpp
    src/engines/sci/graphics/view.cpp
    # parser/*
    src/engines/sci/parser/grammar.cpp
    src/engines/sci/parser/said.cpp
    src/engines/sci/parser/vocabulary.cpp
    # sound/*
    src/engines/sci/sound/audio.cpp
    src/engines/sci/sound/midiparser_sci.cpp
    src/engines/sci/sound/music.cpp
    src/engines/sci/sound/soundcmd.cpp
    src/engines/sci/sound/drivers/adlib.cpp
    src/engines/sci/sound/drivers/amigamac.cpp
    src/engines/sci/sound/drivers/cms.cpp
    src/engines/sci/sound/drivers/fb01.cpp
    src/engines/sci/sound/drivers/fmtowns.cpp
    src/engines/sci/sound/drivers/midi.cpp
    src/engines/sci/sound/drivers/pcjr.cpp
    # video/* (SEQ only — Robot decoder is SCI32)
    src/engines/sci/video/seq_decoder.cpp
)

# SCUMM Engine sources (core v0-v6)
set(SCUMM_ENGINE_SOURCES
    src/engines/scumm/detection.cpp
    src/engines/scumm/scumm.cpp
    src/engines/scumm/actor.cpp
    src/engines/scumm/akos.cpp
    src/engines/scumm/base-costume.cpp
    src/engines/scumm/bomp.cpp
    src/engines/scumm/boxes.cpp
    src/engines/scumm/camera.cpp
    src/engines/scumm/cdda.cpp
    src/engines/scumm/charset.cpp
    src/engines/scumm/charset-fontdata.cpp
    src/engines/scumm/costume.cpp
    src/engines/scumm/cursor.cpp
    src/engines/scumm/debugger.cpp
    src/engines/scumm/dialogs.cpp
    src/engines/scumm/file.cpp
    src/engines/scumm/file_nes.cpp
    src/engines/scumm/gfx.cpp
    src/engines/scumm/gfx_towns.cpp
    src/engines/scumm/help.cpp
    src/engines/scumm/input.cpp
    src/engines/scumm/midiparser_ro.cpp
    # nut_renderer.cpp — v7/v8 SMUSH font renderer
    src/engines/scumm/object.cpp
    src/engines/scumm/palette.cpp
    src/engines/scumm/resource.cpp
    src/engines/scumm/resource_v2.cpp
    src/engines/scumm/resource_v3.cpp
    src/engines/scumm/resource_v4.cpp
    src/engines/scumm/room.cpp
    src/engines/scumm/saveload.cpp
    src/engines/scumm/script.cpp
    src/engines/scumm/script_v0.cpp
    src/engines/scumm/script_v2.cpp
    src/engines/scumm/script_v3.cpp
    src/engines/scumm/script_v4.cpp
    src/engines/scumm/script_v5.cpp
    src/engines/scumm/script_v6.cpp
    src/engines/scumm/sound.cpp
    src/engines/scumm/string.cpp
    src/engines/scumm/usage_bits.cpp
    src/engines/scumm/util.cpp
    src/engines/scumm/vars.cpp
    src/engines/scumm/verbs.cpp
    # iMUSE (classic MIDI-based, used by v5-v6)
    src/engines/scumm/imuse/imuse.cpp
    src/engines/scumm/imuse/imuse_part.cpp
    src/engines/scumm/imuse/imuse_player.cpp
    src/engines/scumm/imuse/instrument.cpp
    src/engines/scumm/imuse/mac_m68k.cpp
    src/engines/scumm/imuse/pcspk.cpp
    src/engines/scumm/imuse/sysex_samnmax.cpp
    src/engines/scumm/imuse/sysex_scumm.cpp
    # Players
    src/engines/scumm/players/player_ad.cpp
    src/engines/scumm/players/player_apple2.cpp
    src/engines/scumm/players/player_mac.cpp
    src/engines/scumm/players/player_mod.cpp
    src/engines/scumm/players/player_nes.cpp
    src/engines/scumm/players/player_pce.cpp
    src/engines/scumm/players/player_sid.cpp
    src/engines/scumm/players/player_towns.cpp
    src/engines/scumm/players/player_v1.cpp
    src/engines/scumm/players/player_v2.cpp
    src/engines/scumm/players/player_v2a.cpp
    src/engines/scumm/players/player_v2base.cpp
    src/engines/scumm/players/player_v2cms.cpp
    src/engines/scumm/players/player_v3a.cpp
    src/engines/scumm/players/player_v3m.cpp
    src/engines/scumm/players/player_v4a.cpp
    src/engines/scumm/players/player_v5m.cpp
    # HE engine files (required by v6/v7 inheritance chain)
    src/engines/scumm/he/sound_he.cpp
    src/engines/scumm/he/resource_he.cpp
    src/engines/scumm/he/script_v60he.cpp
    src/engines/scumm/he/script_v70he.cpp
    src/engines/scumm/he/palette_he.cpp
    src/engines/scumm/he/floodfill_he.cpp
    src/engines/scumm/he/animation_he.cpp
    src/engines/scumm/he/sprite_he.cpp
    src/engines/scumm/he/wiz_he.cpp
    src/engines/scumm/he/logic_he.cpp
    src/engines/scumm/he/cup_player_he.cpp
    src/engines/scumm/he/script_v71he.cpp
    src/engines/scumm/he/script_v72he.cpp
    src/engines/scumm/he/script_v80he.cpp
    src/engines/scumm/he/script_v90he.cpp
    src/engines/scumm/he/script_v100he.cpp
)

# SCUMM v7/v8 sources (Full Throttle, The Dig, COMI)
set(SCUMM_7_8_SOURCES
    src/engines/scumm/script_v8.cpp
    # iMUSE Digital (v7/v8 audio)
    src/engines/scumm/imuse_digi/dimuse.cpp
    src/engines/scumm/imuse_digi/dimuse_bndmgr.cpp
    src/engines/scumm/imuse_digi/dimuse_codecs.cpp
    src/engines/scumm/imuse_digi/dimuse_music.cpp
    src/engines/scumm/imuse_digi/dimuse_script.cpp
    src/engines/scumm/imuse_digi/dimuse_sndmgr.cpp
    src/engines/scumm/imuse_digi/dimuse_tables.cpp
    src/engines/scumm/imuse_digi/dimuse_track.cpp
    # SMUSH video player
    src/engines/scumm/smush/channel.cpp
    src/engines/scumm/smush/codec1.cpp
    src/engines/scumm/smush/codec37.cpp
    src/engines/scumm/smush/codec47.cpp
    src/engines/scumm/smush/imuse_channel.cpp
    src/engines/scumm/smush/saud_channel.cpp
    src/engines/scumm/smush/smush_font.cpp
    src/engines/scumm/smush/smush_mixer.cpp
    src/engines/scumm/smush/smush_player.cpp
    # INSANE (action sequences)
    src/engines/scumm/insane/insane.cpp
    src/engines/scumm/insane/insane_ben.cpp
    src/engines/scumm/insane/insane_enemy.cpp
    src/engines/scumm/insane/insane_iact.cpp
    src/engines/scumm/insane/insane_scenes.cpp
)

# ScummVM Backends
set(BACKEND_SOURCES
    src/backends/base-backend.cpp
    src/backends/fs/abstract-fs.cpp
    src/backends/fs/rp2350/rp2350-fs.cpp
    src/backends/fs/rp2350/rp2350-fs-factory.cpp
    src/backends/saves/default/default-saves.cpp
    src/backends/saves/savefile.cpp
    src/common/zlib.cpp
    src/audio/midiparser_xmidi.cpp
    src/audio/decoders/voc.cpp
    src/audio/softsynth/fmtowns_pc98/towns_audio.cpp
    src/audio/softsynth/fmtowns_pc98/towns_euphony.cpp
    src/audio/softsynth/fmtowns_pc98/towns_pc98_driver.cpp
    src/audio/softsynth/fmtowns_pc98/towns_pc98_fmsynth.cpp
    src/audio/mods/maxtrax.cpp
    src/graphics/thumbnail.cpp
    src/graphics/scaler/thumbnail_intern.cpp
    src/graphics/sjis.cpp
    src/backends/events/default/default-events.cpp
    src/backends/timer/default/default-timer.cpp
    src/backends/platform/rp2350/rp2350-minimal.cpp
    src/backends/platform/rp2350/rp2350-system.cpp
)

# Main executable
add_executable(frank-cabal
    src/main.c
    src/cabal_main.cpp
    src/cabal_new.cpp
    src/cabal_fs.c
    src/cabal_sbrk.c
    ${COMMON_SOURCES}
    ${GRAPHICS_SOURCES}
    ${GUI_SOURCES}
    ${VIDEO_SOURCES}
    ${IMAGE_SOURCES}
    ${AUDIO_SOURCES}
    ${BASE_SOURCES}
    ${ENGINE_SOURCES}
    ${AGI_ENGINE_SOURCES}
    ${GOB_ENGINE_SOURCES}
    ${KYRA_ENGINE_SOURCES}
    ${SCI_ENGINE_SOURCES}
    ${SCUMM_ENGINE_SOURCES}
    # ${SCUMM_7_8_SOURCES}  # SCUMM v7/v8 (Full Throttle, The Dig, COMI) — disabled
    ${BACKEND_SOURCES}
)

target_include_directories(frank-cabal PRIVATE
    src
    src/engines
    src/fatfs
    drivers
    drivers/sdcard
    drivers/ps2
    drivers/ps2/kbd
    drivers/audio
)

# ScummVM include paths - src is the root for all ScummVM headers
# Headers use paths like "common/system.h", "backends/base-backend.h", etc.

target_compile_definitions(frank-cabal PRIVATE
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
    ENABLE_AGI=1          # Sierra AGI games
    ENABLE_GOB=1          # Gobliins games
    ENABLE_SCI=1          # Sierra SCI0/SCI1/SCI1.1 games (no SCI32)
    ENABLE_KYRA=1         # Legend of Kyrandia games
    ENABLE_SCUMM=1        # LucasArts SCUMM v0-v6 (MI1/MI2/DOTT/Sam&Max/FOA/LOOM/etc)
    # ENABLE_SCUMM_7_8=1  # SCUMM v7/v8 (Full Throttle, The Dig, COMI) — disabled

    # Memory optimization flags
    CABAL_LOW_MEMORY=1
    PICO_MALLOC_PANIC=0
    SKIP_PICO_MALLOC=1

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
    target_compile_definitions(frank-cabal PRIVATE
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
    target_compile_definitions(frank-cabal PRIVATE
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


target_link_libraries(frank-cabal
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
    ps2_driver
    ps2kbd
    drivers
)

# Add custom I2S audio driver
include(${CMAKE_CURRENT_LIST_DIR}/drivers/audio/CMakeLists.txt)
target_compile_definitions(frank-cabal PRIVATE USE_I2S_AUDIO=1)

# USB HID configuration
if(USB_HID_ENABLED)
    message(STATUS "USB HID enabled: Using UART for stdio")
    target_link_libraries(frank-cabal usbhid)
    target_compile_definitions(frank-cabal PRIVATE USB_HID_ENABLED=1)
    pico_enable_stdio_usb(frank-cabal 0)
    pico_enable_stdio_uart(frank-cabal 1)
else()
    message(STATUS "USB Serial Console enabled for debug")
    pico_enable_stdio_usb(frank-cabal 1)
    pico_enable_stdio_uart(frank-cabal 0)
endif()

# Use custom linker script with 64KB stack (instead of default 4KB in SCRATCH_Y)
# This is required for the GOB engine's deep call chains
pico_set_linker_script(frank-cabal ${CMAKE_CURRENT_LIST_DIR}/linker/memmap_cabal.ld)

pico_add_extra_outputs(frank-cabal)

# Print memory usage estimate
message(STATUS "")
message(STATUS "Memory Budget:")
message(STATUS "  SRAM: ~520KB total")
message(STATUS "    - Double framebuffer: ~154KB (320x240x2)")
message(STATUS "    - Main stack: 64KB (custom linker script)")
message(STATUS "    - DMA/Code/BSS: ~302KB")
message(STATUS "    - SCRATCH_X: 4KB (Core 1 stack)")
message(STATUS "    - SCRATCH_Y: 4KB (DMA buffers)")
message(STATUS "  PSRAM: 8MB total")
message(STATUS "    - Scratch buffers: 512KB")
message(STATUS "    - Game zone: 3.5MB")
message(STATUS "    - Temp/Audio: 4MB")
message(STATUS "")
