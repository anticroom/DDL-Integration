#include "IDListLayer.hpp"
#include "../DDLIntegration.hpp"
#include <Geode/binding/AppDelegate.hpp>
#include <Geode/binding/CustomListView.hpp>
#include <Geode/binding/GameLevelManager.hpp>
#include <Geode/binding/GJGameLevel.hpp>
#include <Geode/binding/GJListLayer.hpp>
#include <Geode/binding/InfoAlertButton.hpp>
#include <Geode/binding/LoadingCircle.hpp>
#include <Geode/binding/SetIDPopup.hpp>
#include <Geode/binding/GJSearchObject.hpp>
#include <Geode/loader/Mod.hpp>
#include <Geode/utils/random.hpp>

using namespace geode::prelude;

IDListLayer* IDListLayer::create() {
    auto ret = new IDListLayer();
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

CCScene* IDListLayer::scene() {
    auto ret = CCScene::create();
    AppDelegate::get()->m_runningScene = ret;
    ret->addChild(IDListLayer::create());
    return ret;
}

bool dclEnabled = false;
constexpr std::string_view ddlInfo = "The Denouement Demon List (DDL) is a ranking of demons.";
constexpr std::string_view dclInfo = "The Denouement Challenge List (DCL) is a ranking of challenges.";

bool IDListLayer::init() {
    if (!CCLayer::init()) return false;

    setID("IDListLayer");
    auto winSize = CCDirector::get()->getWinSize();

    m_pageCache = CCArray::create();
    m_pageCache->retain();

    auto bg = CCSprite::create("GJ_gradientBG.png");
    bg->setAnchorPoint(ccp(0.0f, 0.0f));
    bg->setScaleX((winSize.width + 10.0f) / bg->getTextureRect().size.width);
    bg->setScaleY((winSize.height + 10.0f) / bg->getTextureRect().size.height);
    bg->setPosition(ccp(-5.0f, -5.0f));
    bg->setColor({ 51, 51, 51 });
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

    m_list = GJListLayer::create(nullptr, dclEnabled ? "DCL" : "DDL", { 0, 0, 0, 180 }, 356.0f, 220.0f, 0);
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
    m_searchButton = CCMenuItemSpriteExtra::create(searchSprite, this, menu_selector(IDListLayer::onSearch));
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
        CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png"), this, menu_selector(IDListLayer::onBack)
    );
    backButton->setPosition(ccp(25.0f, winSize.height - 25.0f));
    backButton->setID("back-button");
    menu->addChild(backButton);

    auto leftBtnSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
    m_leftButton = CCMenuItemSpriteExtra::create(leftBtnSpr, this, menu_selector(IDListLayer::onPrevPage));
    m_leftButton->setPosition(ccp(24.0f, winSize.height / 2.0f));
    m_leftButton->setID("prev-page-button");
    menu->addChild(m_leftButton);

    auto rightBtnSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
    rightBtnSpr->setFlipX(true);
    m_rightButton = CCMenuItemSpriteExtra::create(rightBtnSpr, this, menu_selector(IDListLayer::onNextPage));
    m_rightButton->setPosition(ccp(winSize.width - 24.0f, winSize.height / 2.0f));
    m_rightButton->setID("next-page-button");
    menu->addChild(m_rightButton);

    m_infoButton = InfoAlertButton::create(dclEnabled ? "DCL" : "DDL",
        dclEnabled ? gd::string(dclInfo.data(), dclInfo.size()) : gd::string(ddlInfo.data(), ddlInfo.size()), 1.0f);
    m_infoButton->setPosition(ccp(30.0f, 30.0f));
    m_infoButton->setID("info-button");
    menu->addChild(m_infoButton, 2);

    m_ddlFailure = [this](int code) {
        FLAlertLayer::create(fmt::format("Load Failed ({})", code).c_str(), "Failed to load DDL. Please try again later.", "OK")->show();
        m_loadingCircle->setVisible(false);
    };

    m_dclFailure = [this](int code) {
        FLAlertLayer::create(fmt::format("Load Failed ({})", code).c_str(), "Failed to load DCL. Please try again later.", "OK")->show();
        m_loadingCircle->setVisible(false);
    };

    auto refreshBtnSpr = CCSprite::createWithSpriteFrameName("GJ_updateBtn_001.png");
    auto refreshButton = CCMenuItemSpriteExtra::create(refreshBtnSpr, this, menu_selector(IDListLayer::onRefresh));
    refreshButton->setPosition(ccp(winSize.width - refreshBtnSpr->getContentWidth() / 2.0f - 4.0f, refreshBtnSpr->getContentHeight() / 2.0f + 4.0f));
    refreshButton->setID("refresh-button");
    menu->addChild(refreshButton, 2);

    auto starSprite = CCSprite::createWithSpriteFrameName("GJ_starsIcon_001.png");
    starSprite->setScale(1.1f);
    m_starToggle = CCMenuItemSpriteExtra::create(starSprite, this, menu_selector(IDListLayer::onStar));
    m_starToggle->setPosition(ccp(30.0f, 60.0f));
    m_starToggle->setColor(dclEnabled ? ccColor3B { 125, 125, 125 } : ccColor3B { 255, 255, 255 });
    m_starToggle->setID("ddl-button");
    menu->addChild(m_starToggle, 2);

    auto moonSprite = CCSprite::createWithSpriteFrameName("GJ_moonsIcon_001.png");
    moonSprite->setScale(1.1f);
    m_moonToggle = CCMenuItemSpriteExtra::create(moonSprite, this, menu_selector(IDListLayer::onMoon));
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
    m_pageButton = CCMenuItemSpriteExtra::create(pageBtnSpr, this, menu_selector(IDListLayer::onPage));
    m_pageButton->setPositionY(winSize.height - 39.5f);
    m_pageButton->setID("page-button");
    menu->addChild(m_pageButton);

    auto randomSprite = CCSprite::create("BI_randomBtn_001.png"_spr);
    randomSprite->setScale(0.9f);
    m_randomButton = CCMenuItemSpriteExtra::create(randomSprite, this, menu_selector(IDListLayer::onRandom));
    m_randomButton->setPositionY(
        m_pageButton->getPositionY() - m_pageButton->getContentHeight() / 2.0f - m_randomButton->getContentHeight() / 2.0f - 5.0f);
    m_randomButton->setID("random-button");
    menu->addChild(m_randomButton);

    auto lastArrow = CCSprite::createWithSpriteFrameName("GJ_arrow_02_001.png");
    lastArrow->setFlipX(true);
    auto otherLastArrow = CCSprite::createWithSpriteFrameName("GJ_arrow_02_001.png");
    otherLastArrow->setPosition(lastArrow->getContentSize() / 2.0f + ccp(20.0f, 0.0f));
    otherLastArrow->setFlipX(true);
    lastArrow->addChild(otherLastArrow);
    lastArrow->setScale(0.4f);
    m_lastButton = CCMenuItemSpriteExtra::create(lastArrow, this, menu_selector(IDListLayer::onLast));
    m_lastButton->setPositionY(
        m_randomButton->getPositionY() - m_randomButton->getContentHeight() / 2.0f - m_lastButton->getContentHeight() / 2.0f - 5.0f);
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
    m_firstButton = CCMenuItemSpriteExtra::create(firstArrow, this, menu_selector(IDListLayer::onFirst));
    m_firstButton->setPosition(ccp(21.5f, m_lastButton->getPositionY()));
    m_firstButton->setID("first-button");
    menu->addChild(m_firstButton);

    m_loadingCircle = LoadingCircle::create();
    m_loadingCircle->setParentLayer(this);
    m_loadingCircle->setID("loading-circle");
    m_loadingCircle->show();

    showLoading();
    setKeypadEnabled(true);
    setKeyboardEnabled(true);

    if (dclEnabled && DDLIntegration::dclLoaded) {
        populateList("");
    }
    else if (dclEnabled) {
        DDLIntegration::loadDCL(m_dclListener, [this] {
            populateList("");
        }, m_dclFailure);
    }
    else if (DDLIntegration::ddlLoaded) {
        populateList("");
    }
    else {
        DDLIntegration::loadDDL(m_ddlListener, [this] {
            populateList("");
        }, m_ddlFailure);
    }

    return true;
}

