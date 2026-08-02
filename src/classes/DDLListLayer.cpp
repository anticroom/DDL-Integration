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
#include <Geode/binding/GameManager.hpp>
#include <Geode/utils/random.hpp>
#include <cctype>

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
const char* ddlInfo = "The Denouement Demon List is a list of levels that have the first few denouement inputs, the levels are ranked by difficulty of the level.";
const char* dclInfo = "The Denouement Challenge List is a list of challenges that have the first few denouement inputs with whatever is added afterwards, the challenges are ranked by difficulty of the challenges.";

bool DDLListLayer::init() {
    if (!CCLayer::init()) return false;

    setID("DDLListLayer");
    auto winSize = CCDirector::get()->getWinSize();

    m_pageCache = CCArray::create();
    m_pageCache->retain();

    auto ddlColor = geode::Mod::get()->getSettingValue<cocos2d::ccColor3B>("ddl-bg-color");
    auto dclColor = geode::Mod::get()->getSettingValue<cocos2d::ccColor3B>("dcl-bg-color");
    auto bgColor = dclEnabled ? dclColor : ddlColor;
    float scrollSpeed = static_cast<float>(geode::Mod::get()->getSettingValue<int>("scroll-speed"));

    ccColor3B lightBgColor = {
        static_cast<GLubyte>(std::min(255, bgColor.r + 60)),
        static_cast<GLubyte>(std::min(255, bgColor.g + 60)),
        static_cast<GLubyte>(std::min(255, bgColor.b + 60))
    };

    auto bg = CCSprite::create("game_bg_01_001.png");
    bg->setColor(lightBgColor);
    
    ccTexParams bgParams = {GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT};
    bg->getTexture()->setTexParameters(&bgParams);
    
    float bgWidth = bg->getContentSize().width;
    float bgHeight = bg->getContentSize().height;
    
    float bgScale = (winSize.height / bgHeight) * 1.35f;
    bg->setScale(bgScale);
    
    bg->setTextureRect({0, 0, (winSize.width / bgScale) + (bgWidth * 2.0f), bgHeight});
    bg->setAnchorPoint({0, 0});
    
    float yOffset = -(bgHeight * bgScale - winSize.height) / 2.0f;
    bg->setPosition({0, yOffset});
    bg->setID("scrolling-background");

    auto bgMirroredTop = CCSprite::create("game_bg_01_001.png");
    bgMirroredTop->setColor(lightBgColor);
    bgMirroredTop->getTexture()->setTexParameters(&bgParams);
    bgMirroredTop->setTextureRect({0, 0, (winSize.width / bgScale) + (bgWidth * 2.0f), bgHeight});
    bgMirroredTop->setAnchorPoint({0, 0});
    bgMirroredTop->setPosition({0, bgHeight});
    bgMirroredTop->setFlipY(true);
    bg->addChild(bgMirroredTop);

    auto bgMirroredBottom = CCSprite::create("game_bg_01_001.png");
    bgMirroredBottom->setColor(lightBgColor);
    bgMirroredBottom->getTexture()->setTexParameters(&bgParams);
    bgMirroredBottom->setTextureRect({0, 0, (winSize.width / bgScale) + (bgWidth * 2.0f), bgHeight});
    bgMirroredBottom->setAnchorPoint({0, 0});
    bgMirroredBottom->setPosition({0, -bgHeight});
    bgMirroredBottom->setFlipY(true);
    bg->addChild(bgMirroredBottom);

    addChild(bg);

    float bgLoopDist = bgWidth * bgScale;
    float bgTime = scrollSpeed * (bgLoopDist / winSize.width);
    
    bg->runAction(CCRepeatForever::create(CCSequence::create(
        CCMoveBy::create(bgTime, ccp(-bgLoopDist, 0)),
        CCMoveTo::create(0.0f, ccp(0, yOffset)),
        nullptr
    )));

    auto bottomLeftCorner = CCSprite::createWithSpriteFrameName("GJ_sideArt_001.png");
    bottomLeftCorner->setPosition(ccp(-1.0f, -1.0f));
    bottomLeftCorner->setAnchorPoint(ccp(0.0f, 0.0f));
    bottomLeftCorner->setID("left-corner");
    addChild(bottomLeftCorner, 2);

    auto bottomRightCorner = CCSprite::createWithSpriteFrameName("GJ_sideArt_001.png");
    bottomRightCorner->setPosition(ccp(winSize.width + 1.0f, -1.0f));
    bottomRightCorner->setAnchorPoint(ccp(1.0f, 0.0f));
    bottomRightCorner->setFlipX(true);
    bottomRightCorner->setID("right-corner");
    addChild(bottomRightCorner, 2);

    m_countLabel = CCLabelBMFont::create("", "goldFont.fnt");
    m_countLabel->setAnchorPoint(ccp(1.0f, 1.0f));
    m_countLabel->setScale(0.6f);
    m_countLabel->setPosition(ccp(winSize.width - 7.0f, winSize.height - 3.0f));
    m_countLabel->setID("level-count-label");
    addChild(m_countLabel, 2);

    m_list = GJListLayer::create(nullptr, "DDL", { 0, 0, 0, 180 }, 356.0f, 220.0f, 0);
    m_list->setPosition(winSize / 2.0f - m_list->getContentSize() / 2.0f);
    m_list->setID("GJListLayer");
    addChild(m_list, 3);

    auto createMainTab = [](const char* bgName, const char* text, int tag, float yPos, CCObject* target, SEL_MenuHandler selector) {
        auto container = cocos2d::CCNode::create();
        container->setContentSize({ 40.0f, 95.0f }); 

        auto bg = cocos2d::CCSprite::create(geode::Mod::get()->expandSpriteName(bgName).c_str());
        bg->setID(fmt::format("tab-bg-{}", tag));
        bg->setPosition(ccp(25.0f, 32.5f));
        bg->setScale(0.7f);
        bg->setRotation(-90.0f);
        container->addChild(bg);

        auto label = cocos2d::CCLabelBMFont::create(text, "bigFont.fnt");
        label->setAnchorPoint(ccp(0.5f, 0.5f));
        label->setPosition(ccp(17.0f, 32.5f));
        label->setRotation(-90.0f);
        label->limitLabelWidth(58.0f, tag == 2 ? 0.30f : 0.35f, 0.0f);
        container->addChild(label);

        auto tabBtn = CCMenuItemSpriteExtra::create(container, target, selector);
        tabBtn->setTag(tag);
        tabBtn->setPosition(ccp(-18.0f, yPos));
        tabBtn->setSizeMult(1.1f);
        tabBtn->useAnimationType(MenuAnimationType::Move);
        return tabBtn;
    };

    auto levelsTabBtn = createMainTab("DDL_tab-001.png", "LEVELS", 0, 180.0f, this, menu_selector(DDLListLayer::onModeToggle));
    auto packsTabBtn = createMainTab("DDL_tab-002.png", "PACKS", 1, 115.0f, this, menu_selector(DDLListLayer::onModeToggle));
    auto lboardTabBtn = createMainTab("DDL_tab-002.png", "LBOARD", 2, 50.0f, this, menu_selector(DDLListLayer::onModeToggle));

    if (auto bg = levelsTabBtn->getNormalImage()->getChildByID("tab-bg-0")) {
        static_cast<CCSprite*>(bg)->setScaleX(0.7f * 1.1f);
    }

    auto tabMenu = cocos2d::CCMenu::create();
    tabMenu->setPosition(ccp(0.0f, 0.0f));
    tabMenu->setID("side-tabs-menu");
    tabMenu->addChild(levelsTabBtn);
    tabMenu->addChild(packsTabBtn);
    tabMenu->addChild(lboardTabBtn);
    m_list->addChild(tabMenu);

    m_searchBarMenu = CCMenu::create();
    m_searchBarMenu->setContentSize({ 356.0f, 30.0f });
    m_searchBarMenu->setPosition(ccp(0.0f, 190.0f));
    m_searchBarMenu->setID("search-bar-menu");
    m_list->addChild(m_searchBarMenu);

    m_profileOverlay = CCLayer::create();
    m_profileOverlay->setContentSize(winSize);
    m_profileOverlay->setPosition(ccp(0.0f, 0.0f));
    m_profileOverlay->setVisible(false);
    
    auto touchBlockerMenu = CCMenu::create();
    auto touchBlockerSpr = CCSprite::create("square02b_001.png");
    touchBlockerSpr->setContentSize(winSize * 2);
    touchBlockerSpr->setOpacity(0);
    auto touchBlockerBtn = CCMenuItemSpriteExtra::create(touchBlockerSpr, nullptr, nullptr);
    touchBlockerMenu->addChild(touchBlockerBtn);
    m_profileOverlay->addChild(touchBlockerMenu, -1);
    
    addChild(m_profileOverlay, 999);

    auto overlayBg = CCLayerColor::create({ 0, 0, 0, 180 }, winSize.width, winSize.height);
    overlayBg->setPosition(ccp(0.0f, 0.0f));
    m_profileOverlay->addChild(overlayBg, 0);

    m_profileStatsNode = CCNode::create();
    m_profileStatsNode->setContentSize(winSize);
    m_profileOverlay->addChild(m_profileStatsNode, 1);

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
    auto searchBgSprite = m_searchBar->getBGSprite();
    searchBgSprite->setContentSize({ 620.0f, 40.0f });
    searchBgSprite->setScale(0.5f);
    m_searchBar->setID("search-bar");
    m_searchBarMenu->addChild(m_searchBar);

    auto menu = CCMenu::create();
    menu->setPosition(ccp(0.0f, 0.0f));
    menu->setID("button-menu");
    addChild(menu, 3);

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
    menu->addChild(m_infoButton);

    m_failure = [ this ](int code) {
        FLAlertLayer::create(fmt::format("Load Failed ({})", code).c_str(), "Failed to load API Data. Please try again later.", "OK")->show();
        m_loadingCircle->setVisible(false);
    };

    auto refreshBtnSpr = CCSprite::createWithSpriteFrameName("GJ_updateBtn_001.png");
    auto refreshButton = CCMenuItemSpriteExtra::create(refreshBtnSpr, this, menu_selector(DDLListLayer::onRefresh));
    refreshButton->setPosition(ccp(winSize.width - refreshBtnSpr->getContentWidth() / 2.0f - 4.0f, refreshBtnSpr->getContentHeight() / 2.0f + 4.0f));
    refreshButton->setID("refresh-button");
    menu->addChild(refreshButton);

    auto starSprite = CCSprite::create("ddl-btn.png"_spr);
    starSprite->setScale(0.80f);
    m_starToggle = CCMenuItemSpriteExtra::create(starSprite, this, menu_selector(DDLListLayer::onStar));
    m_starToggle->setPosition(ccp(30.0f, 60.0f));
    m_starToggle->setColor(dclEnabled ? ccColor3B { 125, 125, 125 } : ccColor3B { 255, 255, 255 });
    m_starToggle->setID("ddl-button");
    menu->addChild(m_starToggle);

    auto moonSprite = CCSprite::createWithSpriteFrameName("GJ_timeIcon_001.png");
    moonSprite->setScale(1.2f);
    m_moonToggle = CCMenuItemSpriteExtra::create(moonSprite, this, menu_selector(DDLListLayer::onMoon));
    m_moonToggle->setPosition(ccp(60.0f, 60.0f));
    m_moonToggle->setColor(dclEnabled ? ccColor3B { 255, 255, 255 } : ccColor3B { 125, 125, 125 });
    m_moonToggle->setID("dcl-button");
    menu->addChild(m_moonToggle);

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
    setTouchEnabled(true);

    if (dclEnabled) {
        DDLIntegration::loadDCL(m_dclListener, [ this ] {
            DDLIntegration::loadDCLPacks(m_dclListener, [ this ] {
                populateList("");
            }, m_failure);
        }, m_failure);
    } else {
        DDLIntegration::loadDDL(m_ddlListener, [ this ] {
            DDLIntegration::loadDDLPacks(m_ddlListener, [ this ] {
                populateList("");
            }, m_failure);
        }, m_failure);
    }

    return true;
}

