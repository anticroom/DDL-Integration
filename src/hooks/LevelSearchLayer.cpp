#include "../classes/DDLListLayer.hpp"
#include <Geode/modify/LevelSearchLayer.hpp>

using namespace geode::prelude;

class $modify(DDLLevelSearchLayer, LevelSearchLayer) {
    bool init(int searchType) {
        if (!LevelSearchLayer::init(searchType)) return false;

        auto ddlButtonSprite = CCSprite::create("ddl-btn.png"_spr);
        
        ddlButtonSprite->setScale(1.0f); 
        
        auto ddlButton = CCMenuItemSpriteExtra::create(ddlButtonSprite, this, menu_selector(DDLLevelSearchLayer::onDDLLevels));
        
        ddlButton->setID("ddl-search-button");
        ddlButton->setTag(1);
        
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