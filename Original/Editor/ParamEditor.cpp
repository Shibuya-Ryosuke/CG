#pragma warning(disable: 4866)

#include "ParamEditor.h"
#include <algorithm>
#include <fstream>
#include <json.hpp>
#include <imgui.h>

namespace RyoEngine {

    // --- 各種登録処理 ---

    void ParamEditor::RegisterFlag(const std::string& name, bool* value) {
        auto& inst = GetInstance();
        inst.flagParams_[name] = value;
        if (inst.isLoaded_) {
            auto it = inst.loadedJson_.find(name);
            if (it != inst.loadedJson_.end() && it->is_boolean()) {
                *value = it->get<bool>();
            }
        }
    }

    void ParamEditor::RegisterValue(const std::string& name, int32_t* value, float speed, int32_t min, int32_t max) {
        auto& inst = GetInstance();
        inst.int32Params_[name] = { value, speed, min, max };
        if (inst.isLoaded_) {
            auto it = inst.loadedJson_.find(name);
            if (it != inst.loadedJson_.end() && it->is_number_integer()) {
                *value = it->get<int32_t>();
            }
        }
    }

    void ParamEditor::RegisterValue(const std::string& name, uint32_t* value, float speed, uint32_t min, uint32_t max) {
        auto& inst = GetInstance();
        inst.uint32Params_[name] = { value, speed, min, max };
        if (inst.isLoaded_) {
            auto it = inst.loadedJson_.find(name);
            if (it != inst.loadedJson_.end() && it->is_number_unsigned()) {
                *value = it->get<uint32_t>();
            }
        }
    }

    void ParamEditor::RegisterValue(const std::string& name, float* value, float speed, float min, float max) {
        auto& inst = GetInstance();
        inst.floatParams_[name] = { value, speed, min, max };
        if (inst.isLoaded_) {
            auto it = inst.loadedJson_.find(name);
            if (it != inst.loadedJson_.end() && it->is_number()) {
                *value = it->get<float>();
            }
        }
    }

    void ParamEditor::RegisterValue(const std::string& name, Vector2* value, float speed, float min, float max) {
        auto& inst = GetInstance();
        inst.vec2Params_[name] = { value, speed, {min, min}, {max, max} };
        if (inst.isLoaded_) {
            auto it = inst.loadedJson_.find(name);
            if (it != inst.loadedJson_.end() && it->is_array() && it->size() == 2) {
                value->x = (*it)[0].get<float>();
                value->y = (*it)[1].get<float>();
            }
        }
    }

    void ParamEditor::RegisterValue(const std::string& name, Vector3* value, float speed, float min, float max) {
        auto& inst = GetInstance();
        inst.vec3Params_[name] = { value, speed, {min, min, min}, {max, max, max} };
        if (inst.isLoaded_) {
            auto it = inst.loadedJson_.find(name);
            if (it != inst.loadedJson_.end() && it->is_array() && it->size() == 3) {
                value->x = (*it)[0].get<float>();
                value->y = (*it)[1].get<float>();
                value->z = (*it)[2].get<float>();
            }
        }
    }

    void ParamEditor::RegisterValue(const std::string& name, Vector4* value, float speed, float min, float max) {
        auto& inst = GetInstance();
        inst.vec4Params_[name] = { value, speed, {min, min, min, min}, {max, max, max, max} };
        if (inst.isLoaded_) {
            auto it = inst.loadedJson_.find(name);
            if (it != inst.loadedJson_.end() && it->is_array() && it->size() == 4) {
                value->x = (*it)[0].get<float>();
                value->y = (*it)[1].get<float>();
                value->z = (*it)[2].get<float>();
                value->w = (*it)[3].get<float>();
            }
        }
    }

    void ParamEditor::RegisterColor(const std::string& name, Vector3* value) {
        auto& inst = GetInstance();
        inst.color3Params_[name] = value;
        if (inst.isLoaded_) {
            auto it = inst.loadedJson_.find(name);
            if (it != inst.loadedJson_.end() && it->is_array() && it->size() == 3) {
                value->x = (*it)[0].get<float>();
                value->y = (*it)[1].get<float>();
                value->z = (*it)[2].get<float>();
            }
        }
    }

