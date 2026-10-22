#ifndef EGAMEWIDGET_H
#define EGAMEWIDGET_H

#include <deque>

#include "emainwidget.h"
#include "eframedlabel.h"

#include "engine/etile.h"

#include "textures/eterraintextures.h"
#include "textures/ebuildingtextures.h"

#include "buildings/epatrolbuilding.h"

#include "widgets/egamemenu.h"
#include "widgets/earmymenu.h"
#include "egamemainmenu.h"
#include "etopbarwidget.h"

#include "eviewmode.h"
#include "emessage.h"
#include "echeckbox.h"

#include "engine/eeventdata.h"

class eTerrainEditMenu;
class eDomesticatedAnimal;
struct eSanctBlueprint;
class eWorldWidget;
struct eGodMessages;
struct eHeroMessages;
class eMessageBox;
class eGameBoard;
class eAgoraBase;
class eInfoWidget;
class eFramedButton;

enum class eAgoraOrientation;
enum class eGodType;
enum class eHeroType;
enum class eGodQuestId;
enum class eWorldDirection;

using eBuildingCreator = std::function<stdsptr<eBuilding>()>;

struct eSavedMessage {
    eEventData fEd;
    eMessage fMsg;
};

struct eGameWidgetSettings {
    bool fPaused = false;
    int fSpeedId = 1;
    int fSpeed = 10;
    int fDX = 0;
    int fDY = 0;
    eTileSize fTileSize = eTileSize::s30;
    eWorldDirection fDir = eWorldDirection::N;
    std::map<int, std::pair<int, int>> fBookmarks;

    void read(eReadStream& src) {
        src >> fPaused;
        src >> fSpeedId;
        src >> fSpeed;
        src >> fDX;
        src >> fDY;
        src >> fTileSize;
        src >> fDir;

        int n;
        src >> n;
        for(int i = 0; i < n; i++) {
            int id;
            src >> id;
            auto& b = fBookmarks[id];
            src >> b.first;
            src >> b.second;
        }
    }

    void write(eWriteStream& dst) const {
        dst << fPaused;
        dst << fSpeedId;
        dst << fSpeed;
        dst << fDX;
        dst << fDY;
        dst << fTileSize;
        dst << fDir;

        dst << fBookmarks.size();
        for(const auto& b : fBookmarks) {
            dst << b.first;
            dst << b.second.first;
            dst << b.second.second;
        }
    }
};

class eGameWidget : public eMainWidget {
public:
    eGameWidget(eMainWindow* const window);
    ~eGameWidget();

    void initialize();

    // Touch integration (see platform/etouchgestures.h).
    bool buildToolActive() const;
    // One discrete zoom step (texture size) keeping the map point under
    // (x, y) fixed, modulo map-edge clamping.
    void zoomStepAt(const int dir, const int x, const int y);
    // Port zoom: effective zoom z = texture base (s15 = 0.5, s30 = 1) times
    // a render scale for the world layer (rendered off-screen, then scaled).
    // z = 1 is upstream's default view and uses no off-screen pass.
    double zoom() const;
    // Set zoom keeping the map point under widget pixel (x, y) fixed.
    void setZoomAt(double z, double x, double y);
    // Continuous pinch: begin pins the map point under (x, y); update moves
    // that point to (x, y) and sets zoom = zoom at begin * scale.
    void pinchBegin(float x, float y);
    void pinchUpdate(float x, float y, float scale);
    void pinchEnd();
    double worldScale() const { return mWorldScale; }
    int gameMenuWidth() const { return mGm ? mGm->width() : 0; }
    static double sMaxZoom;
    void setSpeedId(const int id);
    // "dx=.. dy=.. tile=WxH mode=.." for automated tests.
    std::string debugCameraState() const;
    // Mouse-at-screen-edge scrolling; disabled on touch screens.
    static bool sEdgeScrollEnabled;
    // On-screen buttons for actions that are keyboard-only upstream
    // (pause, speed -/+, rotate); enabled on touch screens.
    static bool sTouchBarEnabled;
    void togglePause() { switchPause(); }
    bool paused() const { return mPaused; }
    void changeSpeed(const int delta);
    void rotateView();
    // Number of touch-bar children; excluded from the "dialog open" child
    // count that pauses the simulation.
    int mTouchBarButtons = 0;
    // Touch "X": closes an info panel, leaves the build tool, ends patrol
    // editing, clears army selection. Returns false if nothing was active.
    bool touchCancel();
    bool touchCancelAvailable() const;
    eWidget* mTouchCancelButton = nullptr;
    eFramedButton* mSpeedButton = nullptr;
    void updateSpeedButton();

