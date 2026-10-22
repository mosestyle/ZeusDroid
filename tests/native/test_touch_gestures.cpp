// Unit tests for the touch gesture state machine (engine/platform/etouchgestures).
// Build: see tests/native/CMakeLists.txt or tools/tests/run_native_tests.sh
#include "platform/etouchgestures.h"

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace {

int gFailures = 0;
int gChecks = 0;
#define CHECK(cond) do { gChecks++; if(!(cond)) { gFailures++; \
    std::printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); } } while(0)

struct Ev {
    std::string what;
    int b;
    int x;
    int y;
};

struct RecSink : eTouchSink {
    std::vector<Ev> ev;
    int uiBelowX = -1;      // x >= uiBelowX is UI (side panel), -1 = none
    bool build = false;
    int camX = 0, camY = 0; // emulates eGameWidget middle-drag panning
    int lastX = 0, lastY = 0;
    bool middle = false;
    int zoom = 0;

    eTarget targetAt(int x, int) override {
        return (uiBelowX >= 0 && x >= uiBelowX) ? eTarget::ui : eTarget::map;
    }
    bool buildToolActive() override { return build; }
    void pointerMove(int x, int y) override {
        if(middle) { camX += x - lastX; camY += y - lastY; }
        lastX = x; lastY = y;
        ev.push_back({"move", -1, x, y});
    }
    void pointerPress(eButton b, int x, int y) override {
        if(b == eButton::middle) { middle = true; lastX = x; lastY = y; }
        ev.push_back({"press", int(b), x, y});
    }
    void pointerRelease(eButton b, int x, int y) override {
        if(b == eButton::middle) middle = false;
        ev.push_back({"release", int(b), x, y});
    }
    void zoomStep(int dir, int x, int y) override {
        zoom += dir;
        ev.push_back({"zoom", dir, x, y});
    }
    int count(const std::string& w, int b = -2) const {
        int n = 0;
        for(const auto& e : ev) if(e.what == w && (b == -2 || e.b == b)) n++;
        return n;
    }
};

const int L = int(eTouchSink::eButton::left);
const int R = int(eTouchSink::eButton::right);
const int M = int(eTouchSink::eButton::middle);

void testTapWithJitter() {
    std::puts("tap with minimal jitter");
    RecSink s; eTouchGestures g(s);
    g.fingerDown(1, 100, 100, 0);
    g.fingerMove(1, 103, 98, 30);
    g.fingerMove(1, 101, 102, 60);
    g.fingerUp(1, 102, 101, 90);
    CHECK(s.count("press", L) == 1);
    CHECK(s.count("release", L) == 1);
    CHECK(s.count("press", M) == 0);
    CHECK(s.count("press", R) == 0);
    // Tap is dispatched at the touch-down point, after a hover move there.
    const auto& p = s.ev[s.ev.size() - 2];
    CHECK(p.what == "press" && p.x == 100 && p.y == 100);
    CHECK(s.ev[s.ev.size() - 3].what == "move");
    CHECK(g.state() == eTouchGestures::eState::idle);
}

void testLongPan() {
    std::puts("pan with long movement follows finger exactly");
    RecSink s; eTouchGestures g(s);
    g.fingerDown(1, 500, 300, 0);
    float x = 500;
    for(int i = 1; i <= 40; i++) { x -= 7.3f; g.fingerMove(1, x, 300 + i*0.4f, i*16); }
    g.fingerUp(1, x, 316, 700);
    CHECK(s.count("press", L) == 0);   // no accidental click/build
    CHECK(s.count("press", M) == 1);
    CHECK(s.count("release", M) == 1);
    CHECK(s.camX == int(std::lround(x - 500)));
    CHECK(s.camY == 16);
}

void testSlowPanNoSubpixelLoss() {
    std::puts("slow pan accumulates sub-pixel motion");
    RecSink s; eTouchGestures g(s);
    g.fingerDown(1, 0, 0, 0);
    g.fingerMove(1, 30, 0, 10);       // exceed slop
    float x = 30;
    for(int i = 0; i < 100; i++) { x += 0.3f; g.fingerMove(1, x, 0, 20 + i); }
    g.fingerUp(1, x, 0, 200);
    CHECK(s.camX == int(std::lround(x)));
}

void testLongPress() {
    std::puts("long-press then release => single right click, nothing else");
    RecSink s; eTouchGestures g(s);
    g.fingerDown(1, 50, 60, 1000);
    g.update(1200);
    CHECK(s.count("press", R) == 0);
    g.update(1500);
    CHECK(s.count("press", R) == 1);
    CHECK(s.count("release", R) == 1);
    g.fingerMove(1, 200, 200, 1600);  // moving after long-press does nothing
    g.update(2500);
    g.fingerUp(1, 200, 200, 1700);
    CHECK(s.count("press", R) == 1);
    CHECK(s.count("press", L) == 0);
    CHECK(s.count("press", M) == 0);
}

