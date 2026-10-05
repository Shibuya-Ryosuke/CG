#include "LightManager.h"
#include "../Base/DirectXCommon.h"
#include "../Base/Logger.h"
#include <string>
#include <filesystem>
#include <fstream>
#include "imgui.h"

namespace RyoEngine {

    LightManager* LightManager::GetInstance() {
        static LightManager instance;
        return &instance;
    }

    void LightManager::SetFolderPath(const std::string& folderPath) {
        GetInstance()->folderPath_ = folderPath;
    }

    const std::string& LightManager::GetFolderPath() {
        return GetInstance()->folderPath_;
    }

    std::string LightManager::GetFullFilePath() const {
        namespace fs = std::filesystem;
        fs::path p(folderPath_);
        p /= kFileName;
        return p.string();
    }

    void LightManager::Initialize() {
        auto instance = GetInstance();

        Logger::Log("LightManager : Initializing...\n");
        auto device = DirectXCommon::GetInstance()->GetDevice();

        instance->lightResource_ = DirectXCommon::CreateBufferResource(device, sizeof(Light) * kMaxLightCount);
        instance->lightResource_->Map(0, nullptr, reinterpret_cast<void**>(&instance->lightMappedData_));

        instance->lightCountResource_ = DirectXCommon::CreateBufferResource(device, sizeof(LightCountData));
        instance->lightCountResource_->Map(0, nullptr, reinterpret_cast<void**>(&instance->lightCountData_));
        instance->lightCountData_->lightCount = 0;

        instance->ambientResource_ = DirectXCommon::CreateBufferResource(device, sizeof(AmbientLight));
        instance->ambientResource_->Map(0, nullptr, reinterpret_cast<void**>(&instance->ambientData_));

        // デフォルトライトの登録
        Light defaultLight{};
        defaultLight.type = LightType::Directional;
        defaultLight.color = { 1.0f, 1.0f, 1.0f, 1.0f };
        defaultLight.direction = Normalize(Vector3{ 0.0f, -1.0f, 0.0f });
        defaultLight.intensity = 1.0f;
        AddLight(defaultLight);

        instance->ambientData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
        instance->ambientData_->intensity = 0.1f;

        Update();

        Logger::LogSuccess("LightManager : Initialized\n");
    }

    void LightManager::Finalize() {
        auto instance = GetInstance();

        Logger::Log("LightManager : Finalizing...\n");
        instance->lightResource_.Reset();
        instance->lightMappedData_ = nullptr;
        instance->lightCountResource_.Reset();
        instance->lightCountData_ = nullptr;
        instance->ambientResource_.Reset();
        instance->ambientData_ = nullptr;
        instance->lights_.clear();
        Logger::LogSuccess("LightManager : Finalized\n");
    }

    void LightManager::Update() {
        auto instance = GetInstance();

        uint32_t count = static_cast<uint32_t>(instance->lights_.size());
        if (count > kMaxLightCount) {
            Logger::Log("LightManager : Light count exceeds kMaxLightCount, truncating.\n");
            count = kMaxLightCount;
        }

        for (uint32_t i = 0; i < count; ++i) {
            instance->lightMappedData_[i] = instance->lights_[i];
        }
        instance->lightCountData_->lightCount = count;
    }

    void LightManager::Save() {
        auto instance = GetInstance();
        std::string fullPath = instance->GetFullFilePath();
        if (std::filesystem::exists(fullPath)) {
            instance->showOverwriteModal_ = true;
        } else {
            instance->SaveToFileInternal(fullPath);
        }
    }

    void LightManager::SaveToFileInternal(const std::string& filePath) {
        Logger::Log("[LightManager] Save started: " + filePath);

        namespace fs = std::filesystem;
        fs::path path(filePath);
        if (path.has_parent_path()) {
            try {
                fs::create_directories(path.parent_path());
            }
            catch (...) {
                Logger::LogError("[LightManager] Failed to create directory: " + path.parent_path().string());
                return;
            }
        }

        auto instance = GetInstance();
        nlohmann::json root;

        root["ambient"] = *instance->ambientData_;

        nlohmann::json lightsArray = nlohmann::json::array();
        for (const auto& light : instance->lights_) {
            lightsArray.push_back(light);
        }
        root["lights"] = lightsArray;

        std::ofstream file(filePath);
        if (file.is_open()) {
            file << root.dump(4);
            Logger::LogSuccess("[LightManager] Save Successed.");
        } else {
            Logger::LogWarning("[LightManager] Save failed: Could not open file " + filePath);
        }
    }

