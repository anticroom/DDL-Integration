#include "DDLListLayer.hpp"
#include <Geode/binding/AppDelegate.hpp>
#include <Geode/binding/CustomListView.hpp>
#include <Geode/binding/GameLevelManager.hpp>
#include <Geode/binding/GameStatsManager.hpp>
#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/binding/GJListLayer.hpp>
#include <Geode/binding/InfoAlertButton.hpp>
#include <Geode/binding/LoadingCircle.hpp>
#include <Geode/binding/SetIDPopup.hpp>
#include <Geode/binding/GJSearchObject.hpp>
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/loader/Mod.hpp>
#include <Geode/ui/ListView.hpp>
#include <Geode/utils/random.hpp>

using namespace geode::prelude;

DDLListLayer* DDLListLayer::create() {
    auto ret = new DDLListLayer();
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

CCScene* DDLListLayer::scene() {
    auto ret = CCScene::create();
    AppDelegate::get()->m_runningScene = ret;
    ret->addChild(DDLListLayer::create());
    return ret;
}

bool dclEnabled = false;
const char* ddlInfo ="The Denouement Demon List is a list of levels that have the first few denouement inputs, the levels are ranked by difficulty of the level.";
const char* dclInfo = "The Denouement Challenge List is a list of challenges that have the first few denouement inputs with whatever is added afterwards, the challenges are ranked by difficulty of the challenges.";

bool DDLListLayer::init() {
    if (!CCLayer::init()) return false;

    setID("DDLListLayer");
    auto winSize = CCDirector::get()->getWinSize();

    m_pageCache = CCArray::create();
    m_pageCache->retain();

    auto bg = CCSprite::create("GJ_gradientBG.png");
    bg->setAnchorPoint(ccp(0.0f, 0.0f));
    bg->setScaleX((winSize.width + 10.0f) / bg->getTextureRect().size.width);
    bg->setScaleY((winSize.height + 10.0f) / bg->getTextureRect().size.height);
    bg->setPosition(ccp(-5.0f, -5.0f));
    bg->setColor({ 76, 33, 69 });
    bg->setID("background");
    addChild(bg);

    auto bottomLeftCorner = CCSprite::createWithSpriteFrameName("gauntletCorner_001.png");
    bottomLeftCorner->setPosition(ccp(-1.0f, -1.0f));
    bottomLeftCorner->setAnchorPoint(ccp(0.0f, 0.0f));
    bottomLeftCorner->setID("left-corner");
    addChild(bottomLeftCorner);

    auto bottomRightCorner = CCSprite::createWithSpriteFrameName("gauntletCorner_001.png");
    bottomRightCorner->setPosition(ccp(winSize.width + 1.0f, -1.0f));
    bottomRightCorner->setAnchorPoint(ccp(1.0f, 0.0f));
    bottomRightCorner->setFlipX(true);
    bottomRightCorner->setID("right-corner");
    addChild(bottomRightCorner);

    m_countLabel = CCLabelBMFont::create("", "goldFont.fnt");
    m_countLabel->setAnchorPoint(ccp(1.0f, 1.0f));
    m_countLabel->setScale(0.6f);
    m_countLabel->setPosition(ccp(winSize.width - 7.0f, winSize.height - 3.0f));
    m_countLabel->setID("level-count-label");
    addChild(m_countLabel);

    m_list = GJListLayer::create(nullptr, "DDL", { 0, 0, 0, 180 }, 356.0f, 220.0f, 0);
    m_list->setPosition(winSize / 2.0f - m_list->getContentSize() / 2.0f);
    m_list->setID("GJListLayer");
    addChild(m_list, 2);

    m_searchBarMenu = CCMenu::create();
    m_searchBarMenu->setContentSize({ 356.0f, 30.0f });
    m_searchBarMenu->setPosition(ccp(0.0f, 190.0f));
    m_searchBarMenu->setID("search-bar-menu");
    m_list->addChild(m_searchBarMenu);

    auto searchBackground = CCLayerColor::create({ 194, 114, 62, 255 }, 356.0f, 30.0f);
    searchBackground->setID("search-bar-background");
    m_searchBarMenu->addChild(searchBackground);

    auto searchSprite = CCSprite::createWithSpriteFrameName("gj_findBtn_001.png");
    searchSprite->setScale(0.7f);
    m_searchButton = CCMenuItemSpriteExtra::create(searchSprite, this, menu_selector(DDLListLayer::onSearch));
    m_searchButton->setPosition(ccp(337.0f, 15.0f));
    m_searchButton->setID("search-button");
    m_searchBarMenu->addChild(m_searchButton);

    m_searchBar = TextInput::create(310.0f, "Search...");
    m_searchBar->setPosition(ccp(165.0f, 15.0f));
    m_searchBar->setTextAlign(TextInputAlign::Left);
    auto inputNode = m_searchBar->getInputNode();
    inputNode->setLabelPlaceholderScale(0.4f);
    inputNode->setMaxLabelScale(0.4f);
    auto bgSprite = m_searchBar->getBGSprite();
    bgSprite->setContentSize({ 620.0f, 40.0f });
    bgSprite->setScale(0.5f);
    m_searchBar->setID("search-bar");
    m_searchBarMenu->addChild(m_searchBar);

    auto menu = CCMenu::create();
    menu->setPosition(ccp(0.0f, 0.0f));
    menu->setID("button-menu");
    addChild(menu);

    auto backButton = CCMenuItemSpriteExtra::create(
        CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png"), this, menu_selector(DDLListLayer::onBack)
    );
    backButton->setPosition(ccp(25.0f, winSize.height - 25.0f));
    backButton->setID("back-button");
    menu->addChild(backButton);

    auto leftBtnSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
    m_leftButton = CCMenuItemSpriteExtra::create(leftBtnSpr, this, menu_selector(DDLListLayer::onPrevPage));
    m_leftButton->setPosition(ccp(24.0f, winSize.height / 2.0f));
    m_leftButton->setID("prev-page-button");
    menu->addChild(m_leftButton);

    auto rightBtnSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
    rightBtnSpr->setFlipX(true);
    m_rightButton = CCMenuItemSpriteExtra::create(rightBtnSpr, this, menu_selector(DDLListLayer::onNextPage));
    m_rightButton->setPosition(ccp(winSize.width - 24.0f, winSize.height / 2.0f));
    m_rightButton->setID("next-page-button");
    menu->addChild(m_rightButton);
    
    m_infoButton = InfoAlertButton::create("DDL", gd::string(ddlInfo), 1.0f);
    m_infoButton->setPosition(ccp(30.0f, 30.0f));
    m_infoButton->setID("info-button");
    menu->addChild(m_infoButton, 2);

    auto modeSpr = ButtonSprite::create("View Packs", 40, true, "bigFont.fnt", "GJ_button_01.png", 30.0f, 0.5f);
    m_modeToggleBtn = CCMenuItemSpriteExtra::create(modeSpr, this, menu_selector(DDLListLayer::onModeToggle));
    m_modeToggleBtn->setPosition(ccp(45.0f, 100.0f));
    m_modeToggleBtn->setID("mode-toggle-button");
    menu->addChild(m_modeToggleBtn, 2);

    m_failure = [this](int code) {
        FLAlertLayer::create(fmt::format("Load Failed ({})", code).c_str(), "Failed to load API Data. Please try again later.", "OK")->show();
        m_loadingCircle->setVisible(false);
    };

    auto refreshBtnSpr = CCSprite::createWithSpriteFrameName("GJ_updateBtn_001.png");
    auto refreshButton = CCMenuItemSpriteExtra::create(refreshBtnSpr, this, menu_selector(DDLListLayer::onRefresh));
    refreshButton->setPosition(ccp(winSize.width - refreshBtnSpr->getContentWidth() / 2.0f - 4.0f, refreshBtnSpr->getContentHeight() / 2.0f + 4.0f));
    refreshButton->setID("refresh-button");
    menu->addChild(refreshButton, 2);

    auto starSprite = CCSprite::create("ddl-btn.png"_spr);
    starSprite->setScale(0.80f);
    m_starToggle = CCMenuItemSpriteExtra::create(starSprite, this, menu_selector(DDLListLayer::onStar));
    m_starToggle->setPosition(ccp(30.0f, 60.0f));
    m_starToggle->setColor(dclEnabled ? ccColor3B { 125, 125, 125 } : ccColor3B { 255, 255, 255 });
    m_starToggle->setID("ddl-button");
    menu->addChild(m_starToggle, 2);

    auto moonSprite = CCSprite::createWithSpriteFrameName("GJ_timeIcon_001.png");
    moonSprite->setScale(1.2f);
    m_moonToggle = CCMenuItemSpriteExtra::create(moonSprite, this, menu_selector(DDLListLayer::onMoon));
    m_moonToggle->setPosition(ccp(60.0f, 60.0f));
    m_moonToggle->setColor(dclEnabled ? ccColor3B { 255, 255, 255 } : ccColor3B { 125, 125, 125 });
    m_moonToggle->setID("dcl-button");
    menu->addChild(m_moonToggle, 2);

    auto pageBtnSpr = CCSprite::create("GJ_button_02.png");
    pageBtnSpr->setScale(0.7f);
    m_pageLabel = CCLabelBMFont::create("1", "bigFont.fnt");
    m_pageLabel->setScale(0.8f);
    m_pageLabel->setPosition(pageBtnSpr->getContentSize() / 2.0f);
    pageBtnSpr->addChild(m_pageLabel);
    m_pageButton = CCMenuItemSpriteExtra::create(pageBtnSpr, this, menu_selector(DDLListLayer::onPage));
    m_pageButton->setPositionY(winSize.height - 39.5f);
    m_pageButton->setID("page-button");
    menu->addChild(m_pageButton);

    auto randomSprite = CCSprite::create("BI_randomBtn_001.png"_spr);
    randomSprite->setScale(0.9f);
    m_randomButton = CCMenuItemSpriteExtra::create(randomSprite, this, menu_selector(DDLListLayer::onRandom));
    m_randomButton->setPositionY(m_pageButton->getPositionY() - m_pageButton->getContentHeight() / 2.0f - m_randomButton->getContentHeight() / 2.0f - 5.0f);
    m_randomButton->setID("random-button");
    menu->addChild(m_randomButton);

    auto lastArrow = CCSprite::createWithSpriteFrameName("GJ_arrow_02_001.png");
    lastArrow->setFlipX(true);
    auto otherLastArrow = CCSprite::createWithSpriteFrameName("GJ_arrow_02_001.png");
    otherLastArrow->setPosition(lastArrow->getContentSize() / 2.0f + ccp(20.0f, 0.0f));
    otherLastArrow->setFlipX(true);
    lastArrow->addChild(otherLastArrow);
    lastArrow->setScale(0.4f);
    m_lastButton = CCMenuItemSpriteExtra::create(lastArrow, this, menu_selector(DDLListLayer::onLast));
    m_lastButton->setPositionY(m_randomButton->getPositionY() - m_randomButton->getContentHeight() / 2.0f - m_lastButton->getContentHeight() / 2.0f - 5.0f);
    m_lastButton->setID("last-button");
    menu->addChild(m_lastButton);

    auto x = winSize.width - m_randomButton->getContentWidth() / 2.0f - 3.0f;
    m_pageButton->setPositionX(x);
    m_randomButton->setPositionX(x);
    m_lastButton->setPositionX(x - 4.0f);

    auto firstArrow = CCSprite::createWithSpriteFrameName("GJ_arrow_02_001.png");
    auto otherFirstArrow = CCSprite::createWithSpriteFrameName("GJ_arrow_02_001.png");
    otherFirstArrow->setPosition(firstArrow->getContentSize() / 2.0f - ccp(20.0f, 0.0f));
    firstArrow->addChild(otherFirstArrow);
    firstArrow->setScale(0.4f);
    m_firstButton = CCMenuItemSpriteExtra::create(firstArrow, this, menu_selector(DDLListLayer::onFirst));
    m_firstButton->setPosition(ccp(21.5f, m_lastButton->getPositionY()));
    m_firstButton->setID("first-button");
    menu->addChild(m_firstButton);

    m_loadingCircle = LoadingCircle::create();
    m_loadingCircle->setParentLayer(this);
    m_loadingCircle->setID("loading-circle");
    m_loadingCircle->show();

    showLoading();
    updateHeaders();
    setKeypadEnabled(true);
    setKeyboardEnabled(true);

    if (dclEnabled) {
        DDLIntegration::loadDCL(m_dclListener, [this] {
            DDLIntegration::loadDCLPacks(m_dclListener, [this] {
                populateList("");
            }, m_failure);
        }, m_failure);
    } else {
        DDLIntegration::loadDDL(m_ddlListener, [this] {
            DDLIntegration::loadDDLPacks(m_ddlListener, [this] {
                populateList("");
            }, m_failure);
        }, m_failure);
    }

    return true;
}

void DDLListLayer::updateHeaders() {
    std::string titleStr = dclEnabled ? "DCL" : "DDL";
    if (m_viewMode == 1) titleStr += " Packs";
    else if (m_viewMode == 2) titleStr += " Leaderboard";

    if (auto listTitle = static_cast<CCLabelBMFont*>(m_list->getChildByID("title"))) {
        listTitle->setString(titleStr.c_str());
        listTitle->limitLabelWidth(280.0f, 0.8f, 0.0f);
    }
}

void DDLListLayer::onModeToggle(CCObject* sender) {
    m_viewMode = (m_viewMode + 1) % 3;
    
    std::string btnText = "View Packs";
    if (m_viewMode == 1) btnText = "View LBoard";
    if (m_viewMode == 2) btnText = "View Levels";
    
    auto modeSpr = ButtonSprite::create(btnText.c_str(), 40, true, "bigFont.fnt", "GJ_button_01.png", 30.0f, 0.5f);
    m_modeToggleBtn->setNormalImage(modeSpr);
    
    m_searchBar->setString("");
    m_query = "";
    if (m_searchBar) {
        m_searchBar->getInputNode()->detachWithIME();
    }
    updateHeaders();
    
    if (m_viewMode == 2) {
        showLoading();
        if (dclEnabled) {
            if (!DDLIntegration::dclLoaded || DDLIntegration::dclPacks.empty()) {
                DDLIntegration::loadDCL(m_dclListener, [this] {
                    DDLIntegration::loadDCLPacks(m_dclListener, [this] {
                        DDLIntegration::loadLeaderboard(true, m_lboardListener, [this] { page(0); }, m_failure);
                    }, m_failure);
                }, m_failure);
            } else if (DDLIntegration::dclLeaderboard.empty()) {
                DDLIntegration::loadLeaderboard(true, m_lboardListener, [this] { page(0); }, m_failure);
            } else page(0);
        } else {
            if (!DDLIntegration::ddlLoaded || DDLIntegration::ddlPacks.empty()) {
                DDLIntegration::loadDDL(m_ddlListener, [this] {
                    DDLIntegration::loadDDLPacks(m_ddlListener, [this] {
                        DDLIntegration::loadLeaderboard(false, m_lboardListener, [this] { page(0); }, m_failure);
                    }, m_failure);
                }, m_failure);
            } else if (DDLIntegration::ddlLeaderboard.empty()) {
                DDLIntegration::loadLeaderboard(false, m_lboardListener, [this] { page(0); }, m_failure);
            } else page(0);
        }
    } else {
        page(0);
    }
}

void DDLListLayer::onBack(CCObject* sender) {
    if (m_searchBar) {
        m_searchBar->getInputNode()->detachWithIME();
    }
    
    CCDirector::get()->popSceneWithTransition(0.5f, kPopTransitionFade);
}

void DDLListLayer::onPrevPage(CCObject* sender) {
    page(m_page - 1);
}

void DDLListLayer::onNextPage(CCObject* sender) {
    page(m_page + 1);
}

void DDLListLayer::onRefresh(CCObject* sender) {
    showLoading();
    if (m_viewMode == 2) {
        if (dclEnabled) {
            DDLIntegration::loadDCL(m_dclListener, [this] {
                DDLIntegration::loadDCLPacks(m_dclListener, [this] {
                    DDLIntegration::loadLeaderboard(true, m_lboardListener, [this] { populateList(m_query); }, m_failure);
                }, m_failure);
            }, m_failure);
        } else {
            DDLIntegration::loadDDL(m_ddlListener, [this] {
                DDLIntegration::loadDDLPacks(m_ddlListener, [this] {
                    DDLIntegration::loadLeaderboard(false, m_lboardListener, [this] { populateList(m_query); }, m_failure);
                }, m_failure);
            }, m_failure);
        }
    } else if (m_viewMode == 1) {
        if (dclEnabled) {
            DDLIntegration::loadDCL(m_dclListener, [this] {
                DDLIntegration::loadDCLPacks(m_dclListener, [this] { populateList(m_query); }, m_failure);
            }, m_failure);
        } else {
            DDLIntegration::loadDDL(m_ddlListener, [this] {
                DDLIntegration::loadDDLPacks(m_ddlListener, [this] { populateList(m_query); }, m_failure);
            }, m_failure);
        }
    } else {
        if (dclEnabled) DDLIntegration::loadDCL(m_dclListener, [this] { populateList(m_query); }, m_failure);
        else DDLIntegration::loadDDL(m_ddlListener, [this] { populateList(m_query); }, m_failure);
    }
}

void DDLListLayer::onStar(CCObject* sender) {
    if (!dclEnabled) return;
    dclEnabled = false;
    m_starToggle->setColor({ 255, 255, 255 });
    m_moonToggle->setColor({ 125, 125, 125 });

    if (auto bg = getChildByID("background")) {
        bg->runAction(cocos2d::CCTintTo::create(0.5f, 76, 33, 69));
    }

    updateHeaders();
    showLoading();
    
    if (m_viewMode == 2) {
        if (!DDLIntegration::ddlLoaded || DDLIntegration::ddlPacks.empty()) {
            DDLIntegration::loadDDL(m_ddlListener, [this] {
                DDLIntegration::loadDDLPacks(m_ddlListener, [this] {
                    DDLIntegration::loadLeaderboard(false, m_lboardListener, [this] { page(0); }, m_failure);
                }, m_failure);
            }, m_failure);
        } else if (DDLIntegration::ddlLeaderboard.empty()) {
            DDLIntegration::loadLeaderboard(false, m_lboardListener, [this] { page(0); }, m_failure);
        } else {
            page(0);
        }
    } else if (DDLIntegration::ddlLoaded && !DDLIntegration::ddlPacks.empty()) {
        page(0);
    } else {
        DDLIntegration::loadDDL(m_ddlListener, [this] {
            DDLIntegration::loadDDLPacks(m_ddlListener, [this] { page(0); }, m_failure);
        }, m_failure);
    }
}

void DDLListLayer::onMoon(CCObject* sender) {
    if (dclEnabled) return;
    dclEnabled = true;
    m_starToggle->setColor({ 125, 125, 125 });
    m_moonToggle->setColor({ 255, 255, 255 });

    if (auto bg = getChildByID("background")) {
        bg->runAction(cocos2d::CCTintTo::create(0.5f, 60, 18, 76));
    }

    updateHeaders();
    showLoading();
    
    if (m_viewMode == 2) {
        if (!DDLIntegration::dclLoaded || DDLIntegration::dclPacks.empty()) {
            DDLIntegration::loadDCL(m_dclListener, [this] {
                DDLIntegration::loadDCLPacks(m_dclListener, [this] {
                    DDLIntegration::loadLeaderboard(true, m_lboardListener, [this] { page(0); }, m_failure);
                }, m_failure);
            }, m_failure);
        } else if (DDLIntegration::dclLeaderboard.empty()) {
            DDLIntegration::loadLeaderboard(true, m_lboardListener, [this] { page(0); }, m_failure);
        } else {
            page(0);
        }
    } else if (DDLIntegration::dclLoaded && !DDLIntegration::dclPacks.empty()) {
        page(0);
    } else {
        DDLIntegration::loadDCL(m_dclListener, [this] {
            DDLIntegration::loadDCLPacks(m_dclListener, [this] { page(0); }, m_failure);
        }, m_failure);
    }
}

void DDLListLayer::onSearch(CCObject* sender) {
    if (m_searchBar) {
        m_searchBar->getInputNode()->detachWithIME();
    }

    auto query = m_searchBar->getString();
    if (m_query != query) {
        m_page = 0;
        showLoading();
        populateList(query);
    }
}

void DDLListLayer::page(int page) {
    size_t activeSize = 0;
    if (m_viewMode == 0) activeSize = m_fullSearchResults.size();
    else if (m_viewMode == 1) activeSize = m_fullPackResults.size();
    else activeSize = m_fullLeaderboardResults.size();

    auto maxPage = (activeSize + 9) / 10;
    m_page = maxPage > 0 ? (maxPage + (page % maxPage)) % maxPage : 0;
    showLoading();
    populateList(m_query);
}

void DDLListLayer::onPage(CCObject* sender) {
    size_t activeSize = 0;
    if (m_viewMode == 0) activeSize = m_fullSearchResults.size();
    else if (m_viewMode == 1) activeSize = m_fullPackResults.size();
    else activeSize = m_fullLeaderboardResults.size();

    auto popup = SetIDPopup::create(m_page + 1, 1, (activeSize + 9) / 10, "Go to Page", "Go", true, 1, 60.0f, false, false);
    popup->m_delegate = this;
    popup->show();
}

void DDLListLayer::onRandom(CCObject* sender) {
    size_t activeSize = 0;
    if (m_viewMode == 0) activeSize = m_fullSearchResults.size();
    else if (m_viewMode == 1) activeSize = m_fullPackResults.size();
    else activeSize = m_fullLeaderboardResults.size();
    
    if (activeSize == 0) return;
    page(random::generate(0uz, (activeSize - 1) / 10));
}

void DDLListLayer::onFirst(CCObject* sender) {
    page(0);
}

void DDLListLayer::onLast(CCObject* sender) {
    size_t activeSize = 0;
    if (m_viewMode == 0) activeSize = m_fullSearchResults.size();
    else if (m_viewMode == 1) activeSize = m_fullPackResults.size();
    else activeSize = m_fullLeaderboardResults.size();
    
    if (activeSize == 0) return;
    page((activeSize - 1) / 10);
}

void DDLListLayer::showLoading() {
    m_pageLabel->setString(fmt::to_string(m_page + 1).c_str());
    m_loadingCircle->setVisible(true);
    if (auto listView = m_list->m_listView) listView->setVisible(false);
    m_searchBarMenu->setVisible(false);
    m_countLabel->setVisible(false);
    m_leftButton->setVisible(false);
    m_rightButton->setVisible(false);
    m_firstButton->setVisible(false);
    m_lastButton->setVisible(false);
    m_pageButton->setVisible(false);
    m_randomButton->setVisible(false);
}

void DDLListLayer::populateList(const std::string& query) {
    m_attemptedFetches.clear();
    m_query = query;
    auto searchSprite = static_cast<CCSprite*>(m_searchButton->getNormalImage());
    
    if (auto listView = m_list->m_listView) {
        listView->removeFromParent();
        listView->release();
        m_list->m_listView = nullptr;
    }

    if (m_viewMode == 2) {
        m_fullLeaderboardResults.clear();
        const auto& currentLboard = dclEnabled ? DDLIntegration::dclLeaderboard : DDLIntegration::ddlLeaderboard;

        if (query.empty()) {
            m_fullLeaderboardResults = currentLboard;
            searchSprite->setDisplayFrame(CCSpriteFrameCache::get()->spriteFrameByName("gj_findBtn_001.png"));
        } else {
            auto lowerQuery = string::toLower(query);
            for (auto& entry : currentLboard) {
                if (!string::toLower(entry.user).contains(lowerQuery)) continue;
                m_fullLeaderboardResults.push_back(entry);
            }
            auto texture = CCTextureCache::get()->addImage("DDL_findBtnOn_001.png"_spr, false);
            searchSprite->setDisplayFrame(CCSpriteFrame::createWithTexture(texture, { { 0.0f, 0.0f }, texture->getContentSize() }));
        }

        auto cells = CCArray::create();
        auto start = m_page * 10;
        auto size = m_fullLeaderboardResults.size();
        auto end = std::min<int>(size, (m_page + 1) * 10);
        
        for (auto it = m_fullLeaderboardResults.begin() + start; it < m_fullLeaderboardResults.begin() + end; ++it) {
            cells->addObject(DDLLeaderboardCell::create(*it));
        }
        
        auto listView = ListView::create(cells, 35.0f, 356.0f, 190.0f);
        listView->retain();
        m_list->addChild(listView, 6, 9);
        m_list->m_listView = listView;

        m_searchBarMenu->setVisible(true);
        if (size == 0) m_countLabel->setString("");
        else m_countLabel->setString(fmt::format("{} to {} of {}", start + 1, end, size).c_str());
        m_countLabel->limitLabelWidth(100.0f, 0.6f, 0.0f);
        m_countLabel->setVisible(true);
        m_loadingCircle->setVisible(false);
        
        if (size > 10) {
            auto maxPage = (size - 1) / 10;
            m_leftButton->setVisible(m_page > 0);
            m_rightButton->setVisible(m_page < maxPage);
            m_firstButton->setVisible(m_page > 0);
            m_lastButton->setVisible(m_page < maxPage);
            m_pageButton->setVisible(true);
            m_randomButton->setVisible(true);
        }
    }
    else if (m_viewMode == 1) {
        m_fullPackResults.clear();
        const auto& currentPacks = dclEnabled ? DDLIntegration::dclPacks : DDLIntegration::ddlPacks;

        if (query.empty()) {
            m_fullPackResults = currentPacks;
            searchSprite->setDisplayFrame(CCSpriteFrameCache::get()->spriteFrameByName("gj_findBtn_001.png"));
        } else {
            auto lowerQuery = string::toLower(query);
            for (auto& pack : currentPacks) {
                if (!string::toLower(pack.name).contains(lowerQuery)) continue;
                m_fullPackResults.push_back(pack);
            }
            auto texture = CCTextureCache::get()->addImage("DDL_findBtnOn_001.png"_spr, false);
            searchSprite->setDisplayFrame(CCSpriteFrame::createWithTexture(texture, { { 0.0f, 0.0f }, texture->getContentSize() }));
        }

        auto packs = CCArray::create();
        auto start = m_page * 10;
        auto size = m_fullPackResults.size();
        auto end = std::min<int>(size, (m_page + 1) * 10);
        std::string packTypeString = dclEnabled ? "DCL Pack" : "DDL Pack";
        
        for (auto it = m_fullPackResults.begin() + start; it < m_fullPackResults.begin() + end; ++it) {
            packs->addObject(DDLPackCell::create(it->name, it->points, it->levels, it->color, packTypeString));
        }
        
        auto listView = ListView::create(packs, 100.0f, 356.0f, 190.0f);
        listView->retain();
        m_list->addChild(listView, 6, 9);
        m_list->m_listView = listView;

        m_searchBarMenu->setVisible(true);
        if (size == 0) m_countLabel->setString("");
        else m_countLabel->setString(fmt::format("{} to {} of {}", start + 1, end, size).c_str());
        m_countLabel->limitLabelWidth(100.0f, 0.6f, 0.0f);
        m_countLabel->setVisible(true);
        m_loadingCircle->setVisible(false);
        
        if (size > 10) {
            auto maxPage = (size - 1) / 10;
            m_leftButton->setVisible(m_page > 0);
            m_rightButton->setVisible(m_page < maxPage);
            m_firstButton->setVisible(m_page > 0);
            m_lastButton->setVisible(m_page < maxPage);
            m_pageButton->setVisible(true);
            m_randomButton->setVisible(true);
        }
    } else {
        m_fullSearchResults.clear();
        
        if (query.empty()) {
            for (auto& level : dclEnabled ? DDLIntegration::dcl : DDLIntegration::ddl) {
                m_fullSearchResults.push_back(fmt::to_string(level.id));
            }
            searchSprite->setDisplayFrame(CCSpriteFrameCache::get()->spriteFrameByName("gj_findBtn_001.png"));
        } else {
            auto lowerQuery = string::toLower(query);
            for (auto& level : dclEnabled ? DDLIntegration::dcl : DDLIntegration::ddl) {
                if (!string::toLower(level.name).contains(lowerQuery)) continue;
                m_fullSearchResults.push_back(fmt::to_string(level.id));
            }
            auto texture = CCTextureCache::get()->addImage("DDL_findBtnOn_001.png"_spr, false);
            searchSprite->setDisplayFrame(CCSpriteFrame::createWithTexture(texture, { { 0.0f, 0.0f }, texture->getContentSize() }));
        }

        if (m_fullSearchResults.empty()) {
            loadLevelsFinished(CCArray::create(), "", 0);
            m_countLabel->setString("");
        } else {
            auto glm = GameLevelManager::get();
            glm->m_levelManagerDelegate = this;

            if (m_pageCache) m_pageCache->removeAllObjects();

            std::string idQuery = "";
            size_t startIdx = m_page * 10;
            size_t endIdx = std::min<size_t>(m_fullSearchResults.size(), startIdx + 10);
            for (size_t i = startIdx; i < endIdx; ++i) {
                if (i != startIdx) idQuery += ",";
                idQuery += m_fullSearchResults[i];
            }
            
            auto searchObject = GJSearchObject::create(SearchType::Type19, idQuery);

            if (auto storedLevels = glm->getStoredOnlineLevels(searchObject->getKey())) {
                loadLevelsFinished(storedLevels, "", 0);
                setupPageInfo("", "");
            } else {
                glm->getOnlineLevels(searchObject);
            }
        }
    }
}

void DDLListLayer::loadLevelsFinished(CCArray* levels, const char*, int) {
    if (m_viewMode != 0) return; 

    if (levels && levels->count() > 0) {
        if (!typeinfo_cast<GJGameLevel*>(levels->objectAtIndex(0))) return;
    }

    if (!m_pageCache) {
        m_pageCache = CCArray::create();
        m_pageCache->retain();
    }

    if (levels && levels->count() > 1) {
        m_pageCache->removeAllObjects();
        m_pageCache->addObjectsFromArray(levels);
    } 
    else if (levels && levels->count() == 1) {
        auto newLevel = static_cast<GJGameLevel*>(levels->objectAtIndex(0));
        bool replaced = false;
        for (int i = 0; i < m_pageCache->count(); i++) {
            auto cached = static_cast<GJGameLevel*>(m_pageCache->objectAtIndex(i));
            if (cached->m_levelID.value() == newLevel->m_levelID.value()) {
                m_pageCache->replaceObjectAtIndex(i, newLevel);
                replaced = true;
                break;
            }
        }
        if (!replaced) m_pageCache->addObject(newLevel);
    }

    if (auto listView = m_list->m_listView) {
        listView->removeFromParent();
        listView->release();
        m_list->m_listView = nullptr;
    }

    auto combinedLevels = CCArray::create();
    size_t start = m_page * 10;
    size_t end = std::min<size_t>(m_fullSearchResults.size(), start + 10);

    for (size_t i = start; i < end; i++) {
        int expectedID = 0;
        
        if (auto parsed = geode::utils::numFromString<int>(m_fullSearchResults[i])) {
            expectedID = parsed.unwrap();
        }

        GJGameLevel* foundLevel = nullptr;

        for (int j = 0; j < m_pageCache->count(); j++) {
            auto lvl = static_cast<GJGameLevel*>(m_pageCache->objectAtIndex(j));
            if (lvl->m_levelID.value() == expectedID) {
                foundLevel = lvl;
                break;
            }
        }

        if (!foundLevel) {
            foundLevel = GameLevelManager::get()->getSavedLevel(expectedID);
        }

        if (!foundLevel) {
            for (const auto& demon : (dclEnabled ? DDLIntegration::dcl : DDLIntegration::ddl)) {
                if (demon.id == expectedID) {
                    foundLevel = GJGameLevel::create();
                    foundLevel->m_levelID = demon.id;
                    foundLevel->m_levelName = demon.name;
                    foundLevel->m_creatorName = demon.author;
                    foundLevel->m_accountID = 0;
                    foundLevel->m_userID = 0;
                    foundLevel->m_levelType = GJLevelType::Saved;
                    foundLevel->m_unlisted = true;
                    foundLevel->m_stars = dclEnabled ? 0 : 10;
                    foundLevel->m_demon = 1;
                    foundLevel->m_songID = 714579; 
                    foundLevel->m_audioTrack = 0;   
                    foundLevel->m_demonDifficulty = 5; 
                    foundLevel->m_levelLength = 3;
                    break;
                }
            }
        }
        if (foundLevel) combinedLevels->addObject(foundLevel);
    }

    auto listView = CustomListView::create(combinedLevels, BoomListType::Level, 190.0f, 356.0f);
    listView->retain();
    m_list->addChild(listView, 6, 9);
    m_list->m_listView = listView;

    m_searchBarMenu->setVisible(true);
    m_countLabel->setVisible(true);
    m_loadingCircle->setVisible(false);
    auto size = m_fullSearchResults.size();
    if (size > 10) {
        auto maxPage = (size - 1) / 10;
        m_leftButton->setVisible(m_page > 0);
        m_rightButton->setVisible(m_page < maxPage);
        m_firstButton->setVisible(m_page > 0);
        m_lastButton->setVisible(m_page < maxPage);
        m_pageButton->setVisible(true);
        m_randomButton->setVisible(true);
    }
}

void DDLListLayer::loadLevelsFailed(const char*, int) {
    if (m_viewMode != 0) return;
    loadLevelsFinished(nullptr, "", 0);
}

void DDLListLayer::setupPageInfo(gd::string, const char*) {
    if (m_viewMode != 0) return;
    m_countLabel->setString(fmt::format("{} to {} of {}", m_page * 10 + 1,
        std::min<int>(m_fullSearchResults.size(), (m_page + 1) * 10), m_fullSearchResults.size()).c_str());
    m_countLabel->limitLabelWidth(100.0f, 0.6f, 0.0f);
}

void DDLListLayer::keyDown(enumKeyCodes key, double timestamp) {
    switch (key) {
        case KEY_Left:
        case CONTROLLER_Left:
            if (m_leftButton->isVisible()) page(m_page - 1);
            break;
        case KEY_Right:
        case CONTROLLER_Right:
            if (m_rightButton->isVisible()) page(m_page + 1);
            break;
        case KEY_Enter:
            onSearch(nullptr);
            break;
        default:
            CCLayer::keyDown(key, timestamp);
            break;
    }
}

void DDLListLayer::keyBackClicked() {
    onBack(nullptr);
}

void DDLListLayer::setIDPopupClosed(SetIDPopup*, int page) {
    size_t activeSize = 0;
    if (m_viewMode == 0) activeSize = m_fullSearchResults.size();
    else if (m_viewMode == 1) activeSize = m_fullPackResults.size();
    else activeSize = m_fullLeaderboardResults.size();

    if (activeSize == 0) return;
    m_page = std::clamp<int>(page - 1, 0, (activeSize - 1) / 10);
    showLoading();
    populateList(m_query);
}

DDLListLayer::~DDLListLayer() {
    if (m_pageCache) {
        m_pageCache->release();
        m_pageCache = nullptr;
    }

    auto glm = GameLevelManager::get();
    if (glm->m_levelManagerDelegate == this) glm->m_levelManagerDelegate = nullptr;
}