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
        static LightManager* GetInstance();

        void Initialize();
        void Finalize();

        void Update();

        // --- ライト操作（コードからの追加・削除） ---
        int AddLight(LightType type);
        int AddLight(const Light& light);
        void RemoveLight(int index);
        void ClearLights();

        // --- ライトの個別Getter / Setter（ID指定・インライン実装） ---
        size_t GetLightCount() const { return lights_.size(); }

        Light& GetLight(int index) { return lights_[index]; }
        const Light& GetLight(int index) const { return lights_[index]; }

        void SetLightType(int index, LightType type) {
            if (index >= 0 && index < static_cast<int>(lights_.size())) {
                lights_[index].type = type;
            }
        }
        LightType GetLightType(int index) const { return lights_[index].type; }

        void SetLightColor(int index, const Vector4& color) {
            if (index >= 0 && index < static_cast<int>(lights_.size())) {
                lights_[index].color = color;
            }
        }
        const Vector4& GetLightColor(int index) const { return lights_[index].color; }

        void SetLightIntensity(int index, float intensity) {
            if (index >= 0 && index < static_cast<int>(lights_.size())) {
                lights_[index].intensity = intensity;
            }
        }
        float GetLightIntensity(int index) const { return lights_[index].intensity; }

        void SetLightDirection(int index, const Vector3& direction) {
            if (index >= 0 && index < static_cast<int>(lights_.size())) {
                lights_[index].direction = Normalize(direction);
            }
        }
        const Vector3& GetLightDirection(int index) const { return lights_[index].direction; }

        void SetLightPosition(int index, const Vector3& position) {
            if (index >= 0 && index < static_cast<int>(lights_.size())) {
                lights_[index].position = position;
            }
        }
        const Vector3& GetLightPosition(int index) const { return lights_[index].position; }

        void SetLightRange(int index, float range) {
            if (index >= 0 && index < static_cast<int>(lights_.size())) {
                lights_[index].range = range;
            }
        }
        float GetLightRange(int index) const { return lights_[index].range; }

        void SetLightSpotAngle(int index, float spotAngle) {
            if (index >= 0 && index < static_cast<int>(lights_.size())) {
                lights_[index].spotAngle = spotAngle;
            }
        }
        float GetLightSpotAngle(int index) const { return lights_[index].spotAngle; }

        void SetLightSpotFalloff(int index, float spotFalloff) {
            if (index >= 0 && index < static_cast<int>(lights_.size())) {
                lights_[index].spotFalloff = spotFalloff;
            }
        }
        float GetLightSpotFalloff(int index) const { return lights_[index].spotFalloff; }


        // --- アンビエントライト ---
        void SetAmbientLight(const AmbientLight& ambient) {
            ambientData_->color = ambient.color;
            ambientData_->intensity = ambient.intensity;
        }
        void SetAmbientColor(const Vector4& color) { ambientData_->color = color; }
        void SetAmbientIntensity(float intensity) { ambientData_->intensity = intensity; }

        const AmbientLight& GetAmbientLight() const { return *ambientData_; }
        const Vector4& GetAmbientColor() const { return ambientData_->color; }
        float GetAmbientIntensity() const { return ambientData_->intensity; }


        // --- GPUリソース取得 ---
        ID3D12Resource* GetLightResource() const { return lightResource_.Get(); }
        ID3D12Resource* GetLightCountResource() const { return lightCountResource_.Get(); }
        ID3D12Resource* GetAmbientResource() const { return ambientResource_.Get(); }

        D3D12_GPU_VIRTUAL_ADDRESS GetLightGPUVirtualAddress() const { return lightResource_->GetGPUVirtualAddress(); }
        D3D12_GPU_VIRTUAL_ADDRESS GetLightCountGPUVirtualAddress() const { return lightCountResource_->GetGPUVirtualAddress(); }
        D3D12_GPU_VIRTUAL_ADDRESS GetAmbientGPUVirtualAddress() const { return ambientResource_->GetGPUVirtualAddress(); }

        // --- 後方互換API ---
        DirectionalLight GetDirectionalLight() const {
            const Light& l = lights_[0];
            return DirectionalLight{ l.color, l.direction, l.intensity };
        }
        const Vector4& GetColor() const { return lights_[0].color; }
        const Vector3& GetDirection() const { return lights_[0].direction; }
        float GetIntensity() const { return lights_[0].intensity; }

        void SetDirectionalLight(const DirectionalLight& light) {
            lights_[0].type = LightType::Directional;
            lights_[0].color = light.color;
            lights_[0].direction = Normalize(light.direction);
            lights_[0].intensity = light.intensity;
            Update();
        }
        void SetColor(const Vector4& color) { lights_[0].color = color; Update(); }
        void SetDirection(const Vector3& direction) { lights_[0].direction = Normalize(direction); Update(); }
        void SetIntensity(float intensity) { lights_[0].intensity = intensity; Update(); }

        void DrawImGui();

    private:
        LightManager() = default;
        ~LightManager() = default;
        LightManager(const LightManager&) = delete;
        LightManager& operator=(const LightManager&) = delete;

        std::vector<Light> lights_;

        Microsoft::WRL::ComPtr<ID3D12Resource> lightResource_;
        Light* lightMappedData_ = nullptr;

        Microsoft::WRL::ComPtr<ID3D12Resource> lightCountResource_;
        LightCountData* lightCountData_ = nullptr;

        Microsoft::WRL::ComPtr<ID3D12Resource> ambientResource_;
        AmbientLight* ambientData_ = nullptr;
    };
}