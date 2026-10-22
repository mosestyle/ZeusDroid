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
    int zoom = 0;           // -1/0/+1: zoomed out / unchanged / in
    float scale = 1.f;      // last pinch scale
    float pinX = 0, pinY = 0;
    int camAtPin = 0, camYAtPin = 0;
    int pinchBegins = 0, pinchEnds = 0;
    float lastPinchX = 0, lastPinchY = 0;

    int listAboveY = -1;    // y < listAboveY (and not UI) is a scrollable list
    int listScrolled = 0;
    bool confirm = false;       // widget supports two-step placement
    bool box = false;           // army box-select mode
    bool draw = false;          // build tool draw mode
    bool drawModeActive() override { return draw; }
    bool boxSelectActive() override { return box; }
    int prevX = -1000, prevY = -1000; // pending preview position
    void previewMoveTo(int x, int y) { prevX = x; prevY = y; }
    bool confirmPlacement() override { return confirm; }
    bool previewAt(int x, int y) override {
        return std::abs(x - prevX) < 40 && std::abs(y - prevY) < 40;
    }
    void previewTap(int x, int y) override { ev.push_back({"ptap", -1, x, y}); }
    void previewArea(int sx, int sy, int x, int y) override {
        ev.push_back({"parea", sx*10000 + sy, x, y});
    }
    int pdragBegins = 0;
    void previewDrag(int sx, int sy, int x, int y, bool begin) override {
        if(begin) pdragBegins++;
        ev.push_back({"pdrag", sx*10000 + sy, x, y});
    }
    eTarget targetAt(int x, int y) override {
        if(uiBelowX >= 0 && x >= uiBelowX) return eTarget::ui;
        if(listAboveY >= 0 && y < listAboveY) return eTarget::scrollList;
        return eTarget::map;
    }
    void listScroll(int, int, int dy) override {
        listScrolled += dy;
        ev.push_back({"scroll", -1, 0, dy});
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
    // Emulates the widget: pinned point follows the centroid (pan part only).
    void pinchBegin(float x, float y) override {
        pinchBegins++;
        pinX = x; pinY = y; camAtPin = camX; camYAtPin = camY;
        ev.push_back({"pinchBegin", -1, int(x), int(y)});
    }
    void pinchUpdate(float x, float y, float sc) override {
        scale = sc;
        zoom = sc > 1.001f ? 1 : (sc < 0.999f ? -1 : 0);
        camX = camAtPin + int(std::lround(x - pinX));
        camY = camYAtPin + int(std::lround(y - pinY));
        lastPinchX = x; lastPinchY = y;
        ev.push_back({"zoom", zoom, int(x), int(y)});
    }
    void pinchEnd() override { pinchEnds++; ev.push_back({"pinchEnd", -1, 0, 0}); }
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
    std::puts("continuous pinch zoom pinned at centroid; second finger joining/leaving pan does not jump");
    RecSink s; eTouchGestures g(s);
    g.fingerDown(1, 400, 400, 0);
    g.fingerMove(1, 360, 400, 16);   // start pan
    const int camBefore = s.camX;
    g.fingerDown(2, 560, 400, 32);   // second finger -> pinch
    CHECK(g.state() == eTouchGestures::eState::pinch);
    CHECK(s.camX == camBefore);      // no jump on finger entering
    // spread from 200 -> 280 => scale 1.4, pinned point at the centroid
    g.fingerMove(1, 320, 400, 48);
    g.fingerMove(2, 600, 400, 48);
    g.update(48);
    CHECK(s.zoom == 1);
    CHECK(std::fabs(s.scale - 1.4f) < 1e-4f);
    CHECK(s.pinchBegins == 1);
    CHECK(std::fabs(s.lastPinchX - 460.f) < 1e-3f && std::fabs(s.lastPinchY - 400.f) < 1e-3f);
    const int camAtLift = s.camX;
    g.fingerUp(2, 600, 400, 64);
    CHECK(g.state() == eTouchGestures::eState::waitAllUp);
    CHECK(s.pinchEnds == 1);
    g.fingerMove(1, 100, 100, 80);   // remaining finger must not pan or click
    CHECK(s.camX == camAtLift);
    g.fingerUp(1, 100, 100, 96);
    CHECK(s.count("press", L) == 0);
    CHECK(s.count("press", M) == s.count("release", M));
    CHECK(g.state() == eTouchGestures::eState::idle);
}

