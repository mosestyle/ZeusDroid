#include "emodeldata.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <mutex>
#include <set>
#include <sstream>

#include "egamedir.h"

namespace {
using eT = eBuildingType;

// MODEL_NAME_MAP_BEGIN (parsed by tools/parity/compare_model.py)
const std::vector<std::pair<std::string, eBuildingType>> gNames = {
    {"BUILD_HOUSE1_HUT", eT::commonHouse},
    {"BUILD_ELITE_VACANT", eT::eliteHousing},
    {"BUILD_FLOWER_GARDEN", eT::flowerGarden},
    {"BUILD_GAZEBO", eT::gazebo},
    {"BUILD_HEDGE_MAZE", eT::hedgeMaze},
    {"BUILD_FISH_POND", eT::fishPond},
    {"BUILD_TALL_OBELISK", eT::tallObelisk},
    {"BUILD_SUNDIAL", eT::sundial},
    {"BUILD_TOPIARY", eT::topiary},
    {"BUILD_SPRING", eT::spring},
    {"BUILD_STONE_CIRCLE", eT::stoneCircle},
    {"BUILD_SHORT_OBELISK", eT::shortObelisk},
    {"BUILD_WATER_PARK", eT::waterPark},
    {"BUILD_ROAD", eT::road},
    {"BUILD_DIG", eT::erase},
    {"BUILD_WHEAT_FARM", eT::wheatFarm},
    {"BUILD_ONION_FARM", eT::onionsFarm},
    {"BUILD_CARROT_FARM", eT::carrotsFarm},
    {"BUILD_GRAPEVINE", eT::vine},
    {"BUILD_OLIVE_TREE", eT::oliveTree},
    {"BUILD_GROWERS_LODGE", eT::growersLodge},
    {"BUILD_GOAT", eT::goat},
    {"BUILD_SHEEP", eT::sheep},
    {"BUILD_SHEEP_FARM", eT::cardingShed},
    {"BUILD_GOAT_FARM", eT::dairy},
    {"BUILD_FISHING_WHARF", eT::fishery},
    {"BUILD_URCHIN_COLLECTOR", eT::urchinQuay},
    {"BUILD_HUNTING_LODGE", eT::huntingLodge},
    {"BUILD_MINT", eT::mint},
    {"BUILD_MARBLE_QUARRY", eT::masonryShop},
    {"BUILD_FOUNDRY", eT::foundry},
    {"BUILD_TIMBER_MILL", eT::timberMill},
    {"BUILD_WINERY", eT::winery},
    {"BUILD_OIL_PRESS", eT::olivePress},
    {"BUILD_SCULPTURE_STUDIO", eT::sculptureStudio},
    {"BUILD_ARTISANS_GUILD", eT::artisansGuild},
    {"BUILD_GRANARY", eT::granary},
    {"BUILD_STOREHOUSE", eT::warehouse},
    {"BUILD_DOCK", eT::pier},
    {"BUILD_TRADING_POST", eT::tradePost},
    {"BUILD_AGORA_SMALL", eT::commonAgora},
    {"BUILD_AGORA_LARGE", eT::grandAgora},
    {"BUILD_FOOD_VENDOR", eT::foodVendor},
    {"BUILD_FLEECE_VENDOR", eT::fleeceVendor},
    {"BUILD_OIL_VENDOR", eT::oilVendor},
    {"BUILD_WINE_VENDOR", eT::wineVendor},
    {"BUILD_ARMS_VENDOR", eT::armsVendor},
    {"BUILD_HORSE_TRAINER", eT::horseTrainer},
    {"BUILD_FOUNTAIN", eT::fountain},
    {"BUILD_INFIRMARY", eT::hospital},
    {"BUILD_COLLEGE", eT::college},
    {"BUILD_DRAMA_SCHOOL", eT::dramaSchool},
    {"BUILD_GYMNASIUM", eT::gymnasium},
    {"BUILD_STADIUM", eT::stadium},
    {"BUILD_PODIUM", eT::podium},
    {"BUILD_THEATRE", eT::theater},
    {"BUILD_LARGE_SANC_ZEUS", eT::templeZeus},
    {"BUILD_LARGE_SANC_POSEIDON", eT::templePoseidon},
    {"BUILD_LARGE_SANC_DEMETER", eT::templeDemeter},
    {"BUILD_LARGE_SANC_APOLLO", eT::templeApollo},
    {"BUILD_LARGE_SANC_ARTEMIS", eT::templeArtemis},
    {"BUILD_LARGE_SANC_ARES", eT::templeAres},
    {"BUILD_LARGE_SANC_APHRODITE", eT::templeAphrodite},
    {"BUILD_LARGE_SANC_HERMES", eT::templeHermes},
    {"BUILD_LARGE_SANC_ATHENA", eT::templeAthena},
    {"BUILD_LARGE_SANC_HEPHAISTOS", eT::templeHephaestus},
    {"BUILD_LARGE_SANC_DIONYSUS", eT::templeDionysus},
    {"BUILD_LARGE_SANC_HADES", eT::templeHades},
    {"BUILD_LARGE_SANC_HERA", eT::templeHera},
    {"BUILD_LARGE_SANC_ATLAS", eT::templeAtlas},
    {"BUILD_PALACE", eT::palace},
    {"BUILD_AVENUE", eT::avenue},
    {"BUILD_COMMEMORATIVE", eT::commemorative},
    {"BUILD_LARGE_BRIDGE", eT::bridge},
    {"BUILD_SUPER_OFFICE", eT::maintenanceOffice},
    {"BUILD_TAX_OFFICE", eT::taxOffice},
    {"BUILD_GUARDPOST", eT::watchPost},
    {"BUILD_BENCH", eT::bench},
    {"BUILD_GARDENS", eT::park},
    {"BUILD_DORIC_COLUMN", eT::doricColumn},
    {"BUILD_IONIC_COLUMN", eT::ionicColumn},
    {"BUILD_CORINTHIAN_COLUMN", eT::corinthianColumn},
    {"BUILD_WALL", eT::wall},
    {"BUILD_GATEHOUSE", eT::gatehouse},
    {"BUILD_TOWER", eT::tower},
    {"BUILD_HORSE_RANCH", eT::horseRanch},
    {"BUILD_ARMORY", eT::armory},
    {"BUILD_WARSHIP_WHARF", eT::triremeWharf},
    {"BUILD_DOLPHIN_SCULPTURE", eT::dolphinSculpture},
    {"BUILD_ORRERY", eT::orrery},
    {"BUILD_SHELL_GARDEN", eT::shellGarden},
    {"BUILD_BATHS", eT::baths},
    {"BUILD_BIRD_BATH", eT::birdBath},
    {"BUILD_CROSSWALK", eT::crosswalk},
    {"BUILD_BIBLIOTHEKE", eT::bibliotheke},
    {"BUILD_OBSERVATORY", eT::observatory},
    {"BUILD_UNIVERSITY", eT::university},
    {"BUILD_LABORATORY", eT::laboratory},
    {"BUILD_INVENTORS_WORKSHOP", eT::inventorsWorkshop},
    {"BUILD_MUSUEM", eT::museum},
    {"BUILD_ORICHALC_REFINERY", eT::refinery},
    {"BUILD_CHARIOT_WORKSHOP", eT::chariotFactory},
    {"BUILD_BLACK_MARBLE_SHOP", eT::blackMarbleWorkshop},
    {"BUILD_FRUIT_GROWERS_LODGE", eT::orangeTendersLodge},
    {"BUILD_CHARIOT_VENDOR", eT::chariotVendor},
    {"BUILD_CORRAL_SLAUGHTER", eT::corral},
    {"BUILD_FRUIT_TREE", eT::orangeTree},
    {"BUILD_CATTLE", eT::cattle},
};
// MODEL_NAME_MAP_END

const char* const gFileNames[] = {
    "Zeus_Model_VeryEasy.txt", "Zeus_Model_Easy.txt", "Zeus_Model_Normal.txt",
    "Zeus_Model_Hard.txt", "Zeus_Model_Impossible.txt"
};

std::string trim(std::string s) {
    while(!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '"' ||
                         s.back() == '\r')) s.pop_back();
    size_t i = 0;
    while(i < s.size() && (s[i] == ' ' || s[i] == '"')) i++;
    return s.substr(i);
}