    void ParamEditor::RegisterColor(const std::string& name, Vector4* value) {
        auto& inst = GetInstance();
        inst.color4Params_[name] = value;
        if (inst.isLoaded_) {
            auto it = inst.loadedJson_.find(name);
            if (it != inst.loadedJson_.end() && it->is_array() && it->size() == 4) {
                value->x = (*it)[0].get<float>();
                value->y = (*it)[1].get<float>();
                value->z = (*it)[2].get<float>();
                value->w = (*it)[3].get<float>();
            }
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
            for (auto& [name, ptr] : flagParams_) {
                ImGui::Checkbox(name.c_str(), ptr);
            }
            for (auto& [name, info] : int32Params_) {
                ImGui::DragInt(name.c_str(), info.ptr, info.speed, info.min, info.max);
            }
            for (auto& [name, info] : uint32Params_) {
                int temp = static_cast<int>(*(info.ptr));
                if (ImGui::DragInt(name.c_str(), &temp, info.speed, static_cast<int>(info.min), static_cast<int>(info.max))) {
                    *(info.ptr) = static_cast<uint32_t>(std::max(0, temp));
                }
            }
            for (auto& [name, info] : floatParams_) {
                ImGui::DragFloat(name.c_str(), info.ptr, info.speed, info.min, info.max);
            }
            for (auto& [name, info] : vec2Params_) {
                ImGui::DragFloat2(name.c_str(), &info.ptr->x, info.speed, info.min.x, info.max.x);
            }
            for (auto& [name, info] : vec3Params_) {
                ImGui::DragFloat3(name.c_str(), &info.ptr->x, info.speed, info.min.x, info.max.x);
            }
            for (auto& [name, info] : vec4Params_) {
                ImGui::DragFloat4(name.c_str(), &info.ptr->x, info.speed, info.min.x, info.max.x);
            }
            for (auto& [name, ptr] : color3Params_) {
                ImGui::ColorEdit3(name.c_str(), &ptr->x);
            }
            for (auto& [name, ptr] : color4Params_) {
                ImGui::ColorEdit4(name.c_str(), &ptr->x);
            }
        }
        ImGui::End();
#endif
    }

    void ParamEditor::SaveToJsonInternal(const std::string& filepath) {
        nlohmann::json j;

        for (const auto& [name, ptr] : flagParams_) {
            j[name] = *ptr;
        }
        for (const auto& [name, info] : int32Params_) {
            j[name] = *(info.ptr);
        }
        for (const auto& [name, info] : uint32Params_) {
            j[name] = *(info.ptr);
        }
        for (const auto& [name, info] : floatParams_) {
            j[name] = *(info.ptr);
        }
        for (const auto& [name, info] : vec2Params_) {
            j[name] = { info.ptr->x, info.ptr->y };
        }
        for (const auto& [name, info] : vec3Params_) {
            j[name] = { info.ptr->x, info.ptr->y, info.ptr->z };
        }
        for (const auto& [name, info] : vec4Params_) {
            j[name] = { info.ptr->x, info.ptr->y, info.ptr->z, info.ptr->w };
        }
        for (const auto& [name, ptr] : color3Params_) {
            j[name] = { ptr->x, ptr->y, ptr->z };
        }
        for (const auto& [name, ptr] : color4Params_) {
            j[name] = { ptr->x, ptr->y, ptr->z, ptr->w };
        }

        std::ofstream file(filepath);
        if (file.is_open()) {
            file << j.dump(4);
        }
    }

    void ParamEditor::LoadFromJsonInternal(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) return;

        file >> loadedJson_;
        isLoaded_ = true;

        for (auto& [name, ptr] : flagParams_) {
            auto it = loadedJson_.find(name);
            if (it != loadedJson_.end() && it->is_boolean()) {
                *ptr = it->get<bool>();
            }
        }
        for (auto& [name, info] : int32Params_) {
            auto it = loadedJson_.find(name);
            if (it != loadedJson_.end() && it->is_number_integer()) {
                *(info.ptr) = it->get<int32_t>();
            }
        }
        for (auto& [name, info] : uint32Params_) {
            auto it = loadedJson_.find(name);
            if (it != loadedJson_.end() && it->is_number_unsigned()) {
                *(info.ptr) = it->get<uint32_t>();
            }
        }
        for (auto& [name, info] : floatParams_) {
            auto it = loadedJson_.find(name);
            if (it != loadedJson_.end() && it->is_number()) {
                *(info.ptr) = it->get<float>();
            }
        }
        for (auto& [name, info] : vec2Params_) {
            auto it = loadedJson_.find(name);
            if (it != loadedJson_.end() && it->is_array() && it->size() == 2) {
                info.ptr->x = (*it)[0].get<float>();
                info.ptr->y = (*it)[1].get<float>();
            }
        }
        for (auto& [name, info] : vec3Params_) {
            auto it = loadedJson_.find(name);
            if (it != loadedJson_.end() && it->is_array() && it->size() == 3) {
                info.ptr->x = (*it)[0].get<float>();
                info.ptr->y = (*it)[1].get<float>();
                info.ptr->z = (*it)[2].get<float>();
            }
        }
        for (auto& [name, info] : vec4Params_) {
            auto it = loadedJson_.find(name);
            if (it != loadedJson_.end() && it->is_array() && it->size() == 4) {
                info.ptr->x = (*it)[0].get<float>();
                info.ptr->y = (*it)[1].get<float>();
                info.ptr->z = (*it)[2].get<float>();
                info.ptr->w = (*it)[3].get<float>();
            }
        }
        for (auto& [name, ptr] : color3Params_) {
            auto it = loadedJson_.find(name);
            if (it != loadedJson_.end() && it->is_array() && it->size() == 3) {
                ptr->x = (*it)[0].get<float>();
                ptr->y = (*it)[1].get<float>();
                ptr->z = (*it)[2].get<float>();
            }
        }
        for (auto& [name, ptr] : color4Params_) {
            auto it = loadedJson_.find(name);
            if (it != loadedJson_.end() && it->is_array() && it->size() == 4) {
                ptr->x = (*it)[0].get<float>();
                ptr->y = (*it)[1].get<float>();
                ptr->z = (*it)[2].get<float>();
                ptr->w = (*it)[3].get<float>();
            }
        }
    }
}