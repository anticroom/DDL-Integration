#include "DDLIntegration.hpp"
#include <jasmine/web.hpp>
#include <Geode/utils/file.hpp>
#include <Geode/loader/Mod.hpp>
#include <filesystem>
#include <cmath>
#include <map>
#include <set>
#include <algorithm>

using namespace geode::prelude;
using ListType = DDLIntegration::ListType;

static std::vector<IDListDemon> s_levels[DDLIntegration::listTypeCount];
static std::vector<IDDemonPack> s_packs[DDLIntegration::listTypeCount];
static std::vector<DDLLeaderboardEntry> s_leaderboards[DDLIntegration::listTypeCount];
static bool s_loaded[DDLIntegration::listTypeCount] = {};
static bool s_packsLoaded[DDLIntegration::listTypeCount] = {};

std::vector<IDListDemon> &DDLIntegration::levels(ListType type)
{
    return s_levels[static_cast<int>(type)];
}
std::vector<IDDemonPack> &DDLIntegration::packs(ListType type)
{
    return s_packs[static_cast<int>(type)];
}
std::vector<DDLLeaderboardEntry> &DDLIntegration::leaderboard(ListType type)
{
    return s_leaderboards[static_cast<int>(type)];
}
bool DDLIntegration::isLoaded(ListType type)
{
    return s_loaded[static_cast<int>(type)];
}
bool DDLIntegration::arePacksLoaded(ListType type)
{
    return s_packsLoaded[static_cast<int>(type)];
}
const char *DDLIntegration::listName(ListType type)
{
    switch (type)
    {
    case ListType::DDL:
        return "DDL";
    case ListType::DCL:
        return "DCL";
    case ListType::DVL:
        return "DVL";
    }
    return "DDL";
}

static std::string cachePathFor(ListType type, const char *suffix)
{
    auto name = std::string(DDLIntegration::listName(type));
    for (auto &c : name)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return fmt::format("{}{}.json", name, suffix);
}

static std::string fetchWithCache(const web::WebResponse &res, const std::filesystem::path &cachePath, bool &ok)
{
    ok = true;
    if (res.ok())
    {
        auto body = res.string().unwrapOr("[]");
        (void)geode::utils::file::writeString(cachePath, body);
        return body;
    }
    if (std::filesystem::exists(cachePath))
    {
        return geode::utils::file::readString(cachePath).unwrapOr("[]");
    }
    ok = false;
    return "";
}

static double roundScore(double num)
{
    return std::round(num * 1000.0) / 1000.0;
}
int DDLIntegration::getLegacyCutoff(ListType type)
{
    switch (type)
    {
    case ListType::DDL:
        return 150;
    case ListType::DCL:
    case ListType::DVL:
        return 100;
    }
    return 100;
}

static const char *rankAtlasFor(int position)
{
    if (position == 1)
        return "DDL_RubyFont";
    if (position <= 3)
        return "DDL_DiamondFont";
    if (position <= 5)
        return "DDL_GoldFont";
    if (position <= 10)
        return "DDL_SilverFont";
    if (position <= 25)
        return "DDL_BronzeFont";
    return nullptr;
}

CCLabelBMFont *DDLIntegration::createRankLabel(const std::string &text, int position, float scale)
{
    auto label = CCLabelBMFont::create(text.c_str(), "goldFont.fnt");
    if (!label)
        return nullptr;

    label->setScale(scale);

    if (auto atlas = rankAtlasFor(position))
    {
        auto name = geode::Mod::get()->expandSpriteName(fmt::format("{}.png", atlas));
        if (auto tex = CCTextureCache::get()->addImage(name.c_str(), false))
            label->setTexture(tex);
    }

    return label;
}

double DDLIntegration::calculateScore(int rank, ListType type)
{
    const int legacyCutoff = getLegacyCutoff(type);
    if (rank > legacyCutoff)
        return roundScore(1.0);
    const int listSize = legacyCutoff;
    const double coefficient = -249.0 / std::pow(listSize - 1, 0.4);
    double res = (coefficient * std::pow(rank - 1, 0.4) + 250.0);
    return roundScore(std::max(0.0, res));
}

double DDLIntegration::calculateScore(int rank, int percent, int minPercent, ListType type)
{
    const int legacyCutoff = getLegacyCutoff(type);
    const double qualifyingFloor = minPercent - 1.0;
    const double progressRatio = (percent - qualifyingFloor) / (100.0 - qualifyingFloor);
    double res;
    if (rank > legacyCutoff)
    {
        res = 1.0 * progressRatio;
    }
    else
    {
        const int listSize = legacyCutoff;
        const double coefficient = -249.0 / std::pow(listSize - 1, 0.4);
        res = (coefficient * std::pow(rank - 1, 0.4) + 250.0) * progressRatio;
    }
    res = std::max(0.0, res);
    if (percent != 100)
        return roundScore(res - res / 3.0);
    return std::max(0.0, roundScore(res));
}

