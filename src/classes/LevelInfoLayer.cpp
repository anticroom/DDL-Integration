#include "../DDLIntegration.hpp"
#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/binding/LevelInfoLayer.hpp>
#include <Geode/binding/GJDifficultySprite.hpp>
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
                if (!this->getChildByID("ddl-face-node")) {
                    diffNode->setVisible(false);

                    auto customNode = CCNode::create();
                    customNode->setID("ddl-face-node");
                    customNode->setPosition(diffNode->getPosition());
                    customNode->setAnchorPoint({0.5f, 0.5f});
                    
                    std::string faceSprite = "DDL_difficulty_09_btn_001.png"_spr;
                    std::string ringSprite = "GJ_featuredCoin_001.png";

                    if (bestRank == 1) {
                        faceSprite = "DDL_difficulty_10_btn_001.png"_spr;
                        ringSprite = "GJ_epicCoin3_001.png";
                    } else if (bestRank == 2) {
                        ringSprite = "GJ_epicCoin2_001.png";
                    } else if (bestRank == 3) {
                        ringSprite = "GJ_epicCoin_001.png";
                    }   

                    auto ring = CCSprite::createWithSpriteFrameName(ringSprite.c_str());
                    auto face = CCSprite::create(faceSprite.c_str());
                    
                    auto fakeDDLRateFace = CCSprite::createWithSpriteFrameName("difficulty_09_btn_001.png");

                    if (ring && face && fakeDDLRateFace) {
                        customNode->setContentSize(fakeDDLRateFace->getContentSize());
                        ring->setPosition(customNode->getContentSize() / 2.0f);
                        face->setPosition(customNode->getContentSize() / 2.0f);
                        
                        face->setScaleX(fakeDDLRateFace->getContentSize().width / face->getContentSize().width);
                        face->setScaleY(fakeDDLRateFace->getContentSize().height / face->getContentSize().height);
                        
                        ring->setScale(1.0f);

                        customNode->addChild(ring, -1);
                        customNode->addChild(face, 0);

                        if (bestRank <= 3) {
                            auto fakeDDLRateLevel = GJGameLevel::create();
                            fakeDDLRateLevel->m_featured = 1;
                            if (bestRank == 1) fakeDDLRateLevel->m_isEpic = 3;      
                            else if (bestRank == 2) fakeDDLRateLevel->m_isEpic = 2; 
                            else if (bestRank == 3) fakeDDLRateLevel->m_isEpic = 1; 

                            auto fakeDDLRateSprite = GJDifficultySprite::create(1, static_cast<GJDifficultyName>(0));
                            fakeDDLRateSprite->updateFeatureStateFromLevel(fakeDDLRateLevel);

                            if (fakeDDLRateSprite->getChildrenCount() > 1) {
                                if (auto pNode = static_cast<cocos2d::CCNode*>(fakeDDLRateSprite->getChildren()->objectAtIndex(1))) {
                                    pNode->retain();
                                    pNode->removeFromParentAndCleanup(false);
                                    pNode->setPosition(customNode->getContentSize() / 2.0f);
                                    customNode->addChild(pNode, -2);
                                    pNode->release();
                                }
                            }
                        }

                        float targetHeight = diffNode->getScaledContentSize().height;
                        if (targetHeight < 10.0f) targetHeight = 45.0f; 
                        
                        customNode->setScale(targetHeight / customNode->getContentSize().height);
                        customNode->setZOrder(diffNode->getZOrder());
                        
                        this->addChild(customNode);
                    }
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
            
            facePlacement->setScale(0.5f);

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