struct eCache {
    std::once_flag fOnce[5];
    std::map<eBuildingType, eModelBuilding> fData[5];
    std::map<std::string, eModelBuilding> fByName[5];
    eModelFile fFile[5];
};

eCache& cache() {
    static eCache c;
    return c;
}

bool enabledByEnv() {
    const char* const e = std::getenv("EZEUS_MODEL_DATA");
    return !(e && *e == '0');
}
}

const std::vector<std::pair<std::string, eBuildingType>>& eModelData::nameMap() {
    return gNames;
}

bool eModelData::parseFile(const std::string& path,
                           std::vector<std::pair<eBuildingType, eModelBuilding>>& out) {
    eModelFile f;
    const bool r = parseFile(path, f);
    out = f.fBuildings;
    return r && !out.empty();
}

namespace {
std::vector<int> braceValues(const std::string& line, const size_t brace) {
    std::vector<int> v;
    const auto end = line.find(",}", brace);
    if(end == std::string::npos) return v;
    std::stringstream ss(line.substr(brace + 3, end - brace - 3));
    std::string tok;
    while(std::getline(ss, tok, ',')) v.push_back(std::atoi(tok.c_str()));
    return v;
}
}

bool eModelData::parseFile(const std::string& path, eModelFile& out) {
    std::ifstream f(path);
    if(!f) return false;
    std::map<std::string, eBuildingType> byName(gNames.begin(), gNames.end());
    std::string line;
    int section = 0; // 1 buildings, 2 houses
    while(std::getline(f, line)) {
        if(line.rfind("ALL BUILDINGS", 0) == 0) { section = 1; continue; }
        if(line.rfind("ALL HOUSES", 0) == 0) { section = 2; continue; }
        if(line.rfind("End of model data", 0) == 0) break;
        if(section == 0) continue;
        const auto brace = line.find(",{,");
        if(brace == std::string::npos) continue;
        if(section == 2) {
            const auto colon = line.find(':');
            if(colon == std::string::npos || colon > brace) continue;
            const auto v = braceValues(line, brace);
            if(v.size() < 2) continue;
            const std::string key = trim(line.substr(0, colon));
            eModelHouse h;
            h.fDevolve = v[0];
            h.fEvolve = v[1];
            if(key.rfind("Elite", 0) == 0) {
                const std::string n = trim(key.substr(5));
                if(n.empty()) continue; // vacant lot
                const int l = std::atoi(n.c_str());
                if(l < 0 || l > 4) continue;
                out.fElite[l] = h;
                out.fHasElite[l] = true;
            } else {
                const int l = std::atoi(key.c_str()) - 1;
                if(l < 0 || l > 6) continue;
                out.fCommon[l] = h;
                out.fHasCommon[l] = true;
            }
            continue;
        }
        const auto comma = line.find(',');
        if(comma == std::string::npos || comma >= brace) continue;
        std::string name = trim(line.substr(comma + 1, brace - comma - 1));
        const auto sub = name.find_first_of(" ,(");
        if(sub != std::string::npos) name = trim(name.substr(0, sub));
        const auto v = braceValues(line, brace);
        if(v.size() < 8) continue;
        eModelBuilding b;
        b.fCost = v[0];
        b.fDesirability = v[1];
        b.fDesStep = v[2];
        b.fDesStepSize = v[3];
        b.fDesRange = v[4];
        b.fEmployees = v[5];
        b.fFireRisk = v[6];
        b.fDamageRisk = v[7];
        out.fByName.emplace_back(name, b);
        const auto it = byName.find(name);
        if(it == byName.end()) continue;
        out.fBuildings.emplace_back(it->second, b);
    }
    return !out.fBuildings.empty();
}

