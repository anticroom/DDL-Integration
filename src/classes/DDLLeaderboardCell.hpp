#pragma once
#include <cocos2d.h>
#include <string>
#include <vector>
#include "../DDLIntegration.hpp"

class DDLLeaderboardCell : public cocos2d::CCLayer {
public:
    static DDLLeaderboardCell* create(const DDLLeaderboardEntry& entry);

protected:
    DDLLeaderboardEntry m_entry;

    bool init(const DDLLeaderboardEntry& entry);
    void onInfo(cocos2d::CCObject*);
};