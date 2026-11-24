// ZeusDroid: cheats offered in the in-game Menu (see widgets/echeatswidget.cpp).
#include "egameboard.h"

#include "audio/esounds.h"
#include "buildings/ebuilding.h"
#include "buildings/eelitehousing.h"
#include "buildings/esmallhouse.h"
#include "engine/eprovide.h"
#include "missiles/elavamissile.h"
#include "erand.h"

void eGameBoard::cheatAddDrachmas(const int by) {
    const auto pid = personPlayer();
    setDrachmas(pid, drachmas(pid) + by);
}

void eGameBoard::cheatWinEpisode() {
    // The regular goals check (incTime) then runs the episode-complete
    // events and the victory screen, exactly as for fulfilled goals.
    mGoalsFulfilled = true;
    mGoalsCheckTime = 1000000;
}

void eGameBoard::cheatFeedHouses(const eCityId cid) {
    mCheatCity = cid;
    for(const auto b : mAllBuildings) {
        if(!b || b->cityId() != cid) continue;
        const auto t = b->type();
        if(t != eBuildingType::commonHouse &&
           t != eBuildingType::eliteHousing) continue;
        b->provide(eProvide::food, 100000);
    }
}

void eGameBoard::cheatFireball(const eCityId cid) {
    mCheatCity = cid;
    std::vector<eBuilding*> targets;
    for(const auto b : mAllBuildings) {
        if(!b || b->cityId() != cid) continue;
        if(!b->tileRect().w) continue;
        if(b->type() == eBuildingType::road) continue;
        targets.push_back(b);
    }
    if(targets.empty()) return;
    const auto b = targets[eRand::rand() % targets.size()];
    const auto ct = b->centerTile();
    if(!ct) return;
    const int tx = ct->x();
    const int ty = ct->y();
    const auto m = eMissile::sCreate<eLavaMissile>(
                       *this, tx - 3, ty - 3, 12., tx, ty, 0., 2.);
    if(!m) return;
    m->setSpeed(1.);
    m->incTime(0);
    // Path length is about 6 tiles; eMissile moves 0.025*speed tiles per ms.
    mCheatFireballHits.push_back({ct, 6000/25});
}

void eGameBoard::cheatsIncTime(const int by) {
    auto& c = mCheats;
    if(c.fUnlimitedFood) {
        c.fFoodTime += by;
        if(c.fFoodTime > 1000) {
            c.fFoodTime = 0;
            cheatFeedHouses(mCheatCity);
        }
    }
    if(c.fFireballs) {
        c.fFireballTime += by;
        if(c.fFireballTime > 1500) {
            c.fFireballTime = 0;
            cheatFireball(mCheatCity);
        }
    }
    for(int i = 0; i < (int)mCheatFireballHits.size(); i++) {
        auto& h = mCheatFireballHits[i];
        h.second -= by;
        if(h.second > 0) continue;
        const auto t = h.first;
        mCheatFireballHits.erase(mCheatFireballHits.begin() + i--);
        if(!t) continue;
        ifVisible(t, []() { eSounds::playFireballHitSound(); });
        const auto ub = t->underBuilding();
        if(ub && ub->type() != eBuildingType::road) {
            ub->setOnFire(true);
        }
    }
}
