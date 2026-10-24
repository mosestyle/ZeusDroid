#include "emainwindow.h"

#include "widgets/emainmenu.h"
#include "widgets/esettingsmenu.h"
#include "widgets/egamewidget.h"
#include "widgets/egameloadingwidget.h"
#include "widgets/egamemenu.h"
#include "widgets/emenuloadingwidget.h"
#include "widgets/eworldwidget.h"
#include "widgets/echoosegameeditmenu.h"
#include "widgets/eselectcolonywidget.h"

#include "audio/emusic.h"

#include "engine/ethreadpool.h"

#include "egamedir.h"

#include "fileIO/ereadstream.h"

#include <chrono>
#include <filesystem>
#include <fstream>

#include "widgets/efilewidget.h"
#include "elanguage.h"

#include "evectorhelpers.h"

#include "widgets/eeventbackground.h"
#include "widgets/eepisodeintroductionwidget.h"
#include "widgets/eepisodelostwidget.h"
#include "widgets/erosterofleaders.h"

#include "platform/eplatform.h"
#include "platform/etouchgestures.h"
#include "platform/eautotest.h"
#include "widgets/elineedit.h"
#include "platform/eframeclock.h"
#include "platform/eportsettings.h"
#include "widgets/escrollwidget.h"
#include "widgets/etouchsettingswidget.h"

namespace {
class eWindowTouchSink : public eTouchSink {
public:
    eWindowTouchSink(eMainWindow& w) : mW(w) {}

    eTarget targetAt(const int x, const int y) override {
        const auto top = mW.currentWidget();
        if(!top) return eTarget::ui;
        const auto w = top->widgetAt(x, y);
        if(scrollListAt(x, y)) return eTarget::scrollList;
        const auto gw = mW.gameWidget();
        if(!gw || top != static_cast<eWidget*>(gw)) return eTarget::ui;
        return w == top ? eTarget::map : eTarget::ui;
    }
    bool drawModeActive() override {
        const auto gw = mW.touchGameWidget();
        return gw && gw->touchDrawModeActive();
    }
    bool boxSelectActive() override {
        const auto gw = mW.touchGameWidget();
        return gw && gw->touchBoxSelectActive();
    }
    bool confirmPlacement() override {
        const auto gw = mW.touchGameWidget();
        return gw && gw->touchPreviewSupported();
    }
    bool previewAt(const int x, const int y) override {
        const auto gw = mW.touchGameWidget();
        return gw && gw->touchPreviewAt(x, y);
    }
    void previewTap(const int x, const int y) override {
        if(const auto gw = mW.touchGameWidget()) gw->touchPreviewTap(x, y);
    }
    void previewArea(const int sx, const int sy, const int x, const int y,
                     const bool begin) override {
        if(const auto gw = mW.touchGameWidget()) gw->touchPreviewArea(sx, sy, x, y, begin);
    }
    void previewEnd() override {
        if(const auto gw = mW.touchGameWidget()) gw->touchPreviewEnd();
    }
    void previewDrag(const int sx, const int sy, const int x, const int y,
                     const bool begin) override {
        if(const auto gw = mW.touchGameWidget()) gw->touchPreviewDrag(sx, sy, x, y, begin);
    }
    void listScroll(const int downX, const int downY, const int dy) override {
        if(const auto sw = scrollListAt(downX, downY)) sw->scrollBy(dy);
    }
    bool buildToolActive() override {
        const auto top = mW.currentWidget();
        const auto gw = mW.gameWidget();
        if(!gw || top != static_cast<eWidget*>(gw)) return false;
        return gw->buildToolActive();
    }
    void pointerMove(const int x, const int y) override {
        mW.dispatchPointerMove(x, y);
    }
    void pointerPress(const eButton b, const int x, const int y) override {
        mW.dispatchPointerPress(toButton(b), x, y);
    }
    void pointerRelease(const eButton b, const int x, const int y) override {
        mW.dispatchPointerRelease(toButton(b), x, y);
    }
    void pinchBegin(const float x, const float y) override {
        if(const auto gw = mW.touchGameWidget()) gw->pinchBegin(x, y);
    }
    void pinchUpdate(const float x, const float y, const float scale) override {
        if(const auto gw = mW.touchGameWidget()) gw->pinchUpdate(x, y, scale);
    }
    void pinchEnd() override {
        if(const auto gw = mW.touchGameWidget()) gw->pinchEnd();
    }
private:
    // A list that is taller than its view under (x, y), if any.
    eScrollWidget* scrollListAt(const int x, const int y) const {
        const auto top = mW.currentWidget();
        if(!top) return nullptr;
        const auto sw = dynamic_cast<eScrollWidget*>(top->widgetAt(x, y));
        return sw && sw->canScroll() ? sw : nullptr;
    }
    static eMouseButton toButton(const eButton b) {
        switch(b) {
        case eButton::left: return eMouseButton::left;
        case eButton::right: return eMouseButton::right;
        case eButton::middle: return eMouseButton::middle;
        }
        return eMouseButton::left;
    }
    eMainWindow& mW;
};
}

