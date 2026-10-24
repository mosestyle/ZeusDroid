#ifndef EHEATGETTERS_H
#define EHEATGETTERS_H

#include "buildings/ebuilding.h"
#include "engine/boardData/eheatmap.h"

enum class eGodType;

namespace eHeatGetters {
    using eHeatGetter = std::function<eHeat(eBuildingType)>;

    eHeat appeal(const eBuildingType type);
    // Port (parity): true when the original game's desirability rules (model
    // data, ring/step spread, -100..100 scale) are in use.
    bool originalDesirability();
    // Original desirability of an elite house at a level (-1 = vacant).
    eHeat eliteAppeal(const int level);
    // Converts a desirability value to the scale eZeus' own thresholds
    // (road look, overlay, hero requirements) were written for.
    double legacyScale(const double des);
    eHeat housing(const eBuildingType type);
    eHeat culture(const eBuildingType type);
    eHeat industry(const eBuildingType type);
    eHeat farming(const eBuildingType type);
    eHeat storage(const eBuildingType type);

    eHeatGetter godLeaning(const eGodType gt);

    eHeat empty(const eBuildingType type);
    eHeat any(const eBuildingType type);

    eHeat fertile(eTileBase* const tile);
    eHeat notFertile(eTileBase* const tile);
}

#endif // EHEATGETTERS_H