    void LightManager::Load() {
        auto instance = GetInstance();
        instance->LoadFromFileInternal(instance->GetFullFilePath());
    }

    void LightManager::LoadFromFileInternal(const std::string& filePath) {
        Logger::Log("[LightManager] Load started: " + filePath);

        if (!std::filesystem::exists(filePath)) {
            Logger::LogError("[LightManager] Load failed: Could not open file " + filePath);
            return;
        }

        std::ifstream file(filePath);
        if (!file.is_open()) {
            Logger::LogError("[LightManager] Load failed: Could not open file " + filePath);
            return;
        }

        nlohmann::json root;
        file >> root;
        file.close();

        auto instance = GetInstance();
        instance->lights_.clear();

        if (root.contains("ambient")) {
            *instance->ambientData_ = root["ambient"].get<AmbientLight>();
        }

        if (root.contains("lights") && root["lights"].is_array()) {
            for (const auto& jLight : root["lights"]) {
                if (instance->lights_.size() >= kMaxLightCount) {
                    break;
                }
                Light light = jLight.get<Light>();
                instance->lights_.push_back(light);
            }
        }

        Update();
        Logger::LogSuccess("[LightManager] Load Successed.");
    }

    int LightManager::AddLight(LightType type) {
        auto instance = GetInstance();

        if (instance->lights_.size() >= kMaxLightCount) {
            Logger::Log("LightManager : Cannot add light, kMaxLightCount reached.\n");
            return -1;
        }
        Light newLight{};
        newLight.type = type;
        newLight.color = { 1.0f, 1.0f, 1.0f, 1.0f };
        newLight.intensity = 1.0f;
        newLight.direction = Normalize(Vector3{ 0.0f, -1.0f, 0.0f });
        newLight.position = { 0.0f, 0.0f, 0.0f };
        newLight.range = 10.0f;
        newLight.spotAngle = 0.5f;
        newLight.spotFalloff = 0.1f;

        instance->lights_.push_back(newLight);
        return static_cast<int>(instance->lights_.size() - 1);
    }

    int LightManager::AddLight(const Light& light) {
        auto instance = GetInstance();

        if (instance->lights_.size() >= kMaxLightCount) {
            Logger::Log("LightManager : Cannot add light, kMaxLightCount reached.\n");
            return -1;
        }
        instance->lights_.push_back(light);
        return static_cast<int>(instance->lights_.size() - 1);
    }

    void LightManager::RemoveLight(int index) {
        auto instance = GetInstance();

        if (index < 0 || index >= static_cast<int>(instance->lights_.size())) {
            Logger::Log("LightManager : RemoveLight index out of range.\n");
            return;
        }
        instance->lights_.erase(instance->lights_.begin() + index);
    }

    void LightManager::ClearLights() {
        auto instance = GetInstance();
        instance->lights_.clear();
    }

