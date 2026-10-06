#pragma once
#include <d3d12.h>
#include <wrl.h>
#include "../../Core/Base/DirectXCommon.h"
#include <vector>
#include <functional>
#include <array>
#include "../../Core/Math/BlendMode.h"

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

        // 描画開始（内部で保持している blendMode_ を使用する）
        void BeginDraw(DrawType drawType = DrawType::REAL);
        void Draw();

        void DrawShadow();

        void Finalize();

        // --- ブレンドモードの設定・取得 ---
        void SetBlendMode(BlendMode blendMode) { blendMode_ = blendMode; }
        BlendMode GetBlendMode() const { return blendMode_; }

        ID3D12RootSignature* GetRootSignature() const { return rootSignature_.Get(); }

        ID3D12PipelineState* GetPipelineState(DrawType drawType) const {
            size_t blendIdx = static_cast<size_t>(blendMode_);
            switch (drawType) {
            case DrawType::REAL:          return realPipelineStates_[blendIdx].Get();
            case DrawType::REFLECT:       return reflectPipelineStates_[blendIdx].Get();
            case DrawType::NO_UV:         return noUVPipelineStates_[blendIdx].Get();
            case DrawType::REFLECT_NO_UV: return reflectNoUVPipelineStates_[blendIdx].Get();
            case DrawType::SHADOW:        return shadowPipelineState_.Get();
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

        // --- 内部で保持する現在のブレンドモード ---
        BlendMode blendMode_ = BlendMode::Normal;

        // ブレンドモードごとのPSO配列 (各6個ずつ)
        std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, 6> realPipelineStates_;
        std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, 6> reflectPipelineStates_;
        std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, 6> noUVPipelineStates_;
        std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, 6> reflectNoUVPipelineStates_;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> shadowPipelineState_;

        void CreateRootSignature();
        void CreateRealPipelineStates();
        void CreateReflectPipelineStates();
        void CreateNoUVPipelineStates();
        void CreateReflectNoUVPipelineStates();
        void CreateShadowPipelineState();

        D3D12_BLEND_DESC CreateBlendDesc(BlendMode blendMode);

        std::vector<std::function<void()>> drawCommands_;
        DrawType currentDrawType_ = DrawType::REAL;
    };
}