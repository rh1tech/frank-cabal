/* Cabal - Legacy Game Implementations
 *
 * Minimal GUI Manager stub for embedded platforms
 */

#ifndef GUI_GUI_MANAGER_H
#define GUI_GUI_MANAGER_H

#include "common/scummsys.h"
#include "common/str.h"

namespace GUI {

/**
 * Minimal GuiManager stub for embedded platforms.
 * Provides basic font metrics without full GUI functionality.
 */
class GuiManager {
public:
    GuiManager() {}
    ~GuiManager() {}

    // Font metrics - return reasonable defaults for embedded
    int getStringWidth(const Common::String &str) const {
        return str.size() * 8;  // Assume 8 pixels per character
    }

    int getFontHeight() const {
        return 10;  // Default font height
    }

    // Theme-related stubs
    bool loadNewTheme(const Common::String &name) { return true; }
    const char *getThemeFile() const { return ""; }
    bool isCurrentTheme(const Common::String &name) const { return false; }
};

} // End of namespace GUI

// Global GUI manager instance
extern GUI::GuiManager g_gui;

#endif // GUI_GUI_MANAGER_H
