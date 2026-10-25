#include "efilewidget.h"
#include "platform/eplatform.h"

#include "emainwindow.h"

#include "eacceptbutton.h"
#include "ecancelbutton.h"

#include "escrollwidgetcomplete.h"

#include "elineedit.h"
#include "eframedbutton.h"
#include "equestionwidget.h"

#include <string>
#include <iostream>
#include <filesystem>
namespace fs = std::filesystem;

template <typename TP>
time_t to_time_t(TP tp) {
    using namespace std::chrono;
    auto sctp = time_point_cast<system_clock::duration>(tp - TP::clock::now() + system_clock::now());
    return system_clock::to_time_t(sctp);
}

void eFileWidget::intialize(const std::string& title,
                            const std::string& folder,
                            const eFileFunc& func,
                            const eAction& closeAction) {
    mFolder = folder;

    setType(eFrameType::message);

    const int p = padding();
    const auto res = window()->resolution();
    const int ww = res.centralWidgetSmallWidth();
    const int hh = res.centralWidgetSmallHeight();

    resize(ww, hh);

    mTitleLabel = new eLabel(title, window());
    mTitleLabel->fitContent();
    addWidget(mTitleLabel);
    mTitleLabel->align(eAlignment::top | eAlignment::hcenter);
    mTitleLabel->setY(mTitleLabel->y() + p);

    mOk = new eAcceptButton(window());
    addWidget(mOk);
    mOk->align(eAlignment::bottom | eAlignment::right);
    mOk->move(mOk->x() - 2*p, mOk->y() - 2*p);
    mOk->setPressAction([this, func, closeAction] {
        const auto path = filePath();
        const bool r = func(path);
        if(r) closeAction();
    });

    mCancel = new eCancelButton(window());
    addWidget(mCancel);
    mCancel->align(eAlignment::bottom | eAlignment::left);
    mCancel->move(mCancel->x() + 2*p, mCancel->y() - 2*p);
    mCancel->setPressAction(closeAction);

    const auto lineW = new eFramedWidget(window());
    lineW->setType(eFrameType::inner);
    lineW->setNoPadding();
    mLineEdit = new eLineEdit(window());
    mLineEdit->setTinyPadding();
    mLineEdit->setText("A");
    mLineEdit->fitContent();
    mLineEdit->setSmallFontSize();
    mLineEdit->setText("");
    lineW->addWidget(mLineEdit);
    mLineEdit->setX(p);
    const int lineY = mTitleLabel->y() + mTitleLabel->height() + p;
    lineW->setY(lineY);
    lineW->setX(2*p);
    addWidget(lineW);

    const auto scrollCont = new eScrollWidgetComplete(window());
    addWidget(scrollCont);
    scrollCont->resize(ww - 4*p, hh - lineY - mLineEdit->height() - 10*p);
    scrollCont->setY(lineY + mLineEdit->height() + 2*p);
    scrollCont->setX(2*p);
    scrollCont->initialize();
    const int swwidth = scrollCont->listWidth();

    mScroll = scrollCont;
    fillFiles();

    // Port: delete a saved game (between the cross and the tick).
    mDelete = new eFramedButton(window());
    mDelete->setUnderline(false);
    mDelete->setSmallFontSize();
    mDelete->setSmallPadding();
    mDelete->setText("Delete");
    mDelete->fitContent();
    addWidget(mDelete);
    mDelete->align(eAlignment::hcenter);
    mDelete->setY(mOk->y() + (mOk->height() - mDelete->height())/2);
    mDelete->setPressAction([this]() {
        deleteSelected();
    });

    mLineEdit->resize(swwidth - 2*p, mLineEdit->height());
    lineW->resize(swwidth, mLineEdit->height());

    // Touch: the keyboard opens when the name field is tapped.
    mLineEdit->setSoftKeyboardOnTap(true);
    mLineEdit->grabKeyboard();
}

void eFileWidget::fillFiles() {
    const int swwidth = mScroll->listWidth();
    const auto filesWidget = new eWidget(window());

    std::map<time_t, fs::path> sorted;
    std::error_code ec;
    if(std::filesystem::exists(mFolder, ec)) {
        for(const auto& entry : fs::directory_iterator(mFolder, ec)) {
            const auto path = entry.path();
            const auto ext = path.extension();
            if(ext != ".ez") continue;
            const auto lwt = fs::last_write_time(path, ec);
            const auto time = to_time_t(lwt);
            auto key = -time;
            while(sorted.count(key)) key--;
            sorted[key] = path;
        }
    }

    int y = 0;
    for(const auto& entry : sorted) {
        const auto path = entry.second;
        const auto name = path.filename().stem().u8string();
        const auto b = new eButton(name, window());
        b->setUnderline(false);
        b->setDarkFontColor();
        b->setMouseEnterAction([b]() {
            b->setLightFontColor();
        });
        b->setMouseLeaveAction([b]() {
            b->setDarkFontColor();
        });
        b->setTextAlignment(eAlignment::left | eAlignment::vcenter);
        b->setNoPadding();
        b->fitContent();
        b->setWidth(swwidth);
        b->setHeight(ePlatform::touchRowHeight(b->height()));
        filesWidget->addWidget(b);
        b->setY(y);
        y += b->height();
        b->setPressAction([this, name]() {
            setFileName(name);
        });
    }
    filesWidget->setNoPadding();
    filesWidget->fitContent();

    const auto old = mFiles;
    mFiles = filesWidget;
    mScroll->setScrollArea(filesWidget);
    if(old) old->deleteLater();
}

void eFileWidget::deleteSelected() {
    const auto name = mLineEdit->text();
    if(name.empty()) return;
    const auto path = filePath();
    std::error_code ec;
    if(!fs::exists(path, ec)) return;
    const auto q = new eQuestionWidget(window());
    const auto acceptA = [this, path]() {
        std::error_code ec;
        for(const auto& ext : {"", ".view", ".msgs", ".bak", ".tmp"}) {
            fs::remove(path + ext, ec);
        }
        if(window()->quickSavePath() == path) window()->setQuickSavePath("");
        mLineEdit->setText("");
        fillFiles();
    };
    q->initialize("Delete " + name + "?",
                  "This deletes the saved game. Do you really want to do that?",
                  acceptA, nullptr);
    window()->execDialog(q);
    q->align(eAlignment::center);
}

void eFileWidget::setFileName(const std::string& path) {
    mLineEdit->setText(path);
}

std::string eFileWidget::filePath() const {
    return mFolder + mLineEdit->text() + ".ez";
}