void DDLListLayer::registerWithTouchDispatcher() {
    CCTouchDispatcher::get()->addTargetedDelegate(this, -600, false);
}

bool DDLListLayer::ccTouchBegan(cocos2d::CCTouch* touch, cocos2d::CCEvent*) {
    if (m_searchBar && m_searchBarMenu && m_searchBarMenu->isVisible()) {
        auto localPos = m_searchBarMenu->convertToNodeSpace(touch->getLocation());
        if (!m_searchBar->boundingBox().containsPoint(localPos)) {
            m_searchBar->defocus();
        }
    }
    return false;
}

void DDLListLayer::onExit() {
    if (m_searchBar) {
        m_searchBar->defocus();
    }
    CCLayer::onExit();
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
    auto btn = static_cast<CCMenuItemSpriteExtra*>(sender);
    if (!btn) return;

    int targetMode = btn->getTag(); 
    if (m_viewMode == targetMode) return; 
    
    m_viewMode = targetMode;

    if (auto tabMenu = m_list->getChildByID("side-tabs-menu")) {
        for (int i = 0; i <= 2; i++) {
            if (auto tabBtn = static_cast<CCMenuItemSpriteExtra*>(tabMenu->getChildByTag(i))) {
                if (auto bg = tabBtn->getNormalImage()->getChildByID(fmt::format("tab-bg-{}", i).c_str())) {
                    auto tex = CCTextureCache::get()->addImage(
                        geode::Mod::get()->expandSpriteName(m_viewMode == i ? "DDL_tab-001.png" : "DDL_tab-002.png").c_str(), false
                    );
                    if (tex) static_cast<CCSprite*>(bg)->setTexture(tex);
                    static_cast<CCSprite*>(bg)->setScaleX(m_viewMode == i ? 0.7f * 1.1f : 0.7f);
                }
                tabBtn->setPositionX(-18.0f);
            }
        }
    }

    m_searchBar->setString("");
    m_query = "";
    if (m_searchBar) {
        m_searchBar->defocus();
    }
    updateHeaders();
    
    showLoading();
    
    if (m_viewMode == 2) {
        if (dclEnabled) {
            if (!DDLIntegration::dclLoaded || DDLIntegration::dclPacks.empty()) {
                DDLIntegration::loadDCL(m_dclListener, [ this ] {
                    DDLIntegration::loadDCLPacks(m_dclListener, [ this ] {
                        DDLIntegration::loadLeaderboard(true, m_lboardListener, [ this ] { page(0); }, m_failure);
                    }, m_failure);
                }, m_failure);
            } else if (DDLIntegration::dclLeaderboard.empty()) {
                DDLIntegration::loadLeaderboard(true, m_lboardListener, [ this ] { page(0); }, m_failure);
            } else page(0);
        } else {
            if (!DDLIntegration::ddlLoaded || DDLIntegration::ddlPacks.empty()) {
                DDLIntegration::loadDDL(m_ddlListener, [ this ] {
                    DDLIntegration::loadDDLPacks(m_ddlListener, [ this ] {
                        DDLIntegration::loadLeaderboard(false, m_lboardListener, [ this ] { page(0); }, m_failure);
                    }, m_failure);
                }, m_failure);
            } else if (DDLIntegration::ddlLeaderboard.empty()) {
                DDLIntegration::loadLeaderboard(false, m_lboardListener, [ this ] { page(0); }, m_failure);
            } else page(0);
        }
    } else {
        if (dclEnabled) {
            if (!DDLIntegration::dclLoaded || DDLIntegration::dclPacks.empty()) {
                DDLIntegration::loadDCL(m_dclListener, [ this ] {
                    DDLIntegration::loadDCLPacks(m_dclListener, [ this ] { page(0); }, m_failure);
                }, m_failure);
            } else page(0);
        } else {
            if (!DDLIntegration::ddlLoaded || DDLIntegration::ddlPacks.empty()) {
                DDLIntegration::loadDDL(m_ddlListener, [ this ] {
                    DDLIntegration::loadDDLPacks(m_ddlListener, [ this ] { page(0); }, m_failure);
                }, m_failure);
            } else page(0);
        }
    }
}

