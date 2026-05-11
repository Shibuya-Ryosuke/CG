#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include "../Base/DirectXCommon.h"
#include "../Math/Math.h"

namespace Engine {

    class ReflectCommon {
    public:
        // インスタンスの取得
        static ReflectCommon* GetInstance();

        void Initialize();

        // 反射テクスチャへの描き込み開始（レンダーターゲットの切り替え）
        void PreDraw();

        // 反射テクスチャへの描き込み終了（リソースバリアの変更）
        void PostDraw();

        // 終了処理
        void Finalize();

        // --- ゲッター ---
        // 反射結果が描き込まれたテクスチャのGPUハンドル（鏡に貼る用）
        D3D12_GPU_DESCRIPTOR_HANDLE GetReflectionTextureHandle() const;
        ID3D12RootSignature* GetRootSignature() const { return rootSignature_.Get(); }
        ID3D12PipelineState* GetPipelineState() const { return graphicsPipelineState_.Get(); }
        uint32_t GetSrvIndex() const { return srvIndex_; }

        struct ReflectMaterial {
            Vector4 color;
            int32_t enableLighting;
            int32_t shadingMode;
            float reflectionWeight;
            float shininess; // 16バイト境界を合わせるための調整にもなる
            Matrix4x4 uvTransform;
        };

    private:
        ReflectCommon() = default;
        ~ReflectCommon() = default;
        ReflectCommon(const ReflectCommon&) = delete;
        ReflectCommon& operator=(const ReflectCommon&) = delete;

        void CreateRootSignature();
        void CreatePipelineState();
        void CreateReflectionResource();

    private:
        DirectXCommon* dxCommon_ = nullptr;

        // 反射描画用リソース
        Microsoft::WRL::ComPtr<ID3D12Resource> reflectionResource_;
        // RTV用ヒープ（反射テクスチャをレンダーターゲットにするため）
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap_;
        // SRV用（鏡に貼るため：TextureManagerに登録しても良いが、一旦独立させると管理が楽）
        uint32_t srvIndex_ = 0;

        Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_;
    };

}