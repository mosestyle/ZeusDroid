#include "etouchgestures.h"

#include <cmath>

namespace {
int ri(const float v) { return static_cast<int>(std::lround(v)); }
}

eTouchGestures::eTouchGestures(eTouchSink& sink, const eTouchConfig& config) :
    mSink(sink), mConfig(config) {}

const char* eTouchGestures::sStateName(const eState s) {
    switch(s) {
    case eState::idle: return "idle";
    case eState::uiPress: return "uiPress";
    case eState::mapCandidate: return "mapCandidate";
    case eState::longPress: return "longPress";
    case eState::pan: return "pan";
    case eState::buildDrag: return "buildDrag";
    case eState::pinch: return "pinch";
    case eState::waitAllUp: return "waitAllUp";
    }
    return "?";
}

void eTouchGestures::centroid(float& x, float& y) const {
    x = 0.f;
    y = 0.f;
    if(mFingers.empty()) return;
    int n = 0;
    for(const auto& f : mFingers) {
        x += f.second.fX;
        y += f.second.fY;
        if(++n == 2) break;
    }
    x /= n;
    y /= n;
}

float eTouchGestures::spread() const {
    if(mFingers.size() < 2) return 0.f;
    auto it = mFingers.begin();
    const auto& a = it->second;
    ++it;
    const auto& b = it->second;
    return std::hypot(a.fX - b.fX, a.fY - b.fY);
}

void eTouchGestures::fingerDown(const int64_t id, const float x, const float y,
                                const uint32_t tMs) {
    Finger f;
    f.fX = f.fStartX = x;
    f.fY = f.fStartY = y;
    f.fDownMs = tMs;
    mFingers[id] = f;

    switch(mState) {
    case eState::idle: {
        mPrimary = id;
        const auto target = mSink.targetAt(ri(x), ri(y));
        if(target == eTouchSink::eTarget::ui) {
            mState = eState::uiPress;
            mSink.pointerMove(ri(x), ri(y));
            mSink.pointerPress(eTouchSink::eButton::left, ri(x), ri(y));
        } else {
            mState = eState::mapCandidate;
        }
    } break;
    case eState::mapCandidate:
        // Second finger before anything was dispatched: clean pinch.
        enterPinch();
        break;
    case eState::pan:
        // Convert one-finger pan into pinch without a camera jump: finish the
        // current drag where the primary finger is, then restart at centroid.
        endPan(mPanX, mPanY);
        enterPinch();
        break;
    case eState::uiPress:
    case eState::longPress:
    case eState::buildDrag:
    case eState::pinch:
    case eState::waitAllUp:
        // Extra fingers are ignored in these states (pinch uses the first two).
        break;
    }
}

void eTouchGestures::enterPinch() {
    mState = eState::pinch;
    float cx, cy;
    centroid(cx, cy);
    mPinchBase = spread();
    mPanX = cx;
    mPanY = cy;
    mPanEmittedX = ri(cx);
    mPanEmittedY = ri(cy);
    mSink.pointerMove(mPanEmittedX, mPanEmittedY);
    mSink.pointerPress(eTouchSink::eButton::middle, mPanEmittedX, mPanEmittedY);
}

void eTouchGestures::endPan(const float x, const float y) {
    (void)x;
    (void)y;
    mSink.pointerRelease(eTouchSink::eButton::middle, mPanEmittedX, mPanEmittedY);
}

void eTouchGestures::pinchMoved() {
    float cx, cy;
    centroid(cx, cy);
    // Two-finger pan.
    const float s = mConfig.fPanSensitivity;
    const float tx = mPanEmittedX + (cx - mPanX) * s;
    const float ty = mPanEmittedY + (cy - mPanY) * s;
    const int ix = ri(tx);
    const int iy = ri(ty);
    if(ix != mPanEmittedX || iy != mPanEmittedY) {
        mPanX += (ix - mPanEmittedX) / (s == 0.f ? 1.f : s);
        mPanY += (iy - mPanEmittedY) / (s == 0.f ? 1.f : s);
        mPanEmittedX = ix;
        mPanEmittedY = iy;
        mSink.pointerMove(ix, iy);
    }
    // Zoom steps.
    const float d = spread();
    if(mPinchBase <= 1.f) {
        mPinchBase = d;
        return;
    }
    const float ratio = d / mPinchBase;
    const float step = mConfig.fPinchStepRatio;
    int dir = 0;
    if(ratio >= step) dir = 1;
    else if(ratio <= 1.f/step) dir = -1;
    if(dir != 0) {
        mPinchBase = d;
        // End the pan drag around the zoom so the widget re-anchors cleanly.
        mSink.pointerRelease(eTouchSink::eButton::middle, mPanEmittedX, mPanEmittedY);
        mSink.zoomStep(dir, ri(cx), ri(cy));
        mPanX = cx;
        mPanY = cy;
        mPanEmittedX = ri(cx);
        mPanEmittedY = ri(cy);
        mSink.pointerMove(mPanEmittedX, mPanEmittedY);
        mSink.pointerPress(eTouchSink::eButton::middle, mPanEmittedX, mPanEmittedY);
    }
}

