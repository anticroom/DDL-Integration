#include "../DDLIntegration.hpp"
#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/binding/LevelInfoLayer.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>

using namespace geode::prelude;

class $modify(DDLLevelInfoLayer, LevelInfoLayer) {
    bool init(GJGameLevel* level, bool challenge) {
        if (!LevelInfoLayer::init(level, challenge)) return false;

        int bestRank = 99999;
        auto levelID = level->m_levelID.value();

        if (DDLIntegration::ddlLoaded) {
            for (auto const& lvl : DDLIntegration::ddl) {
                if (lvl.id == levelID) {
                    if (lvl.position < bestRank) bestRank = lvl.position;
                    break;
                }
            }
        }
        
        if (DDLIntegration::dclLoaded) {
            for (auto const& lvl : DDLIntegration::dcl) {
                if (lvl.id == levelID) {
                    if (lvl.position < bestRank) bestRank = lvl.position;
                    break;
                }
            }
        }

        if (bestRank != 99999) {
            auto facePlacement = CCLabelBMFont::create(fmt::format("#", bestRank).c_str(), "bigFont.fnt");
            
            if (auto diffSprite = this->getChildByID("difficulty-sprite")) {
                facePlacement->setPosition({
                    diffSprite->getPositionX(),
                    diffSprite->getPositionY() - 36.0f 
                });
            } else {
                auto winSize = CCDirector::get()->getWinSize();
                facePlacement->setPosition({winSize.width / 2 - 135.0f, winSize.height / 2 + 10.0f});
            }
            
            facePlacement->setScale(0.40f);

            if (bestRank == 1) {
                facePlacement->setColor({255, 200, 50});
            } else if (bestRank == 2) {
                facePlacement->setColor({200, 200, 200});
            } else if (bestRank == 3) {
                facePlacement->setColor({210, 140, 70});
            } else {
                facePlacement->setColor({255, 255, 255});
            }

            facePlacement->setID("level-face-rank-label"_spr);
            this->addChild(facePlacement);
        }

        return true;
    }
};