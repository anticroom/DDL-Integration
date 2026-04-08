#include "../classes/DDLListLayer.hpp" // Changed from IDListLayer.hpp
#include <Geode/modify/LevelSearchLayer.hpp>
#include <Geode/ui/BasedButtonSprite.hpp>

using namespace geode::prelude;

class $modify(DDLLevelSearchLayer, LevelSearchLayer) {
    bool init(int searchType) {
        if (!LevelSearchLayer::init(searchType)) return false;

        auto spr = CCSprite::create("DDL_demonBtn_001.png"_spr);
        spr->setScale(0.85f);
        
        auto ddlButtonSprite = CircleButtonSprite::create(spr, CircleBaseColor::Green, CircleBaseSize::Small);
        ddlButtonSprite->getTopNode()->setScale(1.0f);
        ddlButtonSprite->setScale(0.85f);
        
        auto ddlButton = CCMenuItemSpriteExtra::create(ddlButtonSprite, this, menu_selector(DDLLevelSearchLayer::onDDLLevels));
        ddlButton->setID("ddl-button");
        
        if (auto menu = this->getChildByID("bottom-left-menu")) {
            menu->addChild(ddlButton);
            menu->updateLayout();
        }

        return true;
    }

    void onDDLLevels(CCObject* sender) {
        CCDirector::get()->pushScene(CCTransitionFade::create(0.5f, DDLListLayer::scene()));
    }
};