void DDLIntegration::loadLevels(ListType type, TaskHolder<web::WebResponse> &listener, Function<void()> success, CopyableFunction<void(int)> failure)
{
    auto cachePath = geode::Mod::get()->getSaveDir() / cachePathFor(type, "_cache");

    listener.spawn(
        web::WebRequest().get(fmt::format("https://www.denouementdemonlist.com/api/levels?type={}", listName(type))),
        [type, cachePath, failure = std::move(failure), success = std::move(success)](web::WebResponse res) mutable
        {
            bool ok = false;
            auto jsonStr = fetchWithCache(res, cachePath, ok);
            if (!ok)
                return failure(res.code());

            auto parsed = matjson::parse(jsonStr);
            if (!parsed.isOk())
                return failure(500);

            auto &out = levels(type);
            s_loaded[static_cast<int>(type)] = true;
            out.clear();
            int index = 1;

            for (auto &level : parsed.unwrap().asArray().unwrap())
            {
                auto id = level.get<int>("id");
                auto name = level.get<std::string>("name");
                auto uid = level.get<std::string>("_id");
                auto authorRes = level.get<std::string>("author");
                if (!id.isOk() || !name.isOk() || !uid.isOk())
                    continue;

                out.emplace_back(id.unwrap(), index++, name.unwrap(), authorRes.unwrapOr("Unknown"), uid.unwrap());
            }
            success();
        });
}

void DDLIntegration::loadPacks(ListType type, TaskHolder<web::WebResponse> &listener, Function<void()> success, CopyableFunction<void(int)> failure)
{
    auto cachePath = geode::Mod::get()->getSaveDir() / cachePathFor(type, "_packs_cache");

    listener.spawn(
        web::WebRequest().get(fmt::format("https://www.denouementdemonlist.com/api/packs?type={}", listName(type))),
        [type, cachePath, failure = std::move(failure), success = std::move(success)](web::WebResponse res) mutable
        {
            bool ok = false;
            auto jsonStr = fetchWithCache(res, cachePath, ok);
            if (!ok)
                return failure(res.code());

            auto parsed = matjson::parse(jsonStr);
            if (!parsed.isOk())
                return failure(500);

            auto &demons = levels(type);
            auto &out = packs(type);
            s_packsLoaded[static_cast<int>(type)] = true;
            out.clear();

            for (auto &pack : parsed.unwrap().asArray().unwrap())
            {
                auto name = pack.get<std::string>("name");
                auto levelsUidRes = pack.get<std::vector<matjson::Value>>("levels");
                auto color = pack.get<std::string>("color").unwrapOr("#ffffff");
                if (!name.isOk() || !levelsUidRes.isOk())
                    continue;

                std::vector<int> gdIds;
                double totalPackPoints = 0.0;
                for (auto const &uuidVal : levelsUidRes.unwrap())
                {
                    if (!uuidVal.isString())
                        continue;
                    std::string uuid = uuidVal.asString().unwrap();

                    auto it = std::find_if(demons.begin(), demons.end(), [&](const IDListDemon &d)
                                           { return d.uid == uuid; });
                    if (it != demons.end())
                    {
                        gdIds.push_back(it->id);
                        totalPackPoints += calculateScore(it->position, type);
                    }
                }

                out.emplace_back(name.unwrap(), color, gdIds, roundScore(totalPackPoints * 0.33));
            }
            success();
        });
}

namespace
{
    struct UserTempData
    {
        std::string name;
        double points = 0.0;
        std::set<int> completedGdIds;
        std::vector<std::string> packs;
        double packPoints = 0.0;
        std::vector<DDLLevelRecord> verifiedLevels;
        double verifiedPoints = 0.0;
        std::vector<DDLLevelRecord> completedLevels;
        double completedPoints = 0.0;
        std::vector<DDLLevelRecord> progressedLevels;
        double progressedPoints = 0.0;
        std::set<int> progressedGdIds;
    };