void IDListLayer::onBack(CCObject* sender) {
    CCDirector::get()->popSceneWithTransition(0.5f, kPopTransitionFade);
}

void IDListLayer::onPrevPage(CCObject* sender) {
    page(m_page - 1);
}

void IDListLayer::onNextPage(CCObject* sender) {
    page(m_page + 1);
}

void IDListLayer::onRefresh(CCObject* sender) {
    showLoading();
    if (dclEnabled) {
        DDLIntegration::loadDCL(m_dclListener, [this] {
            populateList(m_query);
        }, m_dclFailure);
    }
    else {
        DDLIntegration::loadDDL(m_ddlListener, [this] {
            populateList(m_query);
        }, m_ddlFailure);
    }
}

void IDListLayer::onStar(CCObject* sender) {
    if (!dclEnabled) return;
    dclEnabled = false;
    m_starToggle->setColor({ 255, 255, 255 });
    m_moonToggle->setColor({ 125, 125, 125 });
    showLoading();
    if (auto listTitle = static_cast<CCLabelBMFont*>(m_list->getChildByID("title"))) {
        listTitle->setString("DDL");
        listTitle->limitLabelWidth(280.0f, 0.8f, 0.0f);
    }
    m_infoButton->m_title = "DDL";
    m_infoButton->m_description = gd::string(ddlInfo.data(), ddlInfo.size());
    m_fullSearchResults.clear();
    if (DDLIntegration::ddlLoaded) {
        page(0);
    }
    else {
        DDLIntegration::loadDDL(m_ddlListener, [this] {
            page(0);
        }, m_ddlFailure);
    }
}

