#include "earrowspearbase.h"

#include "engine/egameboard.h"
#include "textures/egametextures.h"

eArrowSpearBase::eArrowSpearBase(eGameBoard& board,
                                 const eMissileTexs missileTexs,
                                 const eMissileType type,
                                 const std::vector<ePathPoint>& path) :
    eMissile(board, type, path),
    mTexs(missileTexs) {}

std::shared_ptr<eTexture>
eArrowSpearBase::getTexture(const eTileSize size) const {
    auto& board = this->board();
    const auto dir = board.direction();
    const int id = static_cast<int>(size);
    const auto& textures = eGameTextures::destrution();
    const auto& rockTex = textures[id].*mTexs;
    const double a = angle();
    double da;
    if(dir == eWorldDirection::N) {
        da = 225;
    } else if(dir == eWorldDirection::E) {
        da = 315;
    } else if(dir == eWorldDirection::S) {
        da = 45;
    } else { // if(dir == eWorldDirection::W) {
        da = 135;
    }
    const int iid = std::round(((a - da)/11.25));
    const int texId = iid < 0 ? ((32 + iid) % 32) :
                                (iid % 32);
    if(type() == eMissileType::arrow && board.cheats().fBowvine) {
        // ZeusDroid cheat "Bowvine and Arrow": a flying cow instead.
        eGameTextures::loadCattle();
        const auto& chars = eGameTextures::characters();
        const auto& walk = chars[id].fCattle1.fWalk;
        if(!walk.empty()) {
            const int o = ((texId + 2)/4) % static_cast<int>(walk.size());
            const auto& coll = walk[o];
            if(coll.size() > 0) {
                return coll.getTexture(textureTime() % coll.size());
            }
        }
    }
    return rockTex.getTexture(texId);
}