void DDLListLayer::onBack(CCObject* sender) {
    if (m_searchBar) {
        m_searchBar->defocus();
    }
    CCDirector::get()->popSceneWithTransition(0.5f, kPopTransitionFade);
}

void DDLListLayer::onPrevPage(CCObject* sender) { page(m_page - 1); }
void DDLListLayer::onNextPage(CCObject* sender) { page(m_page + 1); }

void DDLListLayer::onRefresh(CCObject* sender) {
    showLoading();
    if (m_viewMode == 2) {
        if (dclEnabled) {
            DDLIntegration::loadDCL(m_dclListener, [ this ] {
                DDLIntegration::loadDCLPacks(m_dclListener, [ this ] {
                    DDLIntegration::loadLeaderboard(true, m_lboardListener, [ this ] { populateList(m_query); }, m_failure);
                }, m_failure);
            }, m_failure);
        } else {
            DDLIntegration::loadDDL(m_ddlListener, [ this ] {
                DDLIntegration::loadDDLPacks(m_ddlListener, [ this ] {
                    DDLIntegration::loadLeaderboard(false, m_lboardListener, [ this ] { populateList(m_query); }, m_failure);
                }, m_failure);
            }, m_failure);
        }
    } else if (m_viewMode == 1) {
        if (dclEnabled) {
            DDLIntegration::loadDCL(m_dclListener, [ this ] {
                DDLIntegration::loadDCLPacks(m_dclListener, [ this ] { populateList(m_query); }, m_failure);
            }, m_failure);
        } else {
            DDLIntegration::loadDDL(m_ddlListener, [ this ] {
                DDLIntegration::loadDDLPacks(m_ddlListener, [ this ] { populateList(m_query); }, m_failure);
            }, m_failure);
        }
    } else {
        if (dclEnabled) DDLIntegration::loadDCL(m_dclListener, [ this ] { populateList(m_query); }, m_failure);
        else DDLIntegration::loadDDL(m_ddlListener, [ this ] { populateList(m_query); }, m_failure);
    }
}

