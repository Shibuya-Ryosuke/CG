#pragma once
#include <d3d12.h>
#include <wrl.h>
#include "../Base/DirectXCommon.h"

namespace RyoEngine {
    class ModelCommon {
    public:
        enum DrawType {
            REAL,
            REFLECT
        };

        /// <summary>
        /// シングルトンインスタンスの取得
        /// </summary>
        static ModelCommon* GetInstance();

        /// <summary>
        /// 初期化
        /// </summary>
        void Initialize();

        void BeginDraw(DrawType drawType = DrawType::REAL);

        /// <summary>
        /// 終了処理
        /// </summary>
        void Finalize();

        // --- ゲッター ---
        ID3D12RootSignature* GetRootSignature() const { return rootSignature_.Get(); }
        ID3D12PipelineState* GetPipelineState() const { return realPipelineState_.Get(); }

    private:
        ModelCommon() = default;
        ~ModelCommon() = default;
        ModelCommon(const ModelCommon&) = delete;
        ModelCommon& operator=(const ModelCommon&) = delete;

        // DirectXCommonのポインタ（初期化時にキャッシュする用）
        DirectXCommon* dxCommon_ = nullptr;

        // ルートシグネチャ
        Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
        // グラフィックスパイプライン
        Microsoft::WRL::ComPtr<ID3D12PipelineState> realPipelineState_;
        // 反射用パイプライン
        Microsoft::WRL::ComPtr<ID3D12PipelineState> reflectPipelineState_;

        // ルートシグネチャー作成
        void CreateRootSignature();
        // パイプライン作成
        void CreateRealPipelineState();
        // 反射用パイプライン生成
        void CreateReflectPipelineState();
    };
}