void eMainWindow::pointerPosition(int& x, int& y) const {
    if(mPointerFromTouch) {
        x = mPointerX;
        y = mPointerY;
    } else {
        SDL_GetMouseState(&x, &y);
    }
}

void eMainWindow::dispatchPointerMove(const int x, const int y) {
    mPointerX = x;
    mPointerY = y;
    const bool shift = mShiftPressed > 0;
    const bool ctrl = mCtrlPressed > 0;
    const eMouseEvent me(x, y, shift, ctrl, mButtons, mButton);
    if(mWidget) mWidget->mouseMove(me);
}

void eMainWindow::dispatchPointerPress(const eMouseButton b, const int x, const int y) {
    mPointerX = x;
    mPointerY = y;
    mButton = b;
    mButtons = b | mButtons;
    const bool shift = mShiftPressed > 0;
    const bool ctrl = mCtrlPressed > 0;
    const eMouseEvent me(x, y, shift, ctrl, mButtons, mButton);
    if(mWidget) mWidget->mousePress(me);
}

void eMainWindow::dispatchPointerRelease(const eMouseButton b, const int x, const int y) {
    mPointerX = x;
    mPointerY = y;
    mButton = b;
    mButtons = mButtons & ~b;
    const bool shift = mShiftPressed > 0;
    const bool ctrl = mCtrlPressed > 0;
    const eMouseEvent me(x, y, shift, ctrl, mButtons, mButton);
    if(mWidget) mWidget->mouseRelease(me);
}

void eMainWindow::applyPortSettings() {
    const auto& ps = ePortSettings::instance();
    eGameWidget::sMaxZoom = ps.fMaxZoom;
    eGameWidget::sBuildConfirm = ps.fBuildConfirm;
    eGameWidget::sBuildRepeat = ps.fBuildRepeat;
    eGameWidget::sBuildHaptics = ps.fVibrate;
    if(mTouch) {
        eTouchConfig tc;
        ps.applyTo(tc);
        mTouch->setConfig(tc);
    }
}

void eMainWindow::showTouchSettings(eWidget* parent) {
    if(!parent) parent = mWidget;
    if(!parent) return;
    const auto tw = new eTouchSettingsWidget(this);
    tw->initialize([tw]() { tw->deleteLater(); });
    parent->addWidget(tw);
    tw->align(eAlignment::center);
    execDialog(tw, true, nullptr, parent);
}

eGameWidget* eMainWindow::touchGameWidget() const {
    if(mGW && mWidget == static_cast<eWidget*>(mGW)) return mGW;
    return nullptr;
}

void eMainWindow::handleFingerEvent(const SDL_Event& e) {
    if(!mTouch) return;
    int ww = width();
    int wh = height();
    if(mSdlWindow) SDL_GetWindowSize(mSdlWindow, &ww, &wh);
    const float x = e.tfinger.x*ww;
    const float y = e.tfinger.y*wh;
    const uint32_t t = e.tfinger.timestamp;
    mPointerFromTouch = true;
    switch(e.type) {
    case SDL_FINGERDOWN:
        mTouch->fingerDown(e.tfinger.fingerId, x, y, t);
        break;
    case SDL_FINGERMOTION:
        mTouch->fingerMove(e.tfinger.fingerId, x, y, t);
        break;
    case SDL_FINGERUP:
        mTouch->fingerUp(e.tfinger.fingerId, x, y, t);
        break;
    default:
        break;
    }
}

eMainWindow::eMainWindow() {}

eMainWindow::~eMainWindow() {
    if(mSdlWindow) SDL_DestroyWindow(mSdlWindow);
    if(mSdlRenderer) SDL_DestroyRenderer(mSdlRenderer);
    setWidget(nullptr);
}

bool eMainWindow::initialize(const eSettings& settings) {
    const auto& res = settings.fRes;
    const int w = res.width();
    const int h = res.height();
    const auto window = SDL_CreateWindow("eZeus",
                                         SDL_WINDOWPOS_UNDEFINED,
                                         SDL_WINDOWPOS_UNDEFINED,
                                         w, h, SDL_WINDOW_SHOWN);

    if(!window) {
        printf("Window could not be created! SDL Error: %s\n",
               SDL_GetError());
        return false;
    }
    const Uint32 flags = SDL_RENDERER_ACCELERATED/* |
                         SDL_RENDERER_PRESENTVSYNC*/;
    const auto renderer = SDL_CreateRenderer(window, -1, flags);
    if(!renderer) {
        printf("Renderer could not be created! SDL Error: %s\n",
               SDL_GetError());
        SDL_DestroyWindow(window);
        return false;
    }

    if(mSdlWindow) SDL_DestroyWindow(mSdlWindow);
    if(mSdlRenderer) SDL_DestroyRenderer(mSdlRenderer);
    mSdlWindow = window;
    mSdlRenderer = renderer;
    setResolution(res);
    setFullscreen(settings.fFullscreen);
    mSettings = settings;
    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    const std::string icoPath = eGameDir::path("zeus.ico");
    const auto icon = IMG_Load(icoPath.c_str());
    SDL_SetWindowIcon(window, icon);
    eGameTextures::setSettings(mSettings);
    return true;
}

