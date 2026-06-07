#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <array>

namespace RyoEngine {
    class DirectXCommon;

    class SpriteCommon {
    public:
        static SpriteCommon* GetInstance();

        void Initialize();
        void BeginDraw();
        void Finalize();

        // Getter
        ID3D12RootSignature* GetRootSignature() const { return rootSignature_.Get(); }
        ID3D12PipelineState* GetPipelineState() const { return graphicsPipelineState_.Get(); }

        uint32_t GetWhiteTex() const { return whiteTex; };

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

        uint32_t whiteTex = 0;
    };
}