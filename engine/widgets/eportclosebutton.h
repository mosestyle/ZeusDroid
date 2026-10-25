#ifndef EPORTCLOSEBUTTON_H
#define EPORTCLOSEBUTTON_H

// Port: a close (X) button in the top-right corner of a dialog that upstream
// closes only with a right-click (not possible on a touch screen).

#include "ecancelbutton.h"
#include "emainwindow.h"

inline eCancelButton* ePortAddCloseButton(eWidget* const d, const eAction& close) {
    const auto x = new eCancelButton(d->window());
    d->addWidget(x);
    const int p = d->window()->resolution().largePadding();
    x->move(d->width() - x->width() - p, p);
    x->setPressAction(close);
    return x;
}

#endif // EPORTCLOSEBUTTON_H
