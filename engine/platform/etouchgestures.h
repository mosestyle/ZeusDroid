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
//   MapCandidate -> LongPress  held still: hover+right click (info/cancel)
//   MapCandidate -> Pan        moved beyond slop, no build tool: middle-drag
//   MapCandidate -> BuildDrag  moved beyond slop with build tool: left-drag
//                              starting at the original touch point
//   MapCandidate/Pan -> Pinch  second finger: zoom steps + two-finger pan
//   Pinch -> WaitAllUp         a finger lifts: nothing more until all lift
class eTouchSink {
public:
    enum class eButton { left, right, middle };
    enum class eTarget { map, ui };

    virtual ~eTouchSink() = default;

    virtual eTarget targetAt(int x, int y) = 0;
    virtual bool buildToolActive() = 0;

    virtual void pointerMove(int x, int y) = 0;
    virtual void pointerPress(eButton b, int x, int y) = 0;
    virtual void pointerRelease(eButton b, int x, int y) = 0;
    // dir > 0 zoom in, dir < 0 zoom out; anchored at (x, y).
    virtual void zoomStep(int dir, int x, int y) = 0;
};

struct eTouchConfig {
    // Movement (window px) below which a touch is still a tap.
    float fTapSlop = 24.f;
    // Hold time for long-press (right click / info).
    uint32_t fLongPressMs = 500;
    // Distance ratio between two fingers that triggers one zoom step.
    float fPinchStepRatio = 1.30f;
    // Pan sensitivity multiplier (1 = finger follows the map exactly).
    float fPanSensitivity = 1.f;
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
    int fingerCount() const { return static_cast<int>(mFingers.size()); }
    const eTouchConfig& config() const { return mConfig; }
    void setConfig(const eTouchConfig& c) { mConfig = c; }

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
};

#endif // ETOUCHGESTURES_H
