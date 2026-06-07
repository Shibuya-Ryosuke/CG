#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include "../Base/DirectXCommon.h"
#include "../Math/Math.h"
#include "ReflectObject.h"

namespace RyoEngine {

    class ReflectCommon {
    public:
        // インスタンスの取得
        static ReflectCommon* GetInstance();

        void Initialize();

        // 
        // テクスチャへの描き込み開始（レンダーターゲットの切り替え）
        void PreDraw(ReflectObject* mirror);

        // 反射テクスチャへの描き込み終了（リソースバリアの変更）
        void PostDraw(ReflectObject* mirror);

        // 終了処理
        void Finalize();

        // --- ゲッター ---
        ID3D12RootSignature* GetRootSignature() const { return rootSignature_.Get(); }
        ID3D12PipelineState* GetPipelineState() const { return graphicsPipelineState_.Get(); }
        ReflectObject* GetActiveMirror() const { return activeMirror_; }

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

    private:
        DirectXCommon* dxCommon_ = nullptr;

        Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
        Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_;

        ReflectObject* activeMirror_ = nullptr;
    };

}