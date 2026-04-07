#include "DDLIntegration.hpp"
#include <jasmine/web.hpp>
#include <cmath>

using namespace geode::prelude;

std::vector<IDListDemon> DDLIntegration::ddl;
std::vector<IDDemonPack> DDLIntegration::ddlPacks;
std::vector<IDListDemon> DDLIntegration::dcl;
std::vector<IDDemonPack> DDLIntegration::dclPacks;
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