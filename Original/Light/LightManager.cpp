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

        // DirectionalLightリソース作成 (シーンで1つだけ)
        lightResource_ = DirectXCommon::CreateBufferResource(device, sizeof(DirectionalLight));
        lightResource_->Map(0, nullptr, reinterpret_cast<void**>(&lightData_));

        // デフォルト値（白い光が斜め下に向いている状態。以前Model::CreateDirectionalLightにあった値と同じ）
        lightData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
        lightData_->direction = { 0.0f, -1.0f, 0.0f };
        lightData_->intensity = 1.0f;

        Logger::LogSuccess("LightManager : Initialized\n");
    }

    void LightManager::Finalize() {
        Logger::Log("LightManager : Finalizing...\n");
        lightResource_.Reset();
        lightData_ = nullptr;
        Logger::LogSuccess("LightManager : Finalized\n");
    }
}
