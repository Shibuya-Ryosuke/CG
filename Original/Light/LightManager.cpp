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
        Logger::Log("LightManager : Initializing...\n");
        auto device = DirectXCommon::GetInstance()->GetDevice();

        lightResource_ = DirectXCommon::CreateBufferResource(device, sizeof(Light) * kMaxLightCount);
        lightResource_->Map(0, nullptr, reinterpret_cast<void**>(&lightMappedData_));

        lightCountResource_ = DirectXCommon::CreateBufferResource(device, sizeof(LightCountData));
        lightCountResource_->Map(0, nullptr, reinterpret_cast<void**>(&lightCountData_));
        lightCountData_->lightCount = 0;

        ambientResource_ = DirectXCommon::CreateBufferResource(device, sizeof(AmbientLight));
        ambientResource_->Map(0, nullptr, reinterpret_cast<void**>(&ambientData_));

        // デフォルトライトの登録
        Light defaultLight{};
        defaultLight.type = LightType::Directional;
        defaultLight.color = { 1.0f, 1.0f, 1.0f, 1.0f };
        defaultLight.direction = Normalize(Vector3{ 0.0f, -1.0f, 0.0f });
        defaultLight.intensity = 1.0f;
        AddLight(defaultLight);

        ambientData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
        ambientData_->intensity = 0.1f;

        Update();

        Logger::LogSuccess("LightManager : Initialized\n");
    }

    void LightManager::Finalize() {
        Logger::Log("LightManager : Finalizing...\n");
        lightResource_.Reset();
        lightMappedData_ = nullptr;
        lightCountResource_.Reset();
        lightCountData_ = nullptr;
        ambientResource_.Reset();
        ambientData_ = nullptr;
        lights_.clear();
        Logger::LogSuccess("LightManager : Finalized\n");
    }

    void LightManager::Update() {
        uint32_t count = static_cast<uint32_t>(lights_.size());
        if (count > kMaxLightCount) {
            Logger::Log("LightManager : Light count exceeds kMaxLightCount, truncating.\n");
            count = kMaxLightCount;
        }

        for (uint32_t i = 0; i < count; ++i) {
            lightMappedData_[i] = lights_[i];
        }
        lightCountData_->lightCount = count;
    }

    int LightManager::AddLight(LightType type) {
        if (lights_.size() >= kMaxLightCount) {
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

        lights_.push_back(newLight);
        return static_cast<int>(lights_.size() - 1);
    }

    int LightManager::AddLight(const Light& light) {
        if (lights_.size() >= kMaxLightCount) {
            Logger::Log("LightManager : Cannot add light, kMaxLightCount reached.\n");
            return -1;
        }
        lights_.push_back(light);
        return static_cast<int>(lights_.size() - 1);
    }

    void LightManager::RemoveLight(int index) {
        if (index < 0 || index >= static_cast<int>(lights_.size())) {
            Logger::Log("LightManager : RemoveLight index out of range.\n");
            return;
        }
        lights_.erase(lights_.begin() + index);
    }

    void LightManager::ClearLights() {
        lights_.clear();
    }

    void LightManager::DrawImGui() {
        ImGui::Begin("LightManager");

        // --- アンビエントライト ---
        if (ImGui::CollapsingHeader("環境光", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::ColorEdit3("環境光の色", &ambientData_->color.x);
            ImGui::SliderFloat("環境光の強さ", &ambientData_->intensity, 0.0f, 2.0f);
        }

        ImGui::Separator();

        // --- ライト一覧 ---
        ImGui::Text("ライト一覧: %zu / %u", lights_.size(), kMaxLightCount);

        if (ImGui::Button("ライトの追加") && lights_.size() < kMaxLightCount) {
            AddLight(LightType::Directional);
        }

        int removeIndex = -1;
        const char* typeNames[] = { "平行光源（サン）", "ポイント", "スポット", "エリア（未実装）" };

        for (size_t i = 0; i < lights_.size(); ++i) {
            ImGui::PushID(static_cast<int>(i));
            Light& light = lights_[i];

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

        Update();
    }
}