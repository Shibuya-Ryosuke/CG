#pragma warning (disable:4866)

#include "ModelLayout.h"
#include <filesystem>
#include <fstream>
#include <map>
#include <unordered_map>
#include <vector>
#include "../Graphics/3D/Model.h"
#include "../Core/Base/Logger.h"

#include <json.hpp>

namespace RyoEngine {

    namespace {
        const char* kFilePath = "Resources/EditorData/ModelLayout.json";

        std::string scope_ = "Default";
        // scope -> key -> Transform
        std::map<std::string, std::map<std::string, Transform>> saved_;
        bool loaded_ = false;

        // 自動キー用: objパスごとの「次の番号」と「生きているModel数」
        struct Counter {
            int next = 0;
            int alive = 0;
        };
        std::unordered_map<std::string, Counter> counters_;
        std::vector<Model*> alive_;

        nlohmann::json ToJson(const Transform& t) {
            return {
                { "scale",     { t.scale.x, t.scale.y, t.scale.z } },
                { "rotate",    { t.rotate.x, t.rotate.y, t.rotate.z } },
                { "translate", { t.translate.x, t.translate.y, t.translate.z } },
            };
        }

        bool FromJson(const nlohmann::json& j, Transform& t) {
            if (!j.contains("scale") || !j.contains("rotate") || !j.contains("translate")) return false;
            t.scale = { j["scale"][0].get<float>(), j["scale"][1].get<float>(), j["scale"][2].get<float>() };
            t.rotate = { j["rotate"][0].get<float>(), j["rotate"][1].get<float>(), j["rotate"][2].get<float>() };
            t.translate = { j["translate"][0].get<float>(), j["translate"][1].get<float>(), j["translate"][2].get<float>() };
            return true;
        }

        void EnsureLoaded() {
            if (loaded_) return;
            loaded_ = true;

            std::ifstream file(kFilePath);
            if (!file) return; // まだ保存されていない
            try {
                nlohmann::json root = nlohmann::json::parse(file);
                for (auto scopeIt = root.begin(); scopeIt != root.end(); ++scopeIt) {
                    for (auto it = scopeIt.value().begin(); it != scopeIt.value().end(); ++it) {
                        Transform t{};
                        if (FromJson(it.value(), t)) {
                            saved_[scopeIt.key()][it.key()] = t;
                        }
                    }
                }
            } catch (const std::exception& e) {
                Logger::LogWarning(std::string("[ModelLayout] JSONの読み込みに失敗: ") + e.what() + "\n");
            }
        }
    }

    void ModelLayout::SetScope(const std::string& scope) { scope_ = scope; }
    const std::string& ModelLayout::GetScope() { return scope_; }

    ModelLayout::Slot ModelLayout::Register(Model* model, const std::string& objPath) {
        EnsureLoaded();
        alive_.push_back(model);

        Counter& counter = counters_[objPath];
        Slot slot;
        slot.path = objPath;
        slot.key = objPath + "#" + std::to_string(counter.next++);
        counter.alive++;
        return slot;
    }

    void ModelLayout::Unregister(Model* model, const Slot& slot) {
        for (size_t i = 0; i < alive_.size(); ++i) {
            if (alive_[i] == model) {
                alive_.erase(alive_.begin() + static_cast<std::ptrdiff_t>(i));
                break;
            }
        }
        if (!slot.path.empty()) {
            auto it = counters_.find(slot.path);
            if (it != counters_.end() && --it->second.alive <= 0) {
                // 同じobjのModelが全部いなくなったら番号を0へ戻す (シーンの再突入で同じ番号から始まる)
                counters_.erase(it);
            }
        }
    }

    ModelLayout::Slot ModelLayout::Rename(Model* model, const Slot& old, const std::string& name) {
        (void)model;
        // 自動キーのカウンタからは外れる (番号は詰めない)
        if (!old.path.empty()) {
            auto it = counters_.find(old.path);
            if (it != counters_.end() && --it->second.alive <= 0) {
                counters_.erase(it);
            }
        }
        Slot slot;
        slot.key = name;
        return slot;
    }

    bool ModelLayout::TryGetSaved(const std::string& key, Transform& out) {
        EnsureLoaded();
        auto scopeIt = saved_.find(scope_);
        if (scopeIt == saved_.end()) return false;
        auto it = scopeIt->second.find(key);
        if (it == scopeIt->second.end()) return false;
        out = it->second;
        return true;
    }

    bool ModelLayout::HasSaved(const std::string& key) {
        Transform t;
        return TryGetSaved(key, t);
    }

    void ModelLayout::Erase(const std::string& key) {
        auto scopeIt = saved_.find(scope_);
        if (scopeIt != saved_.end()) {
            scopeIt->second.erase(key);
        }
    }

    bool ModelLayout::Save() {
        EnsureLoaded();

        // 編集した(ギズモで動かした)Modelの今のTransformを反映
        for (Model* model : alive_) {
            if (model->IsLayoutEdited()) {
                saved_[scope_][model->GetLayoutKey()] = model->GetTransform();
            }
        }

        nlohmann::json root = nlohmann::json::object();
        for (const auto& [scopeName, entries] : saved_) {
            nlohmann::json scopeJson = nlohmann::json::object();
            for (const auto& [key, transform] : entries) {
                scopeJson[key] = ToJson(transform);
            }
            if (!scopeJson.empty()) {
                root[scopeName] = scopeJson;
            }
        }

        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(kFilePath).parent_path(), ec);
        std::ofstream file(kFilePath);
        if (!file) {
            Logger::LogWarning(std::string("[ModelLayout] 保存できませんでした: ") + kFilePath + "\n");
            return false;
        }
        file << root.dump(2);
        {
            const std::string message = std::string("[ModelLayout] 保存しました: ") + kFilePath + "\n";
            Logger::Log(message.c_str());
        }
        return true;
    }
}
