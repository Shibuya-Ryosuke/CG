#pragma warning(disable: 4866)

#include "ParamEditor.h"
#include "../../Core/Base/Logger.h"
#include "../Graphics/2D/Sprite.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <json.hpp>
#include <imgui.h>

namespace RyoEngine {

    // --- フォルダパス管理 ---

    void ParamEditor::SetFolderPath(const std::string& folderPath) {
        GetInstance().folderPath_ = folderPath;
    }

    const std::string& ParamEditor::GetFolderPath() {
        return GetInstance().folderPath_;
    }

    std::string ParamEditor::GetFullFilePath() const {
        namespace fs = std::filesystem;
        fs::path p(folderPath_);
        p /= kFileName;
        return p.string();
    }

    // --- グループ階層管理 ---

    void ParamEditor::BeginGroup(const std::string& name, bool open) {
        GetInstance().BeginGroupInternal(name, open);
    }

    void ParamEditor::EndGroup() {
        GetInstance().EndGroupInternal();
    }

    void ParamEditor::BeginGroupInternal(const std::string& name, bool open) {
        ParamEntry groupEntry{};
        groupEntry.name = name;
        groupEntry.type = EntryType::Group;
        groupEntry.defaultOpen = open;

        AddEntry(groupEntry);

        if (groupStack_.empty()) {
            groupStack_.push_back(&rootEntries_.back());
        } else {
            groupStack_.push_back(&(groupStack_.back()->children.back()));
        }
    }

    void ParamEditor::EndGroupInternal() {
        if (!groupStack_.empty()) {
            groupStack_.pop_back();
        }
    }

    void ParamEditor::AddEntry(ParamEntry entry) {
        if (groupStack_.empty()) {
            rootEntries_.push_back(entry);
        } else {
            groupStack_.back()->children.push_back(entry);
        }
    }

    // --- 各種パラメータ登録 ---

    void ParamEditor::RegisterFlag(const std::string& name, bool* value) {
        ParamEntry e{};
        e.name = name;
        e.type = EntryType::Flag;
        e.ptr = value;
        GetInstance().AddEntry(e);
    }

    void ParamEditor::RegisterValue(const std::string& name, int32_t* value, float speed, int32_t min, int32_t max) {
        ParamEntry e{};
        e.name = name;
        e.type = EntryType::Int32;
        e.ptr = value;
        e.speed = speed;
        e.minInt = min;
        e.maxInt = max;
        GetInstance().AddEntry(e);
    }

    void ParamEditor::RegisterValue(const std::string& name, uint32_t* value, float speed, uint32_t min, uint32_t max) {
        ParamEntry e{};
        e.name = name;
        e.type = EntryType::UInt32;
        e.ptr = value;
        e.speed = speed;
        e.minInt = static_cast<int32_t>(min);
        e.maxInt = static_cast<int32_t>(max);
        GetInstance().AddEntry(e);
    }

    void ParamEditor::RegisterValue(const std::string& name, float* value, float speed, float min, float max) {
        ParamEntry e{};
        e.name = name;
        e.type = EntryType::Float;
        e.ptr = value;
        e.speed = speed;
        e.minVal[0] = min;
        e.maxVal[0] = max;
        GetInstance().AddEntry(e);
    }

    void ParamEditor::RegisterValue(const std::string& name, Vector2* value, float speed, float min, float max) {
        ParamEntry e{};
        e.name = name;
        e.type = EntryType::Vector2;
        e.ptr = value;
        e.speed = speed;
        e.minVal[0] = min; e.minVal[1] = min;
        e.maxVal[0] = max; e.maxVal[1] = max;
        GetInstance().AddEntry(e);
    }

    void ParamEditor::RegisterValue(const std::string& name, Vector3* value, float speed, float min, float max) {
        ParamEntry e{};
        e.name = name;
        e.type = EntryType::Vector3;
        e.ptr = value;
        e.speed = speed;
        e.minVal[0] = min; e.minVal[1] = min; e.minVal[2] = min;
        e.maxVal[0] = max; e.maxVal[1] = max; e.maxVal[2] = max;
        GetInstance().AddEntry(e);
    }

