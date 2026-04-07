#include "IDPackLayer.hpp"
#include "IDPackCell.hpp"
#include <Geode/binding/AppDelegate.hpp>
#include <Geode/binding/GJListLayer.hpp>
#include <Geode/binding/InfoAlertButton.hpp>
#include <Geode/binding/LoadingCircle.hpp>
#include <Geode/binding/SetIDPopup.hpp>
#include <Geode/loader/Mod.hpp>
#include <Geode/ui/ListView.hpp>
#include <Geode/utils/random.hpp>
#include <jasmine/random.hpp>

using namespace geode::prelude;

constexpr std::string_view ddlPackInfo = "DDL Packs contain curated sets of levels from the Denouement Demon List.";
constexpr std::string_view dclPackInfo = "DCL Packs contain curated sets of levels from the Denouement Challenge List.";

IDPackLayer* IDPackLayer::create() {
    auto ret = new IDPackLayer();
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

CCScene* IDPackLayer::scene() {
    auto ret = CCScene::create();
    AppDelegate::get()->m_runningScene = ret;
    ret->addChild(IDPackLayer::create());
    return ret;
}

bool IDPackLayer::init() {
    if (!CCLayer::init()) return false;

    setID("IDPackLayer");
    auto winSize = CCDirector::get()->getWinSize();

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

    m_list = GJListLayer::create(nullptr, dclEnabled ? "DCL Packs" : "DDL Packs", { 0, 0, 0, 180 }, 356.0f, 220.0f, 0);
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
    m_searchButton = CCMenuItemSpriteExtra::create(searchSprite, this, menu_selector(IDPackLayer::onSearch));
    m_searchButton->setPosition(ccp(337.0f, 15.0f));
    m_searchButton->setID("search-button");
    m_searchBarMenu->addChild(m_searchButton);

    m_searchBar = TextInput::create(310.0f, "Search Packs...");
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
        CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png"), this, menu_selector(IDPackLayer::onBack)
    );
    backButton->setPosition(ccp(25.0f, winSize.height - 25.0f));
    backButton->setID("back-button");
    menu->addChild(backButton);

    auto leftBtnSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
    m_leftButton = CCMenuItemSpriteExtra::create(leftBtnSpr, this, menu_selector(IDPackLayer::onPrevPage));
    m_leftButton->setPosition(ccp(24.0f, winSize.height / 2.0f));
    m_leftButton->setID("prev-page-button");
    menu->addChild(m_leftButton);

    auto rightBtnSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
    rightBtnSpr->setFlipX(true);
    m_rightButton = CCMenuItemSpriteExtra::create(rightBtnSpr, this, menu_selector(IDPackLayer::onNextPage));
    m_rightButton->setPosition(ccp(winSize.width - 24.0f, winSize.height / 2.0f));
    m_rightButton->setID("next-page-button");
    menu->addChild(m_rightButton);

    auto infoButton = InfoAlertButton::create(dclEnabled ? "DCL Packs" : "DDL Packs",
        dclEnabled ? gd::string(dclPackInfo.data(), dclPackInfo.size()) : gd::string(ddlPackInfo.data(), ddlPackInfo.size()), 1.0f);
    infoButton->setPosition(ccp(30.0f, 30.0f));
    infoButton->setID("info-button");
    menu->addChild(infoButton, 2);

    m_ddlFailure = [this](int code) {
        FLAlertLayer::create(fmt::format("Load Failed ({})", code).c_str(), "Failed to load DDL packs. Please try again later.", "OK")->show();
        m_loadingCircle->setVisible(false);
    };

    m_dclFailure = [this](int code) {
        FLAlertLayer::create(fmt::format("Load Failed ({})", code).c_str(), "Failed to load DCL packs. Please try again later.", "OK")->show();
        m_loadingCircle->setVisible(false);
    };

    auto refreshBtnSpr = CCSprite::createWithSpriteFrameName("GJ_updateBtn_001.png");
    auto refreshButton = CCMenuItemSpriteExtra::create(refreshBtnSpr, this, menu_selector(IDPackLayer::onRefresh));
    refreshButton->setPosition(ccp(winSize.width - refreshBtnSpr->getContentWidth() / 2.0f - 4.0f, refreshBtnSpr->getContentHeight() / 2.0f + 4.0f));
    refreshButton->setID("refresh-button");
    menu->addChild(refreshButton, 2);

    auto starSprite = CCSprite::createWithSpriteFrameName("GJ_starsIcon_001.png");
    starSprite->setScale(1.1f);
    m_starToggle = CCMenuItemSpriteExtra::create(starSprite, this, menu_selector(IDPackLayer::onStar));
    m_starToggle->setPosition(ccp(30.0f, 60.0f));
    m_starToggle->setColor(dclEnabled ? ccColor3B { 125, 125, 125 } : ccColor3B { 255, 255, 255 });
    m_starToggle->setID("ddl-button");
    menu->addChild(m_starToggle, 2);

    auto moonSprite = CCSprite::createWithSpriteFrameName("GJ_moonsIcon_001.png");
    moonSprite->setScale(1.1f);
    m_moonToggle = CCMenuItemSpriteExtra::create(moonSprite, this, menu_selector(IDPackLayer::onMoon));
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
    m_pageButton = CCMenuItemSpriteExtra::create(pageBtnSpr, this, menu_selector(IDPackLayer::onPage));
    m_pageButton->setPositionY(winSize.height - 39.5f);
    m_pageButton->setID("page-button");
    menu->addChild(m_pageButton);

    auto randomSprite = CCSprite::create("BI_randomBtn_001.png"_spr);
    randomSprite->setScale(0.9f);
    m_randomButton = CCMenuItemSpriteExtra::create(randomSprite, this, menu_selector(IDPackLayer::onRandom));
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
    m_lastButton = CCMenuItemSpriteExtra::create(lastArrow, this, menu_selector(IDPackLayer::onLast));
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
    m_firstButton = CCMenuItemSpriteExtra::create(firstArrow, this, menu_selector(IDPackLayer::onFirst));
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

    auto shaderCache = CCShaderCache::sharedShaderCache();
    if (!shaderCache->programForKey("pack-gradient"_spr)) {
        auto shader = new CCGLProgram();
        if (shader->initWithVertexShaderFilename("gradient.vert"_spr, "gradient.frag"_spr)) {
            shader->addAttribute(kCCAttributeNamePosition, kCCVertexAttrib_Position);
            shader->addAttribute(kCCAttributeNameColor, kCCVertexAttrib_Color);
            shader->addAttribute(kCCAttributeNameTexCoord, kCCVertexAttrib_TexCoords);
            shader->link();
            shader->updateUniforms();
            shaderCache->addProgram(shader, "pack-gradient"_spr);
        }
        shader->release();
    }

    if (dclEnabled && !DDLIntegration::dclPacks.empty()) {
        populateList("");
    }
    else if (dclEnabled) {
        DDLIntegration::loadDCL(m_dclListener, [this] {
            DDLIntegration::loadDCLPacks(m_dclListener, [this] {
                populateList("");
            }, m_dclFailure);
        }, m_dclFailure);
    }
    else if (!DDLIntegration::ddlPacks.empty()) {
        populateList("");
    }
    else {
        DDLIntegration::loadDDL(m_ddlListener, [this] {
            DDLIntegration::loadDDLPacks(m_ddlListener, [this] {
                populateList("");
            }, m_ddlFailure);
        }, m_ddlFailure);
    }

    return true;
}

