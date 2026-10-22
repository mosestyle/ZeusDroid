#ifndef ETOUCHGESTURES_H
#define ETOUCHGESTURES_H

#include <cstdint>
#include <map>

// Touch gesture recogniser for the city-builder UI.
//
// It is deliberately independent of SDL and of eZeus widgets so it can be
// unit-tested on any host. The window feeds it finger events in window pixel
// coordinates; it emits abstract pointer actions through eTouchSink, which the
// window translates into eZeus mouse events (left = select/build,
// right = info/cancel, middle-drag = camera pan) plus anchored zoom steps.
//
// State machine (see docs/TOUCH_CONTROLS.md):
//   Idle -> UiPress            finger lands on a UI widget: immediate left
//                              press, moves are left-drags, lift = release
//   Idle -> MapCandidate       finger lands on the map: nothing sent yet
//   MapCandidate -> Tap        lift within slop/time: hover+left click
//   MapCandidate -> LongPress  held still, no build tool: hover+right click
//   MapCandidate -> Pan        moved beyond slop (no build tool, or
//                              hold-to-draw): middle-drag
//   MapCandidate -> BuildDrag  build tool active and held still for
//                              fBuildHoldMs (hold-to-draw, default) or moved
//                              beyond slop (fHoldToDraw = false): left-drag
//                              starting at the original touch point
//   Idle -> ListCandidate      finger lands on a scrollable list: nothing
//                              sent yet; lift within slop = tap (left click
//                              at the touch-down point)
//   ListCandidate -> ListScroll moved beyond slop: the list follows the finger
//   Confirm placement (fBuildConfirm and sink.confirmPlacement()): nothing is
//   built by a gesture. Tap = previewTap (put / move the preview there; the
//   widget's confirm button builds); hold + drag = previewArea (row/area/road preview);
//   drag that starts on the preview = previewDrag (move it); other drags pan.
//   MapCandidate/Pan -> Pinch  second finger: continuous zoom + pan
//   Pinch -> WaitAllUp         a finger lifts: nothing more until all lift
class eTouchSink {
public:
    enum class eButton { left, right, middle };
    enum class eTarget { map, ui, scrollList };

    virtual ~eTouchSink() = default;

    virtual eTarget targetAt(int x, int y) = 0;
    virtual bool buildToolActive() = 0;

    virtual void pointerMove(int x, int y) = 0;
    virtual void pointerPress(eButton b, int x, int y) = 0;
    virtual void pointerRelease(eButton b, int x, int y) = 0;
    // Two-finger gesture (continuous zoom + pan). pinchBegin pins the map
    // point under (x, y); pinchUpdate moves that point to (x, y) and sets the
    // zoom to (zoom at begin) * scale; pinchEnd finishes the gesture.
    virtual void pinchBegin(float x, float y) = 0;
    virtual void pinchUpdate(float x, float y, float scale) = 0;
    virtual void pinchEnd() {}
    // Scroll the list that was under (downX, downY) by dy pixels
    // (dy > 0 = content moves up, i.e. later rows come into view).
    virtual void listScroll(int downX, int downY, int dy) {
        (void)downX; (void)downY; (void)dy;
    }
    // Box selection (army): a one-finger drag on the map is a left-drag
    // (selection rectangle) instead of a pan while this returns true.
    virtual bool boxSelectActive() { return false; }
    // Draw mode of the active build tool: a one-finger drag draws a road /
    // wall / row / area from the touch-down point at once (no hold needed);
    // the map is panned with two fingers.
    virtual bool drawModeActive() { return false; }
    // Two-step placement (see fBuildConfirm).
    virtual bool confirmPlacement() { return false; }
    virtual bool previewAt(int x, int y) { (void)x; (void)y; return false; }
    virtual void previewTap(int x, int y) { (void)x; (void)y; }
    virtual void previewArea(int sx, int sy, int x, int y) {
        (void)sx; (void)sy; (void)x; (void)y;
    }
    // begin = first call of this drag (the preview's position is captured).
    virtual void previewDrag(int sx, int sy, int x, int y, bool begin) {
        (void)sx; (void)sy; (void)x; (void)y; (void)begin;
    }
};

struct eTouchConfig {
    // Movement (window px) below which a touch is still a tap.
    float fTapSlop = 24.f;
    // Hold time for long-press (right click / info).
    uint32_t fLongPressMs = 500;
    // Finger-distance change (ratio) before a two-finger drag starts
    // zooming; below it the gesture only pans. (Before 0.1.4 this was the
    // ratio for one discrete zoom step.)
    float fPinchStepRatio = 1.04f;
    // Pan sensitivity multiplier (1 = finger follows the map exactly).
    float fPanSensitivity = 1.f;
    // Build tools: true = one-finger drag pans, hold (fBuildHoldMs) then drag
    // draws roads / places rows of buildings; false = drag draws immediately.
    bool fHoldToDraw = true;
    uint32_t fBuildHoldMs = 300;
    // Two-step placement: gestures only move a preview; a confirm button (or
    // a tap on the preview) builds. Applies when sink.confirmPlacement().
    bool fBuildConfirm = true;
};

class eTouchGestures {
public:
    enum class eState {
        idle,
        uiPress,
        mapCandidate,
        longPress,
        pan,
        buildDrag,
        pinch,
        listCandidate,
        listScroll,
        previewDrag,
        waitAllUp
    };

    eTouchGestures(eTouchSink& sink, const eTouchConfig& config = eTouchConfig());

    void fingerDown(int64_t id, float x, float y, uint32_t tMs);
    void fingerMove(int64_t id, float x, float y, uint32_t tMs);
    void fingerUp(int64_t id, float x, float y, uint32_t tMs);
    // App lost focus / was paused: release anything held without clicking.
    void cancel(uint32_t tMs);
    // Called every frame; fires the long-press timer.
    void update(uint32_t tMs);

    eState state() const { return mState; }
    void setConfig(const eTouchConfig& c) { mConfig = c; }
    int fingerCount() const { return static_cast<int>(mFingers.size()); }
    const eTouchConfig& config() const { return mConfig; }

    static const char* sStateName(eState s);
private:
    struct Finger {
        float fX = 0.f;
        float fY = 0.f;
        float fStartX = 0.f;
        float fStartY = 0.f;
        uint32_t fDownMs = 0;
    };

    void enterPinch();
    void pinchMoved();
    void endPan(float x, float y);
    void centroid(float& x, float& y) const;
    float spread() const;

    eTouchSink& mSink;
    eTouchConfig mConfig;
    eState mState = eState::idle;
    std::map<int64_t, Finger> mFingers;
    int64_t mPrimary = -1;

    // Pan / pinch bookkeeping. Pan positions are tracked in float and emitted
    // as accumulated integers so slow drags do not lose sub-pixel motion.
    float mPanX = 0.f;
    float mPanY = 0.f;
    int mPanEmittedX = 0;
    int mPanEmittedY = 0;
    float mPinchBase = 0.f;
    bool mPinchZooming = false;
    bool mPinchDirty = false;
    float mListLastY = 0.f;
    float mListEmitted = 0.f;
    bool mOnPreview = false;   // map touch started on the pending preview
    bool mPreviewArea = false; // buildDrag is a confirm-mode area preview
    bool confirmMode();
    float mPinchStartX = 0.f;
    float mPinchStartY = 0.f;
};

#endif // ETOUCHGESTURES_H