void testPinchOut() {
    std::puts("pinch out => zoom out, two-finger pan moves camera, dead band keeps scale 1");
    RecSink s; eTouchGestures g(s);
    g.fingerDown(1, 300, 300, 0);
    g.fingerDown(2, 700, 300, 5);
    g.fingerMove(1, 400, 300, 20);
    g.fingerMove(2, 600, 300, 20);   // spread 400 -> 200: scale .5
    g.update(20);
    CHECK(s.zoom == -1);
    CHECK(std::fabs(s.scale - 0.5f) < 1e-4f);
    // both fingers move right by 50: pan
    const int cam0 = s.camX;
    g.fingerMove(1, 450, 300, 40);
    g.fingerMove(2, 650, 300, 40);
    g.update(40);
    CHECK(s.camX - cam0 == 50);
    g.fingerUp(1, 450, 300, 60);
    g.fingerUp(2, 650, 300, 60);
    CHECK(s.count("press", M) == s.count("release", M));
}

void testListTapAndScroll() {
    std::puts("list: tap clicks at touch-down point, drag scrolls without clicking");
    RecSink s; s.listAboveY = 500; eTouchGestures g(s);
    g.fingerDown(1, 100, 200, 0);
    CHECK(s.ev.empty());                 // nothing until we know tap vs scroll
    g.fingerMove(1, 102, 205, 20);       // jitter within slop
    g.fingerUp(1, 102, 205, 60);
    CHECK(s.count("press", L) == 1 && s.count("release", L) == 1);
    CHECK(s.ev[1].x == 100 && s.ev[1].y == 200);
    s.ev.clear();
    g.fingerDown(1, 100, 400, 100);
    for(int i = 1; i <= 20; i++) g.fingerMove(1, 100, 400 - 7.5f*i, 100 + 8*i); // up 150 px
    g.fingerUp(1, 100, 250, 300);
    CHECK(s.listScrolled == 150);        // content follows the finger
    CHECK(s.count("press") == 0);
    CHECK(g.state() == eTouchGestures::eState::idle);
    g.fingerDown(1, 100, 100, 400);
    g.fingerMove(1, 100, 160, 420);      // down 60 px: scroll back
    g.fingerUp(1, 100, 160, 440);
    CHECK(s.listScrolled == 90);
}

