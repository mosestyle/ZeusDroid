#ifndef ELINEEDIT_H
#define ELINEEDIT_H

#include "eframedlabel.h"

class eLineEdit : public eFramedLabel {
public:
    eLineEdit(eMainWindow* const window);

    void setChangeAction(const eAction a);

    void setRenderBg(const bool r);

    // Port (touch): with this set, the on-screen keyboard opens only after
    // the field is tapped (and closes on Enter), not when a dialog opens.
    void setSoftKeyboardOnTap(const bool t) { mKeyboardOnTap = t; }
    bool softKeyboardWanted() const { return !mKeyboardOnTap || mKeyboardWanted; }
protected:
    bool mousePressEvent(const eMouseEvent& e);
    bool mouseMoveEvent(const eMouseEvent& e);
    bool mouseEnterEvent(const eMouseEvent& e);
    bool mouseLeaveEvent(const eMouseEvent& e);
    bool keyPressEvent(const eKeyPressEvent& e);
    void paintEvent(ePainter& p);
private:
    bool mHovered = false;
    bool mRenderBg = false;
    eAction mChangeAction;
    int mTime = 0;
    bool mKeyboardOnTap = false;
    bool mKeyboardWanted = false;
};

#endif // ELINEEDIT_H
