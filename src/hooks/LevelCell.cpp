#include "../DDLIntegration.hpp"
#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/binding/LevelBrowserLayer.hpp>
#include <Geode/binding/GJSearchObject.hpp>
#include <Geode/binding/GJDifficultySprite.hpp>
#include <Geode/modify/LevelCell.hpp>
#include <Geode/utils/general.hpp>
#include <jasmine/hook.hpp>
#include <jasmine/setting.hpp>

using namespace geode::prelude;

static TaskHolder<web::WebResponse> s_ddlListener;
static TaskHolder<web::WebResponse> s_dclListener;
static bool s_fetchingDDL = false;
static bool s_fetchingDCL = false;

class $modify(DDLLevelCell, LevelCell) {
    static void onModify(ModifyBase<ModifyDerive<DDLLevelCell, LevelCell>>& self) {
        (void)self.setHookPriorityAfterPost("LevelCell::loadFromLevel", "hiimjustin000.level_size");
        jasmine::hook::modify(self.m_hooks, "LevelCell::loadFromLevel", "enable-rank");
    }

    void loadFromLevel(GJGameLevel* level) {
        LevelCell::loadFromLevel(level);
        if (level->m_levelType == GJLevelType::Editor) return;

        auto levelID = level->m_levelID.value();
        std::vector<std::string> rankStrings;
        int bestRank = 99999;

        if (DDLIntegration::ddlLoaded) {
            for (auto const& lvl : DDLIntegration::ddl) {
                if (lvl.id == levelID) {
                    rankStrings.push_back(fmt::format("#{} DDL ({:.1f} pts)", lvl.position, DDLIntegration::calculateScore(lvl.position)));
                    
                    if (lvl.position < bestRank) bestRank = lvl.position;
                    break;
                }
            }
        } else if (!s_fetchingDDL) {
            s_fetchingDDL = true;
            DDLIntegration::loadDDL(s_ddlListener, [](){ s_fetchingDDL = false; }, [](int){ s_fetchingDDL = false; });
        }

        if (DDLIntegration::dclLoaded) {
            for (auto const& lvl : DDLIntegration::dcl) {
                if (lvl.id == levelID) {
                    rankStrings.push_back(fmt::format("#{} DCL ({:.1f} pts)", lvl.position, DDLIntegration::calculateScore(lvl.position)));
                    if (lvl.position < bestRank) bestRank = lvl.position;
                    break;
                }
            }
        } else if (!s_fetchingDCL) {
            s_fetchingDCL = true;
            DDLIntegration::loadDCL(s_dclListener, [](){ s_fetchingDCL = false; }, [](int){ s_fetchingDCL = false; });
        }

        if (!rankStrings.empty()) {
            this->addRank(rankStrings, bestRank);
            
            CCNode* diffNode = m_mainLayer->getChildByID("difficulty-sprite");
            if (!diffNode) {
                if (auto container = m_mainLayer->getChildByID("difficulty-container")) {
                    diffNode = container->getChildByID("difficulty-sprite");
                }
            }
            
            if (diffNode && !diffNode->getParent()->getChildByID("ddl-face-node")) {
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
                    faceSprite = "DDL_difficulty_10_btn_001.png"_spr;
                    ringSprite = "GJ_epicCoin2_001.png";
                } else if (bestRank == 3) {
                    faceSprite = "DDL_difficulty_10_btn_001.png"_spr;
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
                            if (auto pNode = static_cast<cocos2d::CCParticleSystemQuad*>(fakeDDLRateSprite->getChildren()->objectAtIndex(1))) {
                                pNode->retain();
                                
                                pNode->removeFromParentAndCleanup(false); 
                                
                                pNode->setPosition(customNode->getContentSize() / 2.0f);
                                customNode->addChild(pNode, -2); 
                                
                                pNode->resetSystem();
                                pNode->resumeSystem();
                                
                                pNode->release();
                            }
                        }
                    }

                    float targetHeight = diffNode->getScaledContentSize().height;
                    if (targetHeight < 10.0f) targetHeight = 35.0f; 
                    
                    customNode->setScale(targetHeight / customNode->getContentSize().height);
                    customNode->setZOrder(diffNode->getZOrder());

                    diffNode->getParent()->addChild(customNode);
                }
            }
        }

        if (level->m_unlisted && level->m_songID == 714579 && level->m_accountID == 0) {
            if (auto menu = m_mainLayer->getChildByID("main-menu")) {
                if (auto viewBtn = typeinfo_cast<CCMenuItemSpriteExtra*>(menu->getChildByID("view-button"))) {
                    viewBtn->setTarget(this, menu_selector(DDLLevelCell::onFakeDDLUnlistedView));
                }
            }
        }
    }

    void onFakeDDLUnlistedView(CCObject* sender) {
        auto searchObj = GJSearchObject::create(SearchType::Search, std::to_string(m_level->m_levelID.value()));
        auto browser = LevelBrowserLayer::scene(searchObj);
        CCDirector::get()->pushScene(CCTransitionFade::create(0.5f, browser));
    }

    void addRank(const std::vector<std::string>& ranks, int bestRank) {
        if (m_mainLayer->getChildByID("level-rank-label"_spr)) return;

        auto dailyLevel = m_level->m_dailyID.value() > 0;
        auto isWhite = dailyLevel || jasmine::setting::getValue<bool>("white-rank");

        geode::utils::StringBuffer<> positionsStr;
        for (size_t i = 0; i < ranks.size(); ++i) {
            if (i > 0) positionsStr.append(" / ");
            positionsStr.append(ranks[i]);
        }

        auto rankTextNode = CCLabelBMFont::create(positionsStr.c_str(), "bigFont.fnt");
        if (!rankTextNode) return;

        rankTextNode->setPosition(ccp(346.0f, dailyLevel ? 6.0f : 2.0f));
        rankTextNode->setAnchorPoint(ccp(1.0f, 0.0f));
        rankTextNode->setScale(m_compactView ? 0.25f : 0.35f);

        auto rlc = Loader::get()->getLoadedMod("raydeeux.revisedlevelcells");
        if (rlc && rlc->getSettingValue<bool>("enabled") && rlc->getSettingValue<bool>("blendingText")) {
            rankTextNode->setBlendFunc({ GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA });
        }
        
        if (bestRank == 1) {
            rankTextNode->setColor({255, 200, 50});
        } else if (bestRank == 2) {
            rankTextNode->setColor({200, 200, 200});
        } else if (bestRank == 3) {
            rankTextNode->setColor({210, 140, 70});
        } else if (bestRank > 150) {
            rankTextNode->setColor({255, 75, 75});
        } else {
            if (isWhite) {
                rankTextNode->setOpacity(255);
            } else {
                rankTextNode->setColor({ 255, 255, 255 });
                rankTextNode->setOpacity(255);
            }
        }
        
        rankTextNode->setID("level-rank-label"_spr);
        m_mainLayer->addChild(rankTextNode);

        if (auto levelSizeLabel = m_mainLayer->getChildByID("hiimjustin000.level_size/size-label")) {
            levelSizeLabel->setPosition(ccp(
                m_compactView ? 343.0f - rankTextNode->getScaledContentWidth() : 346.0f,
                m_compactView ? 1.0f : 12.0f
            ));
        }
    }
};