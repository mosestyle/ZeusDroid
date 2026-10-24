#ifndef EHEATMAP_H
#define EHEATMAP_H

#include <vector>
#include <SDL2/SDL_rect.h>

struct eHeat {
    int fValue;
    int fRange;
    // Port (parity): the original game's desirability spread. When fSteps is
    // set, fValue is applied to the square rings around the footprint
    // (distance 1..min(fRange, 6)); after every fStep rings fStepSize is added
    // to the value; each tile is clamped to [-100, 100]. fRectQuirk mirrors
    // the original's routine for non-square/large structures, which applies
    // the first step one ring earlier.
    int fStep = 0;
    int fStepSize = 0;
    bool fSteps = false;
    bool fRectQuirk = false;
};

class eHeatMap {
public:
    struct eHeatTile {
        bool fOutsideRange = false;
        bool fEnabled = false;
        double fAppeal = 0;
    };

    void add(const eHeatMap& other);
    void multiply(const eHeatMap& other);

    void reset();
    void initialize(const int dx, const int dy,
                    const int w, const int h);

    void addHeat(const eHeat& a,
                 const SDL_Rect& tileRect);

    void addHeat(const eHeat& a,
                 const int ax, const int ay,
                 const int sw, const int sh);

    void addHeat(const int x, const int y,
                 const double a);

    void addRaw(const int x, const int y, const int a);
    void enable(const int x, const int y);

    void setOutsideRange(const int x, const int y);
    bool enabled(const int x, const int y) const;
    double heat(const int x, const int y) const;

    int dx() const { return mDX; }
    int dy() const { return mDY; }

    int width() const { return mWidth; }
    int height() const { return mHeight; }

    void set(const int x, const int y,
             const bool e, const double h);
private:
    int mDX = 0;
    int mDY = 0;
    int mWidth = 0;
    int mHeight = 0;
    std::vector<std::vector<eHeatTile>> mMap;
};

#endif // EHEATMAP_H