void eMainWindow::setWidget(eWidget* const w) {
    if(mWidget) {
        if(mWidget != mGW && mWidget != mWW) {
            mWidget->deleteLater();
        }
    }
    mWidget = w;
}

eWidget* eMainWindow::takeWidget() {
    const auto w = mWidget;
    mWidget = nullptr;
    return w;
}

void eMainWindow::addSlot(const eSlot& slot) {
    mSlots.push_back(slot);
}

void eMainWindow::setResolution(const eResolution& res) {
    if(mSettings.fRes == res && !mFirstFullscrenSetting) return;
    mFirstResolutionSetting = false;
    mSettings.fRes = res;
    const int w = res.width();
    const int h = res.height();
    SDL_SetWindowSize(mSdlWindow, w, h);
}

void eMainWindow::setFullscreen(const bool f) {
    if(mSettings.fFullscreen == f && !mFirstFullscrenSetting) return;
    mFirstFullscrenSetting = false;
    mSettings.fFullscreen = f;
    SDL_SetWindowFullscreen(mSdlWindow, f ? SDL_WINDOW_FULLSCREEN : 0);
}

void eMainWindow::startGameAction(eGameBoard* const board,
                                  const eGameWidgetSettings& settings) {
    const auto show = [this, board, settings]() {
        showGame(board, settings);
    };
    startGameAction(show);
}

void eMainWindow::startGameAction(const stdsptr<eCampaign>& c,
                                  const eGameWidgetSettings& settings) {
    const auto show = [this, c, settings]() {
        showGame(c, settings);
    };
    startGameAction(show);
}

void eMainWindow::startGameAction(const eAction& a) {
    clearWidgets();
    const auto l = new eGameLoadingWidget(this);
    l->resize(width(), height());
    l->setDoneAction(a);
    setWidget(l);
    l->initialize();
}

void eMainWindow::showEpisodeIntroduction(
        const stdsptr<eCampaign>& c) {
    clearWidgets();
    if(c) mCampaign = c;
    const auto e = new eEpisodeIntroductionWidget(this);
    const auto proceedA = [this]() {
        mQuickSavePath.clear();
        mCampaign->startEpisode();
        const auto dir = leaderSaveDir();
        saveGame(dir + "autosave replay.ez");
        startGameAction([this]() {
            eGameWidgetSettings settings;
            settings.fPaused = true;
            showGame(mCampaign, settings);
        });
    };
    e->resize(width(), height());
    const auto ee = mCampaign->currentEpisode();

    const auto path = mCampaign->currentEpisodeAudioFilePath(true);
    const bool played = eMusic::playCampaignVoice(path);
    if(!played) eMusic::playMissionIntroMusic();

    e->initialize(mCampaign,
                  mCampaign->titleText(),
                  ee->fIntroduction,
                  ee->fGoals,
                  proceedA,
                  eEpisodeIntroType::intro);
    setWidget(e);
}

std::string eMainWindow::leaderSaveDir() const {
    return eGameDir::saveDir() + mLeader + "/";
}

void eMainWindow::clearWidgets() {
    if(mGW && mWidget != mGW) {
        mGW->setBoard(nullptr);
        mGW->deleteLater();
        mGW = nullptr;
    }
    if(mWW && mWidget != mWW) {
        mWW->deleteLater();
        mWW = nullptr;
    }
}

void eMainWindow::episodeFinished() {
    clearWidgets();
    if(!mCampaign) return;
    mCampaign->episodeFinished();
    const bool f = mCampaign->finished();
    if(f) return adventureComplete();
    const auto n = mCampaign->currentEpisodeType();
    if(n == eEpisodeType::parentCity) {
        showEpisodeIntroduction();
    } else {
        const auto w = new eSelectColonyWidget(this);
        const auto sel = mCampaign->remainingColonies();
        const auto selA = [this](const stdsptr<eWorldCity>& c) {
            int cid = 0;
            const auto& eps = mCampaign->colonyEpisodes();
            for(const auto& e : eps) {
                if(e->fCity == c) break;
                cid++;
            }
            mCampaign->setCurrentColonyEpisode(cid);
            showEpisodeIntroduction();
        };
        w->resize(width(), height());
        w->initialize(sel, selA, &mCampaign->worldBoard());
        setWidget(w);
    }
}

