#include "LightManager.h"
#include "../Base/DirectXCommon.h"
#include "../Base/Logger.h"

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
}