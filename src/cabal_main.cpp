/* Cabal - Legacy Game Implementations
 *
 * Cabal Main Entry Point for RP2350
 *
 * This initializes the ScummVM-compatible OSystem and runs the game.
 */

#include "backends/platform/rp2350/rp2350-system.h"
#include "backends/platform/rp2350/rp2350-minimal.h"
#include "cabal_fs.h"
#include "common/system.h"
#include "common/config-manager.h"
#include "graphics/surface.h"

// AGI Engine includes
#include "agi/agi.h"
#include "engines/advancedDetector.h"
#include "base/plugins.h"
#include "common/fs.h"
#include "common/archive.h"

// GOB Engine includes
#include "gob/gob.h"

// Forward declare AGIGameDescription since it's defined in detection.cpp
namespace Agi {
struct AGIGameDescription {
    ADGameDescription desc;
    int gameID;
    int gameType;
    uint32 features;
    uint16 version;
};
}

// Forward declare GOBGameDescription
namespace Gob {
struct GOBGameDescription {
    ADGameDescription desc;
    GameType gameType;
    int32 features;
    const char *startStkBase;
    const char *startTotBase;
    uint32 demoIndex;
};
}

#include <stdio.h>
#include <string.h>

// Global OSystem instance (declared in common/system.h)
extern OSystem *g_system;

// List files in a directory using cabal_fs
static void listDirectory(const char *path) {
    printf("Contents of %s:\n", path);

    CabalDir *dir = cabal_dir_open(path);
    if (!dir) {
        printf("  (cannot open directory)\n");
        return;
    }

    CabalDirEntry entry;
    int count = 0;
    while (cabal_dir_read(dir, &entry)) {
        if (entry.isDirectory) {
            printf("  [DIR]  %s\n", entry.name);
        } else {
            printf("  %6lu %s\n", (unsigned long)entry.size, entry.name);
        }
        count++;
    }
    cabal_dir_close(dir);

    if (count == 0) {
        printf("  (empty)\n");
    }
    printf("\n");
}

// Test pattern using OSystem API
static void drawTestPatternOSystem(void) {
    printf("Drawing test pattern via OSystem...\n");

    // Set up a test palette
    byte palette[256 * 3];
    memset(palette, 0, sizeof(palette));

    // SMPTE color bars
    // 0 = Black
    palette[0] = 0; palette[1] = 0; palette[2] = 0;
    // 1 = Red
    palette[3] = 255; palette[4] = 0; palette[5] = 0;
    // 2 = Green
    palette[6] = 0; palette[7] = 255; palette[8] = 0;
    // 3 = Blue
    palette[9] = 0; palette[10] = 0; palette[11] = 255;
    // 4 = Yellow
    palette[12] = 255; palette[13] = 255; palette[14] = 0;
    // 5 = Cyan
    palette[15] = 0; palette[16] = 255; palette[17] = 255;
    // 6 = Magenta
    palette[18] = 255; palette[19] = 0; palette[20] = 255;
    // 7 = White
    palette[21] = 255; palette[22] = 255; palette[23] = 255;

    PaletteManager *pal = g_system->getPaletteManager();
    if (pal) {
        pal->setPalette(palette, 0, 256);
    }

    // Draw color bars
    Graphics::Surface *screen = g_system->lockScreen();
    if (screen && screen->getPixels()) {
        int width = screen->getWidth();
        int height = screen->getHeight();
        int barWidth = width / 8;

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int bar = x / barWidth;
                byte color;
                switch (bar) {
                    case 0: color = 7; break;  // White
                    case 1: color = 4; break;  // Yellow
                    case 2: color = 5; break;  // Cyan
                    case 3: color = 2; break;  // Green
                    case 4: color = 6; break;  // Magenta
                    case 5: color = 1; break;  // Red
                    case 6: color = 3; break;  // Blue
                    default: color = 0; break; // Black
                }
                byte *pixel = (byte *)screen->getBasePtr(x, y);
                *pixel = color;
            }
        }
    }
    g_system->unlockScreen();
    g_system->updateScreen();

    printf("Test pattern displayed.\n");
}

