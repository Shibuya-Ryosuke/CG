#pragma warning(disable: 4866)

#include "ParamEditor.h"
#include <algorithm>
#include <fstream>
#include <json.hpp>
#include <imgui.h>

namespace RyoEngine {

    // --- パラメータ登録（登録時にロード済みデータがあれば即時反映する遅延バインド対応） ---

    void ParamEditor::RegisterParam(const std::string& name, bool* value) {
        auto& inst = GetInstance();
        inst.boolParams_[name] = value;
        if (inst.isLoaded_ && inst.loadedJson_.contains(name) && inst.loadedJson_[name].is_boolean()) {
            *value = inst.loadedJson_[name].get<bool>();
        }
    }

    void ParamEditor::RegisterParam(const std::string& name, int32_t* value) {
        auto& inst = GetInstance();
        inst.int32Params_[name] = value;
        if (inst.isLoaded_ && inst.loadedJson_.contains(name) && inst.loadedJson_[name].is_number_integer()) {
            *value = inst.loadedJson_[name].get<int32_t>();
        }
    }

    void ParamEditor::RegisterParam(const std::string& name, uint32_t* value) {
        auto& inst = GetInstance();
        inst.uint32Params_[name] = value;
        if (inst.isLoaded_ && inst.loadedJson_.contains(name) && inst.loadedJson_[name].is_number_unsigned()) {
            *value = inst.loadedJson_[name].get<uint32_t>();
        }
    }

    void ParamEditor::RegisterParam(const std::string& name, float* value) {
        auto& inst = GetInstance();
        inst.floatParams_[name] = value;
        if (inst.isLoaded_ && inst.loadedJson_.contains(name) && inst.loadedJson_[name].is_number()) {
            *value = inst.loadedJson_[name].get<float>();
        }
    }

    void ParamEditor::RegisterParam(const std::string& name, Vector2* value) {
        auto& inst = GetInstance();
        inst.vec2Params_[name] = value;
        if (inst.isLoaded_ && inst.loadedJson_.contains(name) && inst.loadedJson_[name].is_array() && inst.loadedJson_[name].size() == 2) {
            value->x = inst.loadedJson_[name][0].get<float>();
            value->y = inst.loadedJson_[name][1].get<float>();
        }
    }

    void ParamEditor::RegisterParam(const std::string& name, Vector3* value) {
        auto& inst = GetInstance();
        inst.vec3Params_[name] = value;
        if (inst.isLoaded_ && inst.loadedJson_.contains(name) && inst.loadedJson_[name].is_array() && inst.loadedJson_[name].size() == 3) {
            value->x = inst.loadedJson_[name][0].get<float>();
            value->y = inst.loadedJson_[name][1].get<float>();
            value->z = inst.loadedJson_[name][2].get<float>();
        }
    }

    void ParamEditor::RegisterParam(const std::string& name, Vector4* value) {
        auto& inst = GetInstance();
        inst.vec4Params_[name] = value;
        if (inst.isLoaded_ && inst.loadedJson_.contains(name) && inst.loadedJson_[name].is_array() && inst.loadedJson_[name].size() == 4) {
            value->x = inst.loadedJson_[name][0].get<float>();
            value->y = inst.loadedJson_[name][1].get<float>();
            value->z = inst.loadedJson_[name][2].get<float>();
            value->w = inst.loadedJson_[name][3].get<float>();
        }
    }

    // --- 静的インターフェースの転送実装 ---

    void ParamEditor::DrawImGuiWindow(const char* windowName) {
        GetInstance().DrawImGuiInternal(windowName);
    }

    void ParamEditor::SaveToJson(const std::string& filepath) {
        GetInstance().SaveToJsonInternal(filepath);
    }

    void ParamEditor::LoadFromJson(const std::string& filepath) {
        GetInstance().LoadFromJsonInternal(filepath);
    }

    // --- 内部処理の実体 ---

