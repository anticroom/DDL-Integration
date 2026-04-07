#include "../classes/IDListLayer.hpp"
#include <Geode/modify/LevelSearchLayer.hpp>
#include <Geode/ui/BasedButtonSprite.hpp>

using namespace geode::prelude;

class $modify(DDLLevelSearchLayer, LevelSearchLayer) {
    bool init(int searchType) {
        if (!LevelSearchLayer::init(searchType)) return false;

        auto spr = CCSprite::create("ID_demonBtn_001.png"_spr);
        auto ddlButtonSprite = CircleButtonSprite::create(spr, CircleBaseColor::Green, CircleBaseSize::Small);
        ddlButtonSprite->getTopNode()->setScale(1.0f);
        ddlButtonSprite->setScale(0.85f);
        
        auto ddlButton = CCMenuItemSpriteExtra::create(ddlButtonSprite, this, menu_selector(DDLLevelSearchLayer::onDDLLevels));
        ddlButton->setID("ddl-button");
        
        auto winSize = CCDirector::get()->getWinSize();
        auto customMenu = CCMenu::create();
        customMenu->setPosition(0, 0); 
        ddlButton->setPosition(ccp(25.0f, winSize.height - 75.0f)); 
        customMenu->addChild(ddlButton);
        
        addChild(customMenu);

        return true;
    }

    void onDDLLevels(CCObject* sender) {
        CCDirector::get()->pushScene(CCTransitionFade::create(0.5f, IDListLayer::scene()));
    }
};