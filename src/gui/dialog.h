/* Cabal - Legacy Game Implementations
 *
 * Minimal GUI Dialog stub for embedded platforms
 */

#ifndef GUI_DIALOG_H
#define GUI_DIALOG_H

#include "common/scummsys.h"
#include "common/str.h"

namespace GUI {

class CommandSender;

// Dialog result codes
enum {
    kMessageOK = 1,
    kMessageCancel = 0
};

// ThemeEngine stub
class ThemeEngine {
public:
    enum DialogBackground {
        kDialogBackgroundDefault,
        kDialogBackgroundSpecial,
        kDialogBackgroundPlain,
        kDialogBackgroundNone
    };
    
    enum TextAlignVertical {
        kTextAlignVTop,
        kTextAlignVCenter,
        kTextAlignVBottom
    };
};

/**
 * Minimal dialog class for embedded platforms.
 * This is a stub that provides the interface without full functionality.
 */
class Dialog {
protected:
    int _x, _y, _w, _h;
    ThemeEngine::DialogBackground _backgroundType;
    
public:
    Dialog(int x, int y, int w, int h) : _x(x), _y(y), _w(w), _h(h), 
        _backgroundType(ThemeEngine::kDialogBackgroundDefault) {}
    Dialog(const Common::String &name) : _x(0), _y(0), _w(0), _h(0),
        _backgroundType(ThemeEngine::kDialogBackgroundDefault) {}
    virtual ~Dialog() {}

    // Run the dialog modally - returns immediately on embedded
    virtual int runModal() { return kMessageOK; }

    // Handle a command from a widget
    virtual void handleCommand(CommandSender *sender, uint32 cmd, uint32 data) {}

    // Reflow layout after resize
    virtual void reflowLayout() {}

    // Close the dialog
    void close() {}

    // Check if dialog is visible
    bool isVisible() const { return false; }
};

/**
 * Command sender interface
 */
class CommandSender {
public:
    virtual ~CommandSender() {}
};

/**
 * Widget base class stub
 */
class Widget {
protected:
    int _x, _y, _w, _h;
public:
    Widget(Dialog *parent, int x, int y, int w, int h) : _x(x), _y(y), _w(w), _h(h) {}
    virtual ~Widget() {}
    
    void setSize(int w, int h) { _w = w; _h = h; }
    void setPos(int x, int y) { _x = x; _y = y; }
};

class ButtonWidget : public Widget, public CommandSender {
public:
    ButtonWidget(Dialog *parent, int x, int y, int w, int h, const Common::String &label, uint32 cmd = 0)
        : Widget(parent, x, y, w, h) {}
    virtual ~ButtonWidget() {}
};

class GraphicsWidget : public Widget {
public:
    GraphicsWidget(Dialog *parent, int x, int y, int w, int h)
        : Widget(parent, x, y, w, h) {}
    virtual ~GraphicsWidget() {}
};

} // End of namespace GUI

#endif // GUI_DIALOG_H