void eTouchGestures::fingerMove(const int64_t id, const float x, const float y,
                                const uint32_t tMs) {
    (void)tMs;
    const auto it = mFingers.find(id);
    if(it == mFingers.end()) return;
    auto& f = it->second;
    f.fX = x;
    f.fY = y;

    switch(mState) {
    case eState::uiPress:
        if(id == mPrimary) mSink.pointerMove(ri(x), ri(y));
        break;
    case eState::mapCandidate: {
        if(id != mPrimary) break;
        const float d = std::hypot(x - f.fStartX, y - f.fStartY);
        if(d < mConfig.fTapSlop) break;
        if(mSink.buildToolActive()) {
            mState = eState::buildDrag;
            const int sx = ri(f.fStartX);
            const int sy = ri(f.fStartY);
            mSink.pointerMove(sx, sy);
            mSink.pointerPress(eTouchSink::eButton::left, sx, sy);
            mSink.pointerMove(ri(x), ri(y));
        } else {
            mState = eState::pan;
            // Start the drag at the touch-down point so the first move already
            // carries the full slop distance: the map stays under the finger.
            mPanX = f.fStartX;
            mPanY = f.fStartY;
            mPanEmittedX = ri(f.fStartX);
            mPanEmittedY = ri(f.fStartY);
            mSink.pointerMove(mPanEmittedX, mPanEmittedY);
            mSink.pointerPress(eTouchSink::eButton::middle, mPanEmittedX, mPanEmittedY);
            const float s = mConfig.fPanSensitivity;
            mPanEmittedX = ri(f.fStartX + (x - f.fStartX) * s);
            mPanEmittedY = ri(f.fStartY + (y - f.fStartY) * s);
            mPanX = x;
            mPanY = y;
            mSink.pointerMove(mPanEmittedX, mPanEmittedY);
        }
    } break;
    case eState::pan: {
        if(id != mPrimary) break;
        const float s = mConfig.fPanSensitivity;
        const float tx = mPanEmittedX + (x - mPanX) * s;
        const float ty = mPanEmittedY + (y - mPanY) * s;
        const int ix = ri(tx);
        const int iy = ri(ty);
        if(ix != mPanEmittedX || iy != mPanEmittedY) {
            const float k = (s == 0.f ? 1.f : s);
            mPanX += (ix - mPanEmittedX) / k;
            mPanY += (iy - mPanEmittedY) / k;
            mPanEmittedX = ix;
            mPanEmittedY = iy;
            mSink.pointerMove(ix, iy);
        }
    } break;
    case eState::buildDrag:
        if(id == mPrimary) mSink.pointerMove(ri(x), ri(y));
        break;
    case eState::pinch:
        pinchMoved();
        break;
    case eState::longPress:
    case eState::waitAllUp:
    case eState::idle:
        break;
    }
}

void eTouchGestures::fingerUp(const int64_t id, const float x, const float y,
                              const uint32_t tMs) {
    const auto it = mFingers.find(id);
    if(it == mFingers.end()) return;
    it->second.fX = x;
    it->second.fY = y;
    const Finger f = it->second;

    switch(mState) {
    case eState::uiPress:
        if(id == mPrimary) {
            mSink.pointerMove(ri(x), ri(y));
            mSink.pointerRelease(eTouchSink::eButton::left, ri(x), ri(y));
            mState = eState::waitAllUp;
        }
        break;
    case eState::mapCandidate:
        if(id == mPrimary) {
            // Tap at the touch-down point (ignores sub-slop jitter).
            const int sx = ri(f.fStartX);
            const int sy = ri(f.fStartY);
            (void)tMs;
            mSink.pointerMove(sx, sy);
            mSink.pointerPress(eTouchSink::eButton::left, sx, sy);
            mSink.pointerRelease(eTouchSink::eButton::left, sx, sy);
            mState = eState::waitAllUp;
        }
        break;
    case eState::pan:
        if(id == mPrimary) {
            endPan(x, y);
            mState = eState::waitAllUp;
        }
        break;
    case eState::buildDrag:
        if(id == mPrimary) {
            mSink.pointerMove(ri(x), ri(y));
            mSink.pointerRelease(eTouchSink::eButton::left, ri(x), ri(y));
            mState = eState::waitAllUp;
        }
        break;
    case eState::pinch:
        mSink.pointerRelease(eTouchSink::eButton::middle, mPanEmittedX, mPanEmittedY);
        mState = eState::waitAllUp;
        break;
    case eState::longPress:
    case eState::waitAllUp:
    case eState::idle:
        break;
    }

    mFingers.erase(id);
    if(mFingers.empty()) {
        mState = eState::idle;
        mPrimary = -1;
    }
}

void eTouchGestures::update(const uint32_t tMs) {
    if(mState != eState::mapCandidate) return;
    const auto it = mFingers.find(mPrimary);
    if(it == mFingers.end()) return;
    const auto& f = it->second;
    if(tMs - f.fDownMs < mConfig.fLongPressMs) return;
    const int sx = ri(f.fStartX);
    const int sy = ri(f.fStartY);
    mSink.pointerMove(sx, sy);
    mSink.pointerPress(eTouchSink::eButton::right, sx, sy);
    mSink.pointerRelease(eTouchSink::eButton::right, sx, sy);
    mState = eState::longPress;
}

void eTouchGestures::cancel(const uint32_t tMs) {
    (void)tMs;
    switch(mState) {
    case eState::uiPress:
    case eState::buildDrag: {
        // Releasing outside of any meaningful target is not possible for a
        // UI press, so release in place; for build drags cancel with a right
        // click first so no road/wall gets committed.
        const auto it = mFingers.find(mPrimary);
        const int x = it == mFingers.end() ? 0 : ri(it->second.fX);
        const int y = it == mFingers.end() ? 0 : ri(it->second.fY);
        if(mState == eState::buildDrag) {
            mSink.pointerPress(eTouchSink::eButton::right, x, y);
            mSink.pointerRelease(eTouchSink::eButton::right, x, y);
        }
        mSink.pointerRelease(eTouchSink::eButton::left, x, y);
    } break;
    case eState::pan:
    case eState::pinch:
        mSink.pointerRelease(eTouchSink::eButton::middle, mPanEmittedX, mPanEmittedY);
        break;
    default:
        break;
    }
    mFingers.clear();
    mState = eState::idle;
    mPrimary = -1;
}
