#ifndef EMODELDATA_H
#define EMODELDATA_H

#include <string>
#include <utility>
#include <vector>

#include "edifficulty.h"

// Port (gameplay parity): balancing values read at run time from the user's
// own original game data, Model/Zeus_Model_<Difficulty>.txt (one file per
// difficulty: VeryEasy, Easy, Normal, Hard, Impossible = beginner .. olympian).
// eZeus approximates these with one base value times a per-difficulty factor;
// the original files have individual values per difficulty. No numbers from
// the original files are stored in this source code.
//
// Column meaning (from the file's own header): a cost, b desirability,
// c desirability step, d step size, e range, f employees, g fire risk
// increment, h damage risk increment.
struct eModelBuilding {
    int fCost = 0;
    int fDesirability = 0;
    int fDesStep = 0;
    int fDesStepSize = 0;
    int fDesRange = 0;
    int fEmployees = 0;
    int fFireRisk = 0;
    int fDamageRisk = 0;
};

namespace eModelData {
    // nullptr when the file is missing, the building has no entry, or the
    // feature is disabled (EZEUS_MODEL_DATA=0).
    const eModelBuilding* building(const eDifficulty diff,
                                   const eBuildingType type);
    // Original BUILD_* names and the eZeus building type they correspond to
    // (also parsed by tools/parity/compare_model.py).
    const std::vector<std::pair<std::string, eBuildingType>>& nameMap();
    // Employees from the original data (same in every difficulty) when both
    // the original and eZeus give the building workers; else eZeusValue.
    int adjustEmployees(const eBuildingType type, const int eZeusValue);
    // Parses one model file; returns entries keyed by building type.
    bool parseFile(const std::string& path,
                   std::vector<std::pair<eBuildingType, eModelBuilding>>& out);
}

#endif // EMODELDATA_H
