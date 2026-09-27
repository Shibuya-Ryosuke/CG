#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <cstdint>
#include "Light.h"
#include "../Math/Math.h"

namespace RyoEngine {

    // シーン内で同時に扱えるライトの最大数（64灯）
    constexpr uint32_t kMaxLightCount = 64;

    class LightManager {
    public:
        // シングルトンインスタンスの取得
        static LightManager* GetInstance();

        static void Initialize();
        static void Finalize();
        static void Update();

        // --- JSON 保存・読み込み ---
        static void Save(const std::string& filePath = "Resources/ApplicationResources/Json/Manager/lightManager.json");
        static void Load(const std::string& filePath = "Resources/ApplicationResources/Json/Manager/lightManager.json");

        // --- ライト操作（コードからの追加・削除） ---
        static int AddLight(LightType type);
        static int AddLight(const Light& light);
        static void RemoveLight(int index);
        static void ClearLights();

        // --- ライトの個別Getter / Setter（ID指定） ---
        static size_t GetLightCount() { return GetInstance()->lights_.size(); }

        //static Light& GetLight(int index) { return GetInstance()->lights_[index]; }
        static const Light& GetLight(int index) { return GetInstance()->lights_[index]; }

        static void SetLightType(int index, LightType type) {
            if (index >= 0 && index < static_cast<int>(GetInstance()->lights_.size())) {
                GetInstance()->lights_[index].type = type;
            }
        }
        static LightType GetLightType(int index) { return GetInstance()->lights_[index].type; }

        static void SetLightColor(int index, const Vector4& color) {
            if (index >= 0 && index < static_cast<int>(GetInstance()->lights_.size())) {
                GetInstance()->lights_[index].color = color;
            }
        }
        static const Vector4& GetLightColor(int index) { return GetInstance()->lights_[index].color; }

        static void SetLightIntensity(int index, float intensity) {
            if (index >= 0 && index < static_cast<int>(GetInstance()->lights_.size())) {
                GetInstance()->lights_[index].intensity = intensity;
            }
        }
        static float GetLightIntensity(int index) { return GetInstance()->lights_[index].intensity; }

        static void SetLightDirection(int index, const Vector3& direction) {
            if (index >= 0 && index < static_cast<int>(GetInstance()->lights_.size())) {
                GetInstance()->lights_[index].direction = Normalize(direction);
            }
        }
        static const Vector3& GetLightDirection(int index) { return GetInstance()->lights_[index].direction; }

        static void SetLightPosition(int index, const Vector3& position) {
            if (index >= 0 && index < static_cast<int>(GetInstance()->lights_.size())) {
                GetInstance()->lights_[index].position = position;
            }
        }
        static const Vector3& GetLightPosition(int index) { return GetInstance()->lights_[index].position; }

        static void SetLightRange(int index, float range) {
            if (index >= 0 && index < static_cast<int>(GetInstance()->lights_.size())) {
                GetInstance()->lights_[index].range = range;
            }
        }
        static float GetLightRange(int index) { return GetInstance()->lights_[index].range; }

        static void SetLightSpotAngle(int index, float spotAngle) {
            if (index >= 0 && index < static_cast<int>(GetInstance()->lights_.size())) {
                GetInstance()->lights_[index].spotAngle = spotAngle;
            }
        }
        static float GetLightSpotAngle(int index) { return GetInstance()->lights_[index].spotAngle; }

        static void SetLightSpotFalloff(int index, float spotFalloff) {
            if (index >= 0 && index < static_cast<int>(GetInstance()->lights_.size())) {
                GetInstance()->lights_[index].spotFalloff = spotFalloff;
            }
        }
        static float GetLightSpotFalloff(int index) { return GetInstance()->lights_[index].spotFalloff; }


        // --- アンビエントライト ---
        static void SetAmbientLight(const AmbientLight& ambient) {
            GetInstance()->ambientData_->color = ambient.color;
            GetInstance()->ambientData_->intensity = ambient.intensity;
        }
        static void SetAmbientColor(const Vector4& color) { GetInstance()->ambientData_->color = color; }
        static void SetAmbientIntensity(float intensity) { GetInstance()->ambientData_->intensity = intensity; }

        static const AmbientLight& GetAmbientLight() { return *GetInstance()->ambientData_; }
        static const Vector4& GetAmbientColor() { return GetInstance()->ambientData_->color; }
        static float GetAmbientIntensity() { return GetInstance()->ambientData_->intensity; }


        // --- GPUリソース取得 ---
        static ID3D12Resource* GetLightResource() { return GetInstance()->lightResource_.Get(); }
        static ID3D12Resource* GetLightCountResource() { return GetInstance()->lightCountResource_.Get(); }
        static ID3D12Resource* GetAmbientResource() { return GetInstance()->ambientResource_.Get(); }

        static D3D12_GPU_VIRTUAL_ADDRESS GetLightGPUVirtualAddress() { return GetInstance()->lightResource_->GetGPUVirtualAddress(); }
        static D3D12_GPU_VIRTUAL_ADDRESS GetLightCountGPUVirtualAddress() { return GetInstance()->lightCountResource_->GetGPUVirtualAddress(); }
        static D3D12_GPU_VIRTUAL_ADDRESS GetAmbientGPUVirtualAddress() { return GetInstance()->ambientResource_->GetGPUVirtualAddress(); }

        // --- 後方互換API ---
        static DirectionalLight GetDirectionalLight() {
            const Light& l = GetInstance()->lights_[0];
            return DirectionalLight{ l.color, l.direction, l.intensity };
        }
        static const Vector4& GetColor() { return GetInstance()->lights_[0].color; }
        static const Vector3& GetDirection() { return GetInstance()->lights_[0].direction; }
        static float GetIntensity() { return GetInstance()->lights_[0].intensity; }

        static void SetDirectionalLight(const DirectionalLight& light) {
            auto* inst = GetInstance();
            inst->lights_[0].type = LightType::Directional;
            inst->lights_[0].color = light.color;
            inst->lights_[0].direction = Normalize(light.direction);
            inst->lights_[0].intensity = light.intensity;
            Update();
        }
        static void SetColor(const Vector4& color) { GetInstance()->lights_[0].color = color; Update(); }
        static void SetDirection(const Vector3& direction) { GetInstance()->lights_[0].direction = Normalize(direction); Update(); }
        static void SetIntensity(float intensity) { GetInstance()->lights_[0].intensity = intensity; Update(); }

        static void DrawImGui();

    private:
        LightManager() = default;
        ~LightManager() = default;
        LightManager(const LightManager&) = delete;
        LightManager& operator=(const LightManager&) = delete;

        // 非静的メンバ変数（インスタンスごとに保持される実データ）
        std::vector<Light> lights_;

        Microsoft::WRL::ComPtr<ID3D12Resource> lightResource_;
        Light* lightMappedData_ = nullptr;

        Microsoft::WRL::ComPtr<ID3D12Resource> lightCountResource_;
        LightCountData* lightCountData_ = nullptr;

        Microsoft::WRL::ComPtr<ID3D12Resource> ambientResource_;
        AmbientLight* ambientData_ = nullptr;
    };
}