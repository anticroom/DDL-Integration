#pragma once
#include <cocos2d.h>
#include <string>
#include <vector>
#include <Geode/utils/general.hpp>
#include "../DDLIntegration.hpp"

class DDLLeaderboardCell : public cocos2d::CCLayer {
public:
    static DDLLeaderboardCell* create(const DDLLeaderboardEntry& entry, geode::CopyableFunction<void(std::string)> onProfileOpen, int index);

protected:
    DDLLeaderboardEntry m_entry;
    geode::CopyableFunction<void(std::string)> m_onProfileOpen;

    bool init(const DDLLeaderboardEntry& entry, geode::CopyableFunction<void(std::string)> onProfileOpen, int index);
    void onProfile(cocos2d::CCObject*);
};