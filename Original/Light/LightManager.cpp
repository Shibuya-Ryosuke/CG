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

        // Light配列用バッファ（kMaxLightCount件ぶん固定確保）
        lightResource_ = DirectXCommon::CreateBufferResource(device, sizeof(Light) * kMaxLightCount);
        lightResource_->Map(0, nullptr, reinterpret_cast<void**>(&lightMappedData_));

        // ライト数用バッファ
        lightCountResource_ = DirectXCommon::CreateBufferResource(device, sizeof(LightCountData));
        lightCountResource_->Map(0, nullptr, reinterpret_cast<void**>(&lightCountData_));
        lightCountData_->lightCount = 0;

        // アンビエントライト用バッファ
        ambientResource_ = DirectXCommon::CreateBufferResource(device, sizeof(AmbientLight));
        ambientResource_->Map(0, nullptr, reinterpret_cast<void**>(&ambientData_));

        // デフォルト値：以前のDirectionalLight1灯と同じ設定を初期ライトとして登録
        Light defaultLight{};
        defaultLight.type = LightType::Directional;
        defaultLight.color = { 1.0f, 1.0f, 1.0f, 1.0f };
        defaultLight.direction = Normalize(Vector3{ 0.0f, -1.0f, 0.0f });
        defaultLight.intensity = 1.0f;
        AddLight(defaultLight);

        // アンビエントはデフォルトで弱めの白を底上げしておく（0にすると従来と全く同じ見た目になる）
        ambientData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
        ambientData_->intensity = 0.1f;

        // ここまでの変更をGPUバッファへ反映
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
        // CPU側のlights_配列の内容をGPUバッファへコピーする
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
        if (ImGui::CollapsingHeader("Ambient Light", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::ColorEdit3("Ambient Color", &ambientData_->color.x);
            ImGui::SliderFloat("Ambient Intensity", &ambientData_->intensity, 0.0f, 2.0f);
        }

        ImGui::Separator();

        // --- ライト一覧 ---
        ImGui::Text("Lights: %zu / %u", lights_.size(), kMaxLightCount);

        if (ImGui::Button("Add Directional Light") && lights_.size() < kMaxLightCount) {
            Light newLight{};
            newLight.type = LightType::Directional;
            newLight.color = { 1.0f, 1.0f, 1.0f, 1.0f };
            newLight.direction = Normalize(Vector3{ 0.0f, -1.0f, 0.0f });
            newLight.intensity = 1.0f;
            AddLight(newLight);
        }

        int removeIndex = -1;
        const char* typeNames[] = { "Directional", "Point", "Spot", "Area" };

        for (size_t i = 0; i < lights_.size(); ++i) {
            ImGui::PushID(static_cast<int>(i));
            Light& light = lights_[i];

            std::string header = "Light " + std::to_string(i);
            if (ImGui::CollapsingHeader(header.c_str())) {
                int typeIndex = static_cast<int>(light.type);
                if (ImGui::Combo("Type", &typeIndex, typeNames, IM_ARRAYSIZE(typeNames))) {
                    light.type = static_cast<LightType>(typeIndex);
                }

                ImGui::ColorEdit3("Color", &light.color.x);
                ImGui::SliderFloat("Intensity", &light.intensity, 0.0f, 5.0f);

                // NOTE: Point/Spot/Areaはシェーダー側の計算が未実装のため、
                //       ここでいじれても見た目には反映されない(現状はDirectionalのみ反映される)
                if (light.type == LightType::Directional || light.type == LightType::Spot) {
                    if (ImGui::SliderFloat3("Direction", &light.direction.x, -1.0f, 1.0f)) {
                        light.direction = Normalize(light.direction);
                    }
                }
                if (light.type == LightType::Point || light.type == LightType::Spot || light.type == LightType::Area) {
                    ImGui::DragFloat3("Position", &light.position.x, 0.1f);
                    ImGui::SliderFloat("Range", &light.range, 0.0f, 50.0f);
                }
                if (light.type == LightType::Spot) {
                    ImGui::SliderFloat("Spot Angle (cos)", &light.spotAngle, 0.0f, 1.0f);
                    ImGui::SliderFloat("Spot Falloff", &light.spotFalloff, 0.0f, 1.0f);
                }

                if (ImGui::Button("Remove")) {
                    removeIndex = static_cast<int>(i);
                }
            }
            ImGui::PopID();
        }

        if (removeIndex >= 0) {
            RemoveLight(removeIndex);
        }

        ImGui::End();

        // ImGuiで編集した内容をGPUバッファへ反映
        Update();
    }
}