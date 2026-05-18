#pragma once
#include <d3d12.h>
#include <wrl.h>
#include "../Base/DirectXCommon.h"

namespace Engine {
    class Object3dCommon {
    public:
        /// <summary>
        /// シングルトンインスタンスの取得
        /// </summary>
        static Object3dCommon* GetInstance();

        /// <summary>
        /// 初期化
        /// </summary>
        void Initialize();

        void BeginDraw();

        /// <summary>
        /// 終了処理
        /// </summary>
        void Finalize();

        // setter
        void SetCullMode(D3D12_CULL_MODE cullMode) { cullMode_ = cullMode; }

        // --- ゲッター ---
        ID3D12RootSignature* GetRootSignature() const { return rootSignature_.Get(); }
        ID3D12PipelineState* GetPipelineState() const { return graphicsPipelineState_.Get(); }

    private:
        Object3dCommon() = default;
        ~Object3dCommon() = default;
        Object3dCommon(const Object3dCommon&) = delete;
        Object3dCommon& operator=(const Object3dCommon&) = delete;

        // DirectXCommonのポインタ（初期化時にキャッシュする用）
        DirectXCommon* dxCommon_ = nullptr;

        D3D12_CULL_MODE cullMode_ = D3D12_CULL_MODE_BACK;

        // ルートシグネチャ
        Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
        // グラフィックスパイプライン
        Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_;

        // ルートシグネチャー作成
        void CreateRootSignature();
        // パイプライン作成
        void CreatePipelineState();
    };
}