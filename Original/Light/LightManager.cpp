#include "LightManager.h"
#include "../Base/DirectXCommon.h"
#include "../Base/Logger.h"
#include <string>
#include "imgui.h"

namespace RyoEngine {

    LightManager* LightManager::GetInstance() {
        static LightManager instance;
        return &instance;
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

    void LightManager::Save(const std::string& filePath) {
        Logger::Log("[LightManager] Save started: " + filePath);

        namespace fs = std::filesystem;

        // ファイルパスから親ディレクトリのパスを抽出し、存在しない場合は自動で作成する
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

        // アンビエントライトの保存
        root["ambient"] = *instance->ambientData_;

        // ライト一覧の保存
        nlohmann::json lightsArray = nlohmann::json::array();
        for (const auto& light : instance->lights_) {
            lightsArray.push_back(light);
        }
        root["lights"] = lightsArray;

        // ファイルへ書き込み
        std::ofstream file(filePath);
        if (file.is_open()) {
            file << root.dump(4); // インデント付きで綺麗に出力
            Logger::LogSuccess("[LightManager] Save Successed.");
        } else {
            Logger::LogWarning("[LightManager] Save failed: Could not open file " + filePath);
        }
    }

    void LightManager::Load(const std::string& filePath) {
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
        // データのクリア
        instance->lights_.clear();

        // アンビエントライトの読み込み
        if (root.contains("ambient")) {
            *instance->ambientData_ = root["ambient"].get<AmbientLight>();
        }

        // ライト一覧の読み込み
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

        // --- 保存・読み込みボタン ---
        if (ImGui::Button("Save")) {
            Save();
        }
        ImGui::SameLine();
        if (ImGui::Button("Load")) {
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

        ImGui::End();
#endif
        // RyoEngineで呼び出してるのはDrawImguiなのでこのUpdateは残す
        Update();
    }
}