void IDListLayer::onMoon(CCObject* sender) {
    if (dclEnabled) return;
    dclEnabled = true;
    m_starToggle->setColor({ 125, 125, 125 });
    m_moonToggle->setColor({ 255, 255, 255 });
    showLoading();
    if (auto listTitle = static_cast<CCLabelBMFont*>(m_list->getChildByID("title"))) {
        listTitle->setString("DCL");
        listTitle->limitLabelWidth(280.0f, 0.8f, 0.0f);
    }
    m_infoButton->m_title = "DCL";
    m_infoButton->m_description = gd::string(dclInfo.data(), dclInfo.size());
    m_fullSearchResults.clear();
    if (DDLIntegration::dclLoaded) {
        page(0);
    }
    else {
        DDLIntegration::loadDCL(m_dclListener, [this] {
            page(0);
        }, m_dclFailure);
    }
}

void IDListLayer::onPage(CCObject* sender) {
    auto popup = SetIDPopup::create(m_page + 1, 1, (m_fullSearchResults.size() + 9) / 10, "Go to Page", "Go", true, 1, 60.0f, false, false);
    popup->m_delegate = this;
    popup->show();
}

void IDListLayer::onRandom(CCObject* sender) {
    page(random::generate(0uz, (m_fullSearchResults.size() - 1) / 10));
}

void IDListLayer::onFirst(CCObject* sender) {
    page(0);
}

void IDListLayer::onLast(CCObject* sender) {
    page((m_fullSearchResults.size() - 1) / 10);
}

