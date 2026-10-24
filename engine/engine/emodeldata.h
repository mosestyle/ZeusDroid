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

// House rows ("ALL HOUSES" section): a = DES at or below which the house
// devolves, b = DES at or above which it evolves (when the next level's needs
// are met). Common levels 0..6 = Hut..Townhouse, elite levels 0..4 =
// Devolved..Estate.
struct eModelHouse {
    int fDevolve = 0;
    int fEvolve = 0;
};

struct eModelFile {
    std::vector<std::pair<eBuildingType, eModelBuilding>> fBuildings;
    std::vector<std::pair<std::string, eModelBuilding>> fByName;
    eModelHouse fCommon[7];
    bool fHasCommon[7] = {};
    eModelHouse fElite[5];
    bool fHasElite[5] = {};
};

namespace eModelData {
    // True when the original model data is enabled and was found (the
    // original desirability rules are used only then).
    bool available();
    // Raw entry by its BUILD_* name (e.g. BUILD_ELITE_HOUSE2_MANSION).
    const eModelBuilding* byName(const eDifficulty diff,
                                 const std::string& name);
    const eModelHouse* house(const eDifficulty diff,
                             const bool elite, const int level);
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
    bool parseFile(const std::string& path, eModelFile& out);
}

#endif // EMODELDATA_H
