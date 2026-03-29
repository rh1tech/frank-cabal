/* Cabal - Minimal stub for embedded platforms */

#ifndef GUI_WIDGETS_LIST_H
#define GUI_WIDGETS_LIST_H

#include "gui/dialog.h"
#include "common/str.h"

namespace GUI {

class ListWidget : public Widget {
public:
    typedef Common::Array<Common::String> StringArray;
    ListWidget(Dialog *parent, int x, int y, int w, int h)
        : Widget(parent, x, y, w, h) {}
    virtual ~ListWidget() {}

    void setList(const StringArray &list) {}
    void setSelected(int item) {}
    int getSelected() const { return 0; }
    const Common::String &getSelectedString() const { static Common::String s; return s; }
    void setNumberingMode(int mode) {}

    enum {
        kListNumberingOff = 0,
        kListNumberingOne = 1
    };
};

} // End of namespace GUI

#endif // GUI_WIDGETS_LIST_H