    void LightManager::DrawImGui() {
#ifdef _DEBUG
        auto instance = GetInstance();

        ImGui::Begin("LightManager");

        // --- フォルダパス・ファイル名の設定エリア ---
        ImGui::Text("保存・読込先");

        // 一時バッファの初期化（初回のみ、またはSetFolderPath直後など）
        if (instance->folderPathBuffer_[0] == '\0' && !instance->folderPath_.empty()) {
            strcpy_s(instance->folderPathBuffer_, sizeof(instance->folderPathBuffer_), instance->folderPath_.c_str());
        }

        // 1. フォルダ名入力欄
     // Enterを押した際に ImGui::InputText が true を返す
        bool isEnterPressed = ImGui::InputText(
            "フォルダパス",
            instance->folderPathBuffer_,
            sizeof(instance->folderPathBuffer_),
            ImGuiInputTextFlags_EnterReturnsTrue
        );

        // Enterが押されたか、または入力後にフォーカスが外れた（別場所をクリックした）タイミングで確定
        if (isEnterPressed || ImGui::IsItemDeactivatedAfterEdit()) {
            SetFolderPath(instance->folderPathBuffer_);
        }

        // 2. ファイル名（読み取り専用表示）
        ImGui::Text("ファイル名 : %s", kFileName.c_str());

        ImGui::Spacing();

        // --- 保存・読み込みボタン ---
        if (ImGui::Button("Save")) {
            // 保存直前に入力バッファの内容を最終反映させておく
            SetFolderPath(instance->folderPathBuffer_);
            Save();
        }
        ImGui::SameLine();
        if (ImGui::Button("Load")) {
            // 読み込み直前に入力バッファの内容を最終反映させておく
            SetFolderPath(instance->folderPathBuffer_);
            Load();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // --- アンビエントライト ---
        if (ImGui::CollapsingHeader("環境光", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::ColorEdit3("環境光の色", &instance->ambientData_->color.x);
            ImGui::SliderFloat("環境光の強さ", &instance->ambientData_->intensity, 0.0f, 2.0f);
        }

        ImGui::Separator();

        // --- ライト一覧 ---
        ImGui::Text("ライト一覧: %zu / %u", instance->lights_.size(), kMaxLightCount);

        if (ImGui::Button("ライトの追加") && instance->lights_.size() < kMaxLightCount) {
            AddLight(LightType::Directional);
        }

        int removeIndex = -1;
        const char* typeNames[] = { "平行光源（サン）", "ポイント", "スポット", "エリア（未実装）" };

        for (size_t i = 0; i < instance->lights_.size(); ++i) {
            ImGui::PushID(static_cast<int>(i));
            Light& light = instance->lights_[i];

            std::string header = "ライト " + std::to_string(i);
            if (ImGui::CollapsingHeader(header.c_str())) {
                int typeIndex = static_cast<int>(light.type);
                if (ImGui::Combo("種類", &typeIndex, typeNames, IM_ARRAYSIZE(typeNames))) {
                    light.type = static_cast<LightType>(typeIndex);
                }

                ImGui::ColorEdit3("色", &light.color.x);
                ImGui::SliderFloat("強さ", &light.intensity, 0.0f, 10.0f);

                if (light.type == LightType::Directional || light.type == LightType::Spot) {
                    if (ImGui::SliderFloat3("向き", &light.direction.x, -1.0f, 1.0f)) {
                        light.direction = Normalize(light.direction);
                    }
                }
                if (light.type == LightType::Point || light.type == LightType::Spot || light.type == LightType::Area) {
                    ImGui::DragFloat3("座標", &light.position.x, 0.1f);
                    ImGui::SliderFloat("範囲", &light.range, 0.0f, 100.0f);
                }
                if (light.type == LightType::Spot) {
                    ImGui::SliderFloat("角度", &light.spotAngle, 0.0f, 1.0f);
                    ImGui::SliderFloat("減衰", &light.spotFalloff, 0.0f, 1.0f);
                }

                if (ImGui::Button("削除")) {
                    removeIndex = static_cast<int>(i);
                }
            }
            ImGui::PopID();
        }

        if (removeIndex >= 0) {
            RemoveLight(removeIndex);
        }

        // 上書き確認ポップアップ
        if (instance->showOverwriteModal_) {
            ImGui::OpenPopup("LightManager 上書き確認");
            Logger::LogWarning("[LightManager] Overwrite check.");
            instance->showOverwriteModal_ = false;
        }

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

        if (ImGui::BeginPopupModal("LightManager 上書き確認", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("該当フォルダには既に同名のファイルが存在します。上書きしますか？");
            ImGui::Text("フォルダパス : %s", instance->folderPath_.c_str());
            ImGui::Separator();

            if (ImGui::Button("上書きする", ImVec2(120, 0))) {
                instance->SaveToFileInternal(instance->GetFullFilePath());
                ImGui::CloseCurrentPopup();
            }
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if (ImGui::Button("キャンセル", ImVec2(120, 0))) {
                Logger::Log("[LightManager] Save cancelled.");
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::End();
#endif
        Update();
    }
}