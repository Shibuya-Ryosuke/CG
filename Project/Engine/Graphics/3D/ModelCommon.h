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

        static void Initialize();

        // 描画開始（内部で保持している blendMode_ を使用する）
        static void BeginDraw(DrawType drawType = DrawType::REAL);
        static void Draw();

        static void DrawShadow();

        static void Finalize();

        // --- ブレンドモードの設定・取得 ---
        static void SetBlendMode(BlendMode blendMode) { GetInstance()->blendMode_ = blendMode; }
        static BlendMode GetBlendMode() { return GetInstance()->blendMode_; }

        static ID3D12RootSignature* GetRootSignature() { return GetInstance()->rootSignature_.Get(); }

        static ID3D12PipelineState* GetPipelineState(DrawType drawType) {
            size_t blendIdx = static_cast<size_t>(GetInstance()->blendMode_);
            switch (drawType) {
            case DrawType::REAL:          return GetInstance()->realPipelineStates_[blendIdx].Get();
            case DrawType::REFLECT:       return GetInstance()->reflectPipelineStates_[blendIdx].Get();
            case DrawType::NO_UV:         return GetInstance()->noUVPipelineStates_[blendIdx].Get();
            case DrawType::REFLECT_NO_UV: return GetInstance()->reflectNoUVPipelineStates_[blendIdx].Get();
            case DrawType::SHADOW:        return GetInstance()->shadowPipelineState_.Get();
            }
            return GetInstance()->realPipelineStates_[blendIdx].Get();
        }

        static void SetDrawCommands(const std::function<void()>& function) { GetInstance()->drawCommands_.push_back(function); }
        static void CommandsClear() { GetInstance()->drawCommands_.clear(); }

        static DrawType GetCurrentDrawType() { return GetInstance()->currentDrawType_; }

    private:
        static ModelCommon* GetInstance();

        ModelCommon() = default;
        ~ModelCommon() = default;
        ModelCommon(const ModelCommon&) = delete;
        ModelCommon& operator=(const ModelCommon&) = delete;

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