    // Two-step touch placement (port, see docs/TOUCH_CONTROLS.md): taps and
    // drags only move a preview of the building / road / area; the confirm
    // button next to it builds. Coordinates are widget (screen) pixels.
    static bool sBuildConfirm;
    bool touchPreviewSupported() const;
    bool touchPreviewActive() const { return mPreview.fActive; }
    bool touchPreviewAt(int x, int y);
    void touchPreviewTap(int x, int y);
    void touchPreviewArea(int sx, int sy, int x, int y);
    void touchPreviewDrag(int sx, int sy, int x, int y, bool begin);
    void touchPreviewConfirm();
    void touchPreviewClear();

    void pixToId(const int pixX, const int pixY,
                 int& idX, int& idY) const;

    void setViewMode(const eViewMode m);
    eViewMode viewMode() const { return mViewMode; }

    void viewFraction(const double fx, const double fy);
    void viewTile(eTile* const tile);
    eTile* viewedTile() const;
    bool tileVisible(eTile* const tile) const;
    eCityId viewedCity() const;

    void showBuyCity(const eCityId cid);
    void hideBuyCity();

    void setBoard(eGameBoard* const board);

    eGameWidgetSettings settings() const;
    void setSettings(const eGameWidgetSettings& s);

    void updateRequestButtons();
    void showTip(const ePlayerCityTarget& target,
                 const std::string& tip);
    void showQuestion(const std::string& title,
                      const std::string& q,
                      const eAction& action);

    void updateViewBoxSize();
    void updateTopBottomAltitude();
    void updateMinMaxAltitude();
    void updateMaps(const bool totalUpdate);
    void updateMaps(const std::vector<eTile*>& tiles);
    void updateCitiesOnBoard();

    void setWorldDirection(const eWorldDirection dir);

    void centerDialog(eWidget* const d);
    void openDialog(eWidget* const d) override;
protected:
    void paintEvent(ePainter& p) override;
    void paintWorld(ePainter& p);

    bool keyPressEvent(const eKeyPressEvent& e) override;
    bool mousePressEvent(const eMouseEvent& e) override;
    bool mouseMoveEvent(const eMouseEvent& e) override;
    bool mouseReleaseEvent(const eMouseEvent& e) override;
    bool mouseWheelEvent(const eMouseWheelEvent& e) override;
private:
    void renderTargetsReset() override;
    void initializeNumbers();

    void drawXY(int tx, int ty,
                double& rx, double& ry,
                const int wSpan, const int hSpan,
                const int a);

    void showGoals();

    void setDX(const int dx);
    void setDY(const int dy);

    // World-layer geometry in world pixels (screen pixels / mWorldScale).
    int worldViewW() const; // map area left of the game menu
    int worldViewH() const;
    eMouseEvent toWorld(const eMouseEvent& e) const;
    double mWorldScale = 1.0;
    SDL_Texture* mWorldTarget = nullptr;
    int mWorldTargetW = 0;
    int mWorldTargetH = 0;
    struct eTouchPreview {
        bool fActive = false;
        bool fArea = false;          // row / area (start..end), else single
        eBuildingMode fMode = eBuildingMode::none;
        int fSTX = 0, fSTY = 0;      // start tile (area)
        int fETX = 0, fETY = 0;      // end tile = single building tile
        double fAX = 0, fAY = 0;     // anchor in world pixels (map coords)
        // drag bookkeeping
        bool fDragEnd = false;       // area: drag moves the end only
        int fDTX = 0, fDTY = 0;      // finger tile at drag start
        int fBSTX = 0, fBSTY = 0, fBETX = 0, fBETY = 0;
        double fBAX = 0, fBAY = 0;
    };
    eTouchPreview mPreview;
    eWidget* mPreviewPanel = nullptr;
    void screenToTile(int x, int y, int& tx, int& ty) const;
    void applyTouchPreview();
    void updatePreviewPanel();
    double mPinchZ0 = 1.0;
    double mPinchTX = 0.0; // map point (in tile widths/heights) under pinch
    double mPinchTY = 0.0;
    float mPinchLastX = 0.f;
    float mPinchLastY = 0.f;
    void clampViewBox();