void eMainWindow::adventureComplete() {
    clearWidgets();
    if(!mCampaign) return;
    const auto e = new eEpisodeIntroductionWidget(this);
    const auto proceedA = [this]() {
        showMainMenu();
    };
    e->resize(width(), height());

    const auto path = mCampaign->adventureVictoryAudioFilePath();
    const bool played = eMusic::playCampaignVoice(path);
    if(!played) eMusic::playCampaignVictoryMusic();

    e->initialize(mCampaign,
                  eLanguage::zeusText(62, 0),
                  mCampaign->completeText(),
                  {},
                  proceedA,
                  eEpisodeIntroType::campaingVictory);
    setWidget(e);
}

void eMainWindow::episodeLost() {
    clearWidgets();
    const auto e = new eEpisodeLostWidget(this);
    const auto proceedA = [this]() {
        showMainMenu();
    };
    e->resize(width(), height());
    e->initialize(proceedA);
    setWidget(e);
}

bool eMainWindow::saveGame(const std::string& path) {
    const auto fsp = std::filesystem::path(path);
    const auto fspd = fsp.parent_path();
    std::error_code ec;
    std::filesystem::create_directories(fspd, ec);
    // Crash-safe save: write a temporary file, keep the previous save as
    // ".bak", then atomically rename. A process kill (common on Android)
    // can no longer leave a truncated save behind.
    const std::string tmpPath = path + ".tmp";
    {
        std::ofstream file(tmpPath, std::ios::out | std::ios::binary |
                           std::ios::trunc);
        if(!file) return false;
        eWriteTarget target(&file);
        eWriteStream dst(target);
        dst.writeFormat("eZeus.ez");
        if(mGW) {
            const auto s = mGW->settings();
            s.write(dst);
        } else {
            eGameWidgetSettings s;
            s.fPaused = true;
            s.write(dst);
        }
        mCampaign->write(dst);
        file.flush();
        if(!file.good()) {
            file.close();
            std::filesystem::remove(tmpPath, ec);
            printf("Saving '%s' failed (write error, disk full?)\n", path.c_str());
            return false;
        }
        file.close();
    }
    if(std::filesystem::exists(path, ec)) {
        std::filesystem::copy_file(path, path + ".bak",
                                   std::filesystem::copy_options::overwrite_existing, ec);
    }
    std::filesystem::rename(tmpPath, path, ec);
    if(ec) {
        printf("Saving '%s' failed: %s\n", path.c_str(), ec.message().c_str());
        std::filesystem::remove(tmpPath, ec);
        return false;
    }
    return true;
}

bool eMainWindow::loadGame(const std::string& path) {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if(!file) return false;
    eReadSource source(&file);
    eReadStream src(source);
    src.readFormat();
    const auto& format = src.format();
    const int version = src.formatVersion();
    if(format != "eZeus.ez") {
        printf("Invalid file '%s' format '%s', expected 'eZeus.ez'.\n",
               path.c_str(), format.c_str());
        return false;
    }
    if(version > eFileFormat::version) {
        printf("Attempting to read '%s' format '%s' version '%i' newer than the executable.\n",
               path.c_str(), format.c_str(), version);
    }
    eGameWidgetSettings s;
    s.read(src);
    const auto c = std::make_shared<eCampaign>();
    c->read(src);
    c->loadStrings();
    c->loadNumbers();
    src.handlePostFuncs();
    file.close();

    {
        const auto name = std::filesystem::path(path).filename().string();
        mQuickSavePath = name.rfind("autosave", 0) == 0 ? std::string() : path;
    }
    startGameAction(c, s);
    return true;
}

void eMainWindow::closeGame() {
    if(!mGW) return;
    if(mGW) {
        mGW->setBoard(nullptr);
        mGW->deleteLater();
        mGW = nullptr;
    }
    if(mWW) {
        mWW->deleteLater();
        mWW = nullptr;
    }
    showMainMenu();
}

void eMainWindow::showRosterOfLeaders() {
    clearWidgets();
    eMusic::playMenuMusic();
    const auto rol = new eRosterOfLeaders(this);
    rol->resize(width(), height());
    rol->initialize();
    setWidget(rol);
}

void eMainWindow::showMenuLoading() {
    const auto mlw = new eMenuLoadingWidget(this);
    mlw->setDoneAction([this]() {
        const auto ls = eRosterOfLeaders::sLeaders();
        if(ls.size() == 1) setLeader(ls[0]);
        if(mLeader.empty()) {
            showRosterOfLeaders();
        } else {
            showMainMenu();
        }
    });
    mlw->initialize();
    mlw->resize(width(), height());
    setWidget(mlw);
}