    std::vector<DDLLeaderboardEntry> computeLeaderboardData(const matjson::Value &res, ListType type)
    {
        std::map<std::string, UserTempData> userMap;
        if (!res.isArray())
            return {};

        int rank = 1;
        for (auto &lvl : res.asArray().unwrap())
        {
            auto gdIdRes = lvl.get<int>("id");
            if (!gdIdRes.isOk())
                continue;
            int gdId = gdIdRes.unwrap();

            auto lvlNameRes = lvl.get<std::string>("name");
            std::string lvlName = lvlNameRes.isOk() ? lvlNameRes.unwrap() : "Unknown";

            int minPercent = lvl.get<int>("percentToQualify").unwrapOr(100);
            double baseScore = DDLIntegration::calculateScore(rank, 100, minPercent, type);

            auto verifierRes = lvl.get<std::string>("verifier");
            std::string verifier = verifierRes.isOk() ? verifierRes.unwrap() : "";
            if (!verifier.empty())
            {
                auto key = string::toLower(verifier);
                if (userMap.find(key) == userMap.end())
                    userMap[key] = {verifier};

                userMap[key].points += baseScore;
                if (userMap[key].completedGdIds.find(gdId) == userMap[key].completedGdIds.end())
                {
                    userMap[key].completedGdIds.insert(gdId);
                    userMap[key].verifiedLevels.push_back({lvlName, rank, baseScore});
                    userMap[key].verifiedPoints += baseScore;
                }
            }

            auto recordsRes = lvl.get<std::vector<matjson::Value>>("records");
            if (recordsRes.isOk())
            {
                for (auto &rec : recordsRes.unwrap())
                {
                    auto userRes = rec.get<std::string>("user");
                    auto pctRes = rec.get<int>("percent");
                    if (!userRes.isOk() || !pctRes.isOk())
                        continue;

                    std::string user = userRes.unwrap();
                    if (user.empty())
                        continue;

                    auto key = string::toLower(user);
                    if (userMap.find(key) == userMap.end())
                        userMap[key] = {user};

                    int percent = pctRes.unwrap();
                    if (percent == 100)
                    {
                        if (key != string::toLower(verifier))
                        {
                            if (userMap[key].completedGdIds.find(gdId) == userMap[key].completedGdIds.end())
                            {
                                userMap[key].points += baseScore;
                                userMap[key].completedGdIds.insert(gdId);
                                userMap[key].completedLevels.push_back({lvlName, rank, baseScore});
                                userMap[key].completedPoints += baseScore;
                            }
                        }
                    }
                    else if (percent >= minPercent)
                    {
                        if (userMap[key].completedGdIds.find(gdId) == userMap[key].completedGdIds.end() &&
                            userMap[key].progressedGdIds.find(gdId) == userMap[key].progressedGdIds.end())
                        {
                            double progressScore = DDLIntegration::calculateScore(rank, percent, minPercent, type);
                            userMap[key].points += progressScore;
                            userMap[key].progressedGdIds.insert(gdId);
                            userMap[key].progressedLevels.push_back({std::to_string(percent) + "% " + lvlName, rank, progressScore});
                            userMap[key].progressedPoints += progressScore;
                        }
                    }
                }
            }
            rank++;
        }

        const auto &packs = DDLIntegration::packs(type);
        for (auto &[key, user] : userMap)
        {
            for (auto &pack : packs)
            {
                if (pack.levels.empty())
                    continue;
                bool complete = true;
                for (int id : pack.levels)
                {
                    if (user.completedGdIds.find(id) == user.completedGdIds.end())
                    {
                        complete = false;
                        break;
                    }
                }
                if (complete)
                {
                    user.points += pack.points;
                    user.packs.push_back(pack.name);
                    user.packPoints += pack.points;
                }
            }
        }

        std::vector<DDLLeaderboardEntry> result;
        for (auto &[key, user] : userMap)
        {
            if (!user.completedLevels.empty() || !user.verifiedLevels.empty() || !user.packs.empty() || !user.progressedLevels.empty())
            {
                result.push_back({user.name, user.points,
                                  user.packs, user.packPoints,
                                  user.verifiedLevels, user.verifiedPoints,
                                  user.completedLevels, user.completedPoints,
                                  user.progressedLevels, user.progressedPoints,
                                  0});
            }
        }

        std::sort(result.begin(), result.end(), [](const auto &a, const auto &b)
                  {
            if (a.points != b.points) {
                return a.points > b.points;
            }
            size_t aTotal = a.completedLevels.size() + a.verifiedLevels.size();
            size_t bTotal = b.completedLevels.size() + b.verifiedLevels.size();
            return aTotal > bTotal; });

        for (size_t i = 0; i < result.size(); i++)
        {
            result[i].rank = i + 1;
        }

        return result;
    }
}

void DDLIntegration::loadLeaderboard(ListType type, TaskHolder<web::WebResponse> &listener, Function<void()> success, CopyableFunction<void(int)> failure)
{
    auto url = fmt::format("https://www.denouementdemonlist.com/api/levels?type={}&full=true", listName(type));

    listener.spawn(
        web::WebRequest().get(url),
        [type, failure = std::move(failure), success = std::move(success)](web::WebResponse res) mutable
        {
            if (!res.ok())
                return failure(res.code());
            auto jsonRes = res.json();
            if (!jsonRes.isOk())
                return failure(500);

            leaderboard(type) = computeLeaderboardData(jsonRes.unwrap(), type);
            success();
        });
}