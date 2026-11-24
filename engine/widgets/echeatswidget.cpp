#include "echeatswidget.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "emainwindow.h"
#include "elabel.h"
#include "eframedbutton.h"
#include "eokbutton.h"
#include "engine/egameboard.h"
#include "platform/eplatform.h"

void eCheatsWidget::initialize(eGameBoard& board, const eCityId cid,
                               const eAction& closeAction) {
    setType(eFrameType::message);
    const auto win = window();
    const int p = padding();
    const int w = std::min(win->width() - 4*p, std::max(win->width()/2, 900));
    const int valueW = w/4;

    const auto title = new eLabel("Cheats", win);
    title->fitContent();
    addWidget(title);

    const auto ok = new eOkButton(win);
    ok->setPressAction(closeAction);

    const int kRows = 6;
    const int rowWanted = ePlatform::touchRowHeight(win->height()/12);
    const int fixedH = 2*p + title->height() + p + p + ok->height() + 2*p;
    const int rowFit = (win->height() - fixedH)/kRows - p/2;
    const int rowH = std::max(40, std::min(rowWanted, rowFit));

    std::vector<eWidget*> rows;
    const auto addRow = [&](const std::string& name,
                            const std::string& what,
                            const std::function<std::string()>& state,
                            const std::function<void()>& press) {
        const auto row = new eWidget(win);
        row->setNoPadding();
        row->resize(w - 4*p, rowH);
        const auto l = new eLabel(name + " - " + what, win);
        l->setSmallFontSize();
        l->fitContent();
        row->addWidget(l);
        l->align(eAlignment::vcenter | eAlignment::left);
        const auto b = new eFramedButton(win);
        b->setUnderline(false);
        b->setSmallFontSize();
        b->setText(state());
        b->resize(valueW, rowH - p/2);
        row->addWidget(b);
        b->align(eAlignment::vcenter | eAlignment::right);
        b->setPressAction([b, state, press]() {
            press();
            b->setText(state());
        });
        addWidget(row);
        rows.push_back(row);
    };

    const auto bptr = &board;
    auto& c = board.cheats();
    const auto cptr = &c;
    const auto onOff = [cptr](bool eGameBoard::eCheats::* f) {
        return [cptr, f]() { return std::string((cptr->*f) ? "On" : "Off"); };
    };
    const auto toggle = [cptr](bool eGameBoard::eCheats::* f,
                               const std::function<void()>& onEnable) {
        return [cptr, f, onEnable]() {
            cptr->*f = !(cptr->*f);
            if(cptr->*f && onEnable) onEnable();
        };
    };

    const auto used = std::make_shared<int>(0);
    addRow("Delian Treasury", "adds 1000 drachmas",
           [used]() { return *used ? "Use (+" + std::to_string(*used) + ")" :
                                     std::string("Use"); },
           [bptr, used]() {
               bptr->cheatAddDrachmas(1000);
               *used += 1000;
           });
    addRow("Lots of Fishy", "unlimited food",
           onOff(&eGameBoard::eCheats::fUnlimitedFood),
           toggle(&eGameBoard::eCheats::fUnlimitedFood,
                  [bptr, cid]() { bptr->cheatFeedHouses(cid); }));
    addRow("Fireballs from Heaven", "shoot fireballs from above",
           onOff(&eGameBoard::eCheats::fFireballs),
           toggle(&eGameBoard::eCheats::fFireballs,
                  [bptr, cid]() { bptr->cheatFireball(cid); }));
    addRow("Bowvine and Arrow", "towers shoot cows",
           onOff(&eGameBoard::eCheats::fBowvine),
           toggle(&eGameBoard::eCheats::fBowvine, nullptr));
    addRow("SoundFrags", "death sounds",
           onOff(&eGameBoard::eCheats::fSoundFrags),
           toggle(&eGameBoard::eCheats::fSoundFrags, nullptr));
    const auto won = std::make_shared<bool>(false);
    addRow("Ambrosia", "win scenario",
           [won]() { return std::string(*won ? "Done" : "Use"); },
           [bptr, won]() {
               if(*won) return;
               *won = true;
               bptr->cheatWinEpisode();
           });

    int y = 2*p;
    title->move((w - title->width())/2, y);
    y += title->height() + p;
    for(const auto r : rows) {
        r->move(2*p, y);
        y += r->height() + p/2;
    }
    y += p;
    resize(w, y + ok->height() + 2*p);
    addWidget(ok);
    ok->move(w - ok->width() - 2*p, y);
}
