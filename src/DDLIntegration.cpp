#include "DDLIntegration.hpp"
#include <jasmine/web.hpp>
#include <cmath>
#include <map>
#include <set>
#include <algorithm>

using namespace geode::prelude;

std::vector<IDListDemon> DDLIntegration::ddl;
std::vector<IDDemonPack> DDLIntegration::ddlPacks;
std::vector<IDLeaderboardEntry> DDLIntegration::ddlLeaderboard;

std::vector<IDListDemon> DDLIntegration::dcl;
std::vector<IDDemonPack> DDLIntegration::dclPacks;
std::vector<IDLeaderboardEntry> DDLIntegration::dclLeaderboard;

bool DDLIntegration::ddlLoaded = false;
bool DDLIntegration::dclLoaded = false;

double DDLIntegration::calculateScore(int rank) {
    if (rank > 150) return 0.0;
    const int listSize = 150;
    const double coefficient = -249.0 / std::pow(listSize - 1, 0.4);
    double res = (coefficient * std::pow(rank - 1, 0.4) + 250.0);
    return std::max(0.0, res);
}

void DDLIntegration::loadDDL(TaskHolder<web::WebResponse>& listener, Function<void()> success, CopyableFunction<void(int)> failure) {
    listener.spawn(
        web::WebRequest().get("https://denouementdl.vercel.app/api/levels?type=DDL"),
        [failure = std::move(failure), success = std::move(success)](web::WebResponse res) mutable {
            if (!res.ok()) return failure(res.code());
            ddlLoaded = true;
            ddl.clear();
            int index = 1;
            for (auto& level : jasmine::web::getArray(res)) {
                auto id = level.get<int>("id");
                auto name = level.get<std::string>("name");
                auto uid = level.get<std::string>("_id");
                auto authorRes = level.get<std::string>("author");
                if (!id.isOk() || !name.isOk() || !uid.isOk()) continue;
                
                ddl.emplace_back(id.unwrap(), index++, name.unwrap(), authorRes.unwrapOr("Unknown"), uid.unwrap());
            }
            success();
        }
    );
}

void DDLIntegration::loadDCL(TaskHolder<web::WebResponse>& listener, Function<void()> success, CopyableFunction<void(int)> failure) {
    listener.spawn(
        web::WebRequest().get("https://denouementdl.vercel.app/api/levels?type=DCL"),
        [failure = std::move(failure), success = std::move(success)](web::WebResponse res) mutable {
            if (!res.ok()) return failure(res.code());
            dclLoaded = true;
            dcl.clear();
            int index = 1;
            for (auto& level : jasmine::web::getArray(res)) {
                auto id = level.get<int>("id");
                auto name = level.get<std::string>("name");
                auto uid = level.get<std::string>("_id");
                auto authorRes = level.get<std::string>("author");
                if (!id.isOk() || !name.isOk() || !uid.isOk()) continue;

                dcl.emplace_back(id.unwrap(), index++, name.unwrap(), authorRes.unwrapOr("Unknown"), uid.unwrap());
            }
            success();
        }
    );
}

void DDLIntegration::loadDDLPacks(TaskHolder<web::WebResponse>& listener, Function<void()> success, CopyableFunction<void(int)> failure) {
    listener.spawn(
        web::WebRequest().get("https://denouementdl.vercel.app/api/packs?type=DDL"),
        [failure = std::move(failure), success = std::move(success)](web::WebResponse res) mutable {
            if (!res.ok()) return failure(res.code());
            ddlPacks.clear();
            for (auto& pack : jasmine::web::getArray(res)) {
                auto name = pack.get<std::string>("name");
                auto levelsUidRes = pack.get<std::vector<matjson::Value>>("levels");
                auto color = pack.get<std::string>("color").unwrapOr("#ffffff");
                if (!name.isOk() || !levelsUidRes.isOk()) continue;

                std::vector<int> gdIds;
                double totalPackPoints = 0.0;
                for (auto const& uuidVal : levelsUidRes.unwrap()) {
                    if (!uuidVal.isString()) continue;
                    std::string uuid = uuidVal.asString().unwrap();

                    auto it = std::find_if(ddl.begin(), ddl.end(), [&](const IDListDemon& d) { return d.uid == uuid; });
                    if (it != ddl.end()) {
                        gdIds.push_back(it->id);
                        totalPackPoints += calculateScore(it->position);
                    }
                }

                ddlPacks.emplace_back(name.unwrap(), color, gdIds, totalPackPoints * 0.33);
            }
            success();
        }
    );
}

void DDLIntegration::loadDCLPacks(TaskHolder<web::WebResponse>& listener, Function<void()> success, CopyableFunction<void(int)> failure) {
    listener.spawn(
        web::WebRequest().get("https://denouementdl.vercel.app/api/packs?type=DCL"),
        [failure = std::move(failure), success = std::move(success)](web::WebResponse res) mutable {
            if (!res.ok()) return failure(res.code());
            dclPacks.clear();
            for (auto& pack : jasmine::web::getArray(res)) {
                auto name = pack.get<std::string>("name");
                auto levelsUidRes = pack.get<std::vector<matjson::Value>>("levels");
                auto color = pack.get<std::string>("color").unwrapOr("#ffffff");
                if (!name.isOk() || !levelsUidRes.isOk()) continue;

                std::vector<int> gdIds;
                double totalPackPoints = 0.0;
                for (auto const& uuidVal : levelsUidRes.unwrap()) {
                    if (!uuidVal.isString()) continue;
                    std::string uuid = uuidVal.asString().unwrap();

                    auto it = std::find_if(dcl.begin(), dcl.end(), [&](const IDListDemon& d) { return d.uid == uuid; });
                    if (it != dcl.end()) {
                        gdIds.push_back(it->id);
                        totalPackPoints += calculateScore(it->position);
                    }
                }

                dclPacks.emplace_back(name.unwrap(), color, gdIds, totalPackPoints * 0.33);
            }
            success();
        }
    );
}

