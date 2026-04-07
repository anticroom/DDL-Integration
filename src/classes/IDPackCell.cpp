#include "IDPackCell.hpp"
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/binding/GameStatsManager.hpp>
#include <Geode/binding/LevelBrowserLayer.hpp>
#include <Geode/binding/GJSearchObject.hpp>
#include <Geode/loader/Mod.hpp>
#include <cstdio>
#include <string>

using namespace geode::prelude;

ccColor3B hexToRgb(const std::string& hex) {
    if (hex.length() < 7 || hex.front() != '#') return { 255, 255, 255 };
    
    int r = 255, g = 255, b = 255;
    if (sscanf(hex.c_str(), "#%02x%02x%02x", &r, &g, &b) == 3) {
        return { static_cast<GLubyte>(r), static_cast<GLubyte>(g), static_cast<GLubyte>(b) };
    }
    return { 255, 255, 255 };
}

IDPackCell* IDPackCell::create(std::string_view name, double points, std::span<const int> levels, std::string_view colorHex, std::string_view packType) {
    auto ret = new IDPackCell();
    if (ret->init(name, points, levels, colorHex, packType)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool IDPackCell::init(std::string_view name, double points, std::span<const int> levels, std::string_view colorHex, std::string_view packType) {
    if (!CCLayer::init()) return false;

    setID("IDPackCell");

    m_levels.assign(levels.begin(), levels.end());
    
    bool isDCL = (packType == "DCL Pack" || packType == "DCL");

    const char* faceSprite = isDCL ? "difficulty_05_btn_001.png" : "difficulty_09_btn_001.png";

    auto difficultySprite = CCSprite::createWithSpriteFrameName(faceSprite);
    difficultySprite->setPosition(ccp(31.0f, 50.0f));
    difficultySprite->setScale(1.1f);
    difficultySprite->setID("difficulty-sprite");
    addChild(difficultySprite, 2);

    auto diffText = CCLabelBMFont::create(isDCL ? "CHALLENGE" : "DEMON", "chatFont.fnt");
    diffText->setPosition(ccp(31.0f, 15.0f));
    diffText->setScale(isDCL ? 0.45f : 0.6f);
    addChild(diffText, 2);

    if (auto shader = CCShaderCache::sharedShaderCache()->programForKey("pack-gradient"_spr)) {
        m_background = CCSprite::create("ID_background_001.png"_spr);
        m_background->setPosition(ccp(178.0f, 50.0f));
        m_background->setShaderProgram(shader);
        m_background->setFlipX(true);
        m_background->setID("background");
        addChild(m_background);
    }

    auto nameLabel = CCLabelBMFont::create(name.data(), "bigFont.fnt");
    nameLabel->setPosition(ccp(162.0f, 85.0f));
    nameLabel->limitLabelWidth(205.0f, 0.9f, 0.0f);
    nameLabel->setID("name-label");
    addChild(nameLabel);

    auto typeLabel = CCLabelBMFont::create(packType.data(), "bigFont.fnt");
    typeLabel->setPosition(ccp(162.0f, 65.0f));
    typeLabel->setScale(0.4f);
    typeLabel->setID("type-label");
    addChild(typeLabel);

    auto viewSprite = ButtonSprite::create("View", 50, 0, 0.6f, false, "bigFont.fnt", "GJ_button_01.png", 50.0f);
    auto viewMenu = CCMenu::create();
    auto viewButton = CCMenuItemSpriteExtra::create(viewSprite, this, menu_selector(IDPackCell::onClick));
    viewButton->setID("view-button");
    viewMenu->addChild(viewButton);
    viewMenu->setPosition(ccp(347.0f - viewSprite->getContentWidth() / 2.0f, 50.0f));
    viewMenu->setID("view-menu");
    addChild(viewMenu);

    auto progressBackground = CCSprite::create("GJ_progressBar_001.png");
    progressBackground->setColor({ 0, 0, 0 });
    progressBackground->setOpacity(125);
    progressBackground->setScaleX(0.6f);
    progressBackground->setScaleY(0.8f);
    progressBackground->setPosition(ccp(164.0f, 48.0f));
    progressBackground->setID("progress-background");
    addChild(progressBackground, 3);

    auto progressBar = CCSprite::create("GJ_progressBar_001.png");
    progressBar->setColor({ 184, 0, 0 });
    progressBar->setScaleX(0.985f);
    progressBar->setScaleY(0.83f);
    progressBar->setAnchorPoint(ccp(0.0f, 0.5f));
    auto rect = progressBar->getTextureRect();
    progressBar->setPosition(ccp(rect.size.width * 0.0075f, progressBackground->getContentHeight() / 2.0f));
    auto gsm = GameStatsManager::get();
    auto total = levels.size();
    auto completed = std::ranges::count_if(levels, [gsm](int level) {
        return gsm->hasCompletedOnlineLevel(level);
    });
    if (total > 0) {
        rect.size.width *= (float)completed / (float)total;
    } else {
        rect.size.width = 0;
    }
    progressBar->setTextureRect(rect);
    progressBar->setID("progress-bar");
    progressBackground->addChild(progressBar, 1);

    auto progressLabel = CCLabelBMFont::create(fmt::format("{}/{}", completed, total).c_str(), "bigFont.fnt");
    progressLabel->setPosition(ccp(164.0f, 48.0f));
    progressLabel->setScale(0.5f);
    progressLabel->setID("progress-label");
    addChild(progressLabel, 4);

    auto pointsLabel = CCLabelBMFont::create(fmt::format("{:.2f} Points", points).c_str(), "bigFont.fnt");
    pointsLabel->setPosition(ccp(164.0f, 20.0f));
    pointsLabel->setScale(0.7f);
    pointsLabel->setColor(completed >= total ? ccColor3B { 255, 255, 50 } : ccColor3B { 255, 255, 255 });
    pointsLabel->setID("points-label");
    addChild(pointsLabel, 1);

    if (completed >= total && total > 0) {
        auto completedSprite = CCSprite::createWithSpriteFrameName("GJ_completesIcon_001.png");
        completedSprite->setPosition(ccp(250.0f, 49.0f));
        completedSprite->setID("completed-sprite");
        addChild(completedSprite, 5);
    }

    std::string hexStr(colorHex);
    ccColor3B packColor = hexToRgb(hexStr);
    
    m_colors.emplace_back(packColor.r / 255.0f, packColor.g / 255.0f, packColor.b / 255.0f, 1.0f);
    m_colors.emplace_back(255.0f / 255.0f, 255.0f / 255.0f, 255.0f / 255.0f, 1.0f); 
    
    m_colorMode = 1;

    return true;
}

void IDPackCell::onClick(CCObject* sender) {
    if (m_levels.empty()) return;

    std::string query = "";
    for (size_t i = 0; i < m_levels.size(); ++i) {
        if (i > 0) query += ",";
        query += std::to_string(m_levels[i]);
    }

    auto searchObject = GJSearchObject::create(SearchType::Type19, query);
    auto browserLayer = LevelBrowserLayer::scene(searchObject);
    CCDirector::get()->pushScene(CCTransitionFade::create(0.5f, browserLayer));
}

void IDPackCell::draw() {
    if (!m_background) return;

    auto shader = m_background->getShaderProgram();
    if (!shader) return;

    shader->use();
    shader->setUniformsForBuiltins();
    shader->setUniformLocationWith1i(shader->getUniformLocationForName("colorMode"), m_colorMode);
    shader->setUniformLocationWith4fv(shader->getUniformLocationForName("colors"), reinterpret_cast<float*>(m_colors.data()), m_colors.size());
}