void testConfirmPlacement() {
    std::puts("confirm placement: tap previews, drag pans, hold+drag previews area, drag on preview moves it; nothing is built");
    RecSink s; s.build = true; s.confirm = true; eTouchGestures g(s);
    g.fingerDown(1, 300, 300, 0);
    g.fingerUp(1, 303, 298, 50);
    CHECK(s.count("ptap") == 1 && s.ev.back().x == 300 && s.ev.back().y == 300);
    CHECK(s.count("press") == 0);
    s.previewMoveTo(300, 300);
    s.ev.clear();
    // drag away from the preview: pans
    g.fingerDown(1, 600, 600, 100);
    g.fingerMove(1, 650, 600, 120);
    g.fingerUp(1, 650, 600, 140);
    CHECK(s.camX == 50);
    CHECK(s.count("press", L) == 0 && s.count("pdrag") == 0);
    // drag starting on the preview: moves it, no pan
    s.ev.clear();
    g.fingerDown(1, 310, 290, 200);
    g.fingerMove(1, 340, 290, 220);
    g.fingerMove(1, 380, 300, 240);
    g.fingerUp(1, 380, 300, 260);
    CHECK(s.count("pdrag") == 3 && s.pdragBegins == 1);
    CHECK(s.ev.back().what == "pdrag" && s.ev.back().x == 380 && s.ev.back().b == 310*10000 + 290);
    CHECK(s.camX == 50 && s.count("press") == 0);
    // hold on the preview: no area preview, no info click
    s.ev.clear();
    g.fingerDown(1, 300, 300, 300);
    g.update(1300);
    g.fingerUp(1, 300, 300, 1310);
    CHECK(s.count("parea") == 0 && s.count("press") == 0);
    CHECK(s.count("ptap") == 1);   // a tap (widget: put the preview here)
    // hold elsewhere, then drag: area preview, nothing pressed
    s.ev.clear();
    g.fingerDown(1, 700, 400, 2000);
    g.update(2350);
    CHECK(g.state() == eTouchGestures::eState::buildDrag);
    g.fingerMove(1, 800, 450, 2400);
    g.fingerUp(1, 820, 460, 2450);
    CHECK(s.count("parea") == 3);
    CHECK(s.ev.back().what == "parea" && s.ev.back().x == 820 && s.ev.back().b == 700*10000 + 400);
    CHECK(s.count("press") == 0 && s.count("release") == 0);
    // lifecycle cancel during an area preview sends no clicks
    s.ev.clear();
    g.fingerDown(1, 700, 400, 3000);
    g.update(3400);
    g.cancel(3410);
    CHECK(s.count("press") == 0 && s.count("release") == 0);
}

void testBoxSelect() {
    std::puts("box-select mode: one-finger drag is a left-drag from the touch-down point, no pan, no long press");
    RecSink s; s.box = true; eTouchGestures g(s);
    g.fingerDown(1, 100, 100, 0);
    g.update(900);                       // would be a long press otherwise
    CHECK(s.count("press", R) == 0);
    g.fingerMove(1, 200, 160, 920);
    g.fingerMove(1, 300, 220, 940);
    g.fingerUp(1, 300, 220, 960);
    CHECK(s.count("press", L) == 1 && s.count("release", L) == 1);
    CHECK(s.count("press", M) == 0 && s.camX == 0);
    bool startOk = false;
    for(const auto& e : s.ev) if(e.what == "press") startOk = e.x == 100 && e.y == 100;
    CHECK(startOk);
    CHECK(s.ev.back().what == "release" && s.ev.back().x == 300);
    // a tap is still a tap
    s.ev.clear();
    g.fingerDown(1, 50, 50, 2000);
    g.fingerUp(1, 50, 50, 2050);
    CHECK(s.count("press", L) == 1 && s.count("release", L) == 1);
}

void testDrawMode() {
    std::puts("draw mode: one-finger drag previews an area at once (confirm) / draws (instant); two fingers pan");
    {
        RecSink s; s.build = true; s.confirm = true; s.draw = true; eTouchGestures g(s);
        g.fingerDown(1, 100, 100, 0);
        g.fingerMove(1, 140, 120, 30);   // beyond slop immediately, no hold
        g.fingerMove(1, 300, 200, 60);
        g.fingerUp(1, 300, 200, 90);
        CHECK(s.count("parea") == 3 && s.camX == 0);
        CHECK(s.ev.front().what == "parea" && s.ev.front().b == 100*10000 + 100);
        CHECK(s.count("press") == 0);
        // two fingers still pan
        s.ev.clear();
        g.fingerDown(1, 300, 300, 200);
        g.fingerDown(2, 500, 300, 205);
        g.fingerMove(1, 350, 300, 220);
        g.fingerMove(2, 550, 300, 220);
        g.update(220);
        g.fingerUp(1, 350, 300, 240);
        g.fingerUp(2, 550, 300, 240);
        CHECK(s.camX == 50 && s.count("parea") == 0);
        // a tap still places a single preview
        s.ev.clear();
        g.fingerDown(1, 400, 400, 300);
        g.fingerUp(1, 401, 400, 330);
        CHECK(s.count("ptap") == 1);
    }
    {
        RecSink s; s.build = true; s.draw = true; eTouchGestures g(s); // instant, hold-to-draw on
        g.fingerDown(1, 100, 100, 0);
        g.fingerMove(1, 140, 120, 30);
        g.fingerUp(1, 140, 120, 60);
        CHECK(s.count("press", L) == 1 && s.count("release", L) == 1 && s.camX == 0);
    }
}