    void ParamEditor::RegisterValue(const std::string& name, Vector4* value, float speed, float min, float max) {
        ParamEntry e{};
        e.name = name;
        e.type = EntryType::Vector4;
        e.ptr = value;
        e.speed = speed;
        e.minVal[0] = min; e.minVal[1] = min; e.minVal[2] = min; e.minVal[3] = min;
        e.maxVal[0] = max; e.maxVal[1] = max; e.maxVal[2] = max; e.maxVal[3] = max;
        GetInstance().AddEntry(e);
    }

    void ParamEditor::RegisterValue(const std::string& name, Transform* value, float speed, float min, float max) {
        BeginGroup(name, false);
        RegisterValue("scale", &value->scale, speed, min, max);
        RegisterValue("rotate", &value->rotate, speed, -180.0f, 180.0f);
        RegisterValue("translate", &value->translate, speed, -1000.0f, 1000.0f);
        EndGroup();
    }

    void ParamEditor::RegisterValue(const std::string& name, Transform2D* value, float speed, float min, float max) {
        BeginGroup(name, false);
        RegisterValue("scale", &value->scale, speed / 100.0f, min, max);
        RegisterValue("rotate", &value->rotate, speed / 100.0f, -180.0f, 180.0f);
        RegisterValue("translate", &value->translate, speed, -1000.0f, 1000.0f);
        EndGroup();
    }

    void ParamEditor::RegisterValue(const std::string& name, UVTransform* value, float speed, float min, float max) {
        BeginGroup(name, false);
        RegisterValue("scale", &value->scale, speed, min, max);
        RegisterValue("rotate", &value->rotate, speed, -180.0f, 180.0f);
        RegisterValue("translate", &value->translate, speed, -1000.0f, 1000.0f);
        EndGroup();
    }

    void ParamEditor::RegisterColor(const std::string& name, Vector3* value) {
        ParamEntry e{};
        e.name = name;
        e.type = EntryType::Color3;
        e.ptr = value;
        GetInstance().AddEntry(e);
    }

    void ParamEditor::RegisterColor(const std::string& name, Vector4* value) {
        ParamEntry e{};
        e.name = name;
        e.type = EntryType::Color4;
        e.ptr = value;
        GetInstance().AddEntry(e);
    }

    void ParamEditor::RegisterAnchor(const std::string& name, Anchor* value) {
        ParamEntry e{};
        e.name = name;
        e.type = EntryType::Anchor;
        e.ptr = value;
        GetInstance().AddEntry(e);
    }

    // --- インターフェースの転送 ---

    void ParamEditor::DrawImGuiWindow(const char* windowName) {
        GetInstance().DrawImGuiInternal(windowName);
    }

    void ParamEditor::Save() {
        auto& inst = GetInstance();
        std::string fullPath = inst.GetFullFilePath();
        if (std::filesystem::exists(fullPath)) {
            inst.showOverwriteModal_ = true;
        } else {
            inst.SaveToJsonInternal(fullPath);
        }
    }

    void ParamEditor::Load() {
        auto& inst = GetInstance();
        inst.LoadFromJsonInternal(inst.GetFullFilePath());
    }

    // --- 内部処理：ImGui描画 ---

