#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <cstdint>
#include "Light.h"
#include "../Math/Math.h"

namespace RyoEngine {

    // シーン内で同時に扱えるライトの最大数
    // (Light配列用バッファをこのサイズで固定確保するため。増やす場合はここを変更してリソースを作り直す)
    constexpr uint32_t kMaxLightCount = 16;

    /// <summary>
    /// シーン全体で共有するライト群（Directional/Point/Spot/Area）とアンビエントライトを管理するクラス。
    /// 以前は「DirectionalLightを1つだけ」保持していたが、複数灯・複数種別に対応するため
    /// CPU側はstd::vector<Light>で管理し、Update()でGPU用バッファへ反映する方式に変更した。
    ///
    /// GPU側には3つのリソースを渡す想定：
    ///   1. lightResource_       : Light配列そのもの（StructuredBufferとしてSRVを張る。stride = sizeof(Light)）
    ///   2. lightCountResource_  : 現在有効なライト数（cbufferとしてCBVを張る）
    ///   3. ambientResource_     : アンビエントライト（cbufferとしてCBVを張る）
    /// ※ SRV/CBVのDescriptorHeapへの登録、ルートシグネチャ・PSO側の対応は別途必要（このクラスの範囲外）。
    /// </summary>
    class LightManager {
    public:
        static LightManager* GetInstance();

        void Initialize();
        void Finalize();

        /// <summary>
        /// CPU側(lights_)の内容をGPU用バッファへ反映する。
        /// AddLight/RemoveLight/GetLight()経由での編集後、描画前に必ず呼ぶこと。
        /// （Mapしっぱなしのバッファへ直接書き込むだけなので毎フレーム呼んでも軽い）
        /// </summary>
        void Update();

        // --- ライト操作 ---
        // 追加したライトのインデックスを返す。kMaxLightCountを超えると追加できず-1を返す
        int AddLight(const Light& light);
        void RemoveLight(int index);
        void ClearLights();

        Light& GetLight(int index) { return lights_[index]; }
        const Light& GetLight(int index) const { return lights_[index]; }
        size_t GetLightCount() const { return lights_.size(); }

        // --- アンビエントライト ---
        void SetAmbientLight(const AmbientLight& ambient) {
            ambientData_->color = ambient.color;
            ambientData_->intensity = ambient.intensity;
        }
        void SetAmbientColor(const Vector4& color) { ambientData_->color = color; }
        void SetAmbientIntensity(float intensity) { ambientData_->intensity = intensity; }
        const AmbientLight& GetAmbientLight() const { return *ambientData_; }

        // --- GPUリソース取得（Root Descriptorとして直接バインドする想定。DescriptorHeap登録は不要） ---
        ID3D12Resource* GetLightResource() const { return lightResource_.Get(); }
        ID3D12Resource* GetLightCountResource() const { return lightCountResource_.Get(); }
        ID3D12Resource* GetAmbientResource() const { return ambientResource_.Get(); }

        // Light配列(StructuredBuffer)本体。SetGraphicsRootShaderResourceView()でRoot Descriptorとして直接バインドする
        D3D12_GPU_VIRTUAL_ADDRESS GetLightGPUVirtualAddress() const { return lightResource_->GetGPUVirtualAddress(); }
        D3D12_GPU_VIRTUAL_ADDRESS GetLightCountGPUVirtualAddress() const { return lightCountResource_->GetGPUVirtualAddress(); }
        D3D12_GPU_VIRTUAL_ADDRESS GetAmbientGPUVirtualAddress() const { return ambientResource_->GetGPUVirtualAddress(); }

        // ================================================================
        // 後方互換API（旧：シーンにDirectionalLightが1つだけという設計の名残）
        // 内部的には「lights_[0]」を「シーンの代表的な指向性ライト」として読み書きする。
        // Initialize()で必ず1つ目のライトを登録しているため lights_[0] は存在する前提。
        // ClearLights()で全消去した状態でこれらを呼ぶと壊れるので、複数灯を扱うようになったら
        // Model/MeshからはこちらではなくAddLight/GetLight/RemoveLightを直接使うよう移行すること。
        // ================================================================
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

    private:
        LightManager() = default;
        ~LightManager() = default;
        LightManager(const LightManager&) = delete;
        LightManager& operator=(const LightManager&) = delete;

        // CPU側のライト一覧（ここを編集してからUpdate()でGPUへ反映する）
        std::vector<Light> lights_;

        // Light配列用バッファ（kMaxLightCount件ぶん固定確保。StructuredBufferとして扱う想定）
        Microsoft::WRL::ComPtr<ID3D12Resource> lightResource_;
        Light* lightMappedData_ = nullptr;

        // 現在の有効ライト数用バッファ（cbuffer）
        Microsoft::WRL::ComPtr<ID3D12Resource> lightCountResource_;
        LightCountData* lightCountData_ = nullptr;

        // アンビエントライト用バッファ（cbuffer）
        Microsoft::WRL::ComPtr<ID3D12Resource> ambientResource_;
        AmbientLight* ambientData_ = nullptr;
    };
}