void eMainWindow::showMainMenu() {
    mCampaign = nullptr;
    clearWidgets();
    eMusic::playMenuMusic();

    const auto mm = new eMainMenu(this);
    mm->resize(width(), height());
    setWidget(mm);

    const auto newGameAction = [this]() {
        showChooseGameMenu();
    };

    const auto loadGameAction = [this, mm]() {
        const auto fw = new eFileWidget(this);
        const auto func = [this](const std::string& path) {
            return loadGame(path);
        };
        const auto closeAct = [mm, fw]() {
            mm->removeWidget(fw);
            fw->deleteLater();
        };
        const auto dir = leaderSaveDir();
        fw->intialize(eLanguage::zeusText(1, 3),
                      dir, func, closeAct);
        mm->addWidget(fw);
        fw->align(eAlignment::center);
    };

    const auto editGameAction = [this]() {
        showChooseGameEditMenu();
    };

    const auto settingsAction = [this]() {
        showSettingsMenu();
    };

    const auto quitAction = [this]() {
        mQuit = true;
    };

    const auto leaderAction = [this]() {
        showRosterOfLeaders();
    };

    mm->initialize(newGameAction,
                   loadGameAction,
                   editGameAction,
                   settingsAction,
                   quitAction,
                   leaderAction);
}

void eMainWindow::showSettingsMenu() {
    const auto esm = new eSettingsMenu(mSettings, this);
    esm->resize(width(), height());

    const auto applyA = [this](const eSettings& settings) {
        const bool loadNeeded = settings.fRes != mSettings.fRes;
        setResolution(settings.fRes);
        setFullscreen(settings.fFullscreen);
        mSettings = settings;
        mSettings.write();
        if(!mSettings.fTinyTextures &&
           !mSettings.fSmallTextures &&
           !mSettings.fMediumTextures &&
           !mSettings.fLargeTextures) {
            mSettings.fSmallTextures = true;
        }
        eGameTextures::setSettings(mSettings);
        if(loadNeeded) showMenuLoading();
        else showMainMenu();
    };
    const auto fullscrennA = [this](const bool f) {
        setFullscreen(f);
    };
    esm->initialize(applyA, fullscrennA);
    setWidget(esm);
}

void eMainWindow::showChooseGameMenu() {
    const auto gem = new eChooseGameEditMenu(this);
    gem->resize(width(), height());
    gem->initialize(false);
    setWidget(gem);
}

void eMainWindow::showChooseGameEditMenu() {
    const auto gem = new eChooseGameEditMenu(this);
    gem->resize(width(), height());
    gem->initialize(true);
    setWidget(gem);
}

void eMainWindow::showGame(const stdsptr<eCampaign>& c,
                           const eGameWidgetSettings& settings) {
    mCampaign = c;
    const auto e = c->currentEpisode();
    showGame(e->fBoard, settings);
}

void eMainWindow::showGame(eGameBoard* b,
                           const eGameWidgetSettings& settings) {
    if(!b) b = mBoard;

    if(b == mBoard && mGW) {
        return setWidget(mGW);
    }

    if(mGW) {
        mGW->setBoard(nullptr);
        mGW->deleteLater();
        mGW = nullptr;
    }

    mBoard = b;
    if(mBoard) {
        mBoard->updateAppealMapIfNeeded();
        mBoard->waitUntilFinished();
    }

    eMusic::playRandomMusic();
    mGW = new eGameWidget(this);
    mGW->setBoard(b);
    mGW->resize(width(), height());
    mGW->initialize();
    mGW->setSettings(settings);
    setWidget(mGW);
}

void eMainWindow::showWorld() {
    if(mWidget == mWW) return;
    if(!mCampaign) return;
    if(!mWW) {
        mWW = new eWorldWidget(this);
        mWW->resize(width(), height());
        mWW->initialize();
        mWW->setBoard(mBoard);
    } else {
        mWW->update();
    }
    setWidget(mWW);
}

void eMainWindow::execDialog(
        eWidget* const d, const bool closable,
        const eAction &closeFunc,
        eWidget* const parent) {
    if(!mWidget) return;
    const auto bg = new eEventBackground(this);
    if(closeFunc) {
        bg->initialize(parent ? parent : mWidget, d, closable, closeFunc);
    } else {
        const auto closeFunc = [d]() {
            d->deleteLater();
        };
        bg->initialize(parent ? parent : mWidget, d, closable, closeFunc);
    }
}

class eTooltip {
public:
    eTooltip(eMainWindow& w) : mWindow(w) {}

