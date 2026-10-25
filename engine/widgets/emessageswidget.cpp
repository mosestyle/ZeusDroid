#include "emessageswidget.h"

#include "egamewidget.h"
#include "emainwindow.h"
#include "elanguage.h"
#include "elabel.h"
#include "ebutton.h"
#include "eframedbutton.h"
#include "eacceptbutton.h"
#include "escrollwidgetcomplete.h"
#include "platform/eplatform.h"
#include "platform/eportsettings.h"

#include <cctype>

namespace {
// One row of the list: tap reads; right-click (mouse) or a long press
// (touch: holding the row) deletes.
class eMessageRow : public eButton {
public:
    using eButton::eButton;
    eAction fDelete;
protected:
    bool mousePressEvent(const eMouseEvent& e) override {
        if(e.button() == eMouseButton::right) {
            if(fDelete) fDelete();
            return true;
        }
        mPressMs = SDL_GetTicks();
        return eButton::mousePressEvent(e);
    }
    bool mouseReleaseEvent(const eMouseEvent& e) override {
        const uint32_t held = SDL_GetTicks() - mPressMs;
        if(ePlatform::touchUi() && e.button() == eMouseButton::left &&
           mPressMs && held >= static_cast<uint32_t>(ePortSettings::instance().fLongPressMs)) {
            mPressMs = 0;
            if(fDelete) fDelete();
            return true;
        }
        mPressMs = 0;
        return eButton::mouseReleaseEvent(e);
    }
private:
    uint32_t mPressMs = 0;
};
}

void eMessagesWidget::initialize(eGameWidget* const gw) {
    mGW = gw;
    setType(eFrameType::message);

    const int p = padding();
    const auto res = window()->resolution();
    const int ww = res.centralWidgetSmallWidth();
    const int hh = res.centralWidgetSmallHeight();
    resize(ww, hh);

    const auto title = new eLabel(eLanguage::zeusText(63, 0), window()); // Messages
    title->setHugeFontSize();
    title->fitContent();
    addWidget(title);
    title->align(eAlignment::top | eAlignment::hcenter);
    title->setY(title->y() + p);

    // Column headings
    const auto dateH = new eLabel(eLanguage::zeusText(63, 2), window()); // Date
    dateH->setSmallFontSize();
    dateH->fitContent();
    addWidget(dateH);
    const int headY = title->y() + title->height();
    dateH->move(3*p, headY);
    {
        const auto probe = new eLabel("Sep 1750 BC      ", window());
        probe->setSmallFontSize();
        probe->fitContent();
        mDateW = probe->width();
        delete probe;
    }
    const auto subjH = new eLabel(eLanguage::zeusText(63, 3), window()); // Subject
    subjH->setSmallFontSize();
    subjH->fitContent();
    addWidget(subjH);
    subjH->move(3*p + mDateW, headY);

    const auto ok = new eAcceptButton(window());
    addWidget(ok);
    ok->align(eAlignment::bottom | eAlignment::right);
    ok->move(ok->x() - 2*p, ok->y() - 2*p);
    ok->setPressAction([this]() {
        deleteLater();
    });

    const auto delOpened = new eFramedButton(window());
    delOpened->setUnderline(false);
    delOpened->setSmallFontSize();
    delOpened->setSmallPadding();
    delOpened->setText(eLanguage::zeusText(63, 6)); // Delete opened messages
    delOpened->fitContent();
    addWidget(delOpened);
    delOpened->align(eAlignment::hcenter);
    delOpened->setY(ok->y() + (ok->height() - delOpened->height())/2);
    delOpened->setPressAction([this]() {
        mGW->messageLog().removeOpened();
        mGW->messagesChanged();
        fillList();
    });

    const auto hint = new eLabel(window());
    hint->setTinyFontSize();
    hint->setWrapWidth(ww - 6*p);
    hint->setText(eLanguage::zeusText(63, 4));
    hint->fitContent();
    addWidget(hint);
    hint->align(eAlignment::hcenter);
    hint->setY(delOpened->y() - hint->height() - p);

    const int listY = headY + dateH->height();
    mScroll = new eScrollWidgetComplete(window());
    addWidget(mScroll);
    mScroll->resize(ww - 4*p, hint->y() - p - listY);
    mScroll->move(2*p, listY);
    mScroll->initialize();

    fillList();
}

void eMessagesWidget::fillList() {
    const int lw = mScroll->listWidth();
    const auto list = new eWidget(window());
    list->setNoPadding();
    const auto& items = mGW->messageLog().items();
    int y = 0;
    if(items.empty()) {
        const auto l = new eLabel(window());
        l->setSmallFontSize();
        l->setWrapWidth(lw - 2*padding());
        l->setText(eLanguage::zeusText(63, 1)); // no messages
        l->fitContent();
        list->addWidget(l);
        y = l->height();
    }
    for(int i = 0; i < static_cast<int>(items.size()); i++) {
        const auto& m = items[i];
        const auto row = new eMessageRow(window());
        row->setNoPadding();
        // Month and year, like the original list ("Jan 1748 BC").
        auto dateStr = m.fDate.shortString();
        const auto sp = dateStr.find(' ');
        if(sp != std::string::npos && sp > 0 && std::isdigit(static_cast<unsigned char>(dateStr[0]))) {
            dateStr = dateStr.substr(sp + 1);
        }
        const auto date = new eLabel(dateStr, window());
        date->setSmallFontSize();
        date->setNoPadding();
        date->fitContent();
        if(!m.fOpened) date->setYellowFontColor();
        const auto subj = new eLabel(m.fTitle, window());
        subj->setSmallFontSize();
        subj->setNoPadding();
        subj->fitContent();
        if(!m.fOpened) subj->setYellowFontColor();
        row->addWidget(date);
        row->addWidget(subj);
        const int rh = ePlatform::touchRowHeight(std::max(date->height(), subj->height()));
        row->resize(lw, rh);
        date->move(padding(), (rh - date->height())/2);
        subj->move(padding() + mDateW, (rh - subj->height())/2);
        row->setPressAction([this, i]() {
            const auto gw = mGW;
            deleteLater();
            gw->showLoggedMessage(i);
        });
        row->fDelete = [this, i]() {
            mGW->messageLog().remove(i);
            mGW->messagesChanged();
            fillList();
        };
        list->addWidget(row);
        row->setY(y);
        y += rh;
    }
    list->resize(lw, std::max(y, 1));
    const auto old = mList;
    mList = list;
    mScroll->setScrollArea(list);
    if(old) old->deleteLater();
}
