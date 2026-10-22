#ifndef EMAINWINDOW_H
#define EMAINWINDOW_H

#include "widgets/ewidget.h"
#include "widgets/eresolution.h"
#include "textures/eterraintextures.h"
#include "textures/egodtextures.h"
#include "textures/ebuildingtextures.h"
#include "textures/echaractertextures.h"
#include "widgets/esettingsmenu.h"

using eSlot = std::function<void()>;

class eGameBoard;
class eGameWidget;
class eTouchGestures;
class eTouchSink;
class eWorldWidget;
class eCampaign;

struct eGameWidgetSettings;

class eMainWindow {
public:
    eMainWindow();
    ~eMainWindow();

    bool initialize(const eSettings& settings);
public:

    void setWidget(eWidget* const w);
    eWidget* takeWidget();

    int exec();

    void addSlot(const eSlot& slot);

    // Pointer dispatch shared by the mouse path and the touch gesture layer.
    void dispatchPointerMove(const int x, const int y);
    void dispatchPointerPress(const eMouseButton b, const int x, const int y);
    void dispatchPointerRelease(const eMouseButton b, const int x, const int y);
    // The game widget when it is the current screen (touch pinch target).
    eGameWidget* touchGameWidget() const;
    // Applies port_settings (touch config, zoom limit, placement mode) live.
    void applyPortSettings();
    // Port: touch options dialog on top of `parent` (or the current screen).
    void showTouchSettings(eWidget* parent = nullptr);
    // Current pointer position (mouse cursor or last touch point).
    void pointerPosition(int& x, int& y) const;
    bool touchEnabled() const { return mTouchEnabled; }
    eGameWidget* gameWidget() const { return mGW; }

    int width() const { return resolution().width(); }
    int height() const { return resolution().height(); }
    const eResolution& resolution() const { return mSettings.fRes; }
    SDL_Window* window() const { return mSdlWindow; }
    SDL_Renderer* renderer() const { return mSdlRenderer; }

    void setResolution(const eResolution& res);
    void setFullscreen(const bool f);

    void startGameAction(eGameBoard* const board,
                         const eGameWidgetSettings& settings);
    void startGameAction(const stdsptr<eCampaign>& c,
                         const eGameWidgetSettings& settings);
    void startGameAction(const eAction& a);
    void episodeFinished();
    void adventureComplete();
    void episodeLost();

    bool saveGame(const std::string& path);
    bool loadGame(const std::string& path);
    void closeGame();

    void showRosterOfLeaders();
    void showMenuLoading();
    void showMainMenu();
    void showSettingsMenu();
    void showChooseGameMenu();
    void showChooseGameEditMenu();
    void showGame(const stdsptr<eCampaign>& c,
                  const eGameWidgetSettings& settings);
    void showGame(eGameBoard* b,
                  const eGameWidgetSettings& settings);
    void showWorld();

    eWidget* currentWidget() const { return mWidget; }
    eWorldWidget* worldWidget() const { return mWW; }

    const eSettings& settings() const { return mSettings; }

    void execDialog(eWidget* const d,
                    const bool closable = true,
                    const eAction& closeFunc = nullptr,
                    eWidget* const parent = nullptr);

    void showEpisodeIntroduction(const stdsptr<eCampaign>& c = nullptr);
    const stdsptr<eCampaign>& campaign() const { return mCampaign; }

    const std::string& leader() const { return mLeader; }
    void setLeader(const std::string& leader) { mLeader = leader; }
    std::string leaderSaveDir() const;
private:
    void clearWidgets();

    eSettings mSettings;

    std::string mLeader;

    bool mQuit = false;
    bool mFirstFullscrenSetting = true;
    bool mFirstResolutionSetting = true;

    std::vector<eSlot> mSlots;

    int mShiftPressed = 0;
    int mCtrlPressed = 0;

    stdsptr<eCampaign> mCampaign;
    eGameBoard* mBoard = nullptr;
    eGameWidget* mGW = nullptr;
    eWorldWidget* mWW = nullptr;

    eWidget* mWidget = nullptr;

    void handleFingerEvent(const SDL_Event& e);
    void handleAutotest(uint32_t nowMs);
    eMouseButton mButton{eMouseButton::none};
    eMouseButton mButtons{eMouseButton::none};
    int mPointerX = 0;
    int mPointerY = 0;
    bool mPointerFromTouch = false;
    bool mTouchEnabled = false;
    std::shared_ptr<eTouchSink> mTouchSink;
    std::shared_ptr<eTouchGestures> mTouch;

    SDL_Window* mSdlWindow = nullptr;
    SDL_Renderer* mSdlRenderer = nullptr;
};

#endif // EMAINWINDOW_H
