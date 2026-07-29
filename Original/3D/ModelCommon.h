#pragma once
#include <d3d12.h>
#include <wrl.h>
#include "../Base/DirectXCommon.h"
#include <vector>
#include <functional>

namespace RyoEngine {
    class ModelCommon {
    public:
        enum DrawType {
            REAL,
            REFLECT,
            NO_UV,          // UVを持たないメッシュ用 (通常描画)
            REFLECT_NO_UV,  // UVを持たないメッシュ用 (鏡面反射描画)
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
        void Draw();

        /// <summary>
        /// 終了処理
        /// </summary>
        void Finalize();

        // --- ゲッター ---
        ID3D12RootSignature* GetRootSignature() const { return rootSignature_.Get(); }
        ID3D12PipelineState* GetPipelineState() const { return realPipelineState_.Get(); }
        

        /// <summary>
        /// DrawTypeを指定してPSOを取得する (メッシュ単位でPSOを切り替えたい場合に使用)
        /// </summary>
        ID3D12PipelineState* GetPipelineState(DrawType drawType) const {
            switch (drawType) {
            case DrawType::REAL:          return realPipelineState_.Get();
            case DrawType::REFLECT:       return reflectPipelineState_.Get();
            case DrawType::NO_UV:         return noUVPipelineState_.Get();
            case DrawType::REFLECT_NO_UV: return reflectNoUVPipelineState_.Get();
            }
            return realPipelineState_.Get();
        }

        void SetDrawCommands(const std::function<void()>& function) { drawCommands_.push_back(function); }

        void CommandsClear() { drawCommands_.clear(); }

    private:
        ModelCommon() = default;
        ~ModelCommon() = default;
        ModelCommon(const ModelCommon&) = delete;
        ModelCommon& operator=(const ModelCommon&) = delete;

        // DirectXCommonのポインタ（初期化時にキャッシュする用）
        DirectXCommon* dxCommon_ = nullptr;

        // ルートシグネチャ (UV有無・反射有無の全PSOで共通のものを使い回す)
        Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
        // グラフィックスパイプライン
        Microsoft::WRL::ComPtr<ID3D12PipelineState> realPipelineState_;
        // 反射用パイプライン
        Microsoft::WRL::ComPtr<ID3D12PipelineState> reflectPipelineState_;
        // UVを持たないメッシュ用パイプライン (通常描画)
        Microsoft::WRL::ComPtr<ID3D12PipelineState> noUVPipelineState_;
        // UVを持たないメッシュ用パイプライン (反射描画)
        Microsoft::WRL::ComPtr<ID3D12PipelineState> reflectNoUVPipelineState_;

        // ルートシグネチャー作成
        void CreateRootSignature();
        // パイプライン作成
        void CreateRealPipelineState();
        // 反射用パイプライン生成
        void CreateReflectPipelineState();
        // UVを持たないメッシュ用パイプライン生成 (通常描画)
        void CreateNoUVPipelineState();
        // UVを持たないメッシュ用パイプライン生成 (反射描画)
        void CreateReflectNoUVPipelineState();

        std::vector<std::function<void()>> drawCommands_;
    };
}
