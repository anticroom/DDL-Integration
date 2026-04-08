#include "DDLPackCell.hpp"
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/binding/GameStatsManager.hpp>
#include <Geode/binding/LevelBrowserLayer.hpp>
#include <Geode/binding/GJSearchObject.hpp>
#include <Geode/loader/Mod.hpp>
#include <cstdio>
#include <string>

using namespace geode::prelude;

std::string g_currentPackName = "";

ccColor3B hexToRgb(const std::string& hex) {
    if (hex.length() < 7 || hex.front() != '#') return { 255, 255, 255 };
    
    int r = 255, g = 255, b = 255;
    if (sscanf(hex.c_str(), "#%02x%02x%02x", &r, &g, &b) == 3) {
        return { static_cast<GLubyte>(r), static_cast<GLubyte>(g), static_cast<GLubyte>(b) };
    }
    return { 255, 255, 255 };
}
DDLPackCell* DDLPackCell::create(std::string_view name, double points, std::span<const int> levels, std::string_view colorHex, std::string_view packType) {
    auto ret = new DDLPackCell();
    if (ret->init(name, points, levels, colorHex, packType)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool DDLPackCell::init(std::string_view name, double points, std::span<const int> levels, std::string_view colorHex, std::string_view packType) {
    if (!CCLayer::init()) return false;

    setID("DDLPackCell");
    setContentSize({ 356.0f, 100.0f });

    m_levels.assign(levels.begin(), levels.end());
    m_packName = name;

    std::string hexStr(colorHex);
    ccColor3B packColor = hexToRgb(hexStr);

    auto bg = cocos2d::extension::CCScale9Sprite::create("square02b_001.png");
    bg->setContentSize({ 356.0f, 100.0f });
    bg->setPosition(ccp(178.0f, 50.0f));
    bg->setColor(packColor);
    bg->setOpacity(115);
    bg->setID("background");
    addChild(bg, -1);

    auto nameLabel = CCLabelBMFont::create(name.data(), "bigFont.fnt");
    nameLabel->setAnchorPoint(ccp(0.0f, 0.5f));
    nameLabel->setPosition(ccp(15.0f, 78.0f));
    nameLabel->limitLabelWidth(210.0f, 0.85f, 0.0f);
    nameLabel->setID("name-label");
    addChild(nameLabel);

    auto gsm = GameStatsManager::get();
    auto total = levels.size();
    auto completed = std::ranges::count_if(levels, [gsm](int level) {
        return gsm->hasCompletedOnlineLevel(level);
    });

    auto pointsLabel = CCLabelBMFont::create(fmt::format("{:.2f} Points", points).c_str(), "goldFont.fnt");
    pointsLabel->setAnchorPoint(ccp(0.0f, 0.5f));
    pointsLabel->setPosition(ccp(15.0f, 52.0f));
    pointsLabel->setScale(0.6f);
    pointsLabel->setColor(completed >= total && total > 0 ? ccColor3B { 255, 255, 50 } : ccColor3B { 255, 255, 255 });
    pointsLabel->setID("points-label");
    addChild(pointsLabel, 1);

    auto progressBackground = CCSprite::create("GJ_progressBar_001.png");
    progressBackground->setColor({ 0, 0, 0 });
    progressBackground->setOpacity(125);
    progressBackground->setAnchorPoint(ccp(0.0f, 0.5f));
    progressBackground->setScaleX(0.65f); 
    progressBackground->setScaleY(0.8f);
    progressBackground->setPosition(ccp(15.0f, 22.0f));
    progressBackground->setID("progress-background");
    addChild(progressBackground, 3);

    auto progressBar = CCSprite::create("GJ_progressBar_001.png");
    progressBar->setColor({ 184, 0, 0 });
    progressBar->setScaleX(0.985f);
    progressBar->setScaleY(0.83f);
    progressBar->setAnchorPoint(ccp(0.0f, 0.5f));
    
    auto rect = progressBar->getTextureRect();
    progressBar->setPosition(ccp(rect.size.width * 0.0075f, progressBackground->getContentHeight() / 2.0f));
    
    if (total > 0) {
        rect.size.width *= (float)completed / (float)total;
    } else {
        rect.size.width = 0;
    }
    
    progressBar->setTextureRect(rect);
    progressBar->setID("progress-bar");
    progressBackground->addChild(progressBar, 1);

    auto progressLabel = CCLabelBMFont::create(fmt::format("{}/{}", completed, total).c_str(), "bigFont.fnt");
    progressLabel->setPosition(ccp(15.0f + (progressBackground->getScaledContentSize().width / 2.0f), 22.0f));
    progressLabel->setScale(0.45f);
    progressLabel->setID("progress-label");
    addChild(progressLabel, 4);

    bool isDCL = (packType == "DCL Pack" || packType == "DCL");
    const char* faceSprite = isDCL ? "difficulty_05_btn_001.png" : "difficulty_09_btn_001.png";

    auto difficultySprite = CCSprite::createWithSpriteFrameName(faceSprite);
    difficultySprite->setPosition(ccp(280.0f, 60.0f)); 
    difficultySprite->setScale(1.1f);
    difficultySprite->setID("difficulty-sprite");
    addChild(difficultySprite, 2);

    auto diffText = CCLabelBMFont::create(isDCL ? "CHALLENGE" : "DEMON", "chatFont.fnt");
    diffText->setPosition(ccp(280.0f, 25.0f)); 
    diffText->setScale(isDCL ? 0.45f : 0.55f);
    addChild(diffText, 2);

    auto viewSprite = CCSprite::createWithSpriteFrameName("GJ_arrow_02_001.png");
    viewSprite->setFlipX(true);
    viewSprite->setScale(0.6f);
    
    auto viewMenu = CCMenu::create();
    auto viewButton = CCMenuItemSpriteExtra::create(viewSprite, this, menu_selector(DDLPackCell::onClick));
    viewButton->setID("view-button");
    viewMenu->addChild(viewButton);
    viewMenu->setPosition(ccp(335.0f, 50.0f));
    viewMenu->setID("view-menu");
    addChild(viewMenu);

    if (completed >= total && total > 0) {
        auto completedSprite = CCSprite::createWithSpriteFrameName("GJ_completesIcon_001.png");
        completedSprite->setPosition(ccp(15.0f + nameLabel->getScaledContentSize().width + 15.0f, 78.0f));
        completedSprite->setScale(0.7f);
        completedSprite->setID("completed-sprite");
        addChild(completedSprite, 5);
    }

    return true;
}

void DDLPackCell::onClick(CCObject* sender) {
    if (m_levels.empty()) return;

    g_currentPackName = m_packName;

    std::string query = "";
    for (size_t i = 0; i < m_levels.size(); ++i) {
        if (i > 0) query += ",";
        query += std::to_string(m_levels[i]);
    }

    auto searchObject = GJSearchObject::create(SearchType::Type19, query);
    auto browserLayer = LevelBrowserLayer::scene(searchObject);
    CCDirector::get()->pushScene(CCTransitionFade::create(0.5f, browserLayer));
}