namespace {
const eCache* loaded(const eDifficulty diff, int& d) {
    static const bool enabled = enabledByEnv();
    if(!enabled) return nullptr;
    d = static_cast<int>(diff);
    if(d < 0 || d > 4) return nullptr;
    auto& c = cache();
    std::call_once(c.fOnce[d], [&c, d]() {
        const auto path = eGameDir::path(std::string("Model/") + gFileNames[d]);
        auto& file = c.fFile[d];
        if(eModelData::parseFile(path, file)) {
            for(const auto& e : file.fBuildings) c.fData[d].emplace(e.first, e.second);
            for(const auto& e : file.fByName) c.fByName[d].emplace(e.first, e.second);
        }
    });
    return &c;
}
}

bool eModelData::available() {
    int d = 0;
    const auto c = loaded(eDifficulty::hero, d);
    return c && !c->fData[d].empty();
}

const eModelBuilding* eModelData::building(const eDifficulty diff,
                                           const eBuildingType type) {
    int d = 0;
    const auto c = loaded(diff, d);
    if(!c) return nullptr;
    const auto& m = c->fData[d];
    const auto it = m.find(type);
    if(it == m.end()) return nullptr;
    return &it->second;
}

const eModelBuilding* eModelData::byName(const eDifficulty diff,
                                         const std::string& name) {
    int d = 0;
    const auto c = loaded(diff, d);
    if(!c) return nullptr;
    const auto& m = c->fByName[d];
    const auto it = m.find(name);
    if(it == m.end()) return nullptr;
    return &it->second;
}

const eModelHouse* eModelData::house(const eDifficulty diff,
                                     const bool elite, const int level) {
    int d = 0;
    const auto c = loaded(diff, d);
    if(!c) return nullptr;
    const auto& f = c->fFile[d];
    if(elite) {
        if(level < 0 || level > 4 || !f.fHasElite[level]) return nullptr;
        return &f.fElite[level];
    }
    if(level < 0 || level > 6 || !f.fHasCommon[level]) return nullptr;
    return &f.fCommon[level];
}

int eModelData::adjustEmployees(const eBuildingType type, const int eZeusValue) {
    if(eZeusValue <= 0) return eZeusValue;
    const auto m = building(eDifficulty::hero, type);
    if(!m || m->fEmployees <= 0 || m->fEmployees == eZeusValue) return eZeusValue;
    static std::set<eBuildingType> logged;
    static std::mutex lock;
    if(std::getenv("EZEUS_PARITY_LOG")) {
        std::lock_guard<std::mutex> l(lock);
        if(logged.insert(type).second) {
            std::fprintf(stderr, "[parity] employees type=%d eZeus=%d original=%d\n",
                         static_cast<int>(type), eZeusValue, m->fEmployees);
        }
    }
    return m->fEmployees;
}
