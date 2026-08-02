#include "DDLProfilePackCell.hpp"
#include <Geode/utils/cocos.hpp>

using namespace geode::prelude;

DDLProfilePackCell* DDLProfilePackCell::create(const std::string& packName, double packPoints, int index) {
    auto ret = new DDLProfilePackCell();
    if (ret->init(packName, packPoints, index)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool DDLProfilePackCell::init(const std::string& packName, double packPoints, int index) {
    if (!CCLayer::init()) return false;
    
    setID("DDLProfilePackCell");
    setContentSize({ 380.0f, 30.0f });

    auto bg = cocos2d::extension::CCScale9Sprite::create("square02b_001.png");
    bg->setContentSize({ 380.0f, 30.0f });
    bg->setPosition(ccp(190.0f, 15.0f));
    bg->setColor({ 100, 50, 0 });
    bg->setOpacity(index % 2 == 0 ? 120 : 60); 
    bg->setID("background");
    addChild(bg);

    auto nameLabel = CCLabelBMFont::create(packName.c_str(), "bigFont.fnt");
    nameLabel->setAnchorPoint(ccp(0.0f, 0.5f));
    nameLabel->setPosition(ccp(10.0f, 15.0f));
    nameLabel->limitLabelWidth(250.0f, 0.55f, 0.0f);
    nameLabel->setID("name-label");
    addChild(nameLabel);

    auto pointsLabel = CCLabelBMFont::create(fmt::format("{:.3f} PTS", packPoints).c_str(), "goldFont.fnt");
    pointsLabel->setAnchorPoint(ccp(1.0f, 0.5f));
    pointsLabel->setPosition(ccp(370.0f, 15.0f));
    pointsLabel->setScale(0.45f);
    pointsLabel->setID("points-label");
    addChild(pointsLabel);

    return true;
}