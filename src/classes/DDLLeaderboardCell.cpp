#include "DDLLeaderboardCell.hpp"
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/loader/Mod.hpp>

using namespace geode::prelude;

DDLLeaderboardCell* DDLLeaderboardCell::create(const DDLLeaderboardEntry& entry, geode::CopyableFunction<void(std::string)> onProfileOpen, int index) {
    auto ret = new DDLLeaderboardCell();
    if (ret->init(entry, onProfileOpen, index)) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool DDLLeaderboardCell::init(const DDLLeaderboardEntry& entry, geode::CopyableFunction<void(std::string)> onProfileOpen, int index) {
    if (!CCLayer::init()) return false;
    
    m_entry = entry;
    m_onProfileOpen = onProfileOpen;
    setContentSize({356.0f, 35.0f});

    auto bg = CCLayerColor::create(ccc4(0, 0, 0, index % 2 == 0 ? 95 : 55), 356.0f, 35.0f);
    bg->setPosition(ccp(0.0f, 0.0f));
    addChild(bg);

    auto rankLabel = CCLabelBMFont::create(fmt::format("#{}", entry.rank).c_str(), "bigFont.fnt");
    
    if (entry.rank == 1) {
        rankLabel->setColor({255, 200, 50});
    } else if (entry.rank == 2) {
        rankLabel->setColor({200, 200, 200});
    } else if (entry.rank == 3) {
        rankLabel->setColor({210, 140, 70});
    } else {
        rankLabel->setColor({255, 255, 255});
    }

    rankLabel->setPosition(ccp(25.0f, 17.5f));
    rankLabel->setScale(entry.rank <= 3 ? 0.60f : 0.45f);
    addChild(rankLabel);

    auto nameLabel = CCLabelBMFont::create(entry.user.c_str(), "bigFont.fnt");
    nameLabel->setAnchorPoint(ccp(0.0f, 0.5f));
    nameLabel->setPosition(ccp(55.0f, 17.5f));
    nameLabel->limitLabelWidth(140.0f, 0.55f, 0.0f);
    addChild(nameLabel);

    auto ptsLabel = CCLabelBMFont::create(fmt::format("{:.2f} Pts", entry.points).c_str(), "goldFont.fnt");
    ptsLabel->setAnchorPoint(ccp(1.0f, 0.5f));
    ptsLabel->setPosition(ccp(325.0f, 17.5f));
    ptsLabel->setScale(0.45f);
    addChild(ptsLabel);

    auto profileSpr = CCSprite::create("DDL_ProfileIcon_001.png"_spr);
    constexpr float infoIconPointSize = 23.0f * 0.65f;
    float profileScale = infoIconPointSize / std::max(profileSpr->getContentSize().width, profileSpr->getContentSize().height);
    profileSpr->setScale(profileScale);

    auto btn = CCMenuItemSpriteExtra::create(profileSpr, this, menu_selector(DDLLeaderboardCell::onProfile));
    
    auto menu = CCMenu::create();
    menu->addChild(btn);
    menu->setPosition(ccp(342.0f, 17.5f));
    addChild(menu);

    return true;
}

void DDLLeaderboardCell::onProfile(CCObject*) {
    if (m_onProfileOpen) {
        m_onProfileOpen(m_entry.user);
    }
}