void DDLListLayer::onStar(CCObject* sender) {
    if (!dclEnabled) return;
    dclEnabled = false;
    m_starToggle->setColor({ 255, 255, 255 });
    m_moonToggle->setColor({ 125, 125, 125 });

    auto targetColor = geode::Mod::get()->getSettingValue<cocos2d::ccColor3B>("ddl-bg-color");
    GLubyte tr = std::min(255, targetColor.r + 60);
    GLubyte tg = std::min(255, targetColor.g + 60);
    GLubyte tb = std::min(255, targetColor.b + 60);

    if (auto bg = getChildByID("scrolling-background")) {
        bg->runAction(cocos2d::CCTintTo::create(0.5f, tr, tg, tb));
        if (bg->getChildrenCount() > 0) {
            auto children = bg->getChildren();
            for (int i = 0; i < children->count(); i++) {
                if (auto child = typeinfo_cast<CCSprite*>(children->objectAtIndex(i))) {
                    child->runAction(cocos2d::CCTintTo::create(0.5f, tr, tg, tb));
                }
            }
        }
    }

    updateHeaders();
    showLoading();
    
    if (m_viewMode == 2) {
        if (!DDLIntegration::ddlLoaded || DDLIntegration::ddlPacks.empty()) {
            DDLIntegration::loadDDL(m_ddlListener, [ this ] {
                DDLIntegration::loadDDLPacks(m_ddlListener, [ this ] {
                    DDLIntegration::loadLeaderboard(false, m_lboardListener, [ this ] { page(0); }, m_failure);
                }, m_failure);
            }, m_failure);
        } else if (DDLIntegration::ddlLeaderboard.empty()) {
            DDLIntegration::loadLeaderboard(false, m_lboardListener, [ this ] { page(0); }, m_failure);
        } else {
            page(0);
        }
    } else if (DDLIntegration::ddlLoaded && !DDLIntegration::ddlPacks.empty()) {
        page(0);
    } else {
        DDLIntegration::loadDDL(m_ddlListener, [ this ] {
            DDLIntegration::loadDDLPacks(m_ddlListener, [ this ] { page(0); }, m_failure);
        }, m_failure);
    }
}

void DDLListLayer::onMoon(CCObject* sender) {
    if (dclEnabled) return;
    dclEnabled = true;
    m_starToggle->setColor({ 125, 125, 125 });
    m_moonToggle->setColor({ 255, 255, 255 });

    auto targetColor = geode::Mod::get()->getSettingValue<cocos2d::ccColor3B>("dcl-bg-color");
    GLubyte tr = std::min(255, targetColor.r + 60);
    GLubyte tg = std::min(255, targetColor.g + 60);
    GLubyte tb = std::min(255, targetColor.b + 60);

    if (auto bg = getChildByID("scrolling-background")) {
        bg->runAction(cocos2d::CCTintTo::create(0.5f, tr, tg, tb));
        if (bg->getChildrenCount() > 0) {
            auto children = bg->getChildren();
            for (int i = 0; i < children->count(); i++) {
                if (auto child = typeinfo_cast<CCSprite*>(children->objectAtIndex(i))) {
                    child->runAction(cocos2d::CCTintTo::create(0.5f, tr, tg, tb));
                }
            }
        }
    }

    updateHeaders();
    showLoading();
    
    if (m_viewMode == 2) {
        if (!DDLIntegration::dclLoaded || DDLIntegration::dclPacks.empty()) {
            DDLIntegration::loadDCL(m_dclListener, [ this ] {
                DDLIntegration::loadDCLPacks(m_dclListener, [ this ] {
                    DDLIntegration::loadLeaderboard(true, m_lboardListener, [ this ] { page(0); }, m_failure);
                }, m_failure);
            }, m_failure);
        } else if (DDLIntegration::dclLeaderboard.empty()) {
            DDLIntegration::loadLeaderboard(true, m_lboardListener, [ this ] { page(0); }, m_failure);
        } else {
            page(0);
        }
    } else if (DDLIntegration::dclLoaded && !DDLIntegration::dclPacks.empty()) {
        page(0);
    } else {
        DDLIntegration::loadDCL(m_dclListener, [ this ] {
            DDLIntegration::loadDCLPacks(m_dclListener, [ this ] { page(0); }, m_failure);
        }, m_failure);
    }
}

void DDLListLayer::onSearch(CCObject* sender) {
    if (m_searchBar) {
        m_searchBar->defocus();
    }

    auto query = m_searchBar->getString();
    if (m_query != query) {
        m_page = 0;
        showLoading();
        populateList(query);
    }
}

void DDLListLayer::onProfileClose(CCObject* sender) {
    closeProfilePage();
}