void testTwoFingerPanDeadBand() {
    std::puts("two-finger pan with slight spread change does not zoom");
    RecSink s; eTouchGestures g(s);
    g.fingerDown(1, 300, 300, 0);
    g.fingerDown(2, 500, 300, 5);
    g.fingerMove(1, 352, 340, 20);   // fast pan: 52 px per event;
    g.fingerMove(2, 550, 340, 20);   // spread 200 -> 198 (1%) per frame
    g.update(20);
    CHECK(s.zoom == 0);
    CHECK(s.scale == 1.f);
    CHECK(s.camX == 51 && s.camY == 40);
    g.fingerUp(1, 352, 340, 40);
    g.fingerUp(2, 550, 340, 40);
    CHECK(s.pinchEnds == 1);
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
    std::puts("build tool (direct mode): tap places at touch point; drag draws from start point");
    RecSink s; s.build = true;
    eTouchConfig cfg; cfg.fHoldToDraw = false;
    eTouchGestures g(s, cfg);
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

void testHoldToDraw() {
    std::puts("build tool (hold-to-draw, default): drag pans, hold then drag draws, tap places");
    RecSink s; s.build = true; eTouchGestures g(s);
    // quick drag -> pan, nothing built
    g.fingerDown(1, 500, 500, 0);
    g.fingerMove(1, 420, 470, 40);
    g.fingerMove(1, 300, 400, 80);
    g.update(400);                    // timer must not start drawing once panning
    g.fingerUp(1, 300, 400, 120);
    CHECK(s.count("press", L) == 0);
    CHECK(s.camX == -200 && s.camY == -100);
    // hold still 300 ms then drag -> road drawn from the hold point
    s.ev.clear();
    g.fingerDown(1, 600, 300, 1000);
    g.fingerMove(1, 603, 302, 1100);  // jitter below slop
    g.update(1200);
    CHECK(s.count("press", L) == 0);  // not yet
    g.update(1310);
    CHECK(s.count("press", L) == 1);
    CHECK(g.state() == eTouchGestures::eState::buildDrag);
    g.fingerMove(1, 700, 350, 1400);
    g.fingerMove(1, 800, 400, 1500);
    g.fingerUp(1, 800, 400, 1600);
    bool startOk = false;
    for(const auto& e : s.ev) if(e.what == "press" && e.b == L) startOk = (e.x == 600 && e.y == 300);
    CHECK(startOk);
    CHECK(s.ev.back().what == "release" && s.ev.back().b == L && s.ev.back().x == 800);
    CHECK(s.count("press", R) == 0);  // never an info/cancel click in build mode
    // tap -> single placement
    s.ev.clear();
    g.fingerDown(1, 50, 60, 3000);
    g.fingerUp(1, 52, 61, 3080);
    CHECK(s.count("press", L) == 1 && s.count("release", L) == 1);
    // without build tool, long press is still a right click
    s.ev.clear(); s.build = false;
    g.fingerDown(1, 50, 60, 5000);
    g.update(5600);
    g.fingerUp(1, 50, 60, 5700);
    CHECK(s.count("press", R) == 1 && s.count("press", L) == 0);
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
    testHoldToDraw();
    testBuildModeTwoFingerPanBeforeCommit();
    testCancel();
    testThreeFingers();
    testTwoFingerPanDeadBand();
    testListTapAndScroll();
    testConfirmPlacement();
    testBoxSelect();
    testDrawMode();
    std::printf("%d checks, %d failures\n", gChecks, gFailures);
    return gFailures == 0 ? 0 : 1;
}
