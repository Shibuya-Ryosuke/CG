#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <array>
#include <vector>
#include <functional>
#include "../../Core/Math/BlendMode.h" // 追加

namespace RyoEngine {
    class DirectXCommon;

    class SpriteCommon {
    public:
        static void Initialize();
        static void BeginDraw();
        static void Draw();
        static void Finalize();

        // --- ブレンドモードの設定・取得 ---
        static void SetBlendMode(BlendMode blendMode) { GetInstance()->blendMode_ = blendMode; }
        static BlendMode GetBlendMode() { return GetInstance()->blendMode_; }

        // Getter
        static ID3D12RootSignature* GetRootSignature() { return GetInstance()->rootSignature_.Get(); }

        static ID3D12PipelineState* GetPipelineState() {
            return GetInstance()->graphicsPipelineStates_[static_cast<size_t>(GetInstance()->blendMode_)].Get();
        }

        static void SetDrawCommands(const std::function<void()>& function) { GetInstance()->drawCommands_.push_back(function); }
        static void CommandsClear() { GetInstance()->drawCommands_.clear(); }

    private:
        static SpriteCommon* GetInstance();

        SpriteCommon() = default;
        ~SpriteCommon() = default;
        SpriteCommon(const SpriteCommon&) = delete;
        SpriteCommon& operator=(const SpriteCommon&) = delete;

        void CreateRootSignature();
        void CreatePipelineStates(); // 複数作成に変更
        D3D12_BLEND_DESC CreateBlendDesc(BlendMode blendMode); // 追加

    private:
        Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;

        // --- 内部で保持する現在のブレンドモードと、6種類分のPSO配列 ---
        BlendMode blendMode_ = BlendMode::Normal;
        std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, 6> graphicsPipelineStates_;

        std::vector<std::function<void()>> drawCommands_;
    };
}