// GOB (Gobliins) game launcher
// Returns true if a game was found and launched
static bool launchGOBGame(const char *gamePath) {
    printf("GOB: Attempting to launch game from %s\n", gamePath);

    // Set up the game path in config manager
    ConfMan.set("path", gamePath);
    ConfMan.setActiveDomain("cabal-gob");

    // Set default audio/config values that Engine::syncSoundSettings() expects
    ConfMan.setInt("music_volume", 192);
    ConfMan.setInt("sfx_volume", 192);
    ConfMan.setInt("speech_volume", 192);
    ConfMan.setBool("mute", false);
    ConfMan.setBool("speech_mute", false);
    ConfMan.setBool("sfx_mute", false);
    ConfMan.setBool("music_mute", false);
    ConfMan.setInt("autosave_period", 0);  // Disable autosave on embedded
    ConfMan.setBool("enable_unsupported_game_warning", false);

    // Set music driver to null (no MIDI on embedded)
    ConfMan.set("music_driver", "null");
    ConfMan.set("gm_device", "null");
    ConfMan.set("mt32_device", "null");
    ConfMan.setBool("native_mt32", false);
    ConfMan.setBool("enable_gs", false);
    ConfMan.setBool("multi_midi", false);
    ConfMan.setInt("midi_gain", 100);
    ConfMan.setBool("subtitles", true);
    ConfMan.setInt("talkspeed", 60);

    // GOB-specific settings
    ConfMan.setBool("copy_protection", false);  // Skip copy protection
    ConfMan.set("language", "en");
    ConfMan.set("opl_driver", "auto");

    // Graphics settings
    ConfMan.set("gfx_mode", "normal");
    ConfMan.setBool("aspect_ratio", false);
    ConfMan.setBool("fullscreen", false);
    ConfMan.setBool("filtering", false);

    // Load all plugins (engine and music) before creating the engine
    printf("GOB: Loading plugins...\n");
    PluginManager::instance().loadAllPlugins();
    printf("GOB: Plugins loaded.\n");

    // Add game directory to SearchManager so engine can find files
    printf("GOB: Setting up search path...\n");
    Common::FSNode gameDir(gamePath);
    if (gameDir.exists() && gameDir.isDirectory()) {
        SearchMan.addDirectory(gamePath, gameDir, 0, 4);
        printf("GOB: Added %s to search path\n", gamePath);

        // List files in the directory for debugging
        Common::FSList files;
        if (gameDir.getChildren(files, Common::FSNode::kListFilesOnly)) {
            printf("GOB: Found %d files in game directory:\n", (int)files.size());
            for (Common::FSList::iterator it = files.begin(); it != files.end(); ++it) {
                printf("  - %s\n", it->getName().c_str());
            }
        }
    } else {
        printf("GOB: WARNING - Game directory not found or not accessible!\n");
    }

    // Create a minimal GOB game description for Gobliiins 1 (VGA DOS version)
    static Gob::GOBGameDescription gameDesc;
    memset(&gameDesc, 0, sizeof(gameDesc));

    // Set up the ADGameDescription part
    gameDesc.desc.gameid = "gob1";
    gameDesc.desc.extra = "EGA";
    gameDesc.desc.language = Common::EN_ANY;
    gameDesc.desc.platform = Common::kPlatformDOS;
    gameDesc.desc.flags = ADGF_NO_FLAGS;
    gameDesc.desc.guioptions = "";

    // Set GOB-specific fields
    gameDesc.gameType = Gob::kGameTypeGob1;
    gameDesc.features = Gob::kFeaturesEGA | Gob::kFeaturesAdLib;  // EGA DOS version
    gameDesc.startStkBase = 0;
    gameDesc.startTotBase = 0;
    gameDesc.demoIndex = 0;

    printf("GOB: Creating engine instance...\n");

    // Create the engine - GOB uses a two-step init process
    Gob::GobEngine *gobEngine = new Gob::GobEngine(g_system);
    gobEngine->initGame(&gameDesc);

    // Cast to Engine* to access public run() method
    ::Engine *engine = gobEngine;

    printf("GOB: Running game...\n");

    // Run the engine (this calls init() + go() internally)
    Common::Error err = engine->run();

    printf("GOB: Game finished with code %d (%s)\n", err.getCode(), err.getDesc().c_str());
    delete engine;
    return (err.getCode() == Common::kNoError);
}

