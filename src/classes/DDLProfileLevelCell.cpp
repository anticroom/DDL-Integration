#include "DDLProfileLevelCell.hpp"
#include <Geode/utils/cocos.hpp>
#include <Geode/loader/Mod.hpp>

using namespace geode::prelude;

DDLProfileLevelCell* DDLProfileLevelCell::create(const DDLLevelRecord& record, bool isVerification, bool isDCL, int index) {
    auto ret = new DDLProfileLevelCell();
    if (ret->init(record, isVerification, isDCL, index)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool DDLProfileLevelCell::init(const DDLLevelRecord& record, bool isVerification, bool isDCL, int index) {
    if (!CCLayer::init()) return false;
    
    setID("DDLProfileLevelCell");
    setContentSize({ 380.0f, 30.0f });

    auto bg = CCLayerColor::create(ccc4(0, 0, 0, index % 2 == 0 ? 80 : 30), 380.0f, 30.0f);
    bg->setPosition(ccp(0.0f, 0.0f));
    bg->setID("background");
    addChild(bg);

    auto nameLabel = CCLabelBMFont::create(record.name.c_str(), "bigFont.fnt");
    nameLabel->setAnchorPoint(ccp(0.0f, 0.5f));
    nameLabel->setPosition(ccp(10.0f, 15.0f));
    nameLabel->limitLabelWidth(180.0f, 0.55f, 0.0f);
    if (isVerification) nameLabel->setColor({ 100, 255, 100 });
    nameLabel->setID("name-label");
    addChild(nameLabel);

    auto placementLabel = CCLabelBMFont::create(fmt::format("#{}", record.position).c_str(), "goldFont-uhd.fnt");
    placementLabel->setPosition(ccp(230.0f, 15.0f));
    placementLabel->setScale(0.55f * (cocos2d::CC_CONTENT_SCALE_FACTOR() / 4.0f));
    placementLabel->setID("placement-label");

    std::string fontTexFile = "";
    int legacyThreshold = isDCL ? 100 : 150;

    if (record.position == 1) {
        fontTexFile = "DDL_RubyFont-uhd.png";
    } else if (record.position <= 3) {
        fontTexFile = "DDL_DiamondFont-uhd.png";
    } else if (record.position <= 5) {
        fontTexFile = "DDL_GoldFont-uhd.png";
    } else if (record.position <= 10) {
        fontTexFile = "DDL_SilverFont-uhd.png";
    } else if (record.position <= 25) {
        fontTexFile = "DDL_BronzeFont-uhd.png";
    } else if (record.position > legacyThreshold) {
        placementLabel->setColor({ 255, 75, 75 });
    }

    if (!fontTexFile.empty()) {
        if (auto tex = CCTextureCache::get()->addImage(geode::Mod::get()->expandSpriteName(fontTexFile.c_str()).c_str(), false)) {
            placementLabel->setTexture(tex);
        }
    }

    addChild(placementLabel);

    auto pointsLabel = CCLabelBMFont::create(fmt::format("{:.3f} pts", record.points).c_str(), "goldFont.fnt");
    pointsLabel->setAnchorPoint(ccp(1.0f, 0.5f));
    pointsLabel->setPosition(ccp(370.0f, 15.0f));
    pointsLabel->setScale(0.45f);
    pointsLabel->setID("points-label");
    addChild(pointsLabel);

    return true;
}