void testPinchAnchoredAndNoJump() {
    std::puts("pinch zoom steps at centroid; second finger joining/leaving pan does not jump");
    RecSink s; eTouchGestures g(s);
    g.fingerDown(1, 400, 400, 0);
    g.fingerMove(1, 360, 400, 16);   // start pan
    const int camBefore = s.camX;
    g.fingerDown(2, 560, 400, 32);   // second finger -> pinch
    CHECK(g.state() == eTouchGestures::eState::pinch);
    CHECK(s.camX == camBefore);      // no jump on finger entering
    // spread from 200 -> 280 (ratio 1.4) => one zoom-in step
    g.fingerMove(1, 320, 400, 48);
    g.fingerMove(2, 600, 400, 48);
    CHECK(s.zoom == 1);
    const auto& z = s.ev[std::size_t(0)];
    (void)z;
    bool found = false;
    for(const auto& e : s.ev) if(e.what == "zoom") { found = true; CHECK(e.x == 460 && e.y == 400); }
    CHECK(found);
    const int camAtLift = s.camX;
    g.fingerUp(2, 600, 400, 64);
    CHECK(g.state() == eTouchGestures::eState::waitAllUp);
    g.fingerMove(1, 100, 100, 80);   // remaining finger must not pan or click
    CHECK(s.camX == camAtLift);
    g.fingerUp(1, 100, 100, 96);
    CHECK(s.count("press", L) == 0);
    CHECK(s.count("press", M) == s.count("release", M));
    CHECK(g.state() == eTouchGestures::eState::idle);
}

void testPinchOut() {
    std::puts("pinch out => zoom out steps, two-finger pan moves camera");
    RecSink s; eTouchGestures g(s);
    g.fingerDown(1, 300, 300, 0);
    g.fingerDown(2, 700, 300, 5);
    g.fingerMove(1, 400, 300, 20);
    g.fingerMove(2, 600, 300, 20);   // spread 400 -> 200: ratio .5 => one step (rebased)
    CHECK(s.zoom <= -1);
    // both fingers move right by 50: pan
    const int cam0 = s.camX;
    g.fingerMove(1, 450, 300, 40);
    g.fingerMove(2, 650, 300, 40);
    CHECK(s.camX - cam0 == 50);
    g.fingerUp(1, 450, 300, 60);
    g.fingerUp(2, 650, 300, 60);
    CHECK(s.count("press", M) == s.count("release", M));
}

void testUiVsMap() {
    std::puts("UI press is immediate left press; drag is a left-drag, never a pan");
    RecSink s; s.uiBelowX = 800; eTouchGestures g(s);
    g.fingerDown(1, 900, 100, 0);
    CHECK(s.count("press", L) == 1);
    g.fingerMove(1, 900, 300, 16);
    g.fingerDown(2, 100, 100, 20);   // second finger ignored
    g.fingerMove(2, 300, 100, 30);
    g.fingerUp(2, 300, 100, 40);
    g.fingerUp(1, 900, 300, 50);
    CHECK(s.count("release", L) == 1);
    CHECK(s.count("press", M) == 0);
    CHECK(s.count("zoom") == 0);
    CHECK(s.ev.back().what == "release" && s.ev.back().y == 300);
}

void testBuildPlacementAndRoadDrag() {
    std::puts("build tool: tap places at touch point; drag draws from start point");
    RecSink s; s.build = true; eTouchGestures g(s);
    g.fingerDown(1, 200, 200, 0);
    g.fingerUp(1, 201, 199, 40);
    CHECK(s.count("press", L) == 1 && s.count("release", L) == 1);
    s.ev.clear();
    g.fingerDown(1, 200, 200, 100);
    g.fingerMove(1, 260, 230, 116);
    g.fingerMove(1, 320, 260, 132);
    g.fingerUp(1, 320, 260, 148);
    CHECK(s.count("press", M) == 0);
    CHECK(s.ev[1].what == "press" && s.ev[1].b == L && s.ev[1].x == 200 && s.ev[1].y == 200);
    CHECK(s.ev.back().what == "release" && s.ev.back().x == 320);
}

void testBuildModeTwoFingerPanBeforeCommit() {
    std::puts("build tool: second finger before slop => pinch/pan, no placement");
    RecSink s; s.build = true; eTouchGestures g(s);
    g.fingerDown(1, 200, 200, 0);
    g.fingerDown(2, 400, 200, 10);
    g.fingerMove(1, 250, 200, 20);
    g.fingerMove(2, 450, 200, 20);
    g.fingerUp(1, 250, 200, 30);
    g.fingerUp(2, 450, 200, 30);
    CHECK(s.count("press", L) == 0);
    CHECK(s.camX == 50);
}

void testCancel() {
    std::puts("lifecycle cancel releases held buttons, no click");
    RecSink s; eTouchGestures g(s);
    g.fingerDown(1, 10, 10, 0);
    g.fingerMove(1, 100, 10, 10);
    g.cancel(20);
    CHECK(s.count("press", M) == s.count("release", M));
    CHECK(s.count("press", L) == 0);
    CHECK(g.state() == eTouchGestures::eState::idle);
    // finger-up after cancel for an unknown finger is ignored
    g.fingerUp(1, 100, 10, 30);
    CHECK(s.count("press", L) == 0);
}

void testThreeFingers() {
    std::puts("third finger ignored during pinch");
    RecSink s; eTouchGestures g(s);
    g.fingerDown(1, 100, 100, 0);
    g.fingerDown(2, 300, 100, 0);
    g.fingerDown(3, 500, 500, 0);
    g.fingerMove(3, 900, 900, 10);
    CHECK(s.zoom == 0);
    g.fingerUp(3, 900, 900, 20);
    g.fingerUp(2, 300, 100, 20);
    g.fingerUp(1, 100, 100, 20);
    CHECK(g.state() == eTouchGestures::eState::idle);
    CHECK(s.count("press", M) == s.count("release", M));
}

} // namespace

int main() {
    testTapWithJitter();
    testLongPan();
    testSlowPanNoSubpixelLoss();
    testLongPress();
    testPinchAnchoredAndNoJump();
    testPinchOut();
    testUiVsMap();
    testBuildPlacementAndRoadDrag();
    testBuildModeTwoFingerPanBeforeCommit();
    testCancel();
    testThreeFingers();
    std::printf("%d checks, %d failures\n", gChecks, gFailures);
    return gFailures == 0 ? 0 : 1;
}
