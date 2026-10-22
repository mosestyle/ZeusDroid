#include "etouchgestures.h"

#include <algorithm>
#include <cmath>

namespace {
int ri(const float v) { return static_cast<int>(std::lround(v)); }
}

eTouchGestures::eTouchGestures(eTouchSink& sink, const eTouchConfig& config) :
    mSink(sink), mConfig(config) {}

bool eTouchGestures::confirmMode() {
    return mConfig.fBuildConfirm && mSink.buildToolActive() &&
           mSink.confirmPlacement();
}

const char* eTouchGestures::sStateName(const eState s) {
    switch(s) {
    case eState::idle: return "idle";
    case eState::uiPress: return "uiPress";
    case eState::mapCandidate: return "mapCandidate";
    case eState::longPress: return "longPress";
    case eState::pan: return "pan";
    case eState::buildDrag: return "buildDrag";
    case eState::pinch: return "pinch";
    case eState::listCandidate: return "listCandidate";
    case eState::listScroll: return "listScroll";
    case eState::previewDrag: return "previewDrag";
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
        } else if(target == eTouchSink::eTarget::scrollList) {
            mState = eState::listCandidate;
            mListLastY = y;
            mListEmitted = 0.f;
        } else {
            mState = eState::mapCandidate;
            mOnPreview = confirmMode() && mSink.previewAt(ri(x), ri(y));
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
    case eState::listCandidate:
    case eState::listScroll:
    case eState::previewDrag:
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
    mPinchZooming = false;
    mPinchDirty = false;
    mPinchStartX = cx;
    mPinchStartY = cy;
    mSink.pinchBegin(cx, cy);
}

void eTouchGestures::endPan(const float x, const float y) {
    (void)x;
    (void)y;
    mSink.pointerRelease(eTouchSink::eButton::middle, mPanEmittedX, mPanEmittedY);
}

void eTouchGestures::pinchMoved() {
    float cx, cy;
    centroid(cx, cy);
    const float d = spread();
    if(mPinchBase <= 1.f) {
        // Fingers started on the same spot: begin again from here.
        mPinchBase = d;
        mPinchStartX = cx;
        mPinchStartY = cy;
        mSink.pinchBegin(cx, cy);
        return;
    }
    float scale = d / mPinchBase;
    if(!mPinchZooming) {
        // Dead band so a two-finger pan does not drift the zoom.
        const float t = std::max(mConfig.fPinchStepRatio, 1.0001f);
        if(scale < t && scale > 1.f/t) scale = 1.f;
        else mPinchZooming = true;
    }
    // Pan sensitivity scales the centroid's travel.
    const float s = mConfig.fPanSensitivity;
    const float px = mPinchStartX + (cx - mPinchStartX) * s;
    const float py = mPinchStartY + (cy - mPinchStartY) * s;
    mSink.pinchUpdate(px, py, scale);
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
    case eState::listCandidate:
    case eState::listScroll: {
        if(id != mPrimary) break;
        if(mState == eState::listCandidate) {
            const float d = std::hypot(x - f.fStartX, y - f.fStartY);
            if(d < mConfig.fTapSlop) break;
            mState = eState::listScroll;
            mListLastY = f.fStartY; // first scroll carries the slop distance
        }
        // Accumulate in float so slow drags do not lose sub-pixel motion.
        mListEmitted += mListLastY - y;
        mListLastY = y;
        const int dy = static_cast<int>(mListEmitted);
        if(dy != 0) {
            mListEmitted -= dy;
            mSink.listScroll(ri(f.fStartX), ri(f.fStartY), dy);
        }
    } break;
    case eState::mapCandidate: {
        if(id != mPrimary) break;
        const float d = std::hypot(x - f.fStartX, y - f.fStartY);
        if(d < mConfig.fTapSlop) break;
        const bool draw = mSink.buildToolActive() && mSink.drawModeActive();
        if(mOnPreview && confirmMode()) {
            mState = eState::previewDrag;
            mSink.previewDrag(ri(f.fStartX), ri(f.fStartY), ri(x), ri(y), true);
        } else if(draw && confirmMode()) {
            mState = eState::buildDrag;
            mPreviewArea = true;
            mSink.previewArea(ri(f.fStartX), ri(f.fStartY), ri(x), ri(y));
        } else if(mSink.boxSelectActive() ||
                  (mSink.buildToolActive() && (!mConfig.fHoldToDraw || draw) &&
                   !confirmMode())) {
            mState = eState::buildDrag;
            mPreviewArea = false;
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
        if(id != mPrimary) break;
        if(mPreviewArea) {
            mSink.previewArea(ri(f.fStartX), ri(f.fStartY), ri(x), ri(y));
        } else {
            mSink.pointerMove(ri(x), ri(y));
        }
        break;
    case eState::previewDrag:
        if(id == mPrimary) {
            mSink.previewDrag(ri(f.fStartX), ri(f.fStartY), ri(x), ri(y), false);
        }
        break;
    case eState::pinch:
        // Applied once per frame in update(): SDL reports the two fingers in
        // separate events, and their half-applied state would look like a
        // spread change during a fast two-finger pan.
        mPinchDirty = true;
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
    case eState::listCandidate:
    case eState::mapCandidate:
        if(id == mPrimary) {
            // Tap at the touch-down point (ignores sub-slop jitter).
            const int sx = ri(f.fStartX);
            const int sy = ri(f.fStartY);
            (void)tMs;
            if(mState == eState::mapCandidate && confirmMode()) {
                mSink.previewTap(sx, sy);
                mState = eState::waitAllUp;
                break;
            }
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
            if(mPreviewArea) {
                // Stays a preview until confirmed.
                mSink.previewArea(ri(f.fStartX), ri(f.fStartY), ri(x), ri(y));
            } else {
                mSink.pointerMove(ri(x), ri(y));
                mSink.pointerRelease(eTouchSink::eButton::left, ri(x), ri(y));
            }
            mState = eState::waitAllUp;
        }
        break;
    case eState::previewDrag:
        if(id == mPrimary) {
            mSink.previewDrag(ri(f.fStartX), ri(f.fStartY), ri(x), ri(y), false);
            mState = eState::waitAllUp;
        }
        break;
    case eState::listScroll:
        if(id == mPrimary) mState = eState::waitAllUp;
        break;
    case eState::pinch:
        if(mPinchDirty) {
            mPinchDirty = false;
            pinchMoved();
        }
        mSink.pinchEnd();
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
    if(mState == eState::pinch && mPinchDirty) {
        mPinchDirty = false;
        pinchMoved();
    }
    if(mState != eState::mapCandidate) return;
    const auto it = mFingers.find(mPrimary);
    if(it == mFingers.end()) return;
    const auto& f = it->second;
    const int sx = ri(f.fStartX);
    const int sy = ri(f.fStartY);
    if(mSink.boxSelectActive()) return; // no long-press while box-selecting
    if(confirmMode()) {
        // Hold on the preview does nothing (a drag moves it); elsewhere
        // hold + drag previews a row / area.
        if(mOnPreview) return;
        if(tMs - f.fDownMs < mConfig.fBuildHoldMs) return;
        mState = eState::buildDrag;
        mPreviewArea = true;
        mSink.previewArea(sx, sy, ri(f.fX), ri(f.fY));
        return;
    }
    if(mConfig.fHoldToDraw && mSink.buildToolActive()) {
        // Hold-to-draw: start the build drag where the finger rests.
        if(tMs - f.fDownMs < mConfig.fBuildHoldMs) return;
        mState = eState::buildDrag;
        mPreviewArea = false;
        mSink.pointerMove(sx, sy);
        mSink.pointerPress(eTouchSink::eButton::left, sx, sy);
        const int cx = ri(f.fX);
        const int cy = ri(f.fY);
        if(cx != sx || cy != sy) mSink.pointerMove(cx, cy);
        return;
    }
    if(tMs - f.fDownMs < mConfig.fLongPressMs) return;
    mSink.pointerMove(sx, sy);
    mSink.pointerPress(eTouchSink::eButton::right, sx, sy);
    mSink.pointerRelease(eTouchSink::eButton::right, sx, sy);
    mState = eState::longPress;
}

void eTouchGestures::cancel(const uint32_t tMs) {
    (void)tMs;
    switch(mState) {
    case eState::buildDrag:
        if(mPreviewArea) break; // a preview: nothing is held
        [[fallthrough]];
    case eState::uiPress: {
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
        mSink.pointerRelease(eTouchSink::eButton::middle, mPanEmittedX, mPanEmittedY);
        break;
    case eState::pinch:
        mSink.pinchEnd();
        break;
    default:
        break;
    }
    mFingers.clear();
    mState = eState::idle;
    mPrimary = -1;
}
