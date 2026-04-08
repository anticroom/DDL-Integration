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
            if (auto diffNode = this->getChildByID("difficulty-sprite")) {
                if (!this->getChildByID("ddl-custom-face")) {
                    diffNode->setVisible(false);

                    auto customFace = CCSprite::create("ddl-demon.png"_spr);
                    customFace->setID("ddl-custom-face");
                    customFace->setPosition(diffNode->getPosition());
                    
                    float targetHeight = diffNode->getScaledContentSize().height;
                    if (targetHeight < 10.0f) targetHeight = 45.0f; 
                    
                    customFace->setScale(targetHeight / customFace->getContentSize().height);
                    customFace->setZOrder(diffNode->getZOrder());
                    
                    this->addChild(customFace);
                }
            }

            auto facePlacement = CCLabelBMFont::create(fmt::format("#{}", bestRank).c_str(), "bigFont.fnt");
            
            if (auto diffSprite = this->getChildByID("difficulty-sprite")) {
                float yOffset = 36.0f; 
                
                if (level->m_stars > 0) {
                    yOffset += 18.0f; 
                }
                if (level->m_coins > 0) {
                    yOffset += 18.0f; 
                }
                
                facePlacement->setPosition({
                    diffSprite->getPositionX(),
                    diffSprite->getPositionY() - yOffset 
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
            } else if (bestRank > 150) {
                facePlacement->setColor({255, 75, 75});
            } else {
                facePlacement->setColor({255, 255, 255});
            }

            facePlacement->setID("level-face-rank-label"_spr);
            this->addChild(facePlacement, 100);
        }

        return true;
    }
};