// AGI game launcher
// Returns true if a game was found and launched
static bool launchAGIGame(const char *gamePath) {
    printf("AGI: Attempting to launch game from %s\n", gamePath);

    // Set up the game path in config manager
    ConfMan.set("path", gamePath);
    ConfMan.setActiveDomain("cabal-agi");

    // Set default audio/config values that Engine::syncSoundSettings() expects
    ConfMan.setInt("music_volume", 192);
    ConfMan.setInt("sfx_volume", 192);
    ConfMan.setInt("speech_volume", 192);
    ConfMan.setBool("mute", false);
    ConfMan.setBool("speech_mute", false);
    ConfMan.setInt("autosave_period", 0);  // Disable autosave on embedded
    ConfMan.setBool("enable_unsupported_game_warning", false);

    // Set music driver to null (no MIDI on embedded)
    ConfMan.set("music_driver", "null");
    ConfMan.set("gm_device", "null");
    ConfMan.set("mt32_device", "null");
    ConfMan.setBool("native_mt32", false);
    ConfMan.setBool("enable_gs", false);
    ConfMan.setBool("multi_midi", false);
    ConfMan.setInt("midi_gain", 100);
    ConfMan.setBool("subtitles", true);
    ConfMan.setInt("talkspeed", 60);

    // Load all plugins (engine and music) before creating the engine
    printf("AGI: Loading plugins...\n");
    PluginManager::instance().loadAllPlugins();
    printf("AGI: Plugins loaded.\n");

    // Add game directory to SearchManager so engine can find files
    printf("AGI: Setting up search path...\n");
    Common::FSNode gameDir(gamePath);
    if (gameDir.exists() && gameDir.isDirectory()) {
        SearchMan.addDirectory(gamePath, gameDir, 0, 4);
        printf("AGI: Added %s to search path\n", gamePath);

        // List files in the directory for debugging
        Common::FSList files;
        if (gameDir.getChildren(files, Common::FSNode::kListFilesOnly)) {
            printf("AGI: Found %d files in game directory:\n", (int)files.size());
            for (Common::FSList::iterator it = files.begin(); it != files.end(); ++it) {
                printf("  - %s\n", it->getName().c_str());
            }
        }
    } else {
        printf("AGI: WARNING - Game directory not found or not accessible!\n");
    }

    // Create a minimal AGI game description for AGI v2 games
    // This is for King's Quest, Space Quest, etc.
    static Agi::AGIGameDescription gameDesc;
    memset(&gameDesc, 0, sizeof(gameDesc));

    // Set up the ADGameDescription part
    gameDesc.desc.gameid = "agi-fanmade";  // Generic AGI
    gameDesc.desc.extra = "";
    gameDesc.desc.language = Common::EN_ANY;
    gameDesc.desc.platform = Common::kPlatformDOS;
    gameDesc.desc.flags = ADGF_NO_FLAGS;
    gameDesc.desc.guioptions = "";

    // Set AGI-specific fields
    gameDesc.gameID = Agi::GID_FANMADE;
    gameDesc.gameType = Agi::GType_V2;  // Most common AGI games are v2
    gameDesc.features = 0;
    gameDesc.version = 0x2917;  // AGI 2.917

    printf("AGI: Creating engine instance...\n");

    // Create the engine (use Engine pointer to access public run())
    ::Engine *engine = new Agi::AgiEngine(g_system, &gameDesc);

    printf("AGI: Running game...\n");

    // Run the engine (this calls init() + go() internally)
    Common::Error err = engine->run();

    printf("AGI: Game finished with code %d\n", err.getCode());
    delete engine;
    return (err.getCode() == Common::kNoError);
}

