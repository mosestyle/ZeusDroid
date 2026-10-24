// Unit test: the original game's desirability spread as implemented in
// eHeatMap (fSteps mode). Rules (research R6): square rings around the
// footprint, distance 1..min(range, 6); the value changes by the step size
// after every `step` rings; tiles clamp to -100..100; the footprint itself
// gets nothing; non-square structures apply the first step one ring earlier.
#include "engine/boardData/eheatmap.h"
#include "etilehelper.h"

#include <cstdio>

// The only eTileHelper function eheatmap.cpp needs (same formula as
// engine/etilehelper.cpp, which pulls in the whole game board).
void eTileHelper::tileIdToDTileId(const int tx, const int ty, int& dtx, int& dty) {
    dty = tx + ty;
    dtx = (tx + ty)/2 - ty;
}

static int gChecks = 0;
static int gFails = 0;

static void check(const bool ok, const char* what) {
    gChecks++;
    if(!ok) {
        gFails++;
        std::printf("FAIL: %s\n", what);
    }
}

struct eGrid {
    eHeatMap fMap;
    eGrid() { fMap.initialize(-200, -200, 400, 400); }
    double at(const int tx, const int ty) const {
        int dx, dy;
        eTileHelper::tileIdToDTileId(tx, ty, dx, dy);
        return fMap.heat(dx, dy);
    }
    bool enabled(const int tx, const int ty) const {
        int dx, dy;
        eTileHelper::tileIdToDTileId(tx, ty, dx, dy);
        return fMap.enabled(dx, dy);
    }
};

static eHeat steps(const int v, const int step, const int size, const int range) {
    eHeat h{v, range};
    h.fStep = step;
    h.fStepSize = size;
    h.fSteps = true;
    return h;
}

int main() {
    {   // 1x1, step 1: 10, 8, 6 then nothing.
        eGrid g;
        g.fMap.addHeat(steps(10, 1, -2, 3), 50, 50, 1, 1);
        check(g.at(50, 50) == 0 && g.enabled(50, 50), "footprint gets 0 but is enabled");
        check(g.at(51, 50) == 10 && g.at(49, 49) == 10, "ring 1 = 10 (incl. diagonal)");
        check(g.at(52, 50) == 8 && g.at(52, 48) == 8, "ring 2 = 8 (square ring)");
        check(g.at(53, 53) == 6, "ring 3 = 6");
        check(g.at(54, 50) == 0 && !g.enabled(54, 50), "ring 4 untouched");
    }
    {   // 2x2 house-sized, step 2: 6, 6, 5, 5.
        eGrid g;
        g.fMap.addHeat(steps(6, 2, -1, 4), 10, 10, 2, 2);
        check(g.at(10, 10) == 0 && g.at(11, 11) == 0, "2x2 footprint untouched");
        check(g.at(12, 11) == 6 && g.at(9, 9) == 6, "2x2 ring 1 = 6");
        check(g.at(13, 10) == 6, "2x2 ring 2 = 6");
        check(g.at(14, 10) == 5 && g.at(15, 15) == 5, "2x2 rings 3, 4 = 5");
        check(g.at(16, 10) == 0, "2x2 ring 5 untouched");
    }
    {   // Range is capped at 6.
        eGrid g;
        g.fMap.addHeat(steps(-20, 1, 2, 10), 0, 0, 1, 1);
        check(g.at(6, 0) == -10, "ring 6 = -10");
        check(g.at(7, 0) == 0, "range capped at 6");
    }
    {   // Non-square: first step one ring earlier (step 2: 12, 10, 10, 8).
        eGrid g;
        g.fMap.addHeat(steps(12, 2, -2, 4), 0, 0, 3, 2);
        check(g.at(3, 0) == 12, "rect ring 1 = 12");
        check(g.at(4, 0) == 10 && g.at(5, 0) == 10, "rect rings 2, 3 = 10");
        check(g.at(6, 0) == 8, "rect ring 4 = 8");
    }
    {   // Sums and clamps.
        eGrid g;
        for(int i = 0; i < 8; i++) g.fMap.addHeat(steps(30, 1, 0, 1), 0, 0, 1, 1);
        check(g.at(1, 0) == 100, "clamped at 100");
        g.fMap.addHeat(steps(5, 1, 0, 1), 20, 20, 1, 1);
        g.fMap.addHeat(steps(-2, 1, 0, 1), 22, 20, 1, 1);
        check(g.at(21, 20) == 3, "overlapping rings add up");
    }
    {   // Step 0 changes the value after every ring (as the original).
        eGrid g;
        g.fMap.addHeat(steps(4, 0, -1, 3), 0, 0, 1, 1);
        check(g.at(1, 0) == 4 && g.at(2, 0) == 3 && g.at(3, 0) == 2, "step 0");
    }
    std::printf("desirability: %d checks, %d failures\n", gChecks, gFails);
    return gFails ? 1 : 0;
}
