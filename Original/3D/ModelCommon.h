#pragma once
#include <d3d12.h>
#include <wrl.h>
#include "../Base/DirectXCommon.h"
#include <vector>
#include <functional>
#include <array>
#include "../Math/BlendMode.h" // 追加: BlendModeのインクルード

namespace RyoEngine {
    class ModelCommon {
    public:
        enum DrawType {
            REAL,
            REFLECT,
            NO_UV,
            REFLECT_NO_UV,
            SHADOW,
        };

        static ModelCommon* GetInstance();

        void Initialize();

        // 変更: BlendModeを指定してBeginDrawできるようにする（デフォルトはNormal）
        void BeginDraw(DrawType drawType = DrawType::REAL, BlendMode blendMode = BlendMode::Normal);
        void Draw(BlendMode blendMode = BlendMode::Normal);

        void DrawShadow();

        void Finalize();

        ID3D12RootSignature* GetRootSignature() const { return rootSignature_.Get(); }

        // 従来のゲッター（互換性のためNormalを返すか、必要に応じて拡張）
        ID3D12PipelineState* GetPipelineState() const { return realPipelineStates_[static_cast<size_t>(BlendMode::Normal)].Get(); }

        /// <summary>
        /// DrawTypeとBlendModeを指定してPSOを取得する
        /// </summary>
        ID3D12PipelineState* GetPipelineState(DrawType drawType, BlendMode blendMode) const {
            size_t blendIdx = static_cast<size_t>(blendMode);
            switch (drawType) {
            case DrawType::REAL:          return realPipelineStates_[blendIdx].Get();
            case DrawType::REFLECT:       return reflectPipelineStates_[blendIdx].Get();
            case DrawType::NO_UV:         return noUVPipelineStates_[blendIdx].Get();
            case DrawType::REFLECT_NO_UV: return reflectNoUVPipelineStates_[blendIdx].Get();
            case DrawType::SHADOW:        return shadowPipelineState_.Get(); // シャドウはブレンド関係なし
            }
            return realPipelineStates_[blendIdx].Get();
        }

        void SetDrawCommands(const std::function<void()>& function) { drawCommands_.push_back(function); }
        void CommandsClear() { drawCommands_.clear(); }

        DrawType GetCurrentDrawType() const { return currentDrawType_; }

    private:
        ModelCommon() = default;
        ~ModelCommon() = default;
        ModelCommon(const ModelCommon&) = delete;
        ModelCommon& operator=(const ModelCommon&) = delete;

        DirectXCommon* dxCommon_ = nullptr;
        Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;

        // --- ブレンドモードごとのPSO配列 (各6個ずつ) ---
        std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, 6> realPipelineStates_;
        std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, 6> reflectPipelineStates_;
        std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, 6> noUVPipelineStates_;
        std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, 6> reflectNoUVPipelineStates_;

        // シャドウはブレンド不要なので1つでOK
        Microsoft::WRL::ComPtr<ID3D12PipelineState> shadowPipelineState_;

        void CreateRootSignature();

        // 内部で6つのブレンドモード分をまとめて生成するヘルパー関数
        void CreateRealPipelineStates();
        void CreateReflectPipelineStates();
        void CreateNoUVPipelineStates();
        void CreateReflectNoUVPipelineStates();
        void CreateShadowPipelineState();

        // 共通のD3D12_BLEND_DESCを構築するヘルパー
        D3D12_BLEND_DESC CreateBlendDesc(BlendMode blendMode);

        std::vector<std::function<void()>> drawCommands_;
        DrawType currentDrawType_ = DrawType::REAL;
    };
}