#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include "../Math/Math.h"

namespace RyoEngine {

    // シャドウマップの解像度
    constexpr uint32_t kShadowMapSize = 2048;

    /// <summary>
    /// シーンの主要なDirectionalLight(LightManagerのLight 0)から見た深度テクスチャ(シャドウマップ)と、
    /// そのライトのView-Projection行列を管理するクラス。
    /// LightManagerが「ライトの色・向きなどの状態」を管理するのに対し、こちらは
    /// 「その光から見た描画結果(深度)」というレンダリングリソースを管理する、別の責務として分離している。
    /// NOTE: 現状はDirectional 1灯(Light 0)専用。Point/Spotの影は別方式(キューブマップ/パースペクティブ
    ///       シャドウマップ)が必要なため、今回のスコープには含めていない。
    /// </summary>
    class ShadowMap {
    public:
        static ShadowMap* GetInstance();

        static void Initialize();
        static void Finalize();

        /// <summary>
        /// シャドウパス開始。シャドウマップを深度書き込み対象にし、ビューポート/シザーを
        /// シャドウマップ解像度に合わせ、ライトのView-Projection行列を現在のLightManagerの
        /// 状態(Light 0の向き)から再計算する。
        /// </summary>
        static void BeginShadowPass();

        /// <summary>
        /// シャドウパス終了。シャドウマップをピクセルシェーダーから読める状態に遷移させる。
        /// </summary>
        static void EndShadowPass();

        // --- ゲッター ---
        static D3D12_GPU_VIRTUAL_ADDRESS GetLightViewProjGPUVirtualAddress() { return GetInstance()->lightVPResource_->GetGPUVirtualAddress(); }
        static uint32_t GetShadowMapTextureHandle() { return GetInstance()->shadowMapTextureHandle_; }

        // --- セッター(影を落とす範囲の調整用。暫定：固定値運用) ---
        static void SetOrthoHalfSize(float halfSize) { GetInstance()->orthoHalfSize_ = halfSize; }
        static void SetOrthoDepthRange(float nearZ, float farZ) { GetInstance()->nearZ_ = nearZ;  GetInstance()->farZ_ = farZ; }
        static void SetLightDistance(float distance) { GetInstance()->lightDistance_ = distance; }
        static void SetTargetPosition(const Vector3& position) { GetInstance()->targetPosition_ = position; }

    private:
        ShadowMap() = default;
        ~ShadowMap() = default;
        ShadowMap(const ShadowMap&) = delete;
        ShadowMap& operator=(const ShadowMap&) = delete;

        // ライトのView-Projection行列を計算し、lightVPData_へ書き込む
        static void UpdateLightViewProj();

        // リソースの状態(currentState_)と異なる場合のみバリアを発行する
        static void TransitionTo(D3D12_RESOURCE_STATES newState, ID3D12GraphicsCommandList* commandList);

        // 深度テクスチャ本体 (R32_TYPELESSで作り、DSV/SRVで別解釈させる)
        Microsoft::WRL::ComPtr<ID3D12Resource> shadowMapResource_;
        D3D12_RESOURCE_STATES currentState_ = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        // DSV用ディスクリプタヒープ(専用。DirectXCommonのメイン深度バッファとは別)
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap_;
        // TextureManagerに登録した後のハンドル(SRVとして参照する用)
        uint32_t shadowMapTextureHandle_ = 0;

        // ライトのView-Projection行列(cbuffer)
        Microsoft::WRL::ComPtr<ID3D12Resource> lightVPResource_;
        Matrix4x4* lightVPData_ = nullptr;

        // 正射影の範囲(暫定：ターゲット位置を中心とした固定サイズのボックス。
        //  カメラ視錐台に合わせて動的にフィットさせる本格版は今後の課題)
        float orthoHalfSize_ = 20.0f;
        float nearZ_ = 0.1f;
        float farZ_ = 100.0f;
        float lightDistance_ = 30.0f;                     // targetPosition_からどれだけ光源側(-direction方向)へ離すか
        Vector3 targetPosition_ = { 0.0f, 0.0f, 0.0f };   // 影を落とす範囲の中心(暫定：原点固定)
    };
}