void DDLListLayer::onToggleProfileTab(CCObject* sender) {
    auto btn = static_cast<CCMenuItemSpriteExtra*>(sender);
    if (!btn) return;

    int targetTab = btn->getTag();
    if (m_profileTab == targetTab) return;

    m_profileTab = targetTab;
    
    if (m_verifsListView) m_verifsListView->setVisible(m_profileTab == 0);
    if (m_complListView) m_complListView->setVisible(m_profileTab == 1);
    if (m_packsListView) m_packsListView->setVisible(m_profileTab == 2);

    auto greenTex = CCTextureCache::get()->addImage(geode::Mod::get()->expandSpriteName("DDL_tab-001.png").c_str(), false);
    auto brownTex = CCTextureCache::get()->addImage(geode::Mod::get()->expandSpriteName("DDL_tab-002.png").c_str(), false);

    float profileToggleX = m_profileOverlay->getContentSize().width / 2.0f - 210.0f;

    auto updateProfileTabState = [&](CCMenuItemSpriteExtra* b, int t) {
        if (!b) return;
        bool selected = m_profileTab == t;
        if (auto bg = static_cast<CCSprite*>(b->getNormalImage()->getChildByID("bg"))) {
            bg->setTexture(selected ? greenTex : brownTex);
        }
        b->getNormalImage()->setScaleX(selected ? 1.1f : 1.0f);
        float shift = b->getNormalImage()->getContentSize().width * 0.05f;
        b->setPositionX(profileToggleX - (selected ? shift : 0.0f));
    };

    updateProfileTabState(m_toggleVerifsBtn, 0);
    updateProfileTabState(m_toggleComplBtn, 1);
    updateProfileTabState(m_togglePacksBtn, 2);
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

void DDLListLayer::onFirst(CCObject* sender) { page(0); }
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

void DDLListLayer::openProfilePage(const std::string& username) {
    if (username.empty()) return;
    
    if (m_profileOverlay) {
        m_profileOverlay->setVisible(true);
        setUnderlyingVisible(false);
    }
    
    showProfilePage(username);
}

void DDLListLayer::showProfilePage(const std::string& username) {
    if (!m_profileStatsNode) return;
    m_profileStatsNode->setVisible(true);
    m_profileStatsNode->removeAllChildrenWithCleanup(true);
    m_profileStatsNode->setID("profile-stats-node");

    auto overlayBg = cocos2d::extension::CCScale9Sprite::create("GJ_square01.png");
    overlayBg->setContentSize({ 420.0f, 280.0f });
    auto center = ccp(m_profileOverlay->getContentSize().width / 2.0f, m_profileOverlay->getContentSize().height / 2.0f);
    overlayBg->setPosition(center);
    overlayBg->setID("overlay-background");
    m_profileStatsNode->addChild(overlayBg, -1);

    const auto& currentLboard = dclEnabled ? DDLIntegration::dclLeaderboard : DDLIntegration::ddlLeaderboard;
    auto it = std::find_if(currentLboard.begin(), currentLboard.end(), [ & ](const DDLLeaderboardEntry& entry) {
        return string::toLower(entry.user) == string::toLower(username);
    });

    double points = 0.0;
    int rank = 0;
    int verifications = 0;
    int completions = 0;
    int packCount = 0;
    std::vector<DDLLevelRecord> userVerifLevels;
    std::vector<DDLLevelRecord> userComplLevels;
    std::vector<std::string> userPacks;

    if (it != currentLboard.end()) {
        points = it->points;
        rank = it->rank;
        verifications = it->verifiedLevels.size();
        completions = it->completedLevels.size();
        packCount = it->completedPacks.size();
        userPacks = it->completedPacks;
        
        userVerifLevels = it->verifiedLevels;
        userComplLevels = it->completedLevels;
        userComplLevels.insert(userComplLevels.end(), it->progressedLevels.begin(), it->progressedLevels.end());
    }
    m_currentProfilePoints = points;

    auto pointSorter = [](const auto& a, const auto& b) {
        return a.points > b.points;
    };
    std::sort(userVerifLevels.begin(), userVerifLevels.end(), pointSorter);
    std::sort(userComplLevels.begin(), userComplLevels.end(), pointSorter);

    std::string displayUsername = username;

    m_userIconListener.spawn(
        web::WebRequest()
            .bodyString("str=" + username + "&secret=Wmfd2893gb7")
            .post("https://www.boomlings.com/database/getGJUsers20.php"),
        [ this, username, center ](web::WebResponse res) {
            if (!res.ok()) return;
            std::string data = res.string().unwrapOr("");
            if (data.empty() || data == "-1") return;

            std::map<int, std::string> parsedMap;
            auto parts = geode::utils::string::split(data, ":");
            for (size_t i = 0; i + 1 < parts.size(); i += 2) {
                if (auto keyRes = geode::utils::numFromString<int>(parts[ i ])) {
                    parsedMap[ keyRes.unwrap() ] = parts[ i + 1 ];
                }
            }

            if (!parsedMap.contains(9) || !parsedMap.contains(10) || !parsedMap.contains(11)) return;

            auto iconRes = geode::utils::numFromString<int>(parsedMap[ 9 ]);
            auto color1Res = geode::utils::numFromString<int>(parsedMap[ 10 ]);
            auto color2Res = geode::utils::numFromString<int>(parsedMap[ 11 ]);
            if (!iconRes || !color1Res || !color2Res) return;

            auto gm = GameManager::get();

            auto iconAccent = cocos2d::extension::CCScale9Sprite::create("square02b_001.png");
            iconAccent->setContentSize({ 58.0f, 58.0f });
            iconAccent->setColor({ 95, 46, 24 });
            iconAccent->setPosition(ccp(center.x - 150.0f, center.y + 92.5f));
            iconAccent->setID("player-icon-accent");
            m_profileStatsNode->addChild(iconAccent, 1);

            auto playerIcon = SimplePlayer::create(gm->getPlayerFrame());
            playerIcon->updatePlayerFrame(iconRes.unwrap(), IconType::Cube);
            playerIcon->setColor(gm->colorForIdx(color1Res.unwrap()));
            playerIcon->setSecondColor(gm->colorForIdx(color2Res.unwrap()));

            if (parsedMap.contains(15) && parsedMap[ 15 ] == "1") {
                int glowIdx = color2Res.unwrap();
                if (parsedMap.contains(51)) {
                    if (auto glowRes = geode::utils::numFromString<int>(parsedMap[ 51 ])) glowIdx = glowRes.unwrap();
                }
                playerIcon->setGlowOutline(gm->colorForIdx(glowIdx));
            }
            playerIcon->setScale(1.7f);
            playerIcon->setPosition(ccp(center.x - 150.0f, center.y + 92.5f));
            playerIcon->setID("player-icon");
            m_profileStatsNode->addChild(playerIcon, 2);
        }
    );
    for (auto& c : displayUsername) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

    auto usernameLabel = CCLabelBMFont::create(displayUsername.c_str(), "bigFont.fnt");
    usernameLabel->setAnchorPoint(ccp(0.0f, 0.5f));
    usernameLabel->setPosition(ccp(center.x - 90.0f, center.y + 110.0f));
    usernameLabel->setScale(1.1f);
    usernameLabel->limitLabelWidth(220.0f, 0.7f, 0.0f);
    usernameLabel->setID("username-label");
    m_profileStatsNode->addChild(usernameLabel);

    if (rank > 0) {
        auto rankLabel = CCLabelBMFont::create(fmt::format("RANK\n{}", rank).c_str(), "goldFont-uhd.fnt");
        rankLabel->setAnchorPoint(ccp(1.0f, 0.5f));
        rankLabel->setPosition(ccp(center.x + 180.0f, center.y + 110.0f));
        rankLabel->setScale(0.5f * (cocos2d::CC_CONTENT_SCALE_FACTOR() / 4.0f));
        rankLabel->setAlignment(kCCTextAlignmentCenter);
        rankLabel->setID("rank-label");

        std::string trophyFile = "";
        std::string fontTexFile = "";

        if (rank == 1) {
            trophyFile = "DDL_rankicon_ruby.png";
            fontTexFile = "DDL_RubyFont-uhd.png";
        } else if (rank <= 3) {
            trophyFile = "DDL_rankicon_diamond.png";
            fontTexFile = "DDL_DiamondFont-uhd.png";
        } else if (rank <= 5) {
            trophyFile = "DDL_rankicon_gold.png";
            fontTexFile = "DDL_GoldFont-uhd.png";
        } else if (rank <= 10) {
            trophyFile = "DDL_rankicon_silver.png";
            fontTexFile = "DDL_SilverFont-uhd.png";
        } else if (rank <= 25) {
            trophyFile = "DDL_rankicon_bronze.png";
            fontTexFile = "DDL_BronzeFont-uhd.png";
        }

        if (!fontTexFile.empty()) {
            auto tex = CCTextureCache::get()->addImage(geode::Mod::get()->expandSpriteName(fontTexFile.c_str()).c_str(), false);
            if (tex) rankLabel->setTexture(tex);
        }

        m_profileStatsNode->addChild(rankLabel);

        if (!trophyFile.empty()) {
            auto trophy = CCSprite::create(geode::Mod::get()->expandSpriteName(trophyFile.c_str()).c_str());
            if (trophy) {
                trophy->setAnchorPoint(ccp(1.0f, 0.5f));
                trophy->setPosition(ccp(rankLabel->getPositionX() - rankLabel->getScaledContentSize().width - 5.0f, rankLabel->getPositionY()));
                trophy->setScale(1.3f);
                trophy->setID("rank-trophy");
                m_profileStatsNode->addChild(trophy);
            }
        }
    }

    std::vector<int> milestones = {100, 500, 1000, 2000, 5000, 10000, 25000};
    int prevMilestone = 0;
    int nextMilestone = milestones.back();
    for (int m : milestones) {
        if (points < m) {
            nextMilestone = m;
            break;
        }
        prevMilestone = m;
    }

    auto pointsText = CCLabelBMFont::create(fmt::format("PTS: {:.3f} / {}", points, nextMilestone).c_str(), "goldFont.fnt");
    pointsText->setAnchorPoint(ccp(0.0f, 0.5f));
    pointsText->setPosition(ccp(center.x - 90.0f, center.y + 95.0f));
    pointsText->setScale(0.45f);
    pointsText->setZOrder(5);
    pointsText->setID("points-label");
    m_profileStatsNode->addChild(pointsText);

    auto barTexRef = CCSprite::create("DDL_progressBar_001.png"_spr);
    auto barTexSize = barTexRef->getContentSize();

    float barWidth = 270.0f;
    float barHeight = barTexSize.height * 1.3f;

    float barCapInsetX = std::min({ barTexSize.width * 0.3f, barTexSize.width / 2.0f - 1.0f, barWidth / 2.0f - 1.0f });
    float barCapInsetY = std::min(barTexSize.height * 0.3f, barTexSize.height / 2.0f - 1.0f);
    CCRect barCapInsets(barCapInsetX, barCapInsetY, barTexSize.width - barCapInsetX * 2.0f, barTexSize.height - barCapInsetY * 2.0f);
    CCRect barFullRect(0.0f, 0.0f, barTexSize.width, barTexSize.height);

    auto barBg = CCScale9Sprite::create("DDL_progressBar_001.png"_spr, barFullRect, barCapInsets);
    barBg->setContentSize({ barWidth, barHeight });
    barBg->setColor({ 45, 25, 15 });
    barBg->setOpacity(220);
    barBg->setAnchorPoint(ccp(0.0f, 0.5f));
    barBg->setPosition(ccp(center.x - 90.0f, center.y + 75.0f));
    barBg->setID("progress-bar-background");
    m_profileStatsNode->addChild(barBg);

    float progressPct = nextMilestone > prevMilestone
        ? (static_cast<float>(points) - prevMilestone) / static_cast<float>(nextMilestone - prevMilestone)
        : 1.0f;
    progressPct = std::clamp(progressPct, 0.0f, 1.0f);

    float minFillWidth = barCapInsetX * 2.0f + 2.0f;
    float fillWidth = std::clamp(barWidth * progressPct, progressPct > 0.0f ? minFillWidth : 0.0f, barWidth);

    if (fillWidth > 0.0f) {
        auto barFill = CCScale9Sprite::create("DDL_progressBar_001.png"_spr, barFullRect, barCapInsets);
        barFill->setContentSize({ fillWidth, barHeight });
        barFill->setColor({ 0, 255, 0 });
        barFill->setAnchorPoint(ccp(0.0f, 0.5f));
        barFill->setPosition(ccp(0.0f, barHeight / 2.0f));
        barFill->setID("progress-bar-fill");
        barBg->addChild(barFill, 1);

        ccColor4F sparkColor;
        switch (nextMilestone) {
            case 100:   sparkColor = { 0.80f, 0.50f, 0.20f, 1.0f }; break; // bronze
            case 500:   sparkColor = { 0.80f, 0.80f, 0.85f, 1.0f }; break; // silver
            case 1000:
            case 2000:  sparkColor = { 1.00f, 0.84f, 0.10f, 1.0f }; break; // gold
            case 5000:
            case 10000: sparkColor = { 0.45f, 0.85f, 1.00f, 1.0f }; break; // diamond
            default:    sparkColor = { 0.90f, 0.15f, 0.30f, 1.0f }; break; // ruby
        }

        if (progressPct >= 0.5f && progressPct < 1.0f) {
            auto fire = CCSprite::create(
                progressPct >= 0.8f ? "DDL_progressBar-Fire_001.png"_spr : "DDL_progressBar-Fire_002.png"_spr
            );
            if (fire) {
                fire->setScaleX(1.0f);
                fire->setScaleY(1.1f);
                fire->setPosition(ccp(190.0f, barHeight / 2.0f));
                fire->setBlendFunc({ GL_SRC_ALPHA, GL_ONE });
                fire->runAction(CCRepeatForever::create(CCSequence::create(
                    CCFadeTo::create(0.45f, 170),
                    CCFadeTo::create(0.45f, 255),
                    nullptr
                )));
                fire->setID("milestone-fire");
                barBg->addChild(fire, -2);
            }
        }

        if (progressPct >= 0.85f && progressPct < 1.0f) {
            auto sparks = new CCParticleSystemQuad();
            sparks->initWithTotalParticles(30, false);
            sparks->autorelease();
            sparks->setTexture(CCTextureCache::get()->addImage("smallDot.png", false));
            sparks->setDuration(kCCParticleDurationInfinity);
            sparks->setEmitterMode(kCCParticleModeGravity);
            sparks->setGravity(ccp(0.0f, 0.0f));
            sparks->setSpeed(16.0f);
            sparks->setSpeedVar(8.0f);
            sparks->setAngle(90.0f);
            sparks->setAngleVar(25.0f);
            sparks->setLife(0.5f);
            sparks->setLifeVar(0.25f);
            sparks->setStartSize(2.0f);
            sparks->setStartSizeVar(1.0f);
            sparks->setEndSize(0.0f);
            sparks->setStartColor(sparkColor);
            sparks->setStartColorVar({ 0.0f, 0.0f, 0.0f, 0.0f });
            sparks->setEndColor({ sparkColor.r, sparkColor.g, sparkColor.b, 0.0f });
            sparks->setEmissionRate(10.0f + 30.0f * (progressPct - 0.85f) / 0.15f);
            sparks->setPosVar(ccp(2.0f, barHeight * 0.3f));
            sparks->setBlendAdditive(true);
            sparks->setPositionType(kCCPositionTypeRelative);
            sparks->setRotation(90.0f);
            sparks->setPosition(ccp(barWidth, barHeight / 2.0f));
            sparks->setID("milestone-sparks");
            barBg->addChild(sparks, -1);
        }
    }

    auto compLabel = CCLabelBMFont::create(fmt::format("COMPLETIONS: {}", completions).c_str(), "bigFont.fnt");
    compLabel->setAnchorPoint(ccp(0.5f, 0.5f));
    compLabel->setPosition(ccp(center.x - 135.0f, center.y + 40.0f));
    compLabel->setScale(0.4f);
    compLabel->limitLabelWidth(130.0f, 0.4f, 0.0f);
    compLabel->setID("completions-label");
    m_profileStatsNode->addChild(compLabel);

    auto verLabel = CCLabelBMFont::create(fmt::format("VERIFICATIONS: {}", verifications).c_str(), "bigFont.fnt");
    verLabel->setAnchorPoint(ccp(0.5f, 0.5f));
    verLabel->setPosition(ccp(center.x, center.y + 40.0f));
    verLabel->setScale(0.4f);
    verLabel->limitLabelWidth(130.0f, 0.4f, 0.0f);
    verLabel->setID("verifications-label");
    m_profileStatsNode->addChild(verLabel);

    auto packsLabel = CCLabelBMFont::create(fmt::format("PACKS: {}", packCount).c_str(), "bigFont.fnt");
    packsLabel->setAnchorPoint(ccp(0.5f, 0.5f));
    packsLabel->setPosition(ccp(center.x + 135.0f, center.y + 40.0f));
    packsLabel->setScale(0.4f);
    packsLabel->limitLabelWidth(130.0f, 0.4f, 0.0f);
    packsLabel->setID("packs-label");
    m_profileStatsNode->addChild(packsLabel);

    auto listBg = cocos2d::extension::CCScale9Sprite::create("square02b_001.png");
    listBg->setContentSize({ 390.0f, 150.0f });
    listBg->setPosition(ccp(center.x, center.y - 50.0f));
    listBg->setColor({ 130, 64, 33 });
    listBg->setOpacity(255);
    listBg->setZOrder(0);
    listBg->setID("list-background");
    m_profileStatsNode->addChild(listBg);

    m_profileToggleMenu = CCMenu::create();
    m_profileToggleMenu->setPosition(ccp(0, 0));
    m_profileToggleMenu->setZOrder(-2);
    m_profileToggleMenu->setID("profile-toggle-menu");
    m_profileStatsNode->addChild(m_profileToggleMenu);

    auto createProfileTab = [](const char* bgName, const char* text, float& outWidth, float& outHeight) -> CCNode* {
        auto bg = CCSprite::create(geode::Mod::get()->expandSpriteName(bgName).c_str());
        bg->setRotation(-90.0f);
        bg->setScale(0.62f);
        bg->setID("bg");

        auto rawSize = bg->getContentSize();
        outWidth = rawSize.height * 0.62f;
        outHeight = rawSize.width * 0.62f;

        auto node = CCNode::create();
        node->setContentSize({ outWidth, outHeight });
        bg->setPosition(ccp(outWidth / 2.0f, outHeight / 2.0f));
        node->addChild(bg);

        auto label = CCLabelBMFont::create(text, "bigFont.fnt");
        label->setAnchorPoint(ccp(0.5f, 0.5f));
        label->setRotation(-90.0f);
        label->limitLabelWidth(outHeight - 6.0f, 0.28f, 0.0f);
        label->setPosition(ccp(7.0f, outHeight / 2.0f));
        node->addChild(label);

        return node;
    };

    float verifsTabW = 0.0f, verifsTabH = 0.0f;
    float complTabW = 0.0f, complTabH = 0.0f;
    float packsTabW = 0.0f, packsTabH = 0.0f;

    m_toggleVerifsBtn = CCMenuItemSpriteExtra::create(createProfileTab("DDL_tab-002.png", "VERIFS", verifsTabW, verifsTabH), this, menu_selector(DDLListLayer::onToggleProfileTab));
    m_toggleVerifsBtn->setTag(0);
    m_toggleVerifsBtn->setSizeMult(1.1f);
    m_toggleVerifsBtn->useAnimationType(MenuAnimationType::Move);
    m_toggleVerifsBtn->setID("toggle-verifs-button");
    m_profileToggleMenu->addChild(m_toggleVerifsBtn);

    m_toggleComplBtn = CCMenuItemSpriteExtra::create(createProfileTab("DDL_tab-001.png", "COMPL", complTabW, complTabH), this, menu_selector(DDLListLayer::onToggleProfileTab));
    m_toggleComplBtn->setTag(1);
    m_toggleComplBtn->setSizeMult(1.1f);
    m_toggleComplBtn->useAnimationType(MenuAnimationType::Move);
    m_toggleComplBtn->setID("toggle-compl-button");
    m_profileToggleMenu->addChild(m_toggleComplBtn);

    m_togglePacksBtn = CCMenuItemSpriteExtra::create(createProfileTab("DDL_tab-002.png", "PACKS", packsTabW, packsTabH), this, menu_selector(DDLListLayer::onToggleProfileTab));
    m_togglePacksBtn->setTag(2);
    m_togglePacksBtn->setSizeMult(1.1f);
    m_togglePacksBtn->useAnimationType(MenuAnimationType::Move);
    m_togglePacksBtn->setID("toggle-packs-button");
    m_profileToggleMenu->addChild(m_togglePacksBtn);

    auto profileToggleX = center.x - 210.0f;
    float selectedTabShift = complTabW * 0.05f;
    float tabSpacing = complTabH + 2.0f;
    m_toggleVerifsBtn->setPosition(ccp(profileToggleX, center.y + tabSpacing));
    m_toggleComplBtn->setPosition(ccp(profileToggleX - selectedTabShift, center.y));
    m_togglePacksBtn->setPosition(ccp(profileToggleX, center.y - tabSpacing));

    m_toggleVerifsBtn->getNormalImage()->setScaleX(1.0f);
    m_toggleComplBtn->getNormalImage()->setScaleX(1.1f);
    m_togglePacksBtn->getNormalImage()->setScaleX(1.0f);

    auto verifCells = CCArray::create();
    for (size_t i = 0; i < userVerifLevels.size(); i++) {
        verifCells->addObject(DDLProfileLevelCell::create(userVerifLevels[ i ], true, dclEnabled, i));
    }
    m_verifsListView = ListView::create(verifCells, 30.0f, 380.0f, 140.0f);
    m_verifsListView->setPosition(ccp(center.x - 190.0f, center.y - 120.0f));
    m_verifsListView->setVisible(false);
    m_verifsListView->setZOrder(2);
    m_verifsListView->setID("verifs-list-view");
    m_profileStatsNode->addChild(m_verifsListView);

    auto complCells = CCArray::create();
    for (size_t i = 0; i < userComplLevels.size(); i++) {
        complCells->addObject(DDLProfileLevelCell::create(userComplLevels[ i ], false, dclEnabled, i));
    }
    m_complListView = ListView::create(complCells, 30.0f, 380.0f, 140.0f);
    m_complListView->setPosition(ccp(center.x - 190.0f, center.y - 120.0f));
    m_complListView->setVisible(true);
    m_complListView->setZOrder(2);
    m_complListView->setID("compl-list-view");
    m_profileStatsNode->addChild(m_complListView);

    auto packCells = CCArray::create();
    const auto& currentPacks = dclEnabled ? DDLIntegration::dclPacks : DDLIntegration::ddlPacks;
    for (size_t i = 0; i < userPacks.size(); i++) {
        double packPts = 0.0;
        for (const auto& p : currentPacks) {
            if (p.name == userPacks[ i ]) {
                packPts = p.points;
                break;
            }
        }
        packCells->addObject(DDLProfilePackCell::create(userPacks[ i ], packPts, i));
    }
    m_packsListView = ListView::create(packCells, 30.0f, 380.0f, 140.0f);
    m_packsListView->setPosition(ccp(center.x - 190.0f, center.y - 120.0f));
    m_packsListView->setVisible(false);
    m_packsListView->setZOrder(2);
    m_packsListView->setID("packs-list-view");
    m_profileStatsNode->addChild(m_packsListView);

    m_profileTab = 1;

    auto topMenu = CCMenu::create();
    topMenu->setPosition(ccp(0, 0));
    topMenu->setID("profile-top-menu");
    m_profileStatsNode->addChild(topMenu);

    auto closeBtn = CCSprite::createWithSpriteFrameName("GJ_closeBtn_001.png");
    auto closeItem = CCMenuItemSpriteExtra::create(closeBtn, this, menu_selector(DDLListLayer::onProfileClose));
    closeItem->setPosition(ccp(center.x - 210.0f, center.y + 140.0f));
    closeItem->setID("close-button");
    topMenu->addChild(closeItem);
    
    auto infoSpr = CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png");
    infoSpr->setScale(0.5f);
    auto infoBtn = CCMenuItemSpriteExtra::create(infoSpr, this, menu_selector(DDLListLayer::onMilestoneInfo));
    infoBtn->setAnchorPoint(ccp(0.0f, 0.5f));
    infoBtn->setPosition(ccp(pointsText->getPositionX() + pointsText->getScaledContentSize().width + 10.0f, pointsText->getPositionY()));
    infoBtn->setID("info-button");
    topMenu->addChild(infoBtn);
}

void DDLListLayer::onMilestoneInfo(CCObject*) {
    std::vector<int> milestones = {100, 500, 1000, 2000, 5000, 10000, 25000};
    std::string infoStr = "";
    for (size_t i = 0; i < milestones.size(); ++i) {
        if (m_currentProfilePoints >= milestones[ i ]) {
            infoStr += fmt::format("<cg>{} PTS</c> - Achieved!", milestones[ i ]);
        } else {
            infoStr += fmt::format("<cr>{} PTS</c> - Locked", milestones[ i ]);
        }
        if (i < milestones.size() - 1) infoStr += "\n";
    }
    FLAlertLayer::create("Milestones", infoStr.c_str(), "OK")->show();
}

void DDLListLayer::closeProfilePage() {
    if (m_profileOverlay) {
        m_profileOverlay->setVisible(false);
    }
    setUnderlyingVisible(true);
}

void DDLListLayer::setUnderlyingVisible(bool visible) {
    if (m_list) m_list->setVisible(visible);
    if (m_searchBarMenu) m_searchBarMenu->setVisible(visible);
    if (m_countLabel) m_countLabel->setVisible(visible);
    if (m_leftButton) m_leftButton->setVisible(visible);
    if (m_rightButton) m_rightButton->setVisible(visible);
    if (m_firstButton) m_firstButton->setVisible(visible);
    if (m_lastButton) m_lastButton->setVisible(visible);
    if (m_pageButton) m_pageButton->setVisible(visible);
    if (m_randomButton) m_randomButton->setVisible(visible);
    if (m_infoButton) m_infoButton->setVisible(visible);
    if (m_starToggle) m_starToggle->setVisible(visible);
    if (m_moonToggle) m_moonToggle->setVisible(visible);
    if (auto tabs = m_list->getChildByID("side-tabs-menu")) tabs->setVisible(visible);
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

        auto size = m_fullLeaderboardResults.size();
        m_page = size == 0 ? 0 : std::clamp<int>(m_page, 0, static_cast<int>((size - 1) / 10));

        auto cells = CCArray::create();
        auto start = m_page * 10;
        auto end = std::min<int>(size, (m_page + 1) * 10);
        
        auto callback = [ this ](std::string user) {
            openProfilePage(user);
        };
        
        int cellIndex = 0;
        for (auto it = m_fullLeaderboardResults.begin() + start; it < m_fullLeaderboardResults.begin() + end; ++it, ++cellIndex) {
            cells->addObject(DDLLeaderboardCell::create(*it, callback, cellIndex));
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

        auto size = m_fullPackResults.size();
        m_page = size == 0 ? 0 : std::clamp<int>(m_page, 0, static_cast<int>((size - 1) / 10));

        auto packs = CCArray::create();
        auto start = m_page * 10;
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
    } 
    else {
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

        m_page = m_fullSearchResults.empty() ? 0 : std::clamp<int>(m_page, 0, static_cast<int>((m_fullSearchResults.size() - 1) / 10));

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
                idQuery += m_fullSearchResults[ i ];
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
        
        if (auto parsed = geode::utils::numFromString<int>(m_fullSearchResults[ i ])) {
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