    void ParamEditor::DrawImGuiInternal(const char* windowName) {
#ifdef _DEBUG
        ImGui::Begin(windowName);

        // --- フォルダパス・ファイル名の設定エリア ---
        ImGui::Text("保存・読込先");

        // 一時バッファの初期化（初回のみ）
        if (folderPathBuffer_[0] == '\0' && !folderPath_.empty()) {
            strcpy_s(folderPathBuffer_, sizeof(folderPathBuffer_), folderPath_.c_str());
        }

        // 1. フォルダ名入力欄
        bool isEnterPressed = ImGui::InputText(
            "フォルダパス",
            folderPathBuffer_,
            sizeof(folderPathBuffer_),
            ImGuiInputTextFlags_EnterReturnsTrue
        );

        // Enterが押されたか、フォーカスが外れた（他をクリックした）タイミングで確定
        if (isEnterPressed || ImGui::IsItemDeactivatedAfterEdit()) {
            SetFolderPath(folderPathBuffer_);
        }

        // 2. ファイル名（読み取り専用表示）
        ImGui::Text("ファイル名 : %s", kFileName.c_str());

        ImGui::Spacing();

        // --- 保存・読み込みボタン ---
        if (ImGui::Button("Save")) {
            SetFolderPath(folderPathBuffer_);
            Save();
        }
        ImGui::SameLine();
        if (ImGui::Button("Load")) {
            SetFolderPath(folderPathBuffer_);
            Load();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        DrawEntries(rootEntries_);

        // 上書き確認ポップアップ
        if (showOverwriteModal_) {
            ImGui::OpenPopup("ParamEditor 上書き確認");
            Logger::LogWarning("[ParamEditor] Overwrite check.");
            showOverwriteModal_ = false;
        }

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal("ParamEditor 上書き確認", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("該当フォルダには既に同名のファイルが存在します。上書きしますか？");
            ImGui::Text("フォルダパス : %s", folderPath_.c_str());
            ImGui::Separator();

            if (ImGui::Button("上書きする", ImVec2(120, 0))) {
                SaveToJsonInternal(GetFullFilePath());
                ImGui::CloseCurrentPopup();
            }
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if (ImGui::Button("キャンセル", ImVec2(120, 0))) {
                Logger::Log("[ParamEditor] Save cancelled.");
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::End();
#endif
    }

    void ParamEditor::DrawEntries(std::list<ParamEntry>& entries) {
#ifdef _DEBUG
        for (auto& e : entries) {
            ImGui::PushID(&e);

            switch (e.type) {
            case EntryType::Group: {
                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
                if (e.defaultOpen) {
                    flags |= ImGuiTreeNodeFlags_DefaultOpen;
                }
                bool open = ImGui::TreeNodeEx(e.name.c_str(), flags);
                if (open) {
                    DrawEntries(e.children);
                    ImGui::TreePop();
                }
                break;
            }
            case EntryType::Flag:
                if (e.ptr) ImGui::Checkbox(e.name.c_str(), static_cast<bool*>(e.ptr));
                break;
            case EntryType::Int32:
                if (e.ptr) ImGui::DragInt(e.name.c_str(), static_cast<int32_t*>(e.ptr), e.speed, e.minInt, e.maxInt);
                break;
            case EntryType::UInt32:
                if (e.ptr) {
                    int temp = static_cast<int>(*static_cast<uint32_t*>(e.ptr));
                    if (ImGui::DragInt(e.name.c_str(), &temp, e.speed, e.minInt, e.maxInt)) {
                        *static_cast<uint32_t*>(e.ptr) = static_cast<uint32_t>(std::max(0, temp));
                    }
                }
                break;
            case EntryType::Float:
                if (e.ptr) ImGui::DragFloat(e.name.c_str(), static_cast<float*>(e.ptr), e.speed, e.minVal[0], e.maxVal[0]);
                break;
            case EntryType::Vector2:
                if (e.ptr) {
                    auto* v = static_cast<Vector2*>(e.ptr);
                    ImGui::DragFloat2(e.name.c_str(), &v->x, e.speed, e.minVal[0], e.maxVal[0]);
                }
                break;
            case EntryType::Vector3:
                if (e.ptr) {
                    auto* v = static_cast<Vector3*>(e.ptr);
                    ImGui::DragFloat3(e.name.c_str(), &v->x, e.speed, e.minVal[0], e.maxVal[0]);
                }
                break;
            case EntryType::Vector4:
                if (e.ptr) {
                    auto* v = static_cast<Vector4*>(e.ptr);
                    ImGui::DragFloat4(e.name.c_str(), &v->x, e.speed, e.minVal[0], e.maxVal[0]);
                }
                break;
            case EntryType::Color3:
                if (e.ptr) {
                    auto* v = static_cast<Vector3*>(e.ptr);
                    ImGui::ColorEdit3(e.name.c_str(), &v->x);
                }
                break;
            case EntryType::Color4:
                if (e.ptr) {
                    auto* v = static_cast<Vector4*>(e.ptr);
                    ImGui::ColorEdit4(e.name.c_str(), &v->x);
                }
                break;
            case EntryType::Anchor:
                if (e.ptr) {
                    const char* items[] = {
                        "Center", "Top", "Bottom", "Left", "LeftTop", "LeftBottom", "Right", "RightTop", "RightBottom"
                    };
                    int currentItem = static_cast<int>(*static_cast<Anchor*>(e.ptr));
                    if (ImGui::Combo(e.name.c_str(), &currentItem, items, IM_ARRAYSIZE(items))) {
                        *static_cast<Anchor*>(e.ptr) = static_cast<Anchor>(currentItem);
                    }
                }
                break;
            }

            ImGui::PopID();
        }
#endif
    }

    // --- 内部処理：JSONセーブ ---

    void ParamEditor::SaveToJsonInternal(const std::string& filepath) {
        Logger::Log("[ParamEditor] Save started: " + filepath);

        namespace fs = std::filesystem;
        fs::path path(filepath);
        if (path.has_parent_path()) {
            try {
                fs::create_directories(path.parent_path());
            }
            catch (...) {
                Logger::LogError("[ParamEditor] Failed to create directory: " + path.parent_path().string());
                return;
            }
        }

        nlohmann::json j;
        SaveEntriesToJson(j, rootEntries_);

        std::ofstream file(filepath);
        if (file.is_open()) {
            file << j.dump(4);
            Logger::LogSuccess("[ParamEditor] Save Successed.");
        } else {
            Logger::LogWarning("[ParamEditor] Save failed: Could not open file " + filepath);
        }
    }

    void ParamEditor::SaveEntriesToJson(nlohmann::json& j, const std::list<ParamEntry>& entries) {
        for (const auto& e : entries) {
            if (e.type == EntryType::Group) {
                nlohmann::json childJson;
                SaveEntriesToJson(childJson, e.children);
                j[e.name] = childJson;
            } else if (e.ptr) {
                switch (e.type) {
                case EntryType::Flag:
                    j[e.name] = *static_cast<bool*>(e.ptr);
                    break;
                case EntryType::Int32:
                    j[e.name] = *static_cast<int32_t*>(e.ptr);
                    break;
                case EntryType::UInt32:
                    j[e.name] = *static_cast<uint32_t*>(e.ptr);
                    break;
                case EntryType::Float:
                    j[e.name] = *static_cast<float*>(e.ptr);
                    break;
                case EntryType::Vector2: {
                    auto* v = static_cast<Vector2*>(e.ptr);
                    j[e.name] = { v->x, v->y };
                    break;
                }
                case EntryType::Vector3:
                case EntryType::Color3: {
                    auto* v = static_cast<Vector3*>(e.ptr);
                    j[e.name] = { v->x, v->y, v->z };
                    break;
                }
                case EntryType::Vector4:
                case EntryType::Color4: {
                    auto* v = static_cast<Vector4*>(e.ptr);
                    j[e.name] = { v->x, v->y, v->z, v->w };
                    break;
                }
                case EntryType::Anchor: {
                    Anchor anchor = *static_cast<Anchor*>(e.ptr);
                    std::string str = "Center";
                    switch (anchor) {
                    case Anchor::Center:      str = "Center"; break;
                    case Anchor::Top:         str = "Top"; break;
                    case Anchor::Bottom:      str = "Bottom"; break;
                    case Anchor::Left:        str = "Left"; break;
                    case Anchor::LeftTop:     str = "LeftTop"; break;
                    case Anchor::LeftBottom:  str = "LeftBottom"; break;
                    case Anchor::Right:       str = "Right"; break;
                    case Anchor::RightTop:    str = "RightTop"; break;
                    case Anchor::RightBottom: str = "RightBottom"; break;
                    }
                    j[e.name] = str;
                    break;
                }
                default:
                    break;
                }
            }
        }
    }

    // --- 内部処理：JSONロード ---

    void ParamEditor::LoadFromJsonInternal(const std::string& filepath) {
        Logger::Log("[ParamEditor] Load started: " + filepath);

        std::ifstream file(filepath);
        if (!file.is_open()) {
            Logger::LogError("[ParamEditor] Load failed: Could not open file " + filepath);
            return;
        }

        file >> loadedJson_;
        isLoaded_ = true;

        LoadEntriesFromJson(loadedJson_, rootEntries_);

        Logger::LogSuccess("[ParamEditor] Load Successed.");
    }

    void ParamEditor::LoadEntriesFromJson(const nlohmann::json& j, std::list<ParamEntry>& entries) {
        for (auto& e : entries) {
            if (!j.contains(e.name)) continue;

            if (e.type == EntryType::Group) {
                if (j[e.name].is_object()) {
                    LoadEntriesFromJson(j[e.name], e.children);
                }
            } else if (e.ptr) {
                const auto& val = j[e.name];
                switch (e.type) {
                case EntryType::Flag:
                    if (val.is_boolean()) *static_cast<bool*>(e.ptr) = val.get<bool>();
                    break;
                case EntryType::Int32:
                    if (val.is_number_integer()) *static_cast<int32_t*>(e.ptr) = val.get<int32_t>();
                    break;
                case EntryType::UInt32:
                    if (val.is_number_unsigned()) *static_cast<uint32_t*>(e.ptr) = val.get<uint32_t>();
                    break;
                case EntryType::Float:
                    if (val.is_number()) *static_cast<float*>(e.ptr) = val.get<float>();
                    break;
                case EntryType::Vector2:
                    if (val.is_array() && val.size() == 2) {
                        auto* v = static_cast<Vector2*>(e.ptr);
                        v->x = val[0].get<float>();
                        v->y = val[1].get<float>();
                    }
                    break;
                case EntryType::Vector3:
                case EntryType::Color3:
                    if (val.is_array() && val.size() == 3) {
                        auto* v = static_cast<Vector3*>(e.ptr);
                        v->x = val[0].get<float>();
                        v->y = val[1].get<float>();
                        v->z = val[2].get<float>();
                    }
                    break;
                case EntryType::Vector4:
                case EntryType::Color4:
                    if (val.is_array() && val.size() == 4) {
                        auto* v = static_cast<Vector4*>(e.ptr);
                        v->x = val[0].get<float>();
                        v->y = val[1].get<float>();
                        v->z = val[2].get<float>();
                        v->w = val[3].get<float>();
                    }
                    break;
                case EntryType::Anchor:
                    if (val.is_string()) {
                        std::string str = val.get<std::string>();
                        Anchor anchor = Anchor::Center;
                        if (str == "Center")           anchor = Anchor::Center;
                        else if (str == "Top")         anchor = Anchor::Top;
                        else if (str == "Bottom")      anchor = Anchor::Bottom;
                        else if (str == "Left")        anchor = Anchor::Left;
                        else if (str == "LeftTop")     anchor = Anchor::LeftTop;
                        else if (str == "LeftBottom")  anchor = Anchor::LeftBottom;
                        else if (str == "Right")       anchor = Anchor::Right;
                        else if (str == "RightTop")    anchor = Anchor::RightTop;
                        else if (str == "RightBottom") anchor = Anchor::RightBottom;

                        *static_cast<Anchor*>(e.ptr) = anchor;
                    }
                    break;
                default:
                    break;
                }
            }
        }
    }
}