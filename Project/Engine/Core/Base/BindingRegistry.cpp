#include "BindingRegistry.h"
#include "Logger.h"

#ifdef NRELEASE
#include <imgui.h>
#endif

namespace RyoEngine {

    namespace {
#ifdef NRELEASE
        // 登録済みキーの一覧から1つ選ぶコンボ。選択が変わったらtrueを返す
        template <typename Map>
        bool DrawKeyCombo(const char* label, std::string& selectedKey, const Map& registry) {
            std::string preview = selectedKey.empty() ? "(なし)" : selectedKey;
            if (!selectedKey.empty() && registry.find(selectedKey) == registry.end()) {
                preview += " (未登録)";
            }

            bool changed = false;
            if (ImGui::BeginCombo(label, preview.c_str())) {
                if (ImGui::Selectable("(なし)", selectedKey.empty())) {
                    selectedKey.clear();
                    changed = true;
                }
                for (const auto& [key, ptr] : registry) {
                    bool selected = (key == selectedKey);
                    if (ImGui::Selectable(key.c_str(), selected)) {
                        selectedKey = key;
                        changed = true;
                    }
                    if (selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            return changed;
        }
#endif
    }

    BindingRegistry* BindingRegistry::GetInstance() {
        static BindingRegistry instance;
        return &instance;
    }

    void BindingRegistry::RegisterPosition(const std::string& key, const Vector3* position) {
        BindingRegistry* instance = GetInstance();
        if (key.empty()) {
            Logger::LogWarning("[BindingRegistry] RegisterPosition: キーが空です");
            return;
        }
        if (position == nullptr) {
            instance->positions_.erase(key);
            return;
        }
        instance->positions_[key] = position;
    }

    void BindingRegistry::RegisterFlag(const std::string& key, const bool* flag) {
        BindingRegistry* instance = GetInstance();
        if (key.empty()) {
            Logger::LogWarning("[BindingRegistry] RegisterFlag: キーが空です");
            return;
        }
        if (flag == nullptr) {
            instance->flags_.erase(key);
            return;
        }
        instance->flags_[key] = flag;
    }

    void BindingRegistry::UnregisterPosition(const std::string& key) {
        GetInstance()->positions_.erase(key);
    }

    void BindingRegistry::UnregisterFlag(const std::string& key) {
        GetInstance()->flags_.erase(key);
    }

    void BindingRegistry::Unregister(const std::string& key) {
        UnregisterPosition(key);
        UnregisterFlag(key);
    }

    void BindingRegistry::Clear() {
        BindingRegistry* instance = GetInstance();
        instance->positions_.clear();
        instance->flags_.clear();
    }

    const Vector3* BindingRegistry::FindPosition(const std::string& key) {
        if (key.empty()) {
            return nullptr;
        }
        const auto& positions = GetInstance()->positions_;
        auto it = positions.find(key);
        return it != positions.end() ? it->second : nullptr;
    }

    const bool* BindingRegistry::FindFlag(const std::string& key) {
        if (key.empty()) {
            return nullptr;
        }
        const auto& flags = GetInstance()->flags_;
        auto it = flags.find(key);
        return it != flags.end() ? it->second : nullptr;
    }

    bool BindingRegistry::DrawPositionCombo(const char* label, std::string& selectedKey) {
#ifdef NRELEASE
        return DrawKeyCombo(label, selectedKey, GetInstance()->positions_);
#else
        (void)label;
        (void)selectedKey;
        return false;
#endif
    }

    bool BindingRegistry::DrawFlagCombo(const char* label, std::string& selectedKey) {
#ifdef NRELEASE
        return DrawKeyCombo(label, selectedKey, GetInstance()->flags_);
#else
        (void)label;
        (void)selectedKey;
        return false;
#endif
    }
}
