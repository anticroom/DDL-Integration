#include "../DDLIntegration.hpp"
#include <Geode/binding/GJGameLevel.hpp>
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
            
            if (diffNode && !diffNode->getParent()->getChildByID("ddl-custom-face")) {
                diffNode->setVisible(false);

                auto customFace = CCSprite::create("ddl-demon.png"_spr);
                customFace->setID("ddl-custom-face");
                customFace->setPosition(diffNode->getPosition());
                
                float targetHeight = diffNode->getScaledContentSize().height;
                if (targetHeight < 10.0f) targetHeight = 35.0f; 
                
                customFace->setScale(targetHeight / customFace->getContentSize().height);
                customFace->setZOrder(diffNode->getZOrder());

                diffNode->getParent()->addChild(customFace);
            }
        }
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
                rankTextNode->setOpacity(152);
            } else {
                rankTextNode->setColor({ 255, 255, 255 });
                rankTextNode->setOpacity(200);
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