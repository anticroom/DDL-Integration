#pragma once
#include <Geode/utils/web.hpp>
#include <cocos2d.h>
#include <vector>
#include <string>

struct IDListDemon
{
    int id = 0;
    int position = 0;
    std::string name;
    std::string author;
    std::string uid;

    IDListDemon(int id, int position, std::string name, std::string author, std::string uid)
        : id(id), position(position), name(name), author(author), uid(uid) {}

    bool operator==(const IDListDemon &other) const
    {
        return id == other.id && position == other.position;
    }
};

struct IDDemonPack
{
    std::string name;
    std::string color;
    std::vector<int> levels;
    double points = 0.0;

    IDDemonPack(std::string name, std::string color, std::vector<int> levels, double points)
        : name(name), color(color), levels(levels), points(points) {}
};

struct DDLLevelRecord
{
    std::string name;
    int position;
    double points;
};

struct DDLLeaderboardEntry
{
    std::string user;
    double points = 0.0;
    std::vector<std::string> completedPacks;
    double packPoints = 0.0;
    std::vector<DDLLevelRecord> verifiedLevels;
    double verifiedPoints = 0.0;
    std::vector<DDLLevelRecord> completedLevels;
    double completedPoints = 0.0;
    std::vector<DDLLevelRecord> progressedLevels;
    double progressedPoints = 0.0;
    int rank = 0;
};

namespace DDLIntegration
{
    enum class ListType
    {
        DDL,
        DCL,
        DVL
    };
    inline constexpr int listTypeCount = 3;

    std::vector<IDListDemon> &levels(ListType);
    std::vector<IDDemonPack> &packs(ListType);
    std::vector<DDLLeaderboardEntry> &leaderboard(ListType);
    bool isLoaded(ListType);
    bool arePacksLoaded(ListType);
    const char *listName(ListType);

    void loadLevels(ListType, geode::async::TaskHolder<geode::utils::web::WebResponse> &, geode::Function<void()>, geode::CopyableFunction<void(int)>);
    void loadPacks(ListType, geode::async::TaskHolder<geode::utils::web::WebResponse> &, geode::Function<void()>, geode::CopyableFunction<void(int)>);
    void loadLeaderboard(ListType, geode::async::TaskHolder<geode::utils::web::WebResponse> &, geode::Function<void()>, geode::CopyableFunction<void(int)>);

    int getLegacyCutoff(ListType);
    cocos2d::CCLabelBMFont *createRankLabel(const std::string &text, int position, float scale);
    double calculateScore(int rank, ListType);
    double calculateScore(int rank, int percent, int minPercent, ListType);
}