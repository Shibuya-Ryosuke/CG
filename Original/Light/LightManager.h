#pragma once
#include <d3d12.h>
#include <wrl.h>
#include "Light.h"
#include "../Math/Math.h"

namespace RyoEngine {

    /// <summary>
    /// シーン全体で共有する指向性ライトを1つ管理するクラス。
    /// 以前は各Modelがメッシュ単位でライトの定数バッファを個別に持っていたが、
    /// Blenderのランプのように「1つの光源が全オブジェクトに影響する」形にするため、
    /// ライトのリソースをここに一本化する。
    /// 現状はDirectionalLightを1つだけ扱う想定 (複数光源には未対応)。
    /// </summary>
    class LightManager {
    public:
        // インスタンス取得
        static LightManager* GetInstance();

        /// <summary>
        /// 初期化 (定数バッファの生成とデフォルト値の設定)
        /// ModelCommon::Initialize()などと同じタイミングで、エンジン起動時に一度だけ呼ぶこと。
        /// </summary>
        void Initialize();

        /// <summary>
        /// 終了処理
        /// </summary>
        void Finalize();

        // --- ゲッター ---
        const DirectionalLight& GetDirectionalLight() const { return *lightData_; }
        const Vector4& GetColor() const { return lightData_->color; }
        const Vector3& GetDirection() const { return lightData_->direction; }
        float GetIntensity() const { return lightData_->intensity; }

        ID3D12Resource* GetResource() const { return lightResource_.Get(); }
        D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const { return lightResource_->GetGPUVirtualAddress(); }

        // --- セッター ---
        void SetDirectionalLight(const DirectionalLight& light) {
            SetColor(light.color);
            SetDirection(light.direction);
            SetIntensity(light.intensity);
        }
        void SetColor(const Vector4& color) { lightData_->color = color; }
        // 向きは正規化して格納する (Modelが従来持っていたSetDLDirectionと同じ挙動)
        void SetDirection(const Vector3& direction) { lightData_->direction = Normalize(direction); }
        void SetIntensity(float intensity) { lightData_->intensity = intensity; }

    private:
        LightManager() = default;
        ~LightManager() = default;
        LightManager(const LightManager&) = delete;
        LightManager& operator=(const LightManager&) = delete;

        // ライト用定数バッファ (Upload Heapに常時Mapしたまま使う)
        Microsoft::WRL::ComPtr<ID3D12Resource> lightResource_;
        DirectionalLight* lightData_ = nullptr;
    };
}
