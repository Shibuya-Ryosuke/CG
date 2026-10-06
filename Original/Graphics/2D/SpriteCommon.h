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
        static SpriteCommon* GetInstance();

        void Initialize();
        void BeginDraw();
        void Draw();
        void Finalize();

        // --- ブレンドモードの設定・取得 ---
        void SetBlendMode(BlendMode blendMode) { blendMode_ = blendMode; }
        BlendMode GetBlendMode() const { return blendMode_; }

        // Getter
        ID3D12RootSignature* GetRootSignature() const { return rootSignature_.Get(); }

        ID3D12PipelineState* GetPipelineState() const {
            return graphicsPipelineStates_[static_cast<size_t>(blendMode_)].Get();
        }

        void SetDrawCommands(const std::function<void()>& function) { drawCommands_.push_back(function); }
        void CommandsClear() { drawCommands_.clear(); }

    private:
        SpriteCommon() = default;
        ~SpriteCommon() = default;
        SpriteCommon(const SpriteCommon&) = delete;
        SpriteCommon& operator=(const SpriteCommon&) = delete;

        void CreateRootSignature();
        void CreatePipelineStates(); // 複数作成に変更
        D3D12_BLEND_DESC CreateBlendDesc(BlendMode blendMode); // 追加

    private:
        DirectXCommon* dxCommon_ = nullptr;
        Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;

        // --- 内部で保持する現在のブレンドモードと、6種類分のPSO配列 ---
        BlendMode blendMode_ = BlendMode::Normal;
        std::array<Microsoft::WRL::ComPtr<ID3D12PipelineState>, 6> graphicsPipelineStates_;

        std::vector<std::function<void()>> drawCommands_;
    };
}