    void update() {
        const auto txt = eWidget::sTooltip();
        const bool updateTxt = mText != txt;
        if(updateTxt) {
            mText = txt;
        }

        const auto& res = mWindow.resolution();
        const int fontSize = res.verySmallFontSize();
        const bool updateFont = mFontSize != fontSize;
        if(updateFont) {
            mFontSize = fontSize;
            mFont = eFonts::defaultFont(mFontSize);
        }

        const bool updateTexture = updateTxt || updateFont;
        if(updateTexture) {
            const auto r = mWindow.renderer();
            if(mText.empty()) {
                mTexture->reset();
            } else {
                mTexture->loadText(r, mText, eFontColor::light, *mFont, 50*fontSize);
            }
        }
    }

    void paint(const int x, const int y, ePainter& p) {
        const int pp = padding();
        SDL_Rect rect{x, y, width(), height()};
        p.fillRect(rect, SDL_Color{16, 108, 144, 255});
        p.drawRect(rect, SDL_Color{0, 32, 32, 255}, 1);
        p.drawTexture(x + pp, y + pp, mTexture);
    }

    int width() const { return mTexture->width() + 2*padding(); }
    int height() const { return mTexture->height() + 2*padding(); }

    bool empty() const { return mText.empty(); }
private:
    int padding() const { return mFontSize/2; }

    eMainWindow& mWindow;
    int mFontSize = -1;
    TTF_Font* mFont = nullptr;
    std::string mText;
    stdsptr<eTexture> mTexture = std::make_shared<eTexture>();
};