// Event loop using OSystem API
static void eventLoopOSystem(void) {
    printf("Entering event loop. Move mouse, press keys to test input.\n");
    printf("Press ESC to exit.\n");

    bool running = true;
    int frameCount = 0;
    uint32 lastTick = g_system->getMillis();

    g_system->showMouse(true);

    Common::EventManager *eventMan = g_system->getEventManager();

    while (running) {
        Common::Event event;

        // Poll events directly from OSystem_RP2350 since EventManager may not be set up
        OSystem_RP2350 *rp2350sys = static_cast<OSystem_RP2350 *>(g_system);
        while (rp2350sys->pollEvent(event)) {
            switch (event.type) {
                case Common::EVENT_KEYDOWN:
                    printf("Key down: %d (0x%02x)\n", event.kbd.keycode, event.kbd.keycode);
                    if (event.kbd.keycode == Common::KEYCODE_ESCAPE) {
                        running = false;
                    }
                    break;

                case Common::EVENT_KEYUP:
                    printf("Key up: %d\n", event.kbd.keycode);
                    break;

                case Common::EVENT_MOUSEMOVE:
                    if (frameCount % 30 == 0) {
                        printf("Mouse: %d, %d\n", event.mouse.x, event.mouse.y);
                    }
                    break;

                case Common::EVENT_LBUTTONDOWN:
                    printf("Left button down at %d, %d\n", event.mouse.x, event.mouse.y);
                    break;

                case Common::EVENT_LBUTTONUP:
                    printf("Left button up\n");
                    break;

                case Common::EVENT_RBUTTONDOWN:
                    printf("Right button down at %d, %d\n", event.mouse.x, event.mouse.y);
                    break;

                case Common::EVENT_RBUTTONUP:
                    printf("Right button up\n");
                    break;

                case Common::EVENT_QUIT:
                    running = false;
                    break;

                default:
                    break;
            }
        }

        // Update screen
        g_system->updateScreen();

        // Frame timing
        frameCount++;
        uint32 now = g_system->getMillis();
        if (now - lastTick >= 1000) {
            printf("FPS: %d\n", frameCount);
            frameCount = 0;
            lastTick = now;
        }

        // Small delay to prevent busy loop (~60 FPS target)
        g_system->delayMillis(16);
    }
}

// Called from main.c after hardware init
extern "C" void cabal_init(void) {
    printf("Cabal: Initializing OSystem backend...\n");

    // Create and initialize the OSystem
    g_system = new OSystem_RP2350();
    g_system->initBackend();

    // Initialize graphics (320x200 for classic games)
    g_system->initSize(320, 200, nullptr);

    // Initialize filesystem (using cabal_fs directly)
    printf("Cabal: Initializing filesystem...\n");
    CabalFsResult fsResult = cabal_fs_init();
    if (fsResult == CABAL_FS_OK) {
        printf("Cabal: Filesystem ready.\n");

        // List root directory
        listDirectory("/");

        // List cabal directory if it exists
        if (cabal_path_exists("/cabal")) {
            listDirectory("/cabal");
        } else {
            printf("Note: /cabal directory not found on SD card.\n");
        }
    } else {
        printf("Cabal: Filesystem init failed (error=%d)\n", fsResult);
    }

    printf("Cabal: System ready.\n");
}

// Main game loop - called from main.c
extern "C" int cabal_main(void) {
    printf("Cabal: Starting with OSystem backend...\n");

    // Try to launch a GOB (Gobliins) game if found on SD card
    if (cabal_path_exists("/cabal/gob")) {
        printf("Cabal: Found GOB game directory, attempting to launch...\n");
        if (launchGOBGame("/cabal/gob")) {
            printf("Cabal: GOB game completed.\n");
            return 0;
        }
        printf("Cabal: GOB game launch failed, trying AGI...\n");
    }

    // Try to launch an AGI game if found on SD card
    if (cabal_path_exists("/cabal/agi")) {
        printf("Cabal: Found AGI game directory, attempting to launch...\n");
        if (launchAGIGame("/cabal/agi")) {
            printf("Cabal: AGI game completed.\n");
            return 0;
        }
        printf("Cabal: AGI game launch failed, falling back to test mode.\n");
    }

    // Fallback: Draw test pattern using OSystem
    drawTestPatternOSystem();

    // Run event loop
    eventLoopOSystem();

    printf("Cabal: Exiting.\n");
    // Note: Don't delete g_system - OSystem destructor is protected
    // The system will clean up on program exit
    return 0;
}