void IDPackLayer::onBack(CCObject* sender) {
    CCDirector::get()->popSceneWithTransition(0.5f, kPopTransitionFade);
}

void IDPackLayer::onPrevPage(CCObject* sender) {
    page(m_page - 1);
}

void IDPackLayer::onNextPage(CCObject* sender) {
    page(m_page + 1);
}

void IDPackLayer::onRefresh(CCObject* sender) {
    showLoading();
    if (dclEnabled) {
        DDLIntegration::loadDCL(m_dclListener, [this] {
            DDLIntegration::loadDCLPacks(m_dclListener, [this] {
                populateList(m_query);
            }, m_dclFailure);
        }, m_dclFailure);
    }
    else {
        DDLIntegration::loadDDL(m_ddlListener, [this] {
            DDLIntegration::loadDDLPacks(m_ddlListener, [this] {
                populateList(m_query);
            }, m_ddlFailure);
        }, m_ddlFailure);
    }
}

void IDPackLayer::onStar(CCObject* sender) {
    if (!dclEnabled) return;
    dclEnabled = false;
    m_starToggle->setColor({ 255, 255, 255 });
    m_moonToggle->setColor({ 125, 125, 125 });
    showLoading();
    if (auto listTitle = static_cast<CCLabelBMFont*>(m_list->getChildByID("title"))) {
        listTitle->setString("DDL Packs");
        listTitle->limitLabelWidth(280.0f, 0.8f, 0.0f);
    }
    m_fullSearchResults.clear();
    if (!DDLIntegration::ddlPacks.empty()) {
        page(0);
    }
    else {
        DDLIntegration::loadDDL(m_ddlListener, [this] {
            DDLIntegration::loadDDLPacks(m_ddlListener, [this] {
                page(0);
            }, m_ddlFailure);
        }, m_ddlFailure);
    }
}