namespace {
    struct UserTempData {
        std::string name;
        double points = 0.0;
        std::set<int> completedGdIds;
        std::vector<std::string> packs;
        double packPoints = 0.0;
        std::vector<std::string> verifiedLevels;
        double verifiedPoints = 0.0;
        std::vector<std::string> completedLevels;
        double completedPoints = 0.0;
    };

    std::vector<IDLeaderboardEntry> computeLeaderboardData(const matjson::Value& res, bool isDcl) {
        std::map<std::string, UserTempData> userMap;
        if (!res.isArray()) return {};

        int rank = 1;
        for (auto& lvl : res.asArray().unwrap()) {
            auto gdIdRes = lvl.get<int>("id");
            if (!gdIdRes.isOk()) continue;
            int gdId = gdIdRes.unwrap();

            auto lvlNameRes = lvl.get<std::string>("name");
            std::string lvlName = lvlNameRes.isOk() ? lvlNameRes.unwrap() : "Unknown";

            double baseScore = DDLIntegration::calculateScore(rank);

            auto verifierRes = lvl.get<std::string>("verifier");
            std::string verifier = verifierRes.isOk() ? verifierRes.unwrap() : "";
            if (!verifier.empty()) {
                auto key = string::toLower(verifier);
                if (userMap.find(key) == userMap.end()) userMap[key] = {verifier};
                
                userMap[key].points += baseScore;
                if (userMap[key].completedGdIds.find(gdId) == userMap[key].completedGdIds.end()) {
                    userMap[key].completedGdIds.insert(gdId);
                    userMap[key].verifiedLevels.push_back(lvlName);
                    userMap[key].verifiedPoints += baseScore;
                }
            }

            auto recordsRes = lvl.get<std::vector<matjson::Value>>("records");
            if (recordsRes.isOk()) {
                for (auto& rec : recordsRes.unwrap()) {
                    auto userRes = rec.get<std::string>("user");
                    auto pctRes = rec.get<int>("percent");
                    if (!userRes.isOk() || !pctRes.isOk()) continue;
                    
                    std::string user = userRes.unwrap();
                    if (user.empty()) continue;
                    
                    auto key = string::toLower(user);
                    if (userMap.find(key) == userMap.end()) userMap[key] = {user};

                    if (pctRes.unwrap() == 100) {
                        if (key != string::toLower(verifier)) {
                            if (userMap[key].completedGdIds.find(gdId) == userMap[key].completedGdIds.end()) {
                                userMap[key].points += baseScore;
                                userMap[key].completedGdIds.insert(gdId);
                                userMap[key].completedLevels.push_back(lvlName);
                                userMap[key].completedPoints += baseScore;
                            }
                        }
                    }
                }
            }
            rank++;
        }

        const auto& packs = isDcl ? DDLIntegration::dclPacks : DDLIntegration::ddlPacks;
        for (auto& [key, user] : userMap) {
            for (auto& pack : packs) {
                if (pack.levels.empty()) continue;
                bool complete = true;
                for (int id : pack.levels) {
                    if (user.completedGdIds.find(id) == user.completedGdIds.end()) {
                        complete = false;
                        break;
                    }
                }
                if (complete) {
                    user.points += pack.points;
                    user.packs.push_back(pack.name);
                    user.packPoints += pack.points;
                }
            }
        }

        std::vector<IDLeaderboardEntry> result;
        for (auto& [key, user] : userMap) {
            if (!user.completedLevels.empty() || !user.verifiedLevels.empty() || !user.packs.empty()) {
                result.push_back({
                    user.name, user.points, 
                    user.packs, user.packPoints, 
                    user.verifiedLevels, user.verifiedPoints, 
                    user.completedLevels, user.completedPoints, 
                    0
                });
            }
        }

        std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) {
            if (a.points != b.points) {
                return a.points > b.points;
            }
            size_t aTotal = a.completedLevels.size() + a.verifiedLevels.size();
            size_t bTotal = b.completedLevels.size() + b.verifiedLevels.size();
            return aTotal > bTotal;
        });

        for (size_t i = 0; i < result.size(); i++) {
            result[i].rank = i + 1;
        }

        return result;
    }
}

void DDLIntegration::loadLeaderboard(bool isDcl, TaskHolder<web::WebResponse>& listener, Function<void()> success, CopyableFunction<void(int)> failure) {
    std::string typeStr = isDcl ? "DCL" : "DDL";
    std::string url = "https://denouementdl.vercel.app/api/levels?type=" + typeStr + "&full=true";
    
    listener.spawn(
        web::WebRequest().get(url),
        [isDcl, failure = std::move(failure), success = std::move(success)](web::WebResponse res) mutable {
            if (!res.ok()) return failure(res.code());
            auto jsonRes = res.json();
            if (!jsonRes.isOk()) return failure(500);

            auto list = computeLeaderboardData(jsonRes.unwrap(), isDcl);
            if (isDcl) dclLeaderboard = list;
            else ddlLeaderboard = list;
            
            success();
        }
    );
}