void IDListLayer::showLoading() {
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

void IDListLayer::populateList(const std::string& query) {
    m_attemptedFetches.clear();
    m_fullSearchResults.clear();
    auto searchSprite = static_cast<CCSprite*>(m_searchButton->getNormalImage());
    if (query.empty()) {
        for (auto& level : dclEnabled ? DDLIntegration::dcl : DDLIntegration::ddl) {
            m_fullSearchResults.push_back(fmt::to_string(level.id));
        }
        searchSprite->setDisplayFrame(CCSpriteFrameCache::get()->spriteFrameByName("gj_findBtn_001.png"));
    }
    else {
        auto lowerQuery = string::toLower(query);
        for (auto& level : dclEnabled ? DDLIntegration::dcl : DDLIntegration::ddl) {
            if (!string::toLower(level.name).contains(lowerQuery)) continue;
            m_fullSearchResults.push_back(fmt::to_string(level.id));
        }
        auto texture = CCTextureCache::get()->addImage("ID_findBtnOn_001.png"_spr, false);
        searchSprite->setDisplayFrame(CCSpriteFrame::createWithTexture(texture, { { 0.0f, 0.0f }, texture->getContentSize() }));
    }

    m_query = query;

    if (m_fullSearchResults.empty()) {
        loadLevelsFinished(CCArray::create(), "", 0);
        m_countLabel->setString("");
    }
    else {
        auto glm = GameLevelManager::get();
        glm->m_levelManagerDelegate = this;

        if (m_pageCache) m_pageCache->removeAllObjects();

        std::string idQuery = "";
        auto start = m_fullSearchResults.begin() + m_page * 10;
        auto end = std::min(m_fullSearchResults.end(), m_fullSearchResults.begin() + (m_page + 1) * 10);
        for (auto it = start; it != end; ++it) {
            if (it != start) idQuery += ",";
            idQuery += *it;
        }
        
        auto searchObject = GJSearchObject::create(SearchType::Type19, idQuery);

        if (auto storedLevels = glm->getStoredOnlineLevels(searchObject->getKey())) {
            loadLevelsFinished(storedLevels, "", 0);
            setupPageInfo("", "");
        }
        else glm->getOnlineLevels(searchObject);
    }
}

void IDListLayer::loadLevelsFinished(CCArray* levels, const char*, int) {
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
    }

    auto combinedLevels = CCArray::create();
    size_t start = m_page * 10;
    size_t end = std::min<size_t>(m_fullSearchResults.size(), start + 10);

    for (size_t i = start; i < end; i++) {
        int expectedID = 0;
        try { expectedID = std::stoi(m_fullSearchResults[i]); } catch (...) {}

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
                    foundLevel->m_levelType = GJLevelType::Saved;
                    foundLevel->m_unlisted = true;
                    
                    foundLevel->m_stars = dclEnabled ? 0 : 10;
                    foundLevel->m_demon = 0;
                    foundLevel->m_ratingsSum = 50; 
                    foundLevel->m_ratings = 10; 
                    foundLevel->m_levelLength = 3;
                    break;
                }
            }
        }

        if (foundLevel) {
            combinedLevels->addObject(foundLevel);
        }
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

void IDListLayer::loadLevelsFailed(const char*, int) {
    loadLevelsFinished(nullptr, "", 0);
}

void IDListLayer::setupPageInfo(gd::string, const char*) {
    m_countLabel->setString(fmt::format("{} to {} of {}", m_page * 10 + 1,
        std::min<int>(m_fullSearchResults.size(), (m_page + 1) * 10), m_fullSearchResults.size()).c_str());
    m_countLabel->limitLabelWidth(100.0f, 0.6f, 0.0f);
}

void IDListLayer::onSearch(CCObject* sender) {
    auto query = m_searchBar->getString();
    if (m_query != query) {
        showLoading();
        if (dclEnabled) {
            DDLIntegration::loadDCL(m_dclListener, [this, query] {
                m_page = 0;
                populateList(query);
            }, m_dclFailure);
        }
        else {
            DDLIntegration::loadDDL(m_ddlListener, [this, query] {
                m_page = 0;
                populateList(query);
            }, m_ddlFailure);
        }
    }
}

void IDListLayer::page(int page) {
    auto maxPage = (m_fullSearchResults.size() + 9) / 10;
    m_page = maxPage > 0 ? (maxPage + (page % maxPage)) % maxPage : 0;
    showLoading();
    populateList(m_query);
}

void IDListLayer::keyDown(enumKeyCodes key, double timestamp) {
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

void IDListLayer::keyBackClicked() {
    onBack(nullptr);
}

void IDListLayer::setIDPopupClosed(SetIDPopup*, int page) {
    m_page = std::clamp<int>(page - 1, 0, (m_fullSearchResults.size() - 1) / 10);
    showLoading();
    populateList(m_query);
}

IDListLayer::~IDListLayer() {
    if (m_pageCache) {
        m_pageCache->release();
        m_pageCache = nullptr;
    }

    auto glm = GameLevelManager::get();
    if (glm->m_levelManagerDelegate == this) glm->m_levelManagerDelegate = nullptr;
}