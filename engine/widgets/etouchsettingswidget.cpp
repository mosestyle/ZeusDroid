#include "etouchsettingswidget.h"

#include <cmath>
#include <functional>
#include <string>
#include <vector>

#include "emainwindow.h"
#include "elabel.h"
#include "eframedbutton.h"
#include "eokbutton.h"
#include "platform/eplatform.h"
#include "platform/eportsettings.h"

namespace {
struct eChoice {
    std::string fLabel;
    double fValue;
};

int nearestIndex(const std::vector<eChoice>& cs, const double v) {
    int best = 0;
    for(int i = 1; i < static_cast<int>(cs.size()); i++) {
        if(std::abs(cs[i].fValue - v) < std::abs(cs[best].fValue - v)) best = i;
    }
    return best;
}
}

void eTouchSettingsWidget::initialize(const eAction& closeAction) {
    setType(eFrameType::message);
    const auto win = window();
    const int p = padding();
    const int rowWanted = ePlatform::touchRowHeight(win->height()/14);
    const int w = std::min(win->width() - 4*p, std::max(win->width()/2, 900));
    const int valueW = w*2/5;

    auto& ps = ePortSettings::instance();
    const auto changed = [win]() {
        ePortSettings::instance().save();
        win->applyPortSettings();
    };

    const auto title = new eLabel("Touch control options", win);
    title->fitContent();
    addWidget(title);

    const auto ok = new eOkButton(win);
    ok->setPressAction(closeAction);
    // All rows must fit on the screen together with the title and the tick.
    const int kRows = 13;
    const int fixedH = 2*p + title->height() + p + p + ok->height() + 2*p;
    const int rowFit = (win->height() - fixedH)/kRows - p/2;
    const int rowH = std::max(32, std::min(rowWanted, rowFit));

    std::vector<eWidget*> rows;
    const auto addRow = [&](const std::string& name,
                            const std::vector<eChoice>& choices,
                            const std::function<double()>& get,
                            const std::function<void(double)>& set) {
        const auto row = new eWidget(win);
        row->setNoPadding();
        row->resize(w - 4*p, rowH);
        const auto l = new eLabel(name, win);
        l->setSmallFontSize();
        l->fitContent();
        row->addWidget(l);
        l->align(eAlignment::vcenter | eAlignment::left);
        const auto b = new eFramedButton(win);
        b->setUnderline(false);
        b->setSmallFontSize();
        const auto idx = std::make_shared<int>(nearestIndex(choices, get()));
        b->setText(choices[*idx].fLabel);
        b->resize(valueW, rowH - p/2);
        row->addWidget(b);
        b->align(eAlignment::vcenter | eAlignment::right);
        b->setPressAction([b, idx, choices, set, changed]() {
            *idx = (*idx + 1) % static_cast<int>(choices.size());
            b->setText(choices[*idx].fLabel);
            set(choices[*idx].fValue);
            changed();
        });
        addWidget(row);
        rows.push_back(row);
    };

    addRow("Placing buildings",
           {{"Preview, then confirm", 1}, {"Instant (tap builds)", 0}},
           [&ps]() { return ps.fBuildConfirm ? 1. : 0.; },
           [&ps](const double v) { ps.fBuildConfirm = v > 0.5; });
    addRow("After building (preview mode)",
           {{"Next one ready beside it", 1}, {"Nothing", 0}},
           [&ps]() { return ps.fBuildRepeat ? 1. : 0.; },
           [&ps](const double v) { ps.fBuildRepeat = v > 0.5; });
    addRow("Drawing rows, areas, roads",
           {{"Draw button by the tick", 0}, {"Draw switch on the bar", 1}},
           [&ps]() { return ps.fDrawSwitch ? 1. : 0.; },
           [&ps](const double v) { ps.fDrawSwitch = v > 0.5; });
    addRow("Undo button stays for",
           {{"10 seconds", 10}, {"30 seconds", 30}, {"1 minute", 60}},
           [&ps]() { return double(ps.fUndoSeconds); },
           [&ps](const double v) { ps.fUndoSeconds = static_cast<int>(v); });
    addRow("Vibrate when building",
           {{"On", 1}, {"Off", 0}},
           [&ps]() { return ps.fVibrate ? 1. : 0.; },
           [&ps](const double v) { ps.fVibrate = v > 0.5; });
    addRow("Drag with a tool (instant mode)",
           {{"Hold, then drag", 1}, {"Draws at once", 0}},
           [&ps]() { return ps.fHoldToDraw ? 1. : 0.; },
           [&ps](const double v) { ps.fHoldToDraw = v > 0.5; });
    addRow("Hold time before drawing",
           {{"0.2 s", 200}, {"0.3 s", 300}, {"0.45 s", 450}, {"0.6 s", 600}},
           [&ps]() { return double(ps.fBuildHoldMs); },
           [&ps](const double v) { ps.fBuildHoldMs = static_cast<int>(v); });
    addRow("Long press (info / cancel)",
           {{"0.35 s", 350}, {"0.5 s", 500}, {"0.75 s", 750}, {"1 s", 1000}},
           [&ps]() { return double(ps.fLongPressMs); },
           [&ps](const double v) { ps.fLongPressMs = static_cast<int>(v); });
    addRow("Map drag speed",
           {{"0.75x", 0.75}, {"1x", 1}, {"1.25x", 1.25}, {"1.5x", 1.5}, {"2x", 2}},
           [&ps]() { return double(ps.fPanSensitivity); },
           [&ps](const double v) { ps.fPanSensitivity = static_cast<float>(v); });
    addRow("Maximum zoom",
           {{"2x", 2}, {"3x", 3}, {"4x", 4}},
           [&ps]() { return ps.fMaxZoom; },
           [&ps](const double v) { ps.fMaxZoom = v; });
    addRow("Frame rate",
           {{"Auto (60)", -1}, {"30 FPS", 30}, {"60 FPS", 60}, {"90 FPS", 90}, {"120 FPS", 120}},
           [&ps]() { return ps.fRenderFps; },
           [&ps](const double v) { ps.fRenderFps = v; });
    addRow("Show FPS and memory",
           {{"Off", 0}, {"On", 1}},
           [&ps]() { return ps.fShowStats ? 1. : 0.; },
           [&ps](const double v) { ps.fShowStats = v > 0.5; });
    addRow("Interface size (restart)",
           {{"Auto", -1}, {"Small", 2}, {"Medium", 4}, {"Large", 3}},
           [&ps]() { return double(ps.fUIScale); },
           [&ps](const double v) { ps.fUIScale = static_cast<int>(v); });

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
