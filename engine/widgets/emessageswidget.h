#ifndef EMESSAGESWIDGET_H
#define EMESSAGESWIDGET_H

#include "eframedwidget.h"

class eGameWidget;
class eScrollWidgetComplete;

// Port: the original's "Messages" list (Messages button in the right panel):
// date and subject of every received message, newest first. Tap (left-click)
// reads a message, long-press (right-click) deletes it, "Delete opened
// messages" clears the read ones. Texts are the original's (group 63).
class eMessagesWidget : public eFramedWidget {
public:
    using eFramedWidget::eFramedWidget;

    void initialize(eGameWidget* const gw);
private:
    void fillList();

    eGameWidget* mGW = nullptr;
    eScrollWidgetComplete* mScroll = nullptr;
    eWidget* mList = nullptr;
    int mDateW = 0;
};

#endif // EMESSAGESWIDGET_H
