#pragma once
#include <cocos2d.h>
#include <string>

class DDLProfilePackCell : public cocos2d::CCLayer {
public:
    static DDLProfilePackCell* create(const std::string& packName, double packPoints, int index);

protected:
    bool init(const std::string& packName, double packPoints, int index);
};