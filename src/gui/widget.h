/* Cabal - Legacy Game Implementations
 *
 * Minimal Widget stub for embedded platforms
 */

#ifndef GUI_WIDGET_H
#define GUI_WIDGET_H

#include "common/scummsys.h"
#include "common/str.h"
#include "graphics/font.h"
#include "gui/dialog.h"

namespace GUI {

// Widget command codes
enum {
    kCloseCmd = 'clos',
    kOKCmd = 'ok  ',
    kCancelCmd = 'cncl'
};

// StaticTextWidget stub
class StaticTextWidget : public Widget {
public:
    StaticTextWidget(Dialog *parent, int x, int y, int w, int h, const Common::String &text, Graphics::TextAlign align = Graphics::kTextAlignLeft)
        : Widget(parent, x, y, w, h) {}
    StaticTextWidget(Dialog *parent, const Common::String &name, const Common::String &text, const Common::String &tooltip = "")
        : Widget(parent, 0, 0, 0, 0) {}
    virtual ~StaticTextWidget() {}
    
    void setLabel(const Common::String &label) {}
    const Common::String &getLabel() const { static Common::String s; return s; }
};

} // End of namespace GUI

#endif // GUI_WIDGET_H