int eMainWindow::exec() {
    using namespace std::chrono;
    using namespace std::chrono_literals;

    showMenuLoading();

    mTouchEnabled = ePlatform::isTouchPlatform() || eAutotest::forceTouch();
    ePlatform::setTouchUi(mTouchEnabled, height());
    if(mTouchEnabled) {
        eGameWidget::sTouchBarEnabled = true;
        mTouchSink = std::make_shared<eWindowTouchSink>(*this);
        eTouchConfig tc;
        ePortSettings::instance().applyTo(tc);
        mTouch = std::make_shared<eTouchGestures>(*mTouchSink, tc);
    }
    applyPortSettings();
    eAutotest::start(*this);

    SDL_Event e;
    eTooltip tooltip(*this);

    const bool showFPS = false;
    // Render rate: upstream paints (and simulates) at 20 FPS. On touch
    // devices we render at 60 FPS for smooth panning while eFrameClock keeps
    // the simulation at exactly 20 ticks/s (see platform/eframeclock.h).
    double fpsClamp = ePlatform::renderFps();
    const double tickMs = 1000./eFrameClock::kTickRate;
    double simAccum = tickMs;
    auto lastFrame = high_resolution_clock::now();
    if(mTouchEnabled) eGameWidget::sEdgeScrollEnabled = false;

    int c = 0;
    int fpsVal = 0;
    bool resetRenderTargets = false;
    // Port: optional stats overlay (Touch controls -> Show FPS and memory).
    int statFrames = 0;
    int statTicks = 0;
    double statWorkMs = 0;
    auto statStart = high_resolution_clock::now();
    std::string statText;
    stdsptr<eTexture> statTex;
    while(!mQuit) {
        const auto fpsStart = high_resolution_clock::now();
        fpsClamp = ePlatform::renderFps(); // can change in touch settings
        {
            const duration<double, std::milli> sinceLast = fpsStart - lastFrame;
            lastFrame = fpsStart;
            simAccum += sinceLast.count();
            bool tick = true;
            if(fpsClamp > eFrameClock::kTickRate) {
                tick = simAccum >= tickMs - 1.0;
                if(tick) simAccum -= tickMs;
                // No catch-up bursts: like upstream, a slow device slows the
                // game down instead of running several ticks in one frame.
                simAccum = std::min(simAccum, tickMs);
            }
            eFrameClock::setTick(tick);
        }

        while(SDL_PollEvent(&e)) {
            int x, y;
            pointerPosition(x, y);
            const bool shift = mShiftPressed > 0;
            const bool ctrl = mCtrlPressed > 0;
            if(e.type == SDL_QUIT) {
                mQuit = true;
            } else if(e.type == SDL_WINDOWEVENT) {
                const auto we = e.window.event;
                if(we == SDL_WINDOWEVENT_FOCUS_LOST && mTouch) {
                    mTouch->cancel(SDL_GetTicks());
                }
                if(we == SDL_WINDOWEVENT_MINIMIZED) {
                    if(mTouch) mTouch->cancel(SDL_GetTicks());
                    // Port: Android may end a backgrounded app at any time,
                    // so the running game is saved before the pause.
                    if(mGW && currentWidget() == static_cast<eWidget*>(mGW)) {
                        mGW->autosaveNow("autosave");
                    }
                    resetRenderTargets = true;
                    while(SDL_WaitEvent(&e)) {
                        if(e.window.event == SDL_WINDOWEVENT_RESTORED) {
                            break;
                        }
                    }
                } else if(we == SDL_WINDOWEVENT_EXPOSED) {
                    resetRenderTargets = true;
                }
            } else if(e.type == SDL_RENDER_TARGETS_RESET ||
                      e.type == SDL_RENDER_DEVICE_RESET) {
                resetRenderTargets = true;
            } else if(e.type == SDL_FINGERDOWN ||
                      e.type == SDL_FINGERMOTION ||
                      e.type == SDL_FINGERUP) {
                if(mTouchEnabled) handleFingerEvent(e);
            } else if(e.type == SDL_MOUSEMOTION) {
                if(e.motion.which == SDL_TOUCH_MOUSEID && mTouchEnabled) continue;
                mPointerFromTouch = false;
                dispatchPointerMove(x, y);
            } else if(e.type == SDL_MOUSEBUTTONDOWN) {
                if(e.button.which == SDL_TOUCH_MOUSEID && mTouchEnabled) continue;
                mPointerFromTouch = false;
                eMouseButton b;
                switch(e.button.button) {
                case SDL_BUTTON_LEFT:
                    b = eMouseButton::left;
                    break;
                case SDL_BUTTON_RIGHT:
                    b = eMouseButton::right;
                    break;
                case SDL_BUTTON_MIDDLE:
                    b = eMouseButton::middle;
                    break;
                default: continue;
                }
                dispatchPointerPress(b, x, y);
            } else if(e.type == SDL_MOUSEBUTTONUP) {
                if(e.button.which == SDL_TOUCH_MOUSEID && mTouchEnabled) continue;
                mPointerFromTouch = false;
                eMouseButton b;
                switch(e.button.button) {
                case SDL_BUTTON_LEFT:
                    b = eMouseButton::left;
                    break;
                case SDL_BUTTON_RIGHT:
                    b = eMouseButton::right;
                    break;
                case SDL_BUTTON_MIDDLE:
                    b = eMouseButton::middle;
                    break;
                default: continue;
                }
                dispatchPointerRelease(b, x, y);
            } else if(e.type == SDL_MOUSEWHEEL) {
                const eMouseWheelEvent me(x, y, shift, ctrl, mButtons, e.wheel.y);
                if(mWidget) mWidget->mouseWheel(me);
            } else if(e.type == SDL_KEYDOWN) {
                auto k = e.key.keysym.scancode;
                // Android Back: first cancels the active build tool / info
                // panel / selection (like the touch "X"), else acts as Escape.
                if(k == SDL_SCANCODE_AC_BACK) {
                    if(mGW && mWidget == static_cast<eWidget*>(mGW) &&
                       mGW->touchCancel()) continue;
                    k = SDL_SCANCODE_ESCAPE;
                    // Key events are routed to the widget under the pointer;
                    // after a touch that is the last tapped button (e.g. the
                    // speed "+"), which would swallow it. Route Back via the
                    // centre of the map area, where dialogs are centred.
                    if(mTouchEnabled) {
                        const int gmw = mGW && mWidget == static_cast<eWidget*>(mGW) ?
                                            mGW->gameMenuWidth() : 0;
                        x = (width() - gmw)/2;
                        y = height()/2;
                    }
                }
                if(k == SDL_Scancode::SDL_SCANCODE_LSHIFT ||
                   k == SDL_Scancode::SDL_SCANCODE_RSHIFT) {
                    mShiftPressed++;
                } else if(k == SDL_Scancode::SDL_SCANCODE_LCTRL ||
                          k == SDL_Scancode::SDL_SCANCODE_RCTRL) {
                    mCtrlPressed++;
                }
                const eKeyPressEvent ke(x, y, shift, ctrl, mButtons, k);
                if(mWidget) mWidget->keyPress(ke);
            } else if(e.type == SDL_KEYUP) {
                const auto k = e.key.keysym.scancode;
                if(k == SDL_Scancode::SDL_SCANCODE_LSHIFT ||
                   k == SDL_Scancode::SDL_SCANCODE_RSHIFT) {
                    mShiftPressed--;
                } else if(k == SDL_Scancode::SDL_SCANCODE_LCTRL ||
                          k == SDL_Scancode::SDL_SCANCODE_RCTRL) {
                    mCtrlPressed--;
                }
            }
        }

        if(mTouch) mTouch->update(SDL_GetTicks());
        if(mTouchEnabled) {
            // Show the on-screen keyboard while a text field owns the keyboard
            // (Android does not enable SDL text input by default).
            const auto grabber = eWidget::keyboardGrabber();
            const bool wantText = grabber && dynamic_cast<eLineEdit*>(grabber);
            if(wantText && !SDL_IsTextInputActive()) {
                int gx = 0, gy = 0;
                grabber->mapToGlobal(gx, gy);
                SDL_Rect r{gx, gy, grabber->width(), grabber->height()};
                SDL_SetTextInputRect(&r);
                SDL_StartTextInput();
            } else if(!wantText && SDL_IsTextInputActive()) {
                SDL_StopTextInput();
            }
        }
        eAutotest::frame(*this, SDL_GetTicks());

        if(resetRenderTargets) {
            resetRenderTargets = false;
            if(mWidget) mWidget->renderTargetsReset();
        }

        SDL_SetRenderDrawColor(mSdlRenderer, 0x0, 0x0, 0x0, 0xFF);
        SDL_RenderClear(mSdlRenderer);

        ePainter p(mSdlRenderer);

        if(eFrameClock::tick()) eMusic::incTime();
        if(mWidget) {
            mWidget->paint(p);
            tooltip.update();
            // Hover tooltips make no sense for touch (no hover); they would
            // stick at the last touch point and cover the UI.
            if(!tooltip.empty() && !mPointerFromTouch) {
                const auto& res = resolution();
                const int pp = 25*res.multiplier();
                const int wtt = tooltip.width();
                const int htt = tooltip.height();
                int mx, my;
                pointerPosition(mx, my);
                int xtt;
                int ytt;
                if(mx > width()/2) {
                    xtt = mx - wtt;
                } else {
                    xtt = mx;
                }
                if(my > height()/2) {
                    ytt = my - htt - pp;
                } else {
                    ytt = my + pp;
                }
                tooltip.paint(xtt, ytt, p);
            }
        }

        if(showFPS) {
            p.setFont(eFonts::defaultFont(resolution()));
            p.drawText(0, 0, std::to_string(fpsVal), eFontColor::dark);
        }
        if(ePortSettings::instance().fShowStats && !statText.empty()) {
            if(!statTex) {
                statTex = std::make_shared<eTexture>();
                statTex->loadText(mSdlRenderer, statText, eFontColor::light,
                                  *eFonts::defaultFont(resolution()));
            }
            const int pad = 6;
            const SDL_Rect bg{0, height() - statTex->height() - 2*pad,
                              statTex->width() + 2*pad, statTex->height() + 2*pad};
            p.fillRect(bg, {0, 0, 0, 170});
            p.drawTexture(pad, bg.y + pad, statTex);
        }

        eAutotest::beforePresent(*this);
        SDL_RenderPresent(mSdlRenderer);

        for(const auto& s : mSlots) {
            s();
        }
        mSlots.clear();

        const auto fpsEnd = high_resolution_clock::now();
        const duration<double, std::milli> fpsElapsed = fpsEnd - fpsStart;
        {
            statFrames++;
            if(eFrameClock::tick()) statTicks++;
            statWorkMs += fpsElapsed.count();
            const duration<double> statSpan = fpsEnd - statStart;
            if(statSpan.count() >= 1.0) {
                char buf[96];
                const int mb = ePlatform::residentMemoryMB();
                std::snprintf(buf, sizeof(buf), "%.0f fps  %.1f ms  %.0f ticks/s  %d MB",
                              statFrames/statSpan.count(), statWorkMs/std::max(1, statFrames),
                              statTicks/statSpan.count(), mb);
                if(statText != buf) statTex.reset();
                statText = buf;
                statFrames = 0;
                statTicks = 0;
                statWorkMs = 0;
                statStart = fpsEnd;
            }
        }
        const duration<double, std::milli> fpsDuration(1000./fpsClamp);
        const duration<double, std::milli> fpsSleep(fpsDuration - fpsElapsed);
        std::this_thread::sleep_for(fpsSleep);

        if(showFPS) {
            c++;
            if(c % 25 == 0) {
                const auto fpsEnd = high_resolution_clock::now();
                const duration<double, std::milli> fpsElapsed = fpsEnd - fpsStart;
                fpsVal = (int)std::round(1000./fpsElapsed.count());
            }
        }
    }

    return 0;
}

std::string eMainWindow::debugListAdventures() const {
    const auto list = eChooseGameEditMenu::sListAdventures();
    std::string r;
    int i = 0;
    for(const auto& g : list) {
        r += std::to_string(i++) + ": " + g.fTitle + (g.fIsPak ? " [pak " +
             g.fPakPath.substr(g.fPakPath.find_last_of('/') + 1) + "]" : " [eZeus]") + "\n";
    }
    return r;
}

bool eMainWindow::debugStartAdventure(const int index) {
    const auto list = eChooseGameEditMenu::sListAdventures();
    if(index < 0 || index >= (int)list.size()) return false;
    const auto& g = list[index];
    const auto c = std::make_shared<eCampaign>();
    if(g.fIsPak) c->readPak(g.fTitle, g.fPakPath);
    else c->load(g.fFolderName);
    mCampaign = c;
    mQuickSavePath.clear();
    mCampaign->startEpisode();
    startGameAction([this]() {
        eGameWidgetSettings settings;
        settings.fPaused = true;
        showGame(mCampaign, settings);
    });
    return true;
}