    void setBookmark(const int id);
    void viewBookmark(const int id);

    using eApply = std::function<void(eTile* const)>;
    eApply editFunc();
    bool buildMouseRelease();

    using eTileAction = std::function<void(eTile* const)>;
    void iterateOverVisibleTiles(const eTileAction& a);

    void setTileSize(const eTileSize size);

    using eSpecialRequirement = std::function<bool(eTile*)>;
    bool canBuildVendor(const int tx, const int ty,
                        const eResourceType resType) const;
    bool canBuildFishery(const int tx, const int ty,
                         eDiagonalOrientation& o) const;
    bool waterTileHasAccessToSea(const int tx, const int ty) const;
    bool canBuildTriremeWharf(const int tx, const int ty,
                              eDiagonalOrientation& o) const;
    bool canBuildPier(const int tx, const int ty,
                      eDiagonalOrientation& o, const eCityId cid,
                      const ePlayerId pid, const bool forestAllowed) const;

    std::vector<eTile*> agoraBuildPlaceBR(eTile* const tile,
                                          const eCityId cid,
                                          const ePlayerId pid) const;
    std::vector<eTile*> agoraBuildPlaceTL(eTile* const tile,
                                          const eCityId cid,
                                          const ePlayerId pid) const;
    std::vector<eTile*> agoraBuildPlaceBL(eTile* const tile,
                                          const eCityId cid,
                                          const ePlayerId pid) const;
    std::vector<eTile*> agoraBuildPlaceTR(eTile* const tile,
                                          const eCityId cid,
                                          const ePlayerId pid) const;
    std::vector<eTile*> agoraBuildPlaceIter(
            eTile* const tile, const bool grand,
            eAgoraOrientation& bt,
            const eCityId cid,
            const ePlayerId pid) const;

    std::vector<ePatrolGuide>::iterator
        findGuide(const int tx, const int ty);

    void handleEvent(const eEvent e, eEventData& ed);
    void handleGodQuestEvent(eEventData& ed,
                             const bool fulfilled);
    void handleGodVisitEvent(eEventData& ed);
    void handleGodInvasionEvent(eEventData& ed);
    void handleGodHelpEvent(eEventData& ed);
    void handleSanctuaryComplete(eEventData& ed);
    void handleMonsterUnleashEvent(eEventData& ed);
    void handleMonsterInvasionInitialEvent(eEventData& ed);
    void handleMonsterInvasion24Event(eEventData& ed);
    void handleMonsterInvasion12Event(eEventData& ed);
    void handleMonsterInvasion6Event(eEventData& ed);
    void handleMonsterInvasion1Event(eEventData& ed);
    void handleMonsterInvasionEvent(eEventData& ed);
    void handleMonsterSlainEvent(eEventData& ed);
    void handleHeroArrivalEvent(eEventData& ed);

    void handleMonsterInCityEvent(eEventData& ed);

    void mapDimensions(int& mdx, int& mdy) const;
    void viewBoxSize(double& fx, double& fy) const;
    void viewedFraction(double& fx, double& fy) const;
    void tileViewFraction(eTile* const tile,
                          double& xf, double& yf) const;

    void updateMinimap();

    int rotationId() const;
    int hippodromeId() const;
    void updateHippodromeIds();

    void showMessage(eEventData& ed, const eMessage& msg,
                     const bool prepend = false);
    void showMessage(eEventData& ed, const eMessageType& msg,
                     const bool prepend = false);
    void showMessage(eEventData& ed, const eEventMessageType& msg,
                     const bool prepend = false);

    void updateTipPositions();

    bool roadPath(std::vector<eOrientation>& path);
    bool columnPath(std::vector<eOrientation>& path);
    bool bridgeTiles(eTile* const t, const eTerrain terr,
                     std::vector<eTile*>& tiles,
                     bool& rotated);
    bool canBuildAvenue(eTile* const t, const eCityId cid,
                        const ePlayerId pid,
                        const bool forestAllowed) const;

    bool inErase(const int tx, const int ty);
    bool inErase(const SDL_Rect& rect);
    bool inErase(eAgoraBase* const a);
    bool inErase(eBuilding* const b);

