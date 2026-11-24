#ifndef ECHEATSWIDGET_H
#define ECHEATSWIDGET_H

#include "eframedwidget.h"

#include "engine/ecityid.h"

class eGameBoard;

// ZeusDroid: the cheats, opened from the in-game Menu. One-shot cheats have a
// "Use" button; the others are switched On / Off (they last until the game is
// closed or another one is loaded).
class eCheatsWidget : public eFramedWidget {
public:
    using eFramedWidget::eFramedWidget;

    void initialize(eGameBoard& board, const eCityId cid,
                    const eAction& closeAction);
};

#endif // ECHEATSWIDGET_H
