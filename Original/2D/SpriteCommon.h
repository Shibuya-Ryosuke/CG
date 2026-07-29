#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <array>
#include <vector>
#include <functional>

namespace RyoEngine {
    class DirectXCommon;

    class SpriteCommon {
    public:
        static SpriteCommon* GetInstance();

        void Initialize();
        void BeginDraw();
        void Draw();
        void Finalize();

        // Getter
        ID3D12RootSignature* GetRootSignature() const { return rootSignature_.Get(); }
        ID3D12PipelineState* GetPipelineState() const { return graphicsPipelineState_.Get(); }

        void SetDrawCommands(const std::function<void()>& function) { drawCommands_.push_back(function); }

        void CommandsClear() { drawCommands_.clear(); }
    private:
        SpriteCommon() = default;
        ~SpriteCommon() = default;
        SpriteCommon(const SpriteCommon&) = delete;
        SpriteCommon& operator=(const SpriteCommon&) = delete;

        void CreateRootSignature();
        void CreatePipelineState();

    private:
        DirectXCommon* dxCommon_ = nullptr;
        Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_;

        std::vector<std::function<void()>> drawCommands_;
    };
}