void IDPackLayer::onMoon(CCObject* sender) {
    if (dclEnabled) return;
    dclEnabled = true;
    m_starToggle->setColor({ 125, 125, 125 });
    m_moonToggle->setColor({ 255, 255, 255 });
    showLoading();
    if (auto listTitle = static_cast<CCLabelBMFont*>(m_list->getChildByID("title"))) {
        listTitle->setString("DCL Packs");
        listTitle->limitLabelWidth(280.0f, 0.8f, 0.0f);
    }
    m_fullSearchResults.clear();
    if (!DDLIntegration::dclPacks.empty()) {
        page(0);
    }
    else {
        DDLIntegration::loadDCL(m_dclListener, [this] {
            DDLIntegration::loadDCLPacks(m_dclListener, [this] {
                page(0);
            }, m_dclFailure);
        }, m_dclFailure);
    }
}

void IDPackLayer::onPage(CCObject* sender) {
    auto popup = SetIDPopup::create(m_page + 1, 1, (m_fullSearchResults.size() + 9) / 10, "Go to Page", "Go", true, 1, 60.0f, false, false);
    popup->m_delegate = this;
    popup->show();
}

void IDPackLayer::onRandom(CCObject* sender) {
    page(random::generate(0uz, (m_fullSearchResults.size() - 1) / 10));
}

void IDPackLayer::onFirst(CCObject* sender) {
    page(0);
}

void IDPackLayer::onLast(CCObject* sender) {
    page((m_fullSearchResults.size() - 1) / 10);
}

void IDPackLayer::showLoading() {
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

void IDPackLayer::populateList(const std::string& query) {
    m_fullSearchResults.clear();
    auto searchSprite = static_cast<CCSprite*>(m_searchButton->getNormalImage());
    
    const auto& currentPacks = dclEnabled ? DDLIntegration::dclPacks : DDLIntegration::ddlPacks;

    if (query.empty()) {
        m_fullSearchResults = currentPacks;
        searchSprite->setDisplayFrame(CCSpriteFrameCache::get()->spriteFrameByName("gj_findBtn_001.png"));
    }
    else {
        auto lowerQuery = string::toLower(query);
        for (auto& pack : currentPacks) {
            if (!string::toLower(pack.name).contains(lowerQuery)) continue;
            m_fullSearchResults.push_back(pack);
        }
        auto texture = CCTextureCache::get()->addImage("ID_findBtnOn_001.png"_spr, false);
        searchSprite->setDisplayFrame(CCSpriteFrame::createWithTexture(texture, { { 0.0f, 0.0f }, texture->getContentSize() }));
    }

    m_query = query;

    if (auto listView = m_list->m_listView) {
        listView->removeFromParent();
        listView->release();
    }

    auto packs = CCArray::create();
    auto start = m_page * 10;
    auto size = m_fullSearchResults.size();
    auto end = std::min<int>(size, (m_page + 1) * 10);
    auto endIt = m_fullSearchResults.begin() + end;
    
    std::string packTypeString = dclEnabled ? "DCL Pack" : "DDL Pack";
    
    for (auto it = m_fullSearchResults.begin() + start; it < endIt; ++it) {
        packs->addObject(IDPackCell::create(it->name, it->points, it->levels, it->color, packTypeString));
    }
    
    auto listView = ListView::create(packs, 100.0f, 356.0f, 190.0f);
    listView->retain();
    m_list->addChild(listView, 6, 9);
    m_list->m_listView = listView;

    m_searchBarMenu->setVisible(true);
    m_countLabel->setString(fmt::format("{} to {} of {}", start + 1, end, size).c_str());
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

void IDPackLayer::onSearch(CCObject* sender) {
    auto query = m_searchBar->getString();
    if (m_query != query) {
        showLoading();
        if (dclEnabled) {
            DDLIntegration::loadDCLPacks(m_dclListener, [this, query] {
                m_page = 0;
                populateList(query);
            }, m_dclFailure);
        } else {
            DDLIntegration::loadDDLPacks(m_ddlListener, [this, query] {
                m_page = 0;
                populateList(query);
            }, m_ddlFailure);
        }
    }
}

void IDPackLayer::page(int page) {
    auto maxPage = (m_fullSearchResults.size() + 9) / 10;
    m_page = maxPage > 0 ? (maxPage + (page % maxPage)) % maxPage : 0;
    showLoading();
    populateList(m_query);
}

void IDPackLayer::keyDown(enumKeyCodes key, double timestamp) {
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

void IDPackLayer::keyBackClicked() {
    onBack(nullptr);
}

void IDPackLayer::setIDPopupClosed(SetIDPopup*, int page) {
    m_page = std::clamp<int>(page - 1, 0, (m_fullSearchResults.size() - 1) / 10);
    showLoading();
    populateList(m_query);
}