    bool inPatrolBuildingHover(eBuilding* const b);

    void setArmyMenuVisible(const bool v);

    void scheduleConnectedTerrainUpdate(eTile* const startTile);
    void updateTerrainTextures(eTile* const tile,
                               const eTerrainTextures& trrTexs,
                               const eBuildingTextures& builTexs);
    void updateTerrainTextures();

    void updatePatrolPath();
    void setPatrolBuilding(ePatrolBuildingBase* const pb);

    eInfoWidget* openInfoWidget(eBuilding* const b);
    eInfoWidget* openInfoWidget(const std::vector<eCharacter *> chars);

    void switchPause();

    stdsptr<eTexture> getBasementTexture(
            const int tx, const int ty, eBuilding* const d,
            const eTerrainTextures& trrTexs, const eWorldDirection dir,
            const int boardw, const int boardh);

    std::vector<eTile*> selectedTiles() const;

    eMouseButton mPressedButtons = eMouseButton::none;

    bool mEditorMode = false;
    bool mEditorShowBuildings = false;
    bool mTerrainEditMode = false;

    bool mRotate = false;
    int mRotateId = 0;

    const int sSpeeds[6] = {2, 10, 25, 50, 100, 100};
    const int sMaxSpeedId = int(std::size(sSpeeds)) - 1;

    bool mPaused = false;
    bool mLocked = false;
    int mFrame{0};
    int mRotateFrame{0};
    std::vector<int> mValiableHippodromePieces;
    int mTime{0};
    int mSpeedId = 1;
    int mSpeed = sSpeeds[mSpeedId];
    std::map<int, std::pair<int, int>> mBookmarks;

    int mWheel = 0;

    int mMinAltitude = 0;
    int mMaxAltitude = 0;

    int mTopMinAltitude = 0;
    int mBottomMaxAltitude = 0;

    int mDX = 0;
    int mDY = 0;

    bool mLeftPressed = false;
    bool mMovedSincePress = false;

    int mHoverX = -1;
    int mHoverY = -1;
    int mHoverTX = -1;
    int mHoverTY = -1;
    int mPressedX = -1;
    int mPressedY = -1;
    int mPressedTX = -1;
    int mPressedTY = -1;
    int mLastX = -1;
    int mLastY = -1;

    eViewMode mViewMode = eViewMode::defaultView;

    eTileSize mTileSize = eTileSize::s30;
    int mTileW = 60;
    int mTileH = 30;

    int mUpdateRect = 0;
    std::vector<SDL_Rect> mUpdateRects;
    stdptr<eGameBoard> mBoard;

    bool mDrawElevation = true;
    stdptr<ePatrolBuildingBase> mPatrolBuilding;
    std::vector<eTile*> mPatrolPath;
    std::vector<eTile*> mExcessPatrolPath;
    std::vector<eTile*> mPatrolPath1;
    std::vector<eTile*> mExcessPatrolPath1;
    eWidget* mPatrolPathWid = nullptr;
    std::vector<ePatrolGuide> mSavedGuides;

    eFramedLabel* mPausedLabel = nullptr;

    eTopBarWidget* mTopBar = nullptr;
    eInfoWidget* mInfoWidget = nullptr;
    eMessageBox* mMsgBox = nullptr;
    std::deque<eSavedMessage> mSavedMsgs;
    eTerrainEditMenu* mTem = nullptr;
    eGameMenu* mGm = nullptr;
    eArmyMenu* mAm = nullptr;

    eWorldWidget* mWW = nullptr;

    struct eTip {
        ePlayerCityTarget fTarget;
        std::string fText;
        eWidget* fWid = nullptr;
        int fLastFrame = 0;
    };

    std::deque<eTip> mTips;

    std::map<eTileSize, std::vector<stdsptr<eTexture>>> mNumbers;
    std::vector<eTile*> mInflTiles;
    std::vector<eTile*> mHoverTiles;

    eTile* mViewedTile = nullptr;
    eCityId mViewedCityId = eCityId::neutralFriendly;
    bool mUpdateViewedTileScheduled = true;

    eWidget* mBuyCityWidget = nullptr;
    eLabel* mBuyCityName = nullptr;
    eLabel* mBuyCityPrice = nullptr;
    eFramedButton* mBuyCityButton = nullptr;
};

#endif // EGAMEWIDGET_H