    void ParamEditor::DrawImGuiInternal(const char* windowName) {
#ifdef _DEBUG
        ImGui::Begin(windowName);
        {
            for (auto& [name, ptr] : boolParams_) {
                ImGui::Checkbox(name.c_str(), ptr);
            }
            for (auto& [name, ptr] : int32Params_) {
                ImGui::DragInt(name.c_str(), ptr);
            }
            for (auto& [name, ptr] : uint32Params_) {
                // ImGuiにはuint用のDragがないためintへ一時キャスト
                int temp = static_cast<int>(*ptr);
                if (ImGui::DragInt(name.c_str(), &temp, 1, 0, INT_MAX)) {
                    *ptr = static_cast<uint32_t>(std::max(0, temp));
                }
            }
            for (auto& [name, ptr] : floatParams_) {
                ImGui::DragFloat(name.c_str(), ptr, 0.01f);
            }
            for (auto& [name, ptr] : vec2Params_) {
                ImGui::DragFloat2(name.c_str(), &ptr->x, 0.01f);
            }
            for (auto& [name, ptr] : vec3Params_) {
                ImGui::DragFloat3(name.c_str(), &ptr->x, 0.01f);
            }
            for (auto& [name, ptr] : vec4Params_) {
                ImGui::DragFloat4(name.c_str(), &ptr->x, 0.01f);
            }
        }
        ImGui::End();
#endif
    }

    void ParamEditor::SaveToJsonInternal(const std::string& filepath) {
        nlohmann::json j;

        for (const auto& [name, ptr] : boolParams_) {
            j[name] = *ptr;
        }
        for (const auto& [name, ptr] : int32Params_) {
            j[name] = *ptr;
        }
        for (const auto& [name, ptr] : uint32Params_) {
            j[name] = *ptr;
        }
        for (const auto& [name, ptr] : floatParams_) {
            j[name] = *ptr;
        }
        for (const auto& [name, ptr] : vec2Params_) {
            j[name] = { ptr->x, ptr->y };
        }
        for (const auto& [name, ptr] : vec3Params_) {
            j[name] = { ptr->x, ptr->y, ptr->z };
        }
        for (const auto& [name, ptr] : vec4Params_) {
            j[name] = { ptr->x, ptr->y, ptr->z, ptr->w };
        }

        std::ofstream file(filepath);
        if (file.is_open()) {
            file << j.dump(4); // インデント付きで綺麗に出力
        }
    }

    void ParamEditor::LoadFromJsonInternal(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) return;

        file >> loadedJson_;
        isLoaded_ = true;

        // すでに登録されている変数があれば一括反映
        for (auto& [name, ptr] : boolParams_) {
            if (loadedJson_.contains(name) && loadedJson_[name].is_boolean()) {
                *ptr = loadedJson_[name].get<bool>();
            }
        }
        for (auto& [name, ptr] : int32Params_) {
            if (loadedJson_.contains(name) && loadedJson_[name].is_number_integer()) {
                *ptr = loadedJson_[name].get<int32_t>();
            }
        }
        for (auto& [name, ptr] : uint32Params_) {
            if (loadedJson_.contains(name) && loadedJson_[name].is_number_unsigned()) {
                *ptr = loadedJson_[name].get<uint32_t>();
            }
        }
        for (auto& [name, ptr] : floatParams_) {
            if (loadedJson_.contains(name) && loadedJson_[name].is_number()) {
                *ptr = loadedJson_[name].get<float>();
            }
        }
        for (auto& [name, ptr] : vec2Params_) {
            if (loadedJson_.contains(name) && loadedJson_[name].is_array() && loadedJson_[name].size() == 2) {
                ptr->x = loadedJson_[name][0].get<float>();
                ptr->y = loadedJson_[name][1].get<float>();
            }
        }
        for (auto& [name, ptr] : vec3Params_) {
            if (loadedJson_.contains(name) && loadedJson_[name].is_array() && loadedJson_[name].size() == 3) {
                ptr->x = loadedJson_[name][0].get<float>();
                ptr->y = loadedJson_[name][1].get<float>();
                ptr->z = loadedJson_[name][2].get<float>();
            }
        }
        for (auto& [name, ptr] : vec4Params_) {
            if (loadedJson_.contains(name) && loadedJson_[name].is_array() && loadedJson_[name].size() == 4) {
                ptr->x = loadedJson_[name][0].get<float>();
                ptr->y = loadedJson_[name][1].get<float>();
                ptr->z = loadedJson_[name][2].get<float>();
                ptr->w = loadedJson_[name][3